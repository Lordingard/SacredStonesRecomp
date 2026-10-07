# Windows Fullscreen And Save Export - 0.1.13

Final runtime revision: `57a2f66e25b9825c2e46a5df853dcec23801b699` on the project's
`sacred-stones-runtime` fork (initial candidate: `2888a49`).
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

At this initial candidate stage, manual Windows acceptance was pending.
Automated startup checks do not exercise the native destination picker.

## Follow-Up - 2026-10-07

The maintainer accepted borderless fullscreen but reported extra borders in
exclusive mode. The local runtime patch now explicitly selects the desktop mode
for freely resizable games before entering exclusive fullscreen, rather than
letting SDL choose a lower mode from the windowed dimensions.

Sixteen runner scenarios passed under
`build/validation/010b5ae39bd3446aa31b9a730c6b2e32`; the exclusive case reported
both the requested and actual mode as 1920x1080 at 165 Hz. The generation
provenance was refreshed for these local changes, with the same pinned revision
plus its modified-source digest. Exclusive sizing was subsequently accepted
by the maintainer, as recorded below.

The executable now embeds icon resource 101 containing the original Android
256x256 PNG verbatim. Windows resource checks confirm its exact bytes and
successful loading at 16, 32 and 256 pixels. SDL's big/small icon resource hints
are set before either launcher or game window is created.

The final regenerated build also passed all sixteen scenarios from the staged
distribution under `build/validation/9476cb4270a340a4b1b0180bc138ba40`, recorded
in `build/windows-exclusive-package.log`. The ZIP was refreshed after these
checks; its executable matches the final build and its SHA-256 sidecar matches.

## Native Export Dialog Follow-Up

The maintainer accepted exclusive fullscreen sizing, then reported that its
native export picker became minimized with the game. The game-owned export
callback now temporarily exits exclusive mode, restores/raises the SDL window,
uses its explicit native HWND as owner and keeps the picker and confirmation
inside a scoped transition. Scope exit restores exclusive mode without writing
the fullscreen preference. Windowed and borderless modes are left unchanged.

The actual SDL transition test passes for all three modes, including scope exit
after simulated cancellation and unchanged desktop resolution/refresh rate.
This tests the transition, not interaction with the native destination picker;
manual acceptance was pending at that point.

All sixteen staged runtime scenarios also pass for the dialog fix under
`build/validation/47c1d1880a9f4b7fb72e690843de7362`, recorded in
`build/windows-export-dialog-package.log`. The refreshed Windows ZIP contains
the current executable and its SHA-256 sidecar matches.

## Release Acceptance - 2026-10-07

The maintainer confirmed correct borderless and exclusive image sizes and
successful save export in exclusive fullscreen, then authorized commit and
publication. A separate manual cancellation test was not reported; cancellation
and restoration remain covered by the automated transition test. Full-game
coverage and cross-platform save-state compatibility are not claimed.

For publication, generation was repeated against the clean final runtime commit
and the runner rebuilt using the normal project scripts. Logs are
`build/windows-0.1.13-publication-generate.log` and
`build/windows-0.1.13-publication-build.log`. The private signing-backup tool
changes are intentionally excluded from this release's source commits.

The final staged package passed all sixteen scenarios under
`build/validation/28b9bc17af8a4bffb2cc9f6446178dd7`, recorded in
`build/windows-0.1.13-publication-package.log`. Native dialog transitions, the
three project test programs and Windows PowerShell 5.1 exports also passed.
Final ZIP inspection confirmed its root executable matches the built executable,
includes the export helper, excludes private files and has a matching checksum.
