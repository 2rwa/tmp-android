#!/usr/bin/env bash
# Standalone source packaging. Other repository modules are intentionally excluded.
set -euo pipefail
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DEST="${1:?usage: assemble-standalone.sh DESTINATION_DIRECTORY}"
mkdir -p "$DEST/nativeglobe"
cp "$REPO_ROOT/build.gradle" "$DEST/build.gradle"
cp "$REPO_ROOT/gradle.properties" "$DEST/gradle.properties"
cp "$REPO_ROOT/nativeglobe/build.gradle" "$DEST/nativeglobe/build.gradle"
cp "$REPO_ROOT/nativeglobe/README.md" "$DEST/nativeglobe/README.md"
cp -R "$REPO_ROOT/nativeglobe/src" "$DEST/nativeglobe/src"
cat > "$DEST/settings.gradle" <<'SETTINGS'
pluginManagement {
    repositories { google(); mavenCentral(); gradlePluginPortal() }
}
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories { google(); mavenCentral() }
}
rootProject.name = 'native-glass-globe-android9'
include ':nativeglobe'
SETTINGS
cat > "$DEST/README.md" <<'DOC'
# Native Glass Globe — standalone Android 9 source

This is a **standalone Gradle project**, not a complete copy of tmp-android.
Only the :nativeglobe module is included; :app and :jpegviewer are not required.

## Building on macOS / Linux

Install JDK 17 and Android SDK 35, NDK 27.2.12479018 and CMake 3.22.1.
Set ANDROID_HOME or sdk.dir in local.properties. Open the extracted project
root in Android Studio, or run:

  chmod +x gradlew
  ./gradlew :nativeglobe:assembleDebug

The included Gradle Wrapper pins Gradle 8.9, which is used in CI with Android
Gradle Plugin 8.7.3. Avoid invoking an installed Gradle 9.x binary.

APK: nativeglobe/build/outputs/apk/debug/nativeglobe-debug.apk
Requires Android 9 (API 28) or later and a Vulkan-compatible device.
DOC
printf 'Standalone Gradle source created: %s\n' "$DEST"
