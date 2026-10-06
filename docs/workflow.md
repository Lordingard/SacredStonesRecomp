# SacredStonesRecomp workflow

This repository keeps only reproducible project metadata and scripts.

It must not contain:

- the Fire Emblem: The Sacred Stones ROM;
- generated GBARecomp C++;
- generated static libraries or build directories;
- local FireEmblemUniverse/fireemblem8u checkouts.

## First bootstrap

```powershell
pwsh scripts/bootstrap.ps1 -WriteLocalConfig
```

The default local paths are:

- GBARecomp generator: built locally from `extern/gbarecomp` into `build/generator-mingw`
- FE8 US ROM: sibling of this repo, `..\Fire Emblem - The Sacred Stones (U).gba`

Configure ROM, BIOS, and MinGW paths with parameters, environment variables, or
`config/project.local.ps1`. Source generation requires Python 3. External
`GBARECOMP_EXE` and legacy `GbaRecompExe` local settings are no longer used.

## Runtime Dependency Updates

`extern/gbarecomp` is a pinned submodule that points to the Sacred Stones runtime
branch on the project fork:

```powershell
git submodule update --init --recursive extern/gbarecomp
```

The fork tracks upstream `mstan/gbarecomp` through the `sacred-stones-runtime`
branch. To refresh it, update that branch in `Lordingard/gbarecomp`, rebuild this
project, verify boot and internal saves, then bump the submodule pointer in this
repository.

`extern/recomp-ui` remains a separate pinned submodule on the project fork's
`sacred-stones-launcher` branch so launcher UI updates can be evaluated
independently from runtime changes. Its upstream is `RetroPortingToolKit/recomp-ui`
(formerly `mstan/recomp-ui`). Retain the required-BIOS prompt and input-preference
fixes when updating that fork.

## Generate and build

```powershell
pwsh scripts/generate.ps1 -Force
pwsh scripts/build.ps1
```

Or do both, then build the launcher runner:

```powershell
pwsh scripts/generate.ps1 -Force
pwsh scripts/build-runner.ps1
```

The generated project is written to `.generated/gbarecomp/`. The Windows runner
uses its MinGW library, `build-mingw/libgbarecomp_game.a`. `build-runner.ps1`
requires `BiosPath` (or `GBA_BIOS`) and regenerates BIOS output using the pinned
`extern/gbarecomp/bios/gba_bios.toml`. Omitting this configuration loses interrupt
return entries and caused the v0.1.6 startup crash. Generated files stay ignored.
Build parallelism defaults to two jobs; use `-Jobs` to change it.
The runner and generated game library both default to optimized `Release`
builds. Use `-BuildType RelWithDebInfo` for optimized debugging or
`-BuildType Debug` for an unoptimized debug build. Changing the build type
rebuilds the generated library as well as the runner.

The generator is built from the same pinned framework as the runtime.
Generation records both framework revisions, source fingerprints, and ROM,
annotation, and symbol-input hashes in the ignored
`.generated/gbarecomp/sacredstones-generation.json`. Runner builds reject changes
to these inputs until the game is regenerated. Full static resume entries are
enabled in `config/game.fe8u.toml` to cover frame-boundary and interrupt returns.

## Release validation

Run `pwsh scripts/test-release.ps1 -ExtendedInput` for isolated boot, SRAM-file
reload, save overrides, locked-save recovery, and input tests.
`scripts/package-release.ps1 -Version 0.1.9` runs these tests against its staged
files before creating the archive. Both accept `-BiosPath` and `-RomPath`.
Each run preserves logs and disposable saves in a unique `build/validation/`
directory, with developer compiler directories removed from the child PATH.

The `-ExtendedInput` test presses menu buttons for 3,600 headless frames and
enforces static coverage. `-WindowedInput` adds a 1,200-frame windowed replay.
Use `scripts/test-generation-provenance.ps1` to test stale-generation rejection.
Build target `sacredstones_save_file_tests` and run it with an isolated directory
argument to test native file replacement, including a locked destination.
These checks do not certify slot contents, combat, rewind, or physical controller
behavior. Check those manually before publishing.

### Optimized Build Validation (2026-10-05)

The default Release configuration was validated against the unchanged runtime
and launcher pins. Both the runner and generated game library use `-O3 -DNDEBUG`.
The previous executable was retained in `build/runner-mingw`; the optimized
candidate is in `build/runner-release`.

Passed: six isolated boot/save scenarios, 3,600 headless frames with menu input,
1,200 windowed frames with replayed input, native locked-file replacement and
recovery, and all four stale-generation rejection tests.

Three alternating runs of the same 1,200-frame headless boot scenario took
4.054/4.052/4.046 seconds for the previous executable and
1.226/1.215/1.243 seconds for the Release candidate. Median elapsed time fell
by about 70 percent. This measures unthrottled execution without display, not
in-game FPS or end-to-end gameplay correctness.

