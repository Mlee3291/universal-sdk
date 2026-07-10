# Implementation Plan - Build Android APK for Universal SDK

This plan outlines the steps required to transform the current C++ library into a functional Android APK. This involves connecting the core SDK logic to its platform implementation, adding a JNI bridge, and setting up a Gradle-based Android project structure.

## User Review Required

> [!IMPORTANT]
> This plan will introduce a Gradle-based Android project structure to the repository. This includes adding `build.gradle` files, `AndroidManifest.xml`, and a small Kotlin wrapper.

> [!WARNING]
> The `UniversalSDK` class currently has placeholder implementations for `ExecuteCommand` and `GetSystemInfo`. I will refactor it to correctly delegate these calls to the `PlatformImpl` interface.

## Proposed Changes

### Core C++ Refactoring

#### [MODIFY] [universal_sdk.h](file:///C:/Users/micha/OneDrive/Documents/GitHub/universal-sdk/include/universal_sdk.h)
- Add `std::unique_ptr<PlatformImpl> impl_` private member to `UniversalSDK`.
- Include `<memory>` if not already present.

#### [MODIFY] [universal_sdk.cpp](file:///C:/Users/micha/OneDrive/Documents/GitHub/universal-sdk/src/universal_sdk.cpp)
- Update `Initialize()` to instantiate `impl_` using `CreatePlatformImpl()`.
- Implement `ExecuteCommand()` and `GetSystemInfo()` by delegating to `impl_->ExecuteCommand()` and `impl_->GetSystemInfo()`.

### JNI Layer

#### [NEW] [android_jni.cpp](file:///C:/Users/micha/OneDrive/Documents/GitHub/universal-sdk/src/platform/android_jni.cpp)
- Implement JNI entry points for `Initialize`, `GetSystemInfo`, and `ExecuteCommand`.

### Android Project Setup

#### [NEW] [settings.gradle](file:///C:/Users/micha/OneDrive/Documents/GitHub/universal-sdk/settings.gradle)
- Define the project and include the `app` module.

#### [NEW] [build.gradle](file:///C:/Users/micha/OneDrive/Documents/GitHub/universal-sdk/build.gradle)
- Top-level build script with dependencies for the Android Gradle Plugin.

#### [NEW] [app/build.gradle](file:///C:/Users/micha/OneDrive/Documents/GitHub/universal-sdk/app/build.gradle)
- Configure the Android application, NDK integration (pointing to the root `CMakeLists.txt`), and dependencies.

#### [NEW] [AndroidManifest.xml](file:///C:/Users/micha/OneDrive/Documents/GitHub/universal-sdk/app/src/main/AndroidManifest.xml)
- Basic manifest defining the application and `MainActivity`.

#### [NEW] [MainActivity.kt](file:///C:/Users/micha/OneDrive/Documents/GitHub/universal-sdk/app/src/main/java/com/example/universalsdk/MainActivity.kt)
- A simple activity that loads the native library, initializes the SDK, and displays system information on the screen.

### Build System

#### [MODIFY] [CMakeLists.txt](file:///C:/Users/micha/OneDrive/Documents/GitHub/universal-sdk/CMakeLists.txt)
- Ensure compatibility with Gradle by adding the JNI source file when building for Android.

## Verification Plan

### Automated Tests
- I will verify the C++ compilation by running the `build.sh` script (after updates).
- I will attempt to trigger a Gradle build to produce the APK.

### Manual Verification
- The user can verify the resulting APK by installing it on an Android device or emulator and checking if the "Android Platform Information" is displayed correctly.
