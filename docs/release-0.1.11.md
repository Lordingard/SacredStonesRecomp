# v0.1.11 - First Android Release

## Download

Install **SacredStonesRecomp-0.1.11-android-arm64.apk** from Assets.
The source-code archives are not playable packages. The `.apk.sha256` asset
contains the download checksum.

Windows users should keep using [v0.1.10](https://github.com/Lordingard/SacredStonesRecomp/releases/tag/v0.1.10).
This release adds Android and does not change the Windows runtime or launcher.

## Android

- Native ARM64 static recompilation, Android 9 or newer.
- Setup screen imports and verifies your own USA ROM and GBA BIOS.
- Touch controls and physical controller support, including controller-based
  automatic virtual-pad hiding when no manual preference is saved.
- Aspect-correct 3:2 screen fitting and thumb-accessible Start/Select at the bottom.
- In-game settings and Assist Tools: save/load state slots, fast-forward, rewind.
- Internal SRAM save and automatic suspend/resume, separate from save states.
- Eirika launcher icon and dedicated production signature.

The maintainer reports functional gameplay on **Google Pixel 9 Pro, Xiaomi Pad 5,
and Ayn Thor**, plus successful save/load state testing through the menu.
The Ayn Thor uses one landscape screen; dual-screen support is not included.
The game has not been exhaustively tested from start to finish or on every device.

## Updating The Test Builds

Close the game and install the APK **over the existing app**. Do not uninstall
or clear app data: either operation removes private saves, states, and imports.
The production certificate includes a signing lineage from the project's local
experimental APKs. The Pixel accepted a direct test-6 to production update, and
the maintainer confirmed resuming the previous position and loading the previous
save state afterward. Updates on the other two devices remain to be checked.
If Android refuses an update, report the error instead of uninstalling.

The APK is signed and non-debuggable. GitHub installation can still trigger
unknown-app or Play Protect prompts; signing does not guarantee their removal.
There is no in-app save export yet.

## Validation

- Release certificate pinned and signature verified for Android APIs 28, 31, 35.
- APK contains no ROM/BIOS dumps, saves, generated source, or local configuration.
- Native libraries checked for ARM64 and 16 KiB ELF segment alignment.
- Six negative package tests, native screen/touch mapping tests, actual virtual
  pad geometry/hit-region tests, five FE8 transition cases and upstream PPU tests.
- All local engine patches applied with matching hashes on a fresh pinned checkout.
- Windows dependency pins unchanged.

You must supply your own legally obtained ROM and GBA BIOS. No game dump or
BIOS dump is included. Android build/signing instructions and detailed validation
notes are in [android/README.md](https://github.com/Lordingard/SacredStonesRecomp/blob/main/android/README.md).