Regression logs: `build/validation/a3cf34ac84a047eda87ff6d0261f2ef5/`.
Benchmark logs and measurements:
`build/validation/benchmark-d8cd9065fdb04eeba4456d941d0f27d1/`.
Manual combat, in-game save slots, save states, rewind, and controller validation
remain required before publishing this candidate.

### Opt-In Diagnostic Integration (2026-10-05)

The initial optimized build was subsequently validated by the maintainer through
gameplay and fast-forward. The next candidate, `build/runner-perf`, applies only
upstream GBARecomp change `1ea864712561eed358882f48f53701ce4b078f79` and its
ARM generator dependency `763b922f4912708d2704e7833f18addfbf8ddf33` on top of the
existing project runtime. Game and BIOS code were regenerated. Launcher and CPU
timing updates are not included.

Runtime, MMIO, audio FIFO, frame-phase, and presentation-cadence capture, plus
the hang watchdog, are now opt-in. Normal gameplay avoids their capture work.
The overlay cache namespace changes from `abi4` to `abi5`; old caches are not
deleted but cannot be loaded into the new callback layout. Battery-save paths,
safe replacement, and the save-state layout are unchanged.

Passed: six boot/save scenarios, 3,600 headless frames with menu input, two
1,200-frame windowed replays (normal and diagnostic), and a headless diagnostic
run. Enabled diagnostic runs produced nonempty MMIO CSV files; the windowed run
also produced frame-phase and presentation-cadence CSV files. Four upstream tests
passed: color LUT, diagnostic capture disabled/enabled, and code generation.
Native save replacement/recovery and stale-generation rejection also passed.

Use `scripts/test-release.ps1 -ExtendedInput -WindowedInput -DiagnosticCapture`
to repeat this coverage. Packaging also runs the headless diagnostic scenario.
For manual diagnosis, set the needed flags before starting the executable:

- `GBARECOMP_RUNTIME_TRACE=1`
- `GBARECOMP_MMIO_DUMP=<csv path>`
- `GBARECOMP_AUDIO_FIFO_TRACE=1`
- `GBARECOMP_FRAME_PHASE=<csv path>`
- `GBARECOMP_PRESENT_CADENCE=1`
- `GBARECOMP_HANG_WATCHDOG=1`

Three alternating 1,200-frame headless boot runs took 1.214/1.213/1.227 seconds
for the initial Release build and 1.182/1.185/1.199 seconds for this candidate.
The roughly 2 percent median reduction is small and not a guaranteed gameplay
gain. Guest summary fields and SRAM hashes matched in all three comparisons;
this is not a full-state or full-game equivalence proof.

Regression logs: `build/validation/ebe6eb435fdd48d9ad40a42378bfc949/`.
Benchmark records: `build/validation/perf-benchmark-a34df0fab039463daab5214c90003ac2/`.
Upstream test results: `build/runner-perf/gbarecomp-core/upstream-performance-tests.xml`.
Manual gameplay, in-game save slots, save states, and rewind still need validation
on this new candidate before publication.

### Launcher BIOS And Input Follow-Up (2026-10-05)

The maintainer validated combat, battery saves, save states, and rewind on the
diagnostic-performance candidate. The follow-up candidate is in
`build/runner-launcher` and adds a Play-triggered BIOS picker when the host rejects
an empty or invalid BIOS path. Hosts that accept a BIOS fallback remain playable;
retail BIOS backend mismatches retain the existing regeneration prompt.

The GBA launcher seam persists `input_source` and `gamepad_guid` in the existing
executable-local INI file. Closing the launcher exports and saves edited settings
without booting. No transient SDL instance identifier or device handle is stored.
The runtime tries the preferred SDL GUID first, then any available controller;
keyboard input remains available. The preference survives disconnection. GBA
source labels are refreshed from live devices, and connection indicators now
reflect actual availability. SDL GUIDs identify device models, not individual
units of an otherwise identical model.

Target `sacredstones_launcher_policy_tests` checks controller-setting round trips,
invalid-setting sanitization, unrelated INI section preservation, and BIOS Play
gating. The release suite includes a windowed missing-controller fallback test.
All eleven release scenarios passed; generated-source provenance tests also passed.
An actual launcher run with an absent GUID confirmed settings survive Quit,
including when no BIOS path is configured. The maintainer subsequently validated
the native BIOS picker and input-device persistence successfully.

Game regression logs: `build/validation/6b81d926a0e3462b8c642b237b7dabb4/`.
Launcher screenshots and logs:
`build/validation/launcher-ui-6bb0d9464da84775943a67195fa6096f/`.

## v0.1.9 clean-build validation

On 2026-10-05, an isolated clone at
`F:/git/SacredStonesRecompTemp/v0.1.9-clean-5d4a2d5b` rebuilt the generator,
game library, BIOS code, runtime, and launcher without reusing generated code,
objects, or runtime caches. Recursive dependencies were fetched from GitHub:

- GBARecomp: `f8e213861e1880ff73250012fcfe67d4939dba6a`.
- arm-core: `763b922f4912708d2704e7833f18addfbf8ddf33`.
- recomp-ui: `2b0a36e07fef97e9de14d3477f0131ff0383cc3d`.

