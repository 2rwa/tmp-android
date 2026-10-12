# Wadaiko Native — Mobile Multi-Touch v2 Android port

Android 9+ / NDK C physical DSP / Oboe → AAudio (Android 9+) / offline-native UI. **No WebView, no WASM runtime.**

This is a new `:odaiko` module in `2rwa/tmp-android`; existing sample apps are unchanged.

## Scope (v0.1)
- Large circular membrane pad with location-dependent strike shape (modal/Bessel physics)
- Independent multi-touch input, position, DON/TSU/EDGE/SLAP/KA, auto rim
- Strength rail, fixed strength, Android pressure/contact-area input
- 8-strike sensor calibration (weak ×4 / strong ×4), 0/8/12/20 ms attack window
- ECO/NORMAL/HEAVY/MAX modes, room wet, master gain, stop/mute
- Native C v0.13 physical kernel, seeded modal configuration based on the mobile v2 105 cm profile
- No permission or network access, no rhythm machine, no microphone, no WebGPU

## Provenance
Source: `2rwa/odaiko-physical-synth/apps/odaiko_mobile_multitouch_wasm_v2.html` and `archive/wasm-v13/src/odaiko_wasm_kernel.c`.
The C kernel has been ported without changing the signal equations. The JavaScript parameter/mode initialization was reconstructed in C++; **bit-perfect Golden audio equivalence is not yet certified**. Do not promote this as a Golden reference until waveform comparison has passed. Separate .inc/.c files preserve the original kernel layout.

## Build
`gradle --no-daemon :odaiko:assembleDebug` (Android SDK 35, NDK r27c, CMake 3.22.1, JDK 17, Gradle 8.9).
Artifact: `odaiko/build/outputs/apk/debug/odaiko-debug.apk`.

## Validation
`bash odaiko/scripts/test-native-dsp.sh` runs host DSP tests for 4 quality profiles, 5 articulations, one-second finite, non-silent audio. GitHub Actions builds APK and performs API 28 emulator installation, launch and UI checks.
Real-world end-to-end tap-to-speaker latency, distortion, pressure calibration, and five-minute glitch-free qualification require Android-device testing.

## Audio notes
AAudio is selected by Oboe on supported API levels. Requests low-latency / exclusive mode, float stereo output; actual latency depends on device and route. Output applies conservative ceiling sample clamp, **not a true-peak lookahead limiter**. Bluetooth has substantial variable latency.
