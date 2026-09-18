#include "sfx.h"

#include <QByteArray>
#include <QFile>
#include <QtEndian>
#include <QtGlobal>

#include <cstring>

namespace {

enum class Wave { Sine = 0, Square = 1, Triangle = 2, Sawtooth = 3 };

bool g_muted = false;

// Envelope for the synthesised cues: 10ms linear attack, then exponential decay.
constexpr double kAttack = 0.01;
constexpr double kFloor  = 0.0001;

// Recorded cues, embedded as mono 16-bit WAV (see assets/audio/SOURCE.md).
const char *const kSamples[] = { "flap", "point", "milestone", "coin",
                                 "hit", "select", "buy", "denied" };

void playTone(double freq, double durSec, Wave wave, double vol, double delaySec);
void playSample(const char *name);
void loadSamples();
void backendSetMuted(bool m);   // silences whatever is already sounding

}

// ==========================================================================
//  WebAssembly backend — Web Audio via emscripten
// ==========================================================================
#ifdef Q_OS_WASM

#include <emscripten.h>

// Every note and sample runs through a master gain node so muting can silence
// what is already scheduled, not just what comes next.
EM_JS(void, fbSfxResume, (int muted), {
    var Ctx = window.AudioContext || window.webkitAudioContext;
    if (!Ctx) return;
    if (!window._fbAudioCtx) {
        window._fbAudioCtx = new Ctx();
        window._fbAudioGain = window._fbAudioCtx.createGain();
        window._fbAudioGain.gain.value = muted ? 0 : 1;
        window._fbAudioGain.connect(window._fbAudioCtx.destination);
        window._fbAudioBuffers = {};
    }
    if (window._fbAudioCtx.state === 'suspended') window._fbAudioCtx.resume();
});

EM_JS(void, fbSfxSetMuted, (int muted), {
    if (window._fbAudioGain) window._fbAudioGain.gain.value = muted ? 0 : 1;
});

// The assets are plain mono 16-bit PCM, so they are parsed synchronously rather
// than through decodeAudioData: decoding can only start on the first user
// gesture, and an async decode would lose the cue that same gesture triggers.
EM_JS(void, fbSfxDecode, (const char *name, const unsigned char *data, int len), {
    var ctx = window._fbAudioCtx;
    if (!ctx) return;
    try {
        var v = new DataView(HEAPU8.buffer, HEAPU8.byteOffset + data, len);
        if (v.getUint32(0, false) !== 0x52494646 || v.getUint32(8, false) !== 0x57415645) return;
        var pos = 12, rate = 0, channels = 1, bits = 16;
        while (pos + 8 <= len) {
            var id = v.getUint32(pos, false), size = v.getUint32(pos + 4, true);
            if (id === 0x666d7420) {                       // "fmt "
                channels = v.getUint16(pos + 10, true);
                rate = v.getUint32(pos + 12, true);
                bits = v.getUint16(pos + 22, true);
            } else if (id === 0x64617461) {                // "data"
                if (bits !== 16 || rate <= 0) return;
                var frames = Math.floor(size / 2 / channels);
                var buf = ctx.createBuffer(1, frames, rate);
                var out = buf.getChannelData(0);
                for (var i = 0; i < frames; i++)
                    out[i] = v.getInt16(pos + 8 + i * channels * 2, true) / 32768;
                window._fbAudioBuffers[UTF8ToString(name)] = buf;
                return;
            }
            pos += 8 + size + (size & 1);
        }
    } catch (e) {}
});

EM_JS(void, fbSfxPlay, (const char *name), {
    var ctx = window._fbAudioCtx;
    if (!ctx || !window._fbAudioGain) return;
    var buf = window._fbAudioBuffers[UTF8ToString(name)];
    if (!buf) return;                      // still decoding: skip rather than stall
    try {
        var src = ctx.createBufferSource();
        src.buffer = buf;
        src.connect(window._fbAudioGain);
        src.start();
    } catch (e) {}
});

EM_JS(void, fbSfxTone, (double freq, double dur, int wave, double vol, double delay), {
    var ctx = window._fbAudioCtx;
    if (!ctx || !window._fbAudioGain) return;
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
        gain.connect(window._fbAudioGain);
        osc.start(t0);
        osc.stop(t0 + dur + 0.02);
    } catch (e) {}
});

