# tmp-android

Android 9 (API 28) 以降で動く、最小の Hello World APK サンプルです。

## 仕様

- 表示: `Hello world!`
- `minSdk 28`: Android 9 以上
- `targetSdk 35`
- `compileSdk 35`
- Java + Android framework API のみ
- AndroidX / AppCompat なし
- Android Studio 不要で GitHub Actions から APK を生成

> 「Android 9 以降で動く」を `minSdk 28` として設定しています。targetSdk は Android 9 に固定せず、API 35 にしています。

## APK の生成

GitHub Actions の **Build Android APK** workflow が push / pull request / manual dispatch で実行されます。

生成物:

```
app/build/outputs/apk/debug/app-debug.apk
```

Actions artifact 名:

```
tmp-android-debug-apk
```

## テスト

workflow では次を確認します。

1. APK をビルド
2. APK manifest から `minSdk=28`, `targetSdk=35`, launchable Activity を確認
3. Android 9 / API 28 エミュレータへ APK をインストール
4. MainActivity を起動
5. UI Automator で画面上の `Hello world!` を確認

## ローカルビルド

JDK 17、Android SDK 35、Gradle 8.9 がある環境では次でビルドできます。

```sh
gradle --no-daemon :app:assembleDebug
```
