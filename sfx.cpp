#include "sfx.h"

#include <QtGlobal>

namespace {

enum class Wave { Sine = 0, Square = 1, Triangle = 2, Sawtooth = 3 };

bool g_muted = false;

// Envelope shared by both backends: 10ms linear attack, then exponential decay
// to silence at the end of the tone.
constexpr double kAttack = 0.01;
constexpr double kFloor  = 0.0001;

void playTone(double freq, double durSec, Wave wave, double vol, double delaySec);

}

// ==========================================================================
//  WebAssembly backend — Web Audio via emscripten
// ==========================================================================
#ifdef Q_OS_WASM

#include <emscripten.h>

EM_JS(void, fbSfxResume, (), {
    var Ctx = window.AudioContext || window.webkitAudioContext;
    if (!Ctx) return;
    if (!window._fbAudioCtx) window._fbAudioCtx = new Ctx();
    if (window._fbAudioCtx.state === 'suspended') window._fbAudioCtx.resume();
});

EM_JS(void, fbSfxTone, (double freq, double dur, int wave, double vol, double delay), {
    var ctx = window._fbAudioCtx;
    if (!ctx) return;
    try {
        var types = ['sine', 'square', 'triangle', 'sawtooth'];
        var t0 = ctx.currentTime + delay;
        var osc = ctx.createOscillator();
        var gain = ctx.createGain();
        osc.type = types[wave] || 'sine';
        osc.frequency.setValueAtTime(freq, t0);
        gain.gain.setValueAtTime(0.0001, t0);
        gain.gain.linearRampToValueAtTime(vol, t0 + 0.01);
        gain.gain.exponentialRampToValueAtTime(0.0001, t0 + dur);
        osc.connect(gain);
        gain.connect(ctx.destination);
        osc.start(t0);
        osc.stop(t0 + dur + 0.02);
    } catch (e) {}
});

namespace {
void playTone(double freq, double durSec, Wave wave, double vol, double delaySec)
{
    if (g_muted) return;
    fbSfxTone(freq, durSec, int(wave), vol, delaySec);
}
}

void Sfx::noteUserGesture() { fbSfxResume(); }

// ==========================================================================
//  Desktop backend — software-mixed voices pushed through QAudioSink
// ==========================================================================
#elif defined(FB_DESKTOP_AUDIO)

#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>
#include <QMediaDevices>
#include <QMutex>
#include <QMutexLocker>
#include <QtMath>

#include <cmath>
#include <memory>
#include <vector>

namespace {

constexpr int kRate = 44100;

struct Voice {
    double freq;
    Wave   wave;
    double vol;
    double durSec;
    qint64 delaySamples;
    qint64 pos = 0;          // samples since the voice was queued
    qint64 totalSamples = 0;
};

double waveform(Wave w, double phase)
{
    switch (w) {
    case Wave::Square:   return std::sin(phase) >= 0 ? 1.0 : -1.0;
    case Wave::Triangle: return std::asin(std::sin(phase)) * (2.0 / M_PI);
    case Wave::Sawtooth: {
        const double t = phase / (2.0 * M_PI);
        return 2.0 * (t - std::floor(t + 0.5));
    }
    case Wave::Sine:
    default:             return std::sin(phase);
    }
}

double envelope(const Voice &v, double t)
{
    if (t < kAttack)
        return kFloor + (v.vol - kFloor) * (t / kAttack);
    const double span = v.durSec - kAttack;
    if (span <= 0) return kFloor;
    return v.vol * std::pow(kFloor / v.vol, (t - kAttack) / span);
}

// Never reports end-of-stream: the sink stays running and we emit silence when
// nothing is playing, which avoids a restart click on every sound.
class Mixer : public QIODevice
{
public:
    Mixer() { open(QIODevice::ReadOnly); }

    void add(const Voice &v)
    {
        QMutexLocker lock(&m_mutex);
        m_voices.push_back(v);
    }

    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override { return (1 << 16) + QIODevice::bytesAvailable(); }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        const qint64 frames = maxSize / qint64(sizeof(qint16));
        if (frames <= 0) return 0;
        auto *out = reinterpret_cast<qint16 *>(data);

