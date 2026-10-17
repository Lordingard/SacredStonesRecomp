# Direct Touch Controls Prototype

Android ARM64, direct-touch controls released in 0.1.19; updated in 0.1.20.

## Controls

- Enable **Direct touch controls (experimental)** in Android setup or the
  in-game **Input** menu. First-use default is on for this prototype.
- Tap a map tile, action, item or supported menu row to select it.
- The 0.1.15 test increment adds world-map locations and general/node menus.
  Tap a location to select it through the native travel/route handler; tap
  empty map space for the general menu. Drag to pan the world-map camera.
- An attack/other unit-target action requires a separate tap on its previewed
  target. The weapon-selection tap cannot immediately execute the attack.
- Tap with two fingers inside the image to send B (return/cancel).
- Drag one finger anywhere on a free battle/world map to pan the camera.
  Release without selecting the tile underneath. Tutorial direction restrictions
  and scene ownership still govern whether panning is allowed.
- Swipe vertically to scroll supported lists (Options, Unit list, Sound Room).
  This does not change an option value or activate the selected entry.
- Android Back and three-finger tap still open the runtime menu. The floating
  pad toggle still shows/hides virtual buttons; physical pads remain additive.

The selection policy, forced-A tutorial recovery, instruction-state guards,
text priority and target confirmation are shared with Windows, not duplicated.
Unsupported custom interfaces still require the virtual or physical controls.

## Implementation And Safety

- The pinned Android engine's TouchHub owns recognition, display mapping and
  virtual-pad hit testing. Only non-pad touches reach the game policy.
- The game adapter translates gestures into the same read-only PointerInput
  view as desktop mouse input, sampling EWRAM/IWRAM through the active bus.
- Only the existing trusted cursor/camera hooks write their reviewed cursor
  or camera fields. The touch adapter never writes units, path data or scripts.
- The game's `[Mouse]` Windows setting is independent. Android stores one
  byte (0/1) in app-private `touch-controls.cfg`; setup and native menu share
  this file and replace it atomically. Saves and controller preferences are
  untouched. The launcher refreshes this checkbox when returning from play.
- Menus, load state, rewind, replay, background and pause discard queued game
  input and cancel live recognizer points. Their eventual releases cannot
  become taps on the restored/new screen. Physical/virtual keys take priority.
- Disabled direct-touch mode keeps the original long-press menu behavior.

## Verification

- Shared desktop map/camera unit tests pass after extraction of PointerInput.
- Native tests execute the pinned recognizer plus the production touch adapter:
  tap, two-finger B, cancellation, manual input priority, invalid coordinates,
  single edge hold, release suppression and scroll-only vertical swipe.
- Existing Android display fit, cutout mapping, pad hit regions, menu Back/R3,
  and FE8 transition tests pass. Six negative package tests pass.
- All eight Android engine patches apply to fresh pinned source files and
  match the dependency lock hashes. Evidence:
  `build/validation/android-touch-patches-3d72bfa795444e77afd818d1f9a8ca7d/`.
- Both debug and production APKs built. The signed APK passed content, native
  page-alignment, production-certificate and API 28/31/35 signature checks.
- No Android device was connected; there is no new device gameplay acceptance.
  Two desktop tutorial integration attempts remained at stage 0 because their
  input callback stayed reset/blocked; they are not counted as passed. Shared
  policy unit tests do not replace a full tutorial playthrough.

## Device Acceptance

### World Map Increment (0.1.15, Version Code 14)

The maintainer reports the preceding touch build works well; the missing world
map was then identified on both desktop and mobile. Shared policy now recognizes
the idle WorldMapMain process, its IWRAM pixel cursor and its separate camera.
Battle-map graphics locks no longer accidentally block world-map modal menus.

Two reviewed function-entry annotations place only world cursor coordinates
before native WorldMap_LoopExt, and replace only free-selection cursor-follow
while the pointer camera is detached. They do not write unit positions, open
paths, chapter flags or node validity. Native node hit testing and route logic
remain authoritative. Scripted cursor/camera movement and locked/sleeping
processes refuse direct selection. Outer-frame clicks are rejected rather than
clamped onto a different location. Original controls cancel pointer plans.

Policy tests cover world coordinates/camera offsets, pending confirmation,
battlefield-lock independence, native animation ownership, ignored directions,
camera limits and travel-transition cancellation. Hook tests verify exact write
bounds, register preservation, caller identity, last-moment key restrictions,
native-handler continuation and reset. The touch policy selects a synthetic
world location through the same adapter. These are unit tests, not a real
world-map gameplay session: a world-map save state was requested for that.

