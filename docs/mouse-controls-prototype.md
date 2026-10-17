# Mouse Map Controls Prototype (Windows)

Direct mouse controls were released in 0.1.19 and remain disabled by default.
This document also retains historical prototype notes. Enable **Mouse controls (experimental)** in the launcher's Player 1
panel or **Controller > Configure**. The same switch is available in the in-game
menu (Escape) under **Input**. The choice is remembered in the executable-local
`sacredstonesrecomp.ini`, section `[Mouse]`, key `enabled` (0/1), even when the
launcher is skipped. Keyboard/gamepad selection is independent of this switch.

- Left-click a visible map tile: place the map cursor there instantly, then
  press A once after the native path update.
  This selects a unit or confirms its destination using the original game rules.
- Left-click an enabled action or item/weapon menu row: move the selection to
  that row and confirm once. Greyed-out entries and clicks outside the menu
  are ignored, never forwarded to the map beneath it.
- In target selection (Attack, Rescue, Trade, etc.), the first click previews
  an eligible unit/tile, including the target preselected by the game. Click
  the same selected target again to confirm. Opening a target-selection screen
  never reuses the weapon/menu click to execute the action.
- In Trade's two inventories, click the source item, then its destination slot.
  Empty slots are accepted only when the game itself marks them selectable.
- Right-click inside the game image: press B once in the recognized map, menu,
  targeting or Trade state.
- Keyboard/gamepad input cancels a pending mouse target and takes priority.
- On the world map, click a visible location to position its cursor instantly
  and send native A. The game still decides node visibility, route access and
  whether to travel or show a destination menu. Click empty map space to open
  its general menu; right-click cancels the supported menus, not world travel.
  Edge hover pans its own camera independently of the cursor.
- Hover inside the game image near an edge to pan the camera immediately,
  without moving the selection cursor. The sixteen-logical-pixel band has
  progressive speed (one to four game pixels per frame); corners pan diagonally.
  Leaving the edge or holding a mouse button stops panning without snapping
  back. Manual controls and scene/interface changes restore native camera behavior.
- Left-click the recognized title/intro screens to send one Start press. Only
  screens whose original handlers permit skipping respond; the BIOS boot logo
  is not overridden. No click is queued into the following screen.
- The main file menu, difficulty choices and save slots accept row clicks.
  Save confirmation/cancel buttons accept separate clicks and retain the
  game's own save/erase/overwrite logic. Right-click sends B to go back.
- In the map's **Unit** list, click a unit name to perform its native A action
  (focus it on the map in field mode), or click its statistics columns to open
  its detailed stat screen. Click a column header to sort, and the visible
  side arrows to change list pages. The upper/lower parts of the right scroll
  bar step up/down through the list. Row targets track the unit pointer and
  are discarded if sorting changes the selected entry before activation.
- In **Options**, click a row label to select it without changing its value,
  or click a displayed value to choose it. The right scroll bar steps up/down.
  Selecting Solo animation does not automatically enter its unit submenu:
  click the separate blinking A prompt after selecting Solo to open it.
- Detailed stat screens accept their top-left/right page arrows. Right-click
  returns to the unit list or dismisses a recognized native help box first.
- **Status** accepts the Player/Enemy faction labels. Clicking the displayed
  leader performs the native focus-and-return action. Right-click returns
  without focusing a leader.
- Wheel over the Options, Unit or Sound Room list (including its scroll bar)
  navigates rows without confirming, playing a song or changing setting values.
  Multiple notches accumulate within a bounded pending target. Wheel outside
  these lists, over black bars or while a mouse button is held is ignored.
- In standard dialogue and CG narration, left-click reveals/advances text with
  native A. Right-click sends native Start only when the recognized dialogue
  permits skipping. Yes/No and Buy/Sell choices require clicks on their own
  displayed answers; a preceding advance click never confirms a new choice.
- Chapter/world-map cutscenes and the specific new-game subtitle introduction
  accept native Start through a click. Event flags, battle/death exclusions and
  native phase handling still decide whether skipping is permitted.
- The Extras list accepts displayed row clicks. Its save-file selection uses
  the existing slot/confirmation controls. Sound Room's main grid accepts song
  clicks and wheel navigation. Right-click exits the grid and recognized
  shuffle/help screens. Support screens currently support right-click return
  only; their character/conversation selection is not mouse-mapped yet.
