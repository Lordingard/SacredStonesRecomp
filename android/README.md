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

### Version 0.1.12 - Saves, Back And Display

The signed 0.1.12 APK (version code 11) adds **Export saves** to the setup
screen, the in-game **Assist Tools** menu, and a dynamic **Saved games** launcher
shortcut. The shortcut opens setup without starting
the game, even when skip-launcher is enabled. Dedicated-console launchers may
not expose dynamic shortcuts; the in-game action remains available even when
setup is skipped. The game Activity stays paused while choosing a destination
and exporting; dismissing the result returns to the in-game menu.

One press of Android Back opens the menu; while open, Back navigates back.
Hardware `KEYCODE_BACK` is intercepted before SDL's controller processing.
Android 13+ additionally registers `OnBackInvokedCallback` on the game Activity,
with explicit manifest opt-in. It is registered on resume and removed on pause,
so document pickers and the export Activity retain their own Back navigation.
Android 9-12 keep the legacy handlers without loading API 33 types.
The controller Back/Select button remains GBA Select, not a menu shortcut.
Right-stick click (R3) also toggles the menu on Android; Guide is still supported
where Android delivers it. Home remains a system action. The Ayn Thor's actual
hardware Back access was confirmed working by the maintainer in test build 10.

Build 11 separates display-cutout insets from touch/gesture insets. Native 3:2
presentation fits the largest unobstructed display area, without reserving
invisible gesture bands above/below the game. Touch controls retain the larger
safe area. Aspect-related side bands remain; no stretching or cropping is added.
Synthetic-drawable tests exercise the actual Android presentation branch for
phone, tablet, handheld and portrait surfaces, including cutout protection and
touch mapping. The maintainer accepted the visual correction after reporting
the size issue on Pixel 9 Pro, Xiaomi Pad 5 and Ayn Thor.

The player chooses a ZIP destination through Android's document picker. The
archive contains `backup.json`, the 32 KiB battery save (if present), slots 1-9
and the automatic suspend state. ROM, BIOS, preferences and unrelated files are
excluded. Export is read-only: canonical paths and file sizes are checked, and
changes during copying fail the export. The archive is staged privately before
writing the selected destination. A destination-write failure may leave an
incomplete document; only the app-created temporary file is automatically removed.
Do not use a failed export as a backup.

Host tests verify exact bytes, unchanged originals, exclusion of ROM/BIOS and
other files, state-only export, empty/invalid/oversized data, and write failure.
Host menu tests compile the actual patched event mapper and check R3 press/release,
Back routing and preserved Select. Display, touch geometry, FE8 transitions and
PPU smoke tests also pass. The ARM64 APK passes signing, certificate pinning and
package checks; patches apply with matching hashes to a fresh pinned checkout.
The maintainer confirmed working save export on the Pixel 9 Pro and modern Back
on the Ayn Thor. The launcher shortcut still requires device acceptance.
Host tests use a dispatcher test double to verify one action per modern Back,
callback removal and the pre-Android-13 guard; they do not simulate Thor firmware.
No import is included yet. Keep the original app installed and keep existing
backups; this release does not promise cross-version or cross-platform save-state
compatibility. Latest build log: `build/android-display-fit-test-build.log`.

```powershell
$jdk = 'build/android-tools/jdk/jdk-17.0.20.1+1/bin'
New-Item -ItemType Directory -Force build/android-save-export-tests/classes
& "$jdk/javac.exe" -d build/android-save-export-tests/classes `
    android/app/src/main/java/org/gbarecomp/SaveArchive.java tests/AndroidSaveArchiveTest.java
& "$jdk/java.exe" -cp build/android-save-export-tests/classes AndroidSaveArchiveTest `
    build/android-save-export-tests/fixtures
```

Initial devices: Google Pixel 9 Pro, Xiaomi Pad 5, and Ayn Thor. Test the setup
screen, normal boot, map and combat, touch and controller input, internal
save/reload after restarting, save states, rewind, audio, and background/resume.
The prototype uses a single landscape surface; a second screen is out of scope.

Internal saves use `files/saves/SacredStonesRecomp.sav` inside the app's private
storage. Uninstalling the app removes that storage. Do not uninstall to update
or clear data with valuable saves; export a backup first. Save states
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
The APK also carries the engine, UI, SDL2, ImGui, ARM core and Android NDK
license notices. Version 0.1.12 publishes the exact APK validated as test build 11;
its SHA-256 is `74d0a4d013c5d5acc7a83dd12e04eeae285f7d008606d874e5166657a719a150`.

Windows and Android keep separate dependency pins and validation. Windows's
complete persistent-control and save-safety feature set is not assumed to apply
to Android merely because they share an upstream engine. Android save export
is included in 0.1.12; users must not uninstall to update.

## Production Signing

Version 0.1.13 (version code 12) aligns the release number with Windows.
All three ARM64 native libraries are byte-identical to the validated 0.1.12 APK;
Android dependency pins and gameplay code are unchanged. It retains the same
production certificate and signing lineage. This package has automated build
and signature checks; the previous three-device gameplay validation applies to
the unchanged runtime, not a new manual installation test.

First generate game/BIOS sources with `scripts/build-android.ps1`, then use:

```powershell
pwsh scripts/package-android-release.ps1 -Version 0.1.13 -VersionCode 12
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
Use `-SigningDirectory` if the existing protected signing material is stored
outside the default directory. Do not regenerate a key to work around an access
problem; keep signing material outside Git and keep a portable encrypted backup.
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
cmake --build build/android-render-tests --target android_game_fit_tests android_menu_tests android_pad_tests android_display_tests fe8_transition_tests ppu_smoke_tests --parallel 2
```
