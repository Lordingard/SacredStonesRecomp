# Windows Fullscreen And Save Export - 0.1.13 Candidate

Runtime revision: `2888a49` on the project's `sacred-stones-runtime` fork.
The launcher pin and Android dependency lock are unchanged.

The Windows game and BIOS were regenerated and built using the normal project
scripts with two compilation jobs. Generation provenance verification passed.
The fifteen packaged runtime scenarios passed under
`build/validation/3999e2f0ddf14dc6905a4eb574052a46`; the packaging log is
`build/windows-0.1.13-package.log`.

The save-file, launcher-policy and runtime-control test programs passed in
isolated directories. The export helper passed under Windows PowerShell 5.1,
including state-only archives, slot 9, suspend snapshots, preserved originals
and failure-safe replacement of an existing ZIP.

The Windows archive contains twenty entries, including its root executable and
`tools/export-windows-saves.ps1`. It contains no ROM, BIOS dump, player data,
runtime cache, credentials or generated source.

The Android APK uses version code 12 and the existing production certificate.
All three ARM64 native libraries match the validated 0.1.12 APK byte for byte.
No new device installation test has been claimed.

Before publication, the maintainer should verify the Windows menu layout,
fullscreen persistence after restart, and a real save export. Automated startup
checks do not exercise the native destination picker or replace that acceptance.