- Clicks outside the game image, during combat, transitions, enemy
  phases, frozen menus and unsupported interfaces are ignored. Help boxes do
  not accept left-click selection; supported native help controllers accept B.

The adapter recognizes the FE8 USA player-phase, standard menu, target selection
and Trade inventory callbacks. It reads the camera, map dimensions, live menu
rectangles, item availability, target links, cursor animation and live
process tree. The input adapter remains read-only and never changes movement
range or bypasses confirmations. Two reviewed native function hooks allow a
detached camera: one replaces cursor-follow and writes only camera coordinates
and interpolation fields; the other hides offscreen cursor sprites to avoid GBA
OAM coordinate wrapping. Neither camera hook writes unit/cursor/inventory/save
data. A third hook replaces cursor interpolation for explicit map clicks and
writes only the logical, target and displayed cursor coordinate pairs, matching
the game's SetCursorMapPosition. Native path recalculation runs before the
following confirmation; movement ranges and unit walking animations are unchanged.
Keyboard/gamepad cursor animation is unchanged.
Visible map clicks retain the current bounded camera instead of starting native
cursor-follow. When mouse camera ownership ends, its position is clamped and
tile-aligned and its residual scrolling fields are cleared before native follow
runs. This restores the alignment assumed by the game's 4/8-pixel camera steps.
Keyboard/gamepad scrolling remains available. General hover previews are not implemented.

Other generic scrolling menus with more than nine rows, help-box
navigation still uses the original controls. Guided Trade input now follows
the tutorial's native instruction states; full chapter-1 acceptance is pending.
Guide/Records, full Support navigation and other custom interfaces have not been mapped yet.
An Android direct-touch prototype now reuses this policy; see
[touch controls](touch-controls-prototype.md). Virtual controls remain available.
Extras interfaces not listed above still use the original controls.
New-game slot selection deliberately ignores occupied slots in this prototype.

Pending actions are discarded on lost focus, runtime menu/pause, save-state
load, rewind, disabled mouse controls, changed game state, manual input or timeout.
Only one click is accepted per event pump; clicks are not queued across scenes.

## Validation

Build and run `sacredstones_map_pointer_tests` for synthetic FE8 memory tests.
They cover camera offsets, instant cursor requests, deferred one-shot A/B, blocking menus,
process lifetimes, bad memory, physical input priority and state resets. These
checks do not establish end-to-end gameplay correctness.

`sacredstones_pointer_host_tests` checks SDL logical-coordinate conversion,
resizing, black bars, synthetic-touch exclusion and focus gating. It can run
without input focus and does not establish interactive gameplay acceptance.
The policy test also accepts a path to an external FE8 USA savestate currently
at an idle, visible map cursor; that optional fixture stays outside the repo.

Manual acceptance before enabling by default or publishing:

1. Select a friendly unit and confirm reachable/unreachable destinations.
2. Cancel movement with right-click and resume with the controller.
3. Confirm no clicks pass through dialogue, unit-action menus or battle scenes.
   In enabled action menus, confirm clicks select the displayed row only.
4. Check a scrolled map and clicks near each image edge in windowed, borderless
   and exclusive fullscreen, including black bars and non-default DPI.
5. Interrupt pending cursor travel using Escape, Alt+Tab, load state and rewind.
6. Disable the switch and verify keyboard/controller play is unchanged.
7. Test Attack, weapon selection and target preview/confirmation. Changing the
   target should display the new forecast without attacking until another click.
8. Test Rescue/Drop and Trade target selection; select a source item and exchange
   it with an occupied or game-enabled empty slot. Cancel each stage with B/right-click.
9. Verify that selecting a weapon never launches an attack automatically. Even
   a preselected target must be clicked to preview it, then clicked to confirm.
10. Hover at each edge/corner, leave the image and enter black bars. Verify that
    menus, targeting, lost focus and held mouse buttons suppress edge scrolling.
11. Confirm panning starts without waiting for the selection frame, and that
    the camera remains stationary after leaving the edge. Try selecting a tile
    after panning, then switch back to keyboard/controller.
