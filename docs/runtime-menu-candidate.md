# Runtime menu candidate

Implementation and validation record for the v0.1.10 release. The previously
validated launcher window/navigation batch remains included.

The existing GBARecomp runtime menu is enabled through its SDL_Renderer2
adapter, using the launcher's shared ImGui implementation. Open it with Escape
or the controller Guide button. Guide availability depends on the driver and
operating system; Escape is always the keyboard entry point.

The menu exposes the existing state-slot selection (1-9), save/load actions,
fast-forward toggle and multiplier (2-10), and one-second rewind action. A new
independent Enable rewind toggle stops captures and discards the temporary
history when disabled. Re-enabling starts a fresh history. Internal SRAM saves
and the existing function-key state bindings are unchanged.

Changes to the fast-forward multiplier, rewind enable switch, Assist Tools
enable switch, and selected state slot are now persisted immediately to
`runtime-controls.toml` beside the executable. Starting from another working
directory does not change that location. Fast-forward activation itself stays
session-only, so the game never restarts unexpectedly accelerated. Rewind
history remains temporary; only its enabled preference is persisted.

Runtime controls are exposed in the in-game menu, not the pre-boot Assist Tools
page (which remains hidden). French localization is intentionally deferred
while the supported game ROM remains English-only.
Malformed preferences fall back to defaults without stopping the game. A
malformed or unreadable existing file is not overwritten: the menu edits still
apply to the session and an error is logged until the file is repaired or
removed. Writes use a temporary file and atomic replacement; a locked original
and its recoverable temporary copy are preserved on replacement failure.

Input routing is corrected so a closed menu cannot intercept gameplay arrows
or controller buttons. The project policy test covers closed/open ownership
and both opening inputs. The upstream runtime ImGui and launcher focus tests
pass. Gameplay regressions do not themselves prove menu-triggered save/load
or physical controller navigation; those need candidate acceptance testing.

All eleven existing game regression scenarios passed on the final rebuilt
candidate, including windowed input and diagnostics. Evidence:
`build/validation/06ef9fbf14434942aac816dc8b4d221d/`. The executable in
`build/launcher-candidate-validation/` matches that tested build.

Acceptance checklist:

- Move and confirm normally while the menu is closed.
- Open/close with Escape and, if exposed by the controller driver, Guide.
- Select a state slot, save, progress in-game, reopen, and load that slot.
- Compare the fast-forward multiplier at 2x and 4x, then turn it off.
- Disable rewind while leaving other assist controls enabled; re-enable and
  wait for new history before trying the left trigger.
- Confirm internal game saves still work independently of savestates.

No ROM, BIOS, or player save has been added to version control.

Preference unit tests cover round trips, unknown-key preservation, absent and
invalid values, malformed TOML, unsupported slots, accented paths, locked
replacement, and unwritable destinations. Release validation now adds two
windowed scenarios for executable-local preference loading from another
working directory and safe malformed-file fallback.
All thirteen runtime scenarios passed on this candidate; evidence is retained
under `build/validation/377b1fc3977f4908a93c82ceabb06fa0/`. The isolated candidate
executable matches the rebuilt runner. These automated checks cover preference
round trips and startup; the maintainer accepted publishing the resulting work.

## Layout correction after maintainer testing

The maintainer validated the runtime controls except for overlapping state-slot
text and step buttons. Runtime rows now reserve a separate control column,
wrap labels/descriptions to the remaining width, clip text within that column,
and expand row height for multiline text. Step-button dimensions remain stable.
The same correction applies to the fast-forward description.

The real-ImGui test includes the reported slot description at 1200x800 and
720x480 and checks for a bounded multiline text column. Assertions remain
enabled in Release test builds. The runner rebuilt and the candidate was
updated without changing its saves/configuration. The maintainer subsequently
accepted the layout correction.

The maintainer subsequently accepted the layout correction. Launcher startup
defaults now match the last accepted saved dimensions: 940x799 logical units.
Existing saved dimensions still take precedence, and the platform still fits
the initial window to smaller displays. A clean first-launch capture at
`build/launcher-default-size-check/default-launcher.png` confirms the ROM
status, filename, and selection control are visible without scrolling.
