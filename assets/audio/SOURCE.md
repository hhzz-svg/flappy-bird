# Game sound effect provenance

- Upstream package: `uisfx` (npm), version `0.4.0`
- Tarball: <https://registry.npmjs.org/uisfx/-/uisfx-0.4.0.tgz>
- Verified npm integrity: `sha512-W891N75C09cvpaNeChP+7BiP0Yp5VSbrIcRgZvzpFUT3o/2DxZqROmUcX50oxeyMnY4Hqxu4jEfjndVUpmheEQ==`
- Audio license: **CC0 1.0 Universal** (public domain dedication); the upstream
  text is kept verbatim in `LICENSE-CC0.txt`. The package's *code* is MIT, but
  no code from it is used here — only the audio.
- Source pack: `sounds/arcade` (the package ships 12 themed packs; the arcade
  set suits this game).

Cues were picked from all 78 arcade cues by measuring duration, peak level and
spectral centroid, favouring short and unobtrusive sounds for the cues that fire
constantly (flap, select) and darker, louder ones for the crash.

| Embedded file | Upstream cue | Peak (dBFS) | Bytes | SHA-256 |
|---|---|---|---|---|
| `flap.wav` | `arcade/snap.ogg` | -18 | 5806 | `086945F9CD8963803116DCEEF54F1DE6…` |
| `point.wav` | `arcade/check.ogg` | -12 | 9524 | `9DC00676417A4331BF6806304233632B…` |
| `milestone.wav` | `arcade/level-up.ogg` | -9 | 30186 | `89F24AE690E54E25F616F24FA2A6AD5C…` |
| `coin.wav` | `arcade/reward.ogg` | -12 | 19864 | `6BB7CC9C07BE1BB191392AB265A9BA26…` |
| `hit.wav` | `arcade/stop.ogg` | -7 | 13770 | `DC9D99F64016423C4165B3EE7ACCB9D4…` |
| `select.wav` | `arcade/press.ogg` | -16 | 5462 | `8186126914D3B9571913C9A15DBB3392…` |
| `buy.wav` | `arcade/purchase.ogg` | -10 | 21148 | `E944058A6FE9CCA1987217661A89EA5B…` |
| `denied.wav` | `arcade/blocked.ogg` | -12 | 13684 | `21ECE806BA59BF43063A18FEF2A48C30…` |

The storm thunderclap is **not** from this pack: every cue in it is a UI sound
with no low-frequency rumble, so that one stays synthesised in `sfx.cpp`.

## Reproducing the conversion

Each file was decoded from the upstream `.ogg`, mixed to mono, resampled to
22050 Hz, trimmed of near-silence (threshold 0.5% of peak, 5ms pad), given a 4ms
fade in and out so the trim cannot click, peak-normalised to the level above,
and written as 16-bit PCM WAV:

```python
x, sr = soundfile.read(f"sounds/arcade/{cue}.ogg", dtype="float64", always_2d=True)
m = x.mean(axis=1)
m = numpy.interp(numpy.linspace(0, len(m) - 1, round(len(m) * 22050 / sr)),
                 numpy.arange(len(m)), m)
# trim, fade, then scale so max(abs(m)) == 10 ** (target_db / 20)
soundfile.write(f"assets/audio/{event}.wav", pcm_int16, 22050, subtype="PCM_16")
```