12. Try the health/safety screen, opening animation, title, main file menu,
    difficulty and an available save slot. Check that a click never acts on
    the following screen or confirms an overwrite/erase prompt automatically.
13. Click a distant map tile, including after camera panning: the cursor should
    jump directly there. Select a friendly unit and check both reachable and
    unreachable destinations, path arrows and cancellation. Verify keyboard and
    controller cursor movement still use their original animation.
14. Open Unit from the map menu. Test name activation, statistics, page arrows,
    sorting, scrolling, right-click return and interruption by manual input.
    Verify that list/header transitions never reuse the preceding A/R press.
15. In Options, click different row labels and each displayed setting; scroll
    down to Music, Sound Effects and Window Color. Check native audio/frame
    side effects and reopening of the screen. Explicitly test the Solo prompt.
16. In Status, select each faction, focus a displayed leader and return without
    focusing via right-click. Check that no click is forwarded to the map.
17. Wheel over Options/Unit/Sound Room rows and scroll bars, then outside the
    image, during transitions and after focus loss. Confirm no values/actions
    are activated by scrolling and rapid/opposite wheel events stay bounded.
18. Reveal and advance dialogue with left-click, skip eligible scenes with
    right-click and explicitly choose a displayed Yes/No answer. Check that a
    click never leaks from text into a choice, menu or the playable map.
19. Start a new game, skip its subtitle introduction, navigate the Extras list
    and Sound Room, and return from Support/help without using a keyboard.

## Local Results (2026-10-16)

- Standard generation and runner build completed with two jobs. Generation
  provenance matches the modified runtime; ROM and BIOS remain local inputs.
- Synthetic policy tests and SDL coordinate tests passed.
- A read-only test against the existing external slot-1 savestate recognized
  its actual player-phase process, camera and cursor; its RAM was not modified.
- All 16 `test-release.ps1 -ExtendedInput -WindowedInput -DiagnosticCapture`
  scenarios passed. Logs: `build/validation/09d84d4ee22e4ce3bde469ee21c81f7b/`.
- These runtime regressions keep the prototype disabled and use replay input;
  they do not replace interactive validation with mouse controls enabled.
- The maintainer validated mouse-driven map movement and the initial action
  interactions. The confirmation/edge-scrolling corrections below still need
  interactive acceptance.
- Direct Android touch support remains pending.

### Action Extension (2026-10-16)

- Added standard menu hit testing, disabled-item checks, target-ring navigation,
  explicit target confirmation and two-column Trade inventory navigation.
- Added synthetic tests for modal menus, disabled/changed entries, target
  previews, camera waits, malformed target links, Trade cells and tutorial locks.
- All policy tests passed with `-Wall -Wextra -Werror`; SDL coordinate tests and
  the read-only external savestate check passed again.
- All six boot/save regression scenarios passed after this extension. Logs:
  `build/validation/ae43f37eda92491886b149ece1ec1b29/`.
- These checks do not establish that every action works in live gameplay.

### Explicit Confirmation And Edge Scrolling (2026-10-16)

- The maintainer validated the previous action-control prototype in gameplay.
- A new same-frame regression reproduced an A pulse leaking across an interface
  change. Every process/mode/menu-identity change now discards that pulse and
  any pending confirmation, even if cursor travel has already completed.
- Target preview must now be explicitly armed by a first click, including
  preselected targets. Focus loss, manual input, state restoration and changing
  interfaces disarm it.
- Added level-triggered, focus- and viewport-filtered native mouse hover input.
  Only map idle/movement states use it for edge scrolling; target previews and
  action/inventory menus do not scroll the map underneath them.
- New tests cover interface transitions within one frame, first/second target
  clicks, reset/disarm, edge dwell, corners, map limits, outside-image hover,
  held mouse buttons, modal states and manual-input priority.
- Policy tests (also compiled with `-Wall -Wextra -Werror`), the external
  savestate fixture and SDL click/hover-coordinate tests passed. Standard source
  generation, runner build and provenance verification also passed.
- All six boot/save scenarios passed again; logs:
  `build/validation/08d4a9cca6e94df78e9945ade5f3cfd4/`.
  An isolated windowed smoke run presented 180 frames successfully; log:
  `build/validation/mouse-edge-window-5abc54a854bf4bd7bb225ca6cc0bd9ef/window.log`.
