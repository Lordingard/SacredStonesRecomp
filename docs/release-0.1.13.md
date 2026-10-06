# v0.1.13 - Windows Fullscreen And Save Export

## Downloads

- **SacredStonesRecomp-0.1.13-win64.zip**: extract and run the executable at the
  archive root. Keep its DLLs, assets and tools folders together.
- **SacredStonesRecomp-0.1.13-android-arm64.apk**: Android 9+ and ARM64.
- The corresponding `.sha256` assets contain download checksums.

The source-code archives are not playable packages. Neither package includes
the ROM or GBA BIOS; provide your own legally obtained USA ROM and BIOS.

## Windows

- Remember fullscreen across restarts, whether changed in the launcher,
  **Escape > Display > Fullscreen**, or using **Alt+Enter**.
- Borderless fullscreen is recommended. Windowed and exclusive modes remain
  available; an explicit command-line fullscreen mode overrides startup preferences.
- Preserve the original 3:2 aspect ratio. Side bands on wider displays are normal.
- Add **Assist Tools > Export saves**, with a native ZIP destination picker.
  Flush the current battery save before exporting and pause gameplay during export.
- Export the battery save, existing state slots 1-9, any suspend snapshot and
  backup manifest, never the ROM, BIOS or settings. Original saves remain intact.
- Use Windows' built-in PowerShell export helper, included under `tools/`.
  Failed exports preserve an existing destination archive.

## Android

The release number is aligned with Windows. Android retains the gameplay,
display fitting, modern Back handling and export features validated in 0.1.12.
Its engine and launcher dependencies remain independently pinned; this is not
a Windows runtime migration. The existing production signing key is retained.

## Updating

Close the game before updating. On Windows, extract over the existing installation
and retain your saves and settings. On Android, install over the existing app;
do not uninstall or clear its data. Back up saves first and keep the ZIP separately.

Archive import is not implemented. Save-state compatibility across platforms or
runtime versions is not guaranteed. Full-game testing remains incomplete.

## Automated Validation

- Regenerated Windows game/BIOS code with the recorded runtime revision;
  generation provenance matches dependency sources and inputs.
- Fifteen isolated runtime scenarios pass, including SRAM reload, save paths,
  locked-save recovery, input, diagnostics, preferences, remembered borderless
  fullscreen and explicit windowed overrides.
- Save-file, launcher-policy and runtime-control test programs pass.
- Windows PowerShell 5.1 export tests cover exact content, unchanged originals,
  state-only archives, slot 9, suspend snapshots, invalid/empty storage and locked ZIPs.
- Package inspection verifies the executable at the ZIP root, current export
  helper and exclusion of private game files and player data.
- Android production signature verified; all ARM64 native libraries match 0.1.12.

Manual Windows menu/export and fullscreen restart acceptance is still pending;
these automated checks do not substitute for gameplay validation.
