# JPEG Stream Viewer (Android 5.1+)

A standalone Android receiver for `tmp-mac/JPEGStreamSender`. The existing Hello World app remains unchanged.

* `minSdk 22` (Android 5.1), Java + Android SDK, no NDK, FFmpeg or Play Services.
* TCP client to a Mac on the same trusted LAN, port **5055**.
* Standard `BitmapFactory.decodeByteArray` and `Canvas` JPEG rendering.
* Letterboxed preview plus centered or edge-corrected cursor crosshair.
* Latest-frame-only rendering; drops old decoded frames, updates fps / bitrate.
* Designed for phone landscape viewing; no special permissions beyond INTERNET.
* Runs without a background service. Disconnects on activity destruction.

## Steps

1. Build/run `JPEGStreamSender.xcodeproj` from `tmp-mac`; approve Screen Recording.
2. Read the Mac's LAN IPv4 address (`ipconfig getifaddr en0`, or system network settings).
3. Install `jpegviewer-debug.apk` from GitHub Actions artifact **jpeg-stream-viewer-android51-apk**.
4. Connect to that IP from the phone. Keep Mac and phone on the same Wi-Fi/LAN.
5. Use the zoom/JPEG quality controls on the Mac; display scales the *raw crop* to the Android window.

## Wire format (version 1)

Repeated binary frames on one TCP connection:

| Offset | Bytes | Content |
| - | - | - |
| 0 | 4 | ASCII `MJP1` |
| 4 | 4 | unsigned JPEG byte length, big endian (1..4MiB) |
| 8 | 2 | cursor X as 0..65535, big endian |
| 10 | 2 | cursor Y as 0..65535, big endian |
| 12 | variable | JPEG file bytes |

Cursor coordinates are normalized within the captured rectangle and use **top-left origin**. JPEG is sent upright; Android maps the crop to an aspect-fit canvas without mirroring/vertical flipping.

Security: **unencrypted and unauthenticated** LAN-only prototype. Do not expose TCP 5055 to the public Internet, untrusted Wi-Fi or port-forward it.