- Interactive validation of these two corrections remains pending.

### Independent Camera And Startup Screens (2026-10-16)

- The maintainer validated the previous confirmation and cursor-follow scrolling.
- Replaced cursor-follow edge scrolling with a detached camera, clamped to the
  native map bounds. Disabling mouse controls, focus/state resets, manual input
  and scene changes disable the trusted replacements.
- Added exact code-generation annotations for `HandleMoveCameraWithMapCursor`
  and `PutMapCursor`. Android's pinned generator/runtime also recognize these
  annotations; without registered/enabled plugins those guards remain inactive.
  The Android app was not changed, rebuilt or installed during this stage.
- Added recognized startup title/intro handlers and native menu hit testing for
  the main file menu, difficulty, slots and confirmation buttons.
- Policy tests cover startup navigation and confirmation boundaries. Dedicated
  camera tests check the guest return ABI, camera-only RAM writes, offscreen
  cursor clipping and reset/disable behavior.
- An isolated game-level test loaded an existing external slot-2 state and
  rendered 120 frames with the camera enabled: panning, staying detached,
  native-camera restoration and unchanged cursor position all passed, with
  fully static execution. No player save/state file was modified. Log:
  `build/validation/mouse-camera-9111602e6e2e46389925b48ef348499b/game.log`.
- These changes still need manual acceptance; no new release was published.
- All six boot/save regressions passed after the extension; logs:
  `build/validation/254ef717f0aa4be7aec59d5d4b48200b/`.

### Instant Map Cursor (2026-10-16)

- The maintainer validated the independent camera and startup mouse controls.
- Explicit map clicks now place all three native cursor coordinate pairs at
  once through HandleMoveMapCursor. The hook validates the live player-phase
  process, idle/movement callback, map bounds and native lock flags.
- Confirmation is deferred until the next input frame, allowing the original
  path-arrow recalculation to complete. Unit positions, path data, movement
  maps and saves are not written by the cursor hook.
- Policy and native-hook tests passed, covering delayed confirmation, bounded
  cursor-only writes, return ABI, one-shot placement and unchanged camera guards.
- An isolated test loaded the external slot-2 state: the cursor jumped from
  (1,10) to (12,8) in one guest update, with matching display coordinates.
  It rendered 120 frames with fully static execution and cancelled before A
  could select a unit/menu. Logs:
  `build/validation/mouse-instant-272472653a284dc195f2d98141b34812/game.log`.
- All six boot/save regressions passed; logs:
  `build/validation/4ed3b9319be94e0eacaebcabc068bccd/`.
- Gameplay acceptance of instant selection remains pending. Android and public
  release packages were not rebuilt or replaced.

### Camera Handover Fix (2026-10-16)

- The maintainer reported camera oscillation when instantly selecting near map
  edges. Free-camera pixel offsets and native residual scrolling were not safely
  handed back to the original cursor-follow routine.
- Map clicks now retain the bounded mouse camera. Handover restores native
  tile alignment, clamps the position and clears residual motion, without
  changing unit coordinates, movement ranges or path data.
- Unit tests cover a non-aligned camera near its maximum bounds and verify
  that cursor placement does not trigger native camera pursuit.
- Isolated game tests passed on the external slot-2 state: instant placement
  at (14,14), stable camera before/after handover, and three-pixel edge panning
  followed by a stable native camera. Both rendered 120 frames fully statically.
  The cursor test suppresses A to keep the map idle; interactive unit selection
  near the edges still requires maintainer acceptance. Logs:
  `build/validation/mouse-camera-handover-a4061fad98d341a7b74005e0efa5e20f/`.
- All six boot/save regressions passed again; logs:
  `build/validation/ac253715ede846e9b41041a1666d0f14/`.

### In-Game Menus (2026-10-16)

- Added callback/script-identified Unit list, Options, stat-screen, Status and
  native help-dismissal contexts. These modal screens suppress map input and
  mouse edge panning even while battle-map graphics are suspended.
- Unit rows use live scroll offsets and validated sorted-unit pointers. Column
  sorting, page changes and activation remain native. No list/option/unit data
  is written by the input adapter; no new native function hooks were needed.
