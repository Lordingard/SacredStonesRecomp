# Android

This is an isolated ARM64 target. Version 0.1.11 is the first public Android
release, following maintainer testing on Pixel 9 Pro, Xiaomi Pad 5, and Ayn Thor.
The Windows runner and its pinned submodules are unchanged. Android initially
uses separate pinned upstream checkouts under `build/android-deps`, selected
from WarioWareTwistedRecomp's shared mobile infrastructure. Their identities
are recorded in `dependencies.json`; never silently track upstream main.
The Android-only Gradle version-code, FE8 transitions, display, and touch fixes are
applied from tracked patches; the script checks the resulting file hashes and
rejects unrelated engine edits.

On 2026-10-06, the first ARM64 debug APK built successfully with JDK 17,
NDK r27b and the pinned dependencies. Signature, application identity, private
asset exclusion, and 16 KiB native-library alignment checks passed. Six negative
package tests and patch application on a fresh engine checkout also passed.
Build evidence is in `build/android-prototype-build-retry.log`.
There were upstream/compiler warnings; this was not a warning-free build.
The first Pixel 9 Pro test exposed a bootstrap integration error: the shared
mobile setup adds `--no-launcher`, but the runner originally passed it straight
to the runtime's strict argument parser. Experimental build 2 calls the shared
launcher seam first, which consumes that flag without opening another launcher.
Its signed APK passed the package checks and was installed as an in-place update
after a verified private-data backup. On 2026-10-06, the Pixel log confirmed
`cpu_backend=static-recompiled`, and a screenshot confirmed the opening dialogue
with the virtual gamepad visible. Evidence is in `build/android-test2-build.log`,
`build/android-pixel-test2-runtime.log`, and `build/android-pixel-test2.png`.
Later build-3 user testing covered gameplay successfully on all three devices;
this is user acceptance, not an automated gameplay test.

Experimental build 3 restores the Windows fork's reviewed distinction between
ordinary first-target OBJ pixels and semitransparent OBJ pixels. Without it,
ordinary sprites ignored brightness and alpha fades, appearing before or after
the background. The targeted test failed on the unpatched Android engine and
passed all five cases after correction; upstream `ppu_smoke_tests` also passed.
Both patches were verified on a fresh checkout. The signed ARM64 APK passed the
package checks and was installed on the Pixel with permission after a private
backup; the existing 32768-byte SRAM save hash was unchanged after the update.
Evidence is in `build/android-test3-build.log`. Visual transition acceptance
on device was confirmed by the maintainer. Build 3 was subsequently reported
fully functional on the Pixel 9 Pro, Xiaomi Pad 5, and Ayn Thor.

Experimental build 4 fits the native 3:2 image to the available safe display
area, without stretching or retaining the old reserved control margins. Touch
controls overlay the edges when the remaining side bands are too narrow.
With no saved manual preference, connecting a physical controller hides the
virtual pad and disconnecting the last controller restores it. The existing
gamepad toggle remains available and saves a manual shown/hidden preference;
that preference takes precedence over automatic detection. The supplied Eirika
PNG replaces the placeholder launcher icon. These changes are Android-only.
The signed ARM64 build passed package validation (no ROM/BIOS or saves, 16 KiB
native alignment), six negative package tests, presentation/touch-coordinate
tests, FE8 transition tests, and upstream PPU smoke tests. All three patches
also applied with matching hashes to a fresh engine checkout. Build evidence:
`build/android-test4-build.log`. Display and controller hotplug behavior still
require device acceptance. Install over the existing app; do not uninstall.

Experimental build 5 replaces the anydpi bitmap declaration with an Android
adaptive icon and a nodpi legacy fallback, both using the supplied Eirika PNG.
Select sits just inside the D-pad and Start just inside the A/B cluster in
landscape, rather than in the center of the display. Tests execute the engine's
actual pad geometry and hit testing on four representative screen/density
combinations, checking thumb reach and exclusive Start/Select activation.
Device acceptance is still required for icon appearance and comfort.

Experimental build 6 keeps build 5's left/right positions for Select and Start
but restores their original bottom alignment in landscape. The pad geometry
tests also check bottom alignment and exclusive button activation.

The maintainer reports that saving appears functional and closing/reopening
resumes the current position. Automatic suspend-state restoration is enabled
by `resume_suspend_state_on_launch`; this is separate from the game's SRAM save.
Validate the game's own Load Game path too, rather than treating automatic
resume alone as proof of cartridge-save reload.

The shared engine supplies the Android setup screen, document picker, private
storage, SDL2 activity, virtual gamepad, and runtime menu. No gyro or dual-screen
feature is enabled. The game and BIOS are regenerated with the Android-pinned
generator using our reviewed FE8 annotations and imported decompilation symbols.
The Android build never reuses a Windows archive or Windows-generated corpus.

## Build