namespace {

bool g_loaded = false;

void loadSamples()
{
    if (g_loaded) return;
    g_loaded = true;
    for (const char *name : kSamples) {
        QFile f(QStringLiteral(":/audio/%1.wav").arg(QLatin1String(name)));
        if (!f.open(QIODevice::ReadOnly)) continue;
        const QByteArray raw = f.readAll();
        fbSfxDecode(name, reinterpret_cast<const unsigned char *>(raw.constData()), raw.size());
    }
}

void playTone(double freq, double durSec, Wave wave, double vol, double delaySec)
{
    if (g_muted) return;
    fbSfxTone(freq, durSec, int(wave), vol, delaySec);
}

void playSample(const char *name)
{
    if (g_muted) return;
    fbSfxPlay(name);
}

void backendSetMuted(bool m) { fbSfxSetMuted(m ? 1 : 0); }

}

void Sfx::noteUserGesture()
{
    fbSfxResume(g_muted ? 1 : 0);
    loadSamples();          // needs the context, so it waits for the first gesture
}

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
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace {

constexpr int kPreferredRate = 44100;

struct Sample {
    std::vector<float> data;
    int rate = 0;
};

std::map<std::string, Sample> g_samples;
bool g_loaded = false;

// Minimal RIFF/WAVE reader: the files are ours, mono 16-bit PCM.
bool parseWav(const QByteArray &raw, Sample &out)
{
    const char *p = raw.constData();
    if (raw.size() < 44 || std::memcmp(p, "RIFF", 4) || std::memcmp(p + 8, "WAVE", 4))
        return false;

    int channels = 1, bits = 16;
    qint64 pos = 12;
    while (pos + 8 <= raw.size()) {
        const char *id = p + pos;
        const auto size = qint64(qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar *>(p + pos + 4)));
        const char *body = p + pos + 8;
        if (pos + 8 + size > raw.size()) return false;

        if (!std::memcmp(id, "fmt ", 4) && size >= 16) {
            channels = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(body + 2));
            out.rate = int(qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(body + 4)));
            bits = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(body + 14));
        } else if (!std::memcmp(id, "data", 4)) {
            if (bits != 16 || channels < 1 || out.rate <= 0) return false;
            const qint64 frames = size / 2 / channels;
            out.data.resize(size_t(frames));
            const auto *s = reinterpret_cast<const uchar *>(body);
            for (qint64 i = 0; i < frames; ++i) {       // mix down to mono
                int acc = 0;
                for (int c = 0; c < channels; ++c)
                    acc += qFromLittleEndian<qint16>(s + (i * channels + c) * 2);
                out.data[size_t(i)] = float(acc) / (channels * 32768.0f);
            }
            return true;
        }
        pos += 8 + size + (size & 1);
    }
    return false;
}

void loadSamples()
{
    if (g_loaded) return;
    g_loaded = true;
    for (const char *name : kSamples) {
        QFile f(QStringLiteral(":/audio/%1.wav").arg(QLatin1String(name)));
        if (!f.open(QIODevice::ReadOnly)) continue;
        Sample s;
        if (parseWav(f.readAll(), s)) g_samples[name] = std::move(s);
    }
}

// Timing is kept in seconds so a voice does not care what rate the device
// eventually negotiates; the mixer converts using its own rate.
struct Voice {
    double freq;
    Wave   wave;
    double vol;
    double durSec;
    double delaySec;
    qint64 pos = 0;          // frames since the voice was queued
};

struct SampleVoice {
    const Sample *sample;
    qint64 pos = 0;          // frames at the device rate
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
    Mixer(int rate, int channels) : m_rate(rate), m_channels(channels)
    {
        // Unbuffered: a live synth must never hand back audio it generated
        // earlier, or muting would still play out whatever QIODevice cached.
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    }

    void add(const Voice &v)
    {
        QMutexLocker lock(&m_mutex);
        m_voices.push_back(v);
    }

    void add(const SampleVoice &v)
    {
        QMutexLocker lock(&m_mutex);
        m_sampleVoices.push_back(v);
    }

    void clear()
    {
        QMutexLocker lock(&m_mutex);
        m_voices.clear();
        m_sampleVoices.clear();
    }

    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override { return (1 << 16) + QIODevice::bytesAvailable(); }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        const qint64 frames = maxSize / qint64(sizeof(qint16) * m_channels);
        if (frames <= 0) return 0;
        auto *out = reinterpret_cast<qint16 *>(data);