- All 13 option rows use the FE8 USA selector geometry and packed option values,
  including the animation-value remapping. Settings changes emit native D-pad
  input so music and window-color handlers still run.
- Menu A confirmation waits for the native held-A bit to clear. Screen commands
  have neutral input gaps and wait for scrolling to settle, including motion
  observed between multiple input pumps in one guest frame. This prevents a
  click from merging with the press that opened a menu or being lost in a scroll.
- Expanded policy tests cover all 13 options, scrolled rows, exact values,
  read-only RAM, Solo activation, native A release, unit identity changes,
  sorting, page changes, help dismissal, Status and invalid/outside-image input.
  Policy/camera tests, the external slot-2 fixture and strict compiler checks
  passed.
- Three isolated real-game mouse runs passed end to end on the external slot-2
  state, each rendering 360 frames: Unit -> stats -> next page -> back -> sort
  -> back to map; Options -> change game speed -> scroll -> back to map;
  Status -> Enemy -> back to map. Camera-pan and instant-cursor regressions also
  passed (120 frames each); all five runs remained fully static. Logs:
  `build/validation/mouse-menus-final-870e83527549497998c10236325bf785/`.
- All six boot/save scenarios passed; logs:
  `build/validation/f457891fa3ec4b48845bc86611880616/`.
- Mouse comfort, every menu page and preparation/solo-animation modes still
  need manual acceptance. Android and public releases were not changed.

### Wheel, Dialogue And Extras (2026-10-16)

- The maintainer validated the preceding in-game menu controls.
- Added bounded SDL wheel events with logical image coordinates, normalized
  direction, black-bar/focus/overlay/touch filtering and no repeated deltas.
  SDL 2.26+ already transforms wheel positions; a resized-window regression
  caught and fixed an initial double-coordinate conversion.
- Added read-only IWRAM to the pointer input view so dialogue no-fast/no-skip
  flags can be checked. The reviewed FE8 USA TalkState address comes from the
  ROM's sTalkState pointer (0x03000048); no BIOS/ROM content was embedded.
- Added native dialogue/CG input, explicit choice hit testing, cinematic skip
  listeners and the new-game OpSubtitle controller. Scene/dialogue listeners
  yield to actual menus/choices and never write text, event, fade or save state.
- Added Extras row/slot selection, main Sound Room grid navigation/playback,
  scroll-only wheel plans and native return from Support/shuffle/help screens.
- Expanded policy tests and SDL tests passed, including no-skip/no-fast flags,
  battle exclusions, incoming choices, wheel accumulation and read-only input.
  Full interactive dialogue/Extras coverage remains pending; Support navigation
  beyond return is deliberately unsupported.
- Six boot/save regressions passed. Logs:
  `build/validation/e235c8eb9c734675a8996d391d8c3ced/`.
- Standard generation, runner build and strict compiler checks completed.
  Android and public packages were not rebuilt or published.
- Final isolated game runs passed for Options wheel navigation, Unit/stat
  screens, Status, the new-game subtitle skip, detached camera and instant
  cursor. The introduction test checks the exit of its actual subtitle
  controller after native Start input; it does not validate dialogue advance
  (that run emitted no dialogue A). All runs stayed fully static. Logs:
  `build/validation/mouse-wheel-final-9a58ea112e4d44deaaaf696fcb705d22/`.

### Tutorial Safety (2026-10-16)

- Instant map selection now respects native ignored A/directions, including a
  second check at native-hook execution. Restricted tutorial movement disables
  edge panning. Native scripts retain ownership of restrictions and unit data.
- Recognized the forced-A range-selection callback. A click can recover a
  cursor stuck on an occupied enemy tile by restoring only the cursor to the
  active unit. Recovery is never applied to an unoccupied guided destination;
  native forced A remains responsible for confirmation.
- Added tutorial BoxDialogue input and priority over underlying combat targets
  while text is printing/waiting. Choices retain explicit hit testing; a text
  click cannot confirm the target that appears afterwards. Frozen interactive
  controls cannot fall through to a cinematic skip listener.
- Guided Trade handles native states 2/3/5/8, restricting clicks to the requested
  side, Vulnerary, current destination and final B respectively. Instruction
  changes cancel pending clicks; automatic instruction phases stay native.