Windows and Android were regenerated with their own pinned generators and
rebuilt. Six Windows boot/save scenarios passed; evidence:
`build/validation/c524f1637fde4fff9efb627248296612/`.
Android debug and signed packages passed their content/alignment/signature
checks. Logs: `build/world-map-windows-generate.log`,
`build/world-map-windows-build.log`, `build/world-map-android-build.log`, and
`build/world-map-android-signed-final.log`. No GitHub release was published.

Install the signed APK over the existing app, without uninstalling. On Pixel
9 Pro, Xiaomi Pad 5 and Ayn Thor, check tap accuracy, tutorial completion,
weapon/target double confirmation, two-finger cancel, edge pan, menu scrolling,
pad visibility, physical-pad handover, save-state load and setting persistence
after closing/reopening (including setup-skip mode). If a sequence blocks,
retain a save state immediately before it and note the exact gesture.

## Between-Chapter Saves (2026-10-17)

Signed test APK: `dist/android/SacredStonesRecomp-0.1.16-android-arm64.apk`
(version code 15). Install over the existing app without uninstalling.

The shared pointer policy now handles the post-chapter save menu, including
empty slots. Tap a slot, then explicitly tap Yes or No; two-finger cancel uses
the existing Back gesture. Native save code remains responsible for writing.
Selection waits through the native help-box closing animation.

Touch policy tests passed. Four isolated headless Windows runs from the supplied
snapshot verified native saving to an empty slot, overwriting, refusal and Back;
see [mouse validation](mouse-controls-prototype.md). Android packaging and
production signature checks passed, but this increment has not been tested on
an Android device. No GitHub release was published.

## World-Map Idle Gate Fix (2026-10-17)

Signed test APK: `dist/android/SacredStonesRecomp-0.1.17-android-arm64.apk`
(version code 16). This replaces the earlier unpublished local test artifact
of the same version; it contains the world-map fix, not unfinished Guide work.

The shared input policy and trusted hooks no longer mistake unused recycled
WorldMapMain data for an active scroll lock. Live scripted cursor movement and
camera scrolling remain protected. Windows snapshot integration reached the
native Mulan introduction with a selection and a separate confirmation click;
see [mouse validation](mouse-controls-prototype.md). Device acceptance remains
to be checked. Install over the existing app, without uninstalling.

## World-Map Dialogue Skip (2026-10-17)

Signed test APK: `dist/android/SacredStonesRecomp-0.1.18-android-arm64.apk`
(version code 17). The shared right-click/two-finger-cancel input supplies
Start during skippable world-map dialogue, following the world event engine
rather than text-local NOSKIP. Menus and modal choices retain Back behavior.
Windows snapshot integration verified the native skip flag and event closure;
see [mouse validation](mouse-controls-prototype.md). Android packaging and
signing checks passed; on-device acceptance remains pending. Guide input is
still deferred. Install over the existing app without uninstalling.

## Guide Navigation (2026-10-17)

Signed test APK: `dist/android/SacredStonesRecomp-0.1.19-android-arm64.apk`
(version code 18). The Guide work deferred above is now integrated. Tap
categories and articles; swipe vertically over a list or text to scroll it.
Text taps advance a page. Back and Sort badges accept taps; the two-finger
cancel gesture retains native Back behavior rather than skipping the Guide.

The shared policy preserves the world-map destination and dialogue-skip fixes.
Touch policy tests and two native Windows snapshot Guide scenarios passed;
see [mouse validation](mouse-controls-prototype.md). Android device acceptance
remains pending. Install over the existing app without uninstalling.

## Drag To Pan And Item Inventory (0.1.20)

Production-signed APK: `dist/android/SacredStonesRecomp-0.1.20-android-arm64.apk`
(version code 19). Install over the existing app without uninstalling.

Drag a finger anywhere on a free battle/world map to move the content with it.
This replaces Android's stationary edge hold; Windows mouse edge pan is unchanged.
Movement is incremental, bounded and independent of unit selection. Fractional
pixels accumulate; release never selects or confirms. Dragging cancels pending
selection plans. Physical controls, multitouch, native scene/tutorial restrictions
and state/menu resets retain ownership. Remaining fingers after multitouch must
all lift before panning can resume. Existing engine drag recognition is claimed.

Vertical list/Guide swipes still scroll lists rather than moving the map.
Tests exercise real DragBegin/DragEnd, center-origin and fractional movement,
stationary edges, cancellation and read-only camera requests. Policy/hook tests
cover map/world limits, stale gestures, physical input and scripted restrictions.
Android device acceptance remains pending.

The shared Item fix opens native actions for grey inventory entries without
enabling grey Use/Equip commands. A native Windows snapshot run verified item
selection, separate Equip confirmation and the resulting inventory; see
[mouse validation](mouse-controls-prototype.md).
