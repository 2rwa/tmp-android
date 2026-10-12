#!/usr/bin/env bash
set -euo pipefail
# Create a *standalone* Gradle source project from tmp-android's :odaiko module.
# Usage: bash odaiko/scripts/package-standalone.sh /absolute/output/path
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${1:?Specify an empty destination directory}"
mkdir -p "$OUT/odaiko"
cp "$ROOT/build.gradle" "$ROOT/gradle.properties" "$OUT/"
cp "$ROOT/odaiko/build.gradle" "$ROOT/odaiko/README.md" "$OUT/odaiko/"
cp -a "$ROOT/odaiko/src" "$ROOT/odaiko/tests" "$ROOT/odaiko/scripts" "$OUT/odaiko/"
cat > "$OUT/settings.gradle" <<'EOF'
pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
    }
}
rootProject.name = 'wadaiko-native-standalone'
include ':odaiko'
EOF
cat > "$OUT/README.md" <<'EOF'
# Wadaiko Native — standalone Android Studio / Gradle source

This archive contains **only the Mobile Multi-Touch v2 Android app**, not
the other experiments in 2rwa/tmp-android. Source is in `odaiko/src`.
Target: Android 9+; Java UI + C/C++ NDK + Oboe/AAudio native DSP.

Build:
  ./gradlew :odaiko:assembleDebug

APK:
  odaiko/build/outputs/apk/debug/odaiko-debug.apk

Requirements: JDK 17, Android SDK Platform 35, NDK 27.2.12479018,
CMake 3.22.1, internet connectivity for first Gradle dependency fetch.
Android Studio can open this folder as a Gradle project.
See odaiko/README.md for features and DSP validation limits.
EOF
printf 'Standalone source prepared: %s\n' "$OUT"
