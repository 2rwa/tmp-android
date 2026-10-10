# Liquid Glass Globe — native Android 15 Vulkan prototype

Independent Android application module `:nativeglobe` within `2rwa/tmp-android`.
Uses a **native C++ Vulkan 1.0 swapchain**, not WebView or Android Canvas for 3D rendering.

- Min Android API 35 (Android 15). Targets Redmi 12 5G; needs `VK_KHR_android_surface`, graphics + present, and `VK_KHR_swapchain`.
- Java `SurfaceView` and native C++ renderer with a dedicated thread.
- Queries Vulkan loader and physical-device versions; requests the highest instance API up to 1.3, uses the actual physical-driver API level, and displays GPU name, API version, FPS and frame time. Snapdragon 4 Gen 2 is listed by Qualcomm as Vulkan 1.1; never assume Vulkan 1.3 because Android is 15.
- Core rendering commands and SPIR-V are still Vulkan 1.0 compatible. API version increase alone is NOT a performance optimization.
- 7 opaque colored balls + configurable **0–64 transparent metaball droplets**.
- Glass-to-glass soft merging using polynomial smooth union, refractive entry/exit raymarching.
- Colored balls versus glass centers and the enclosing globe have collision constraints; **the smoothly merged glass surface can still approach an opaque sphere more closely than the center-based collision approximation**. Not a real fluid simulation.
- Drag to orbit; adjust droplet count, fusion, speed, pause, resolution (160/240/320 short-side px).
- GLSL 450 automatically converted by Android Gradle Plugin to SPIR-V APK assets (`shaders/globe.*.spv`).
- If Vulkan is unavailable, the status label explains the failure and the Activity remains open.

## Build

`gradle --no-daemon :nativeglobe:assembleDebug`

Requires Android SDK 35, Android NDK 27.2.12479018, CMake 3.22.1, JDK 17.
Output `nativeglobe/build/outputs/apk/debug/nativeglobe-debug.apk`.

## CI

`.github/workflows/native-glass-globe.yml`:
- NDK and SPIR-V shader compile / APK build / manifest and assets check
- Android 15 API 35 x86_64 emulator install and UI test
- Vulkan feature / status probe; screenshot retained as artifact

A CI virtual GPU may not expose Vulkan; **a successful Activity smoke test is not a Vulkan graphics pass**.
For real performance or native Vulkan pixel checks, use a Vulkan-capable Android 9+ physical device.

## Source

WebGPU baseline: https://2rwa.github.io/hello-world-pages/glass-metaball-globe/
Shader follows similar glassField, merged glass exit, Fresnel and non-SDF conservative marching patterns. This is a Vulkan-native port, not pixel-identical to the WebGPU baseline.