Both the generated game library and runner used Release optimization.
Generation-provenance checks passed all four cases. All eleven release-test
scenarios passed with extended input, windowed input, and diagnostic capture,
including an unavailable preferred controller. Four upstream native tests and
the project's save-file and launcher-policy test programs also passed.

Evidence is retained in that clone under `build/clean-generate.log`,
`build/clean-build.log`, `build/clean-regression.log`, and
`build/clean-native-tests.log`. Compiler warnings in upstream generation/test
code remain; there were no build errors. These automated checks complement the
maintainer's gameplay validation but do not certify every chapter or game path.

## Symbol import

GBARecomp accepts imported function seed symbols as:

```text
0xADDR<TAB>mode<TAB>name
```

`mode` is `arm` or `thumb`.

Once a local `fireemblem8u` checkout has produced `fireemblem8.elf`, import
function symbols with:

```powershell
pwsh scripts/import-fireemblem8u-symbols.ps1 `
  -ElfPath .\extern\fireemblem8u\fireemblem8.elf `
  -UseWsl
```

If `fireemblem8u` is not built yet, WSL is the recommended path on Windows.
`bootstrap-fireemblem8u.ps1` only needs Git; `build-fireemblem8u.ps1` requires
a working WSL distribution:

```powershell
pwsh scripts/bootstrap-fireemblem8u.ps1
pwsh scripts/build-fireemblem8u.ps1
pwsh scripts/import-fireemblem8u-symbols.ps1 `
  -ElfPath .\extern\fireemblem8u\fireemblem8.elf `
  -UseWsl
```

If Windows reports that WSL is not installed, install a distribution first:

```powershell
wsl --install Ubuntu
```

Review the resulting `symbols/imported_symbols.tsv` before regenerating.

## Launcher runtime state

### v0.1.10 clean-build validation

On 2026-10-05, a fresh clone at
`F:/git/SacredStonesRecompTemp/v0.1.10-clean-b97283a1` regenerated the game and
BIOS and built the optimized MinGW runner. Its launcher pin was
`c4055c2238ff23fe0d3edf74f3e99002d874120e`; its runtime pin was
`97736f608714b34e2fad2b0e540c082e8a8d5e37`. Both were fetched from GitHub.

The save-file, launcher-policy, and runtime-controls programs passed, as did
the real ImGui layout and launcher Backspace navigation tests. All thirteen
release scenarios passed, including preference restoration from another
working directory and malformed-preference fallback. Evidence is retained in
`build/clean-generate.log`, `build/clean-build.log`, `build/clean-regression.log`,
and `build/validation/ffc469fadc1544f08645ea874ca241a1/` in that clone.
This automated coverage complements maintainer gameplay validation; it is not
a claim of full-game coverage.

The Windows runner uses `recomp-ui` from `extern/recomp-ui` and calls the
GBARecomp launcher seam before `run_game()`.

Expected runtime behavior:

- `src/main.cpp` forces the executable-local `game.toml` through `--config` so
  launches from Explorer, terminals, and tests resolve the same save type and
  ROM identity settings.
- The packaged game uses a user-provided GBA BIOS by default. The launcher
  exposes the BIOS picker and caches the selected path next to the executable.
- `src/main.cpp` forces FE8's SRAM save type at process startup because this
  single-game runner must not depend on launcher/config propagation for save-chip
  setup.
- Battery saves are anchored to `saves/SacredStonesRecomp.sav` next to the
  executable, not beside the ROM.
- On launch, `src/main.cpp` creates the `saves/` directory and migrates useful
  legacy saves from `saves/<ROM filename>.sav`, a `.sav` beside the ROM, a
  `.sav` beside the executable, or the temporary `save/` folder.
- `Assist Tools` is hidden in the launcher. Escape or the controller Guide
  button opens the in-game menu with save-state, fast-forward, and rewind controls.
- The executable-local `runtime-controls.toml` persists the selected slot,
  fast-forward multiplier, Assist Tools/rewind enable switches, and fullscreen.
  Fullscreen is shared by the launcher, menu and Alt+Enter; an explicit CLI mode
  overrides startup without rewriting the preference. Other display/audio menu
  changes, active fast-forward, and rewind history are session-only.
- Windows save export flushes SRAM before the game-owned menu callback, pauses
  gameplay and invokes the packaged `tools/export-windows-saves.ps1` helper with
  Windows PowerShell. Archives contain only saves and a manifest. Export tests
  exercise original-file preservation and failure-safe ZIP replacement.
- Save states remain beside the selected ROM as `.state1` through `.state9`;
  they do not share the standardized battery-save path.
- `launcher-window.ini` remembers launcher dimensions; a new installation starts
  at 940x799, constrained to the display's usable area.

Local files that must not be committed include `build/`, `recomp_cache/`,
`keybinds.ini`, `sacredstonesrecomp.ini`, `launcher-window.ini`,
`runtime-controls.toml`, `*rom.cfg`, `*bios.cfg`, `*.sav`, and savestate files.