        QMutexLocker lock(&m_mutex);
        for (qint64 i = 0; i < frames; ++i) {
            double sample = 0;
            for (auto &v : m_voices) {
                const qint64 local = v.pos + i - v.delaySamples;
                if (local < 0 || local >= v.totalSamples) continue;
                const double t = double(local) / kRate;
                sample += waveform(v.wave, 2.0 * M_PI * v.freq * t) * envelope(v, t);
            }
            out[i] = qint16(qBound(-1.0, sample, 1.0) * 32767);
        }
        for (auto &v : m_voices) v.pos += frames;
        m_voices.erase(std::remove_if(m_voices.begin(), m_voices.end(),
                                      [](const Voice &v) {
                                          return v.pos - v.delaySamples >= v.totalSamples;
                                      }),
                       m_voices.end());
        return frames * qint64(sizeof(qint16));
    }

    qint64 writeData(const char *, qint64) override { return 0; }

private:
    QMutex m_mutex;
    std::vector<Voice> m_voices;
};

std::unique_ptr<Mixer> g_mixer;
std::unique_ptr<QAudioSink> g_sink;
bool g_audioUnavailable = false;

Mixer *ensureMixer()
{
    if (g_audioUnavailable) return nullptr;
    if (g_mixer) return g_mixer.get();

    const QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (device.isNull()) {          // headless machine or no sound card
        g_audioUnavailable = true;
        return nullptr;
    }

    QAudioFormat format;
    format.setSampleRate(kRate);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    if (!device.isFormatSupported(format)) {
        g_audioUnavailable = true;
        return nullptr;
    }

    g_mixer = std::make_unique<Mixer>();
    g_sink = std::make_unique<QAudioSink>(device, format);
    g_sink->start(g_mixer.get());
    return g_mixer.get();
}

void playTone(double freq, double durSec, Wave wave, double vol, double delaySec)
{
    if (g_muted) return;
    Mixer *mixer = ensureMixer();
    if (!mixer) return;

    Voice v;
    v.freq = freq;
    v.wave = wave;
    v.vol = vol;
    v.durSec = durSec;
    v.delaySamples = qint64(delaySec * kRate);
    v.totalSamples = qint64(durSec * kRate);
    mixer->add(v);
}

}

void Sfx::noteUserGesture() { ensureMixer(); }

// ==========================================================================
//  No audio backend available
// ==========================================================================
#else

namespace {
void playTone(double, double, Wave, double, double) {}
}

void Sfx::noteUserGesture() {}

#endif

// ==========================================================================
//  Sound table — mirrors the web build's synth (flappy-bird-enhanced.html)
// ==========================================================================
void Sfx::setMuted(bool m) { g_muted = m; }
bool Sfx::muted()          { return g_muted; }

void Sfx::flap()
{
    playTone(720, 0.09, Wave::Square, 0.045, 0);
    playTone(1040, 0.06, Wave::Square, 0.03, 0.02);
}
void Sfx::point()
{
    playTone(1320, 0.09, Wave::Sine, 0.07, 0);
    playTone(1760, 0.13, Wave::Sine, 0.05, 0.05);
}
void Sfx::milestone()
{
    for (int i = 0; i < 3; ++i)
        playTone(880 + i * 220, 0.16, Wave::Sine, 0.08, i * 0.1);
}
void Sfx::coin()
{
    playTone(1568, 0.06, Wave::Triangle, 0.07, 0);
    playTone(2093, 0.12, Wave::Triangle, 0.06, 0.05);
}
void Sfx::hit()
{
    playTone(140, 0.18, Wave::Sawtooth, 0.14, 0);
    playTone(90, 0.4, Wave::Sawtooth, 0.12, 0.05);
}
void Sfx::select()
{
    playTone(560, 0.05, Wave::Triangle, 0.05, 0);
}
void Sfx::thunder()
{
    playTone(70, 0.5, Wave::Sawtooth, 0.10, 0);
    playTone(52, 0.7, Wave::Sawtooth, 0.08, 0.04);
}
void Sfx::buy()
{
    playTone(1046, 0.08, Wave::Triangle, 0.07, 0);
    playTone(1568, 0.14, Wave::Triangle, 0.06, 0.06);
}
void Sfx::denied()
{
    playTone(220, 0.10, Wave::Square, 0.06, 0);
    playTone(160, 0.14, Wave::Square, 0.05, 0.06);
}