- Policy and native-hook tests pass, including read-only input, occupied-tile
  recovery, stale-unit/mask rejection, text priority and guided Trade states.
- A real-game run from the maintainer's October-16 snapshot reached combat:
  cursor recovery -> Attack -> weapon -> tutorial forecast messages -> explicit
  attack confirmation. Eirika remained at (4,5) when the action menu opened.
  The 900-frame run stayed fully static. Logs:
  `build/validation/mouse-tutorial-665a7b533653479890b82ff50cc4ff92/`.
- Six boot/save regressions passed. Logs:
  `build/validation/312019c95b524ad9a21bde1deaa51c7e/`.
- See [tutorial audit](mouse-tutorial-audit.md) for the reviewed shared paths
  and remaining chapter-by-chapter acceptance work. These checks are not an
  end-to-end playthrough of the game.

### Launcher Activation (2026-10-16)

- The launcher mirrors supported Input boolean settings through a bounded,
  optional host-owned toggle list. It reuses the runtime setting key and
  callbacks rather than introducing another mouse mode or aim sensitivity.
- Activation is saved beside the executable and loaded before launcher skip
  decisions. Changing the in-game switch uses the same persistence callback.
  Missing or invalid mouse settings default to off; unrelated INI sections and
  input-device choices are preserved. Save failures are reported to stderr.
- Policy tests passed for callback activation, unchanged-value commits, model
  ownership/count bounds, default-off and preference round trips. A real
  launcher checkbox click persisted, and reopening displayed it checked.
  Default-size dashboard/controller screenshots and logs:
  `build/validation/mouse-launcher-450186faffad42d19e62e4b8f34b4834/`.
- All six boot/save regressions passed again. Logs:
  `build/validation/3c6f552304514aa5ba8d87876e51ef8d/`.

Development executable: `build/runner-mingw/SacredStonesRecomp.exe`.
The stable release packages were not replaced and no release was published.

### Item Inventory Selection (0.1.20)

- Native unit Item inventory entries can be grey but still open their action
  submenu. Pointer selection now accepts that availability only for
  `gItemSelectMenuDef`. Disabled commands elsewhere remain rejected.
- From the supplied state1, all three inventory rows were grey. Native input
  selected the second item, waited in its action submenu, explicitly chose
  Equip, verified the equipped slot, then returned through Back. The adapter
  never wrote inventory data or command availability.
  Evidence: `build/validation/mouse-item-a9e00273d54e4d5db41ce1fa4194d8a9/`.
  The run stayed fully static. Tests also cover hidden/disabled entries,
  read-only input and preventing inherited submenu confirmation.
- Six boot/save regressions passed:
  `build/validation/5d10433b57da43ebaacb773d6603adce/`.
  Windows mouse edge scrolling is unchanged. Included in release 0.1.20.

### Guide Navigation (2026-10-17)

- Recognize both native Guide entry scripts and their shared idle callback.
  Category clicks select the left list; article clicks enter the right list,
  move to the selected visible row and open it. A category click from the
  reading view returns through the native Back states before selecting it.
- The wheel scrolls the pane under the pointer: categories, article entries
  or individual text lines. List-wheel input never opens an article. A click
  inside the text advances a native page; its upper/lower arrows scroll lines.
  Sort and Back badges are clickable, and right-click remains native Back.
- All navigation uses native input only, respecting redraw processes and key
  release gaps. The adapter does not write Guide buffers, read/unlock flags,
  save data or story state. Existing world-map gates and dialogue skip remain.
- Policy and touch tests cover list offsets, wheel limits, modal transitions,
  read-only memory, redraw waits, alternate entry script, invalid data and
  physical-control priority. Native tests from the supplied 16:00 state1
  verified category scrolling, article opening, text wheel down/up, page
  advance, returning to categories, sort and closing the Guide. A second
  run verified wheel selection in a list of three articles before opening.
  Evidence: `build/validation/mouse-guide-bc381b17065740628f676d609be4ec16/`
  and `build/validation/mouse-guide-list-3d0fafbf8def4f38861f2cf174529e0b/`.
  Both stayed fully static. Six boot/save regression scenarios passed:
  `build/validation/9e3d4c84976b46d2b4ef2f1b839bef40/`.
