# v0.1.19 - Direct Mouse And Touch Controls

## Downloads

- **SacredStonesRecomp-0.1.19-win64.zip**: extract and run the executable at the
  archive root. Keep the DLLs, assets and tools with it.
- **SacredStonesRecomp-0.1.19-android-arm64.apk**: signed Android 9+ ARM64 build.
- Matching `.sha256` assets provide download checksums.

The source archives are not playable packages. Supply your own legally obtained
USA ROM and GBA BIOS; neither is included. Local test versions 0.1.14-0.1.18 were
not published releases.

## New Controls

Enable **Mouse controls (experimental)** in Windows launcher input settings or
the in-game Input page. Android's **Direct touch controls (experimental)** is
enabled by default and configurable in setup and in-game. Both choices persist.

- Select units, tiles, actions, menus and Guide articles with a click or tap.
- Inspect the combat forecast before separately confirming the target.
- Select and confirm world-map destinations. Right-click skips skippable dialogue
  like Start; Android uses a two-finger tap. In menus it remains Back.
- Pan maps at the screen edges, or briefly hold a finger near an edge on Android.
- Scroll lists, scrollbars and Guide text using the wheel or a vertical swipe.
  Guide Back and Sort badges are interactive; a text click advances a page.
- Use chapter-end save slots with a separate Yes/No confirmation.

Original controls remain available. Native tutorial restrictions, inventory and
route validity, scripted movement and save operations retain authority. Pointer
plans reset when the original controls take over or a state/menu transition occurs.

## Updating

Back up saves through Assist Tools > Export saves first. Close the game; retain
Windows saves/settings when replacing application files. On Android, install
over the existing app without uninstalling or clearing data. The production key
is unchanged. Export import and cross-version save-state compatibility are not
guaranteed; retain a normal in-game save as well.

## Validation And Limits

Release checks passed: sixteen isolated Windows runtime scenarios (including
controller fallback, diagnostics and all fullscreen modes), save-file and
preference tests, launcher policy, SDL pointer routing, shared pointer/gesture
policy and trusted hooks. Generation provenance was regenerated and its four
negative tests passed. Windows dependencies are pinned to runtime `8a3b5dd`
and launcher `f8e784d`, both available from the project forks. Android signature,
package identity, private-asset exclusions and 16 KiB alignment checks passed.

Maintainer acceptance covers gameplay, the prologue, action/weapon confirmation,
menus and Guide navigation. Native snapshot tests cover the guided Trade chain,
chapter-end saves including cancellation, world-map selection and dialogue skip,
and Guide lists/text/sort/back. Shared policy and hook tests check read-only
input, bounded cursor/camera writes, state cancellation and gesture ownership.

This is not an exhaustive playthrough. Direct pointer controls remain experimental;
later chapters, optional screens and all Android device/gesture combinations have
not been exhaustively checked. The game, launcher and Guide remain in English.
Please report unsupported screens with a save state and the precise action.
