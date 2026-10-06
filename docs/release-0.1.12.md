# v0.1.12 - Android Save Export And Display Fixes

## Download

Install **SacredStonesRecomp-0.1.12-android-arm64.apk** from Assets.
The source-code archives are not playable packages. The `.apk.sha256` asset
contains the download checksum. Requires Android 9+ and ARM64.

Windows users should keep using [v0.1.10](https://github.com/Lordingard/SacredStonesRecomp/releases/tag/v0.1.10).
This release does not change the Windows runtime or launcher.

## Changes

- **Export saves** from setup or **Assist Tools > Export saves** in the game.
  Export remains accessible when the setup launcher is skipped.
- Choose a ZIP destination using Android's document picker. The game stays
  paused while exporting; the original saves are not modified.
- Archives include the battery save, existing save-state slots 1-9, automatic
  suspend snapshot, and backup manifest. ROM, BIOS and preferences are excluded.
- **Saved games** app shortcut opens setup without automatically starting the
  game, on launchers that support shortcuts. Availability varies by launcher.
- Modern Android 13+ Back handling, with legacy support for Android 9-12.
  One press of Ayn Thor's physical Back button now opens the in-game menu.
  Controller Select remains GBA Select; Home remains a system action.
- Larger aspect-correct image: touch-gesture margins no longer shrink the game
  viewport. Display cutouts remain protected, as do the virtual controls' own
  touch-safe margins. Side bands remain necessary on wider-than-3:2 screens.

## Updating And Backups

Close the game and install the APK **over the existing app**. Do not uninstall
or clear app data; those operations remove private saves, states and imports.
The app uses the existing production certificate and signing lineage.

Wait for **Saves exported** before treating a ZIP as a backup. An export failure
can leave an incomplete destination file. There is no archive import yet, and
save-state compatibility across platforms or runtime versions is not guaranteed.
Keep the original app and its data intact.

## Validation

The maintainer confirmed export on **Pixel 9 Pro**, physical Back on **Ayn Thor**,
and accepted the display correction after reporting it on Pixel 9 Pro,
**Xiaomi Pad 5** and Ayn Thor. Gameplay was already validated on these devices.
This is not exhaustive start-to-finish testing or support for every device.

- Save archives tested for exact bytes, unchanged originals, ROM/BIOS exclusion,
  state-only export, invalid sizes, empty storage and write failure.
- Actual native menu mapper and display branch tested, along with pad geometry,
  touch-coordinate mapping, cutout protection, FE8 transitions and PPU smoke tests.
- Modern Back registration/removal and legacy guards tested with a dispatcher
  test double; actual Ayn Thor button access confirmed by the maintainer.
- Local Android engine patches apply to a fresh pinned checkout with matching hashes.
- APK certificate pinned; signatures verified for Android APIs 28, 31 and 35.
- ARM64/16 KiB native alignment and exclusion of private assets checked.
- Publishes the exact signed APK validated as test build 11 (version code 11).

The APK contains no ROM or BIOS dump. Supply your own legally obtained USA ROM
and GBA BIOS. See the [README](https://github.com/Lordingard/SacredStonesRecomp#quick-start---android)
for installation and controls.