        QMutexLocker lock(&m_mutex);
        for (qint64 i = 0; i < frames; ++i) {
            double sample = 0;
            for (const auto &v : m_voices) {
                const double t = double(v.pos + i) / m_rate - v.delaySec;
                if (t < 0 || t >= v.durSec) continue;
                sample += waveform(v.wave, 2.0 * M_PI * v.freq * t) * envelope(v, t);
            }
            for (const auto &sv : m_sampleVoices) {
                const auto &d = sv.sample->data;
                const double src = double(sv.pos + i) * sv.sample->rate / m_rate;
                const auto i0 = qint64(src);
                if (i0 < 0 || i0 + 1 >= qint64(d.size())) continue;
                const double f = src - double(i0);      // linear resample
                sample += d[size_t(i0)] * (1.0 - f) + d[size_t(i0) + 1] * f;
            }
            const auto s = qint16(qBound(-1.0, sample, 1.0) * 32767);
            for (int c = 0; c < m_channels; ++c)      // same content on every channel
                out[i * m_channels + c] = s;
        }

        const double rate = m_rate;
        for (auto &v : m_voices) v.pos += frames;
        m_voices.erase(std::remove_if(m_voices.begin(), m_voices.end(),
                                      [rate](const Voice &v) {
                                          return double(v.pos) / rate - v.delaySec >= v.durSec;
                                      }),
                       m_voices.end());
        for (auto &sv : m_sampleVoices) sv.pos += frames;
        m_sampleVoices.erase(std::remove_if(m_sampleVoices.begin(), m_sampleVoices.end(),
                                            [rate](const SampleVoice &sv) {
                                                return double(sv.pos) * sv.sample->rate / rate
                                                       >= double(sv.sample->data.size());
                                            }),
                             m_sampleVoices.end());
        return frames * qint64(sizeof(qint16) * m_channels);
    }

    qint64 writeData(const char *, qint64) override { return 0; }

private:
    QMutex m_mutex;
    std::vector<Voice> m_voices;
    std::vector<SampleVoice> m_sampleVoices;
    int m_rate;
    int m_channels;
};

std::unique_ptr<Mixer> g_mixer;
std::unique_ptr<QAudioSink> g_sink;

// Nothing here latches failure: a machine with no output device yet (or one
// whose default changes) gets another chance on the next sound.
Mixer *ensureMixer()
{
    if (g_mixer) return g_mixer.get();

    const QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (device.isNull()) return nullptr;

    QAudioFormat format;
    format.setSampleRate(kPreferredRate);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    if (!device.isFormatSupported(format)) {
        // Mono or 44.1kHz can be refused (some Bluetooth and USB outputs), so
        // take whatever the device wants and let the mixer match it.
        format = device.preferredFormat();
        format.setSampleFormat(QAudioFormat::Int16);
        if (!device.isFormatSupported(format)) return nullptr;
    }

    g_mixer = std::make_unique<Mixer>(format.sampleRate(), format.channelCount());
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
    v.delaySec = delaySec;
    mixer->add(v);
}

void playSample(const char *name)
{
    if (g_muted) return;
    Mixer *mixer = ensureMixer();
    if (!mixer) return;
    loadSamples();

    const auto it = g_samples.find(name);
    if (it == g_samples.end()) return;
    mixer->add(SampleVoice{ &it->second, 0 });
}

void backendSetMuted(bool m)
{
    if (m && g_mixer) g_mixer->clear();   // cut off whatever is still sounding
}

}

void Sfx::noteUserGesture()
{
    ensureMixer();
    loadSamples();
}

// ==========================================================================
//  No audio backend available
// ==========================================================================
#else

namespace {
void playTone(double, double, Wave, double, double) {}
void playSample(const char *) {}
void loadSamples() {}
void backendSetMuted(bool) {}
}

void Sfx::noteUserGesture() {}

#endif

// ==========================================================================
//  Cue table
// ==========================================================================
void Sfx::setMuted(bool m) { g_muted = m; backendSetMuted(m); }
bool Sfx::muted()          { return g_muted; }

void Sfx::flap()      { playSample("flap"); }
void Sfx::point()     { playSample("point"); }
void Sfx::milestone() { playSample("milestone"); }
void Sfx::coin()      { playSample("coin"); }
void Sfx::hit()       { playSample("hit"); }
void Sfx::select()    { playSample("select"); }
void Sfx::buy()       { playSample("buy"); }
void Sfx::denied()    { playSample("denied"); }

// Kept synthesised: the sample pack is all UI cues and has no low rumble, which
// is exactly what a thunderclap needs.
void Sfx::thunder()
{
    playTone(70, 0.5, Wave::Sawtooth, 0.10, 0);
    playTone(52, 0.7, Wave::Sawtooth, 0.08, 0.04);
}