- Rebuilt Windows executable and Android test APK 0.1.19 (version code 18).
  Device acceptance remains pending; no GitHub release was published.

### World-Map Dialogue Skip (2026-10-17)

- World-map sprite dialogue explicitly sets text-local NOSKIP, while its
  world event engine still accepts Start. Right-click now follows the event
  engine's existing skip gate in that context. No dialogue flags, scene
  timers or story state are modified by the adapter.
- Event-level NOSKIP, already-skipping events, battles and modal choices
  retain their protections. Menus and choices still receive Back, not Start.
  Unit tests cover native gate changes, read-only RAM and single-pulse input.
- From the supplied world-map snapshot, the native integration test selected
  Mulan, reached Seth's dialogue, emitted Start from a right-click, observed
  the native SKIPPING flag and waited for that event and dialogue to close.
  Evidence: `build/validation/mouse-world-skip-a855b1af3be84e75a77e8a8c714e02fa/`.
  The run stayed fully static. Six boot/save regression scenarios also passed:
  `build/validation/3b69e699f7c048099ba7e07a7f562059/`.
- Updated Windows development executable and signed Android test APK 0.1.18
  (version code 17). Android device acceptance remains pending. Guide work
  remains deferred and no GitHub release was published.

### World-Map Snapshot Follow-Up (2026-10-17)

- Reproduced a completely blocked world-map click from the updated state1.
  The idle WorldMapMain process contained `0x40000000` at offset `+0x30`.
  That field is not initialized or used by the native idle cursor loop; it
  must not be treated as a scrolling lock. Both pointer policy and native
  cursor/camera hooks previously rejected it.
- Removed that false gate. Live cursor-move and scroll-manager processes
  still block input, including a last-moment check in both native hooks.
  No process fields, route flags, unit locations or saves are cleared.
- The isolated headless snapshot test failed before the fix. Afterward,
  clicking Mulan then confirming at the same destination triggered the
  native node-2 introduction and Seth's dialogue. The test deliberately
  uses a separate confirmation click; it does not establish reliable
  single-click automatic confirmation immediately after cursor placement.
  Evidence: `build/validation/mouse-world-c1e4fb2452b64afc908da4644e5055fd/`.
  Dispatch stayed fully static. Policy, hook and touch tests passed, as did
  six boot/save regressions in
  `build/validation/7e793cf380a14c4c9e9ebf6a1399665a/`.
- Guide work was deferred at the maintainer's request and is not included
  in the rebuilt executable. Its unfinished source/test copies are parked
  under ignored `build/guide-pointer-wip/`, not ready to restore wholesale.

### Between-Chapter Saves (2026-10-17)

- Recognize the native post-chapter save-slot loop separately from the title
  save menu. Its save action accepts both empty and occupied slots, matching
  native behavior. Saving and scrolling animations retain input ownership.
- A slot click opens the confirmation only. Yes requires a separate click;
  No and right-click Back retain native cancellation behavior. Queued clicks
  wait for the native help-box closing animation rather than being dropped.
- Policy and touch tests cover these paths. Real-game tests from the supplied
  snapshot verified empty-slot saving, overwriting, No and Back with native
  inputs in an isolated headless runner. Successful saves refreshed the slot
  metadata and changed SRAM; both cancellation paths left SRAM unchanged.
  No save data or slot metadata is written by the pointer adapter.
- Evidence: `build/validation/mouse-chapter-save-290441cb5b444e5fa224ba97ba9ade44/`,
  `build/validation/mouse-chapter-save-overwrite-e36402db36224d4290bcaafa905028db/`,
  `build/validation/mouse-chapter-save-no-50148d264eb544a7b7112598b251a892/`,
  `build/validation/mouse-chapter-save-back-6013af61dfbc419fa8b25261f2cc16df/`.
  All four stayed fully static. Six boot/save regression scenarios passed:
  `build/validation/2b7201670f12426abea5e0e4dda13740/`.
- Windows development executable rebuilt; Android signed test APK 0.1.16
  (version code 15) includes the same policy. Mobile gameplay acceptance
  remains to be checked on a device. No GitHub release was published.