Requirements: JDK 17, Android SDK platform 35, build-tools 35.0.0, NDK
27.1.12297006, Android CMake 3.22.1, host CMake/Ninja/MinGW, Git, and Python.
The script uses project-local tools under `build/android-tools` when present,
or accepts `-JavaHome` and `-SdkRoot`. It does not modify system settings.

```powershell
pwsh scripts/build-android.ps1 -BiosPath D:/path/to/gba_bios.bin `
    -RomPath F:/path/to/FireEmblem.gba -PythonPath C:/Python314/python.exe
```

Compilation is limited to two jobs by default. The APK is debug-signed for
local testing and requires Android 9 or newer on ARM64. It contains neither
ROM nor BIOS. On first launch, select your own matching USA ROM and retail
GBA BIOS. The setup screen verifies and imports them into app-private storage.

Game execution remains on the static recompilation path: unlike WarioWare's
Android workaround, this target does not force the interpreter or HLE BIOS.
The engine reports missing dispatches rather than attempting compilation on
the device. A successful build is not proof that this path works on hardware.

## Validation

Initial devices: Google Pixel 9 Pro, Xiaomi Pad 5, and Ayn Thor. Test the setup
screen, normal boot, map and combat, touch and controller input, internal
save/reload after restarting, save states, rewind, audio, and background/resume.
The prototype uses a single landscape surface; a second screen is out of scope.

Internal saves use `files/saves/SacredStonesRecomp.sav` inside the app's private
storage. Uninstalling the app removes that storage. Do not uninstall to update
or test with valuable saves until backup/export has been validated. Save states
remain beside the imported ROM in private storage. Desktop persistent controls
are not yet promised to have equivalent Android behavior.

Enable USB debugging and authorize the development PC only when testing.
Shared validation tooling is in
`build/android-deps/gbarecomp/platform/android/tools/`. No device changes or
installation are performed by the build script.

The release is non-debuggable and signed with a dedicated RSA-3072 key. Its v3
signing lineage permits in-place updates from the project's local test builds on
Android 9+. Signature checks cover APIs 28, 31 and 35. A verified private-data
backup preceded the successful test-6 to production update on the Pixel; Android
accepted the lineage and kept the same app identity. This does not replace
manual verification of gameplay and saves after the update.
The maintainer subsequently confirmed normal startup at the previous suspended
position and successful loading of the pre-update save state on the Pixel.
Release packaging evidence is in `build/android-release-0.1.11-final.log`.
The final APK also carries the engine, UI, SDL2, ImGui, ARM core and Android NDK
license notices. Native libraries remain identical to the validated test 6.

Windows and Android keep separate dependency pins and validation. Windows's
complete persistent-control and save-safety feature set is not assumed to apply
to Android merely because they share an upstream engine. Android save export
remains a future task; users must not uninstall to update.

## Production Signing

First generate game/BIOS sources with `scripts/build-android.ps1`, then use:

```powershell
pwsh scripts/package-android-release.ps1 -Version 0.1.11 -VersionCode 7
```

Release packaging intentionally reuses the generated Android corpus, builds
the non-debuggable Release variant with two native jobs, signs with the reviewed
lineage, validates signatures and package contents, and emits APK plus SHA-256
under `dist/android/`. Local tests still use the Debug APK.

Signing material is outside the repository at
`$env:LOCALAPPDATA/SacredStonesRecomp/signing`, restricted to the Windows user
and SYSTEM. `-InitializeSigning` is a one-time operation for an empty signing
directory; never use it to replace an established public release key.
The password is stored using Windows DPAPI; its encrypted credential file alone
cannot be restored under a different Windows account or installation.

Create a portable backup on private offline storage with:

```powershell
pwsh scripts/backup-android-signing.ps1 -Destination X:/Private/SacredStonesSigning
```

The helper asks for a new backup password locally, exports an encrypted portable
keystore and the signing lineage, and never prints passwords. Keep the password
in a password manager separately from the backup. Private keys and credentials
must never enter Git, logs, GitHub secrets output, or release assets.
The public production certificate SHA-256 is recorded in `release-signing.json`.
APK signatures do not guarantee elimination of unknown-source/Play Protect
prompts; see https://developer.android.com/tools/apksigner.

Reference implementation:
https://github.com/mstan/WarioWareTwistedRecomp/tree/405b030197136fa048be370874fb1c91498f8cc5
Shared engine and launcher retain their own license and attribution notices.

Targeted render checks can be built independently of the Windows runner:

```powershell
cmake -S tests/android-render -B build/android-render-tests -G Ninja `
    -DGBARECOMP_ROOT="$PWD/build/android-deps/gbarecomp" `
    -DCMAKE_BUILD_TYPE=Release
cmake --build build/android-render-tests --target android_pad_tests android_display_tests fe8_transition_tests ppu_smoke_tests --parallel 2
```
