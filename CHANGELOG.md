# CarryweightOnLevelUp - changelog

Rule 61: this mod's own history, kept beside the code it describes.

<!-- VERSIONING-RULES -->
> **Versioning rules (CLAUDE.md rules 6 and 48):** `X.Y.Z`; a change increments the THIRD
> number; at `.9` the MINOR rolls. The next number is LAST WORKING + 1; failed/scratch/
> untested numbers are reused. Numbers come from version-ledger.ps1 + set-version.ps1.

## 1.0.8 - 2026-10-24 - untested

### Changed
- **Carry weight is added on top of your race's, so race mods keep their numbers** (the owner, 2026-10-24: "should
  read what the race's base carry weight ... and then apply over the top of it so that it's compatible with race
  mods"; a Nexus question about Imperious). The mod set the base carry weight to its formula, so an Altmer's 250 or an
  Argonian's 325 under Imperious became 300 at level 1. Now it holds only its own share - fOffset + fPerLevel x
  (level - 1) - on top of the race's carry weight and whatever other mods add. An INI from 1.0.7 has only
  fStartingWeight; it is read as the offset (that - 300) and logged once, and Save now adds keys an older INI lacks.
  The page shows your race's carry weight and your base carry weight, read-only.
- **The share is a permanent modifier, not a change to the base.** The base is the race's number, and the game
  rebuilds the player's base carry weight from the race at every load (1.0.7's test: base 400 saved, 300 after the
  load), which is why 1.0.7 had to reconcile against the saved base; that reconcile is gone. Slot 06 with kPermanent
  is used on both build lines (SE's RestoreActorValue, the 1.7 line's ModActorValue(modifier, ...)) - to be proven in
  game. A save from 1.0.7 starts the share from 0 (its share was in the base, which that load rebuilt). Co-save v3.

### Added
- Two switches ([General]; 11 languages): "Use our base values instead of the race's" (off by default - on: carry
  weight is set to fStartingWeight + fPerLevel x (level - 1), replacing what the race and other mods set; off again
  gives the race's value back exactly) and "Set starting values" (on by default - off: only fPerLevel x (level - 1) is
  added, the starting carry weight stays the race's). include\RaceShare.h is the arithmetic, shared with Character
  Progression Control 1.1.10; tools\race_share_check.bat checks it (20 checks).
- When Character Progression Control is loaded too, the page and the log say that its Carry Weight tab stands down,
  so one owner is named in both places.

## 1.0.7 - 2026-10-08 - untested

### Fixed
- Carry weight from enchantments and abilities is no longer cancelled when a save loads. A backpack with Fortify Carry
  Weight +50 now reads 350 after loading (300 + 50) and 300 once taken off; the Steed Stone, Extra Pockets and carry
  weight rings stack on top the same way. The formula now sets the BASE carry weight; up to 1.0.6 it was measured
  against base + permanent modifiers, and constant enchantments and abilities are permanent modifiers, so every load
  took them back off the base (wolf1438's report on the Discord hub, 2026-10-08, with the cause found: getavinfo showed
  base 250 + permanent +50 = 300). A save already affected corrects itself on the next load.
- A new character gets the formula as soon as it is placed in the world. SKSE's New Game message arrives before
  character creation, the player was not placed yet, and nothing asked again until a save loaded, a level-up or Apply
  now - so a custom Starting weight did not reach a new character. The mod now asks again every 2 seconds until the
  player is placed (at most 20 minutes), then stops. Found in the in-game test of this release.
- The co-save's "applied" amount no longer climbs on every load. The game does not keep this mod's change to the
  carry-weight base across a save, so each load re-applies it, and the re-application used to be added on top of the
  saved amount (100 became 200, then 300). The co-save now also stores the base after the last apply (record v2), and the
  first apply after a load first takes off what the game dropped. Carry weight itself was always right; only the
  reported number (log, DevBench state) drifted.

## 1.0.6 - 2026-09-18 - untested

### Changed
- The Address Library pre-check runs before anything else at load: a missing Address Library file for the game version
  gets a message naming the file and the plugin loads inert, instead of CommonLibSSE-NG's bare failure line
  (oproso's report on the Perfected Wheeler page, 2026-09-18: a guard placed after SKSE::Init never ran). No other change.

## 1.0.4 - 2026-09-07 - working

### Added
- The settings page is shown in the game's language: Japanese, Korean, Chinese, Russian, German, French, Spanish, Italian, Polish and Czech translation files ship beside the DLL (Interface/Translations/CarryweightOnLevelUp_<language>.txt) and the page follows the Apocrypha Menu Framework's Language setting; English is the fallback. The framework is looked up by its sort-first name first; cwlu.control gained op=strings.

## 1.0.3 - 2026-09-05 - untested

### Added
- Added a Skyrim 1.7.99 / 1.7.104 build; the mod now installs as a FOMOD that picks the build for your game version (SE 1.5.97 / AE 1.6.1170, or Skyrim 1.7.x).

## 1.0.2 - 2026-09-01 - working

### Changed
- No background tick any more (design decision 2026-09-01). The formula is applied on demand:
  when a save loads, when you level up (SKSE level-up event), when a slider changes, and
  from the new "Apply now" button on the settings page. Nothing runs between those moments.

## 1.0.1 - 2026-09-01 - working

### Changed
- The page is two sliders now (design decision 2026-09-01): Starting weight (carry weight
  at level 1, vanilla 300) and Per level. Carry weight = starting + per-level x (level - 1),
  recalculated for the CURRENT level on every tick, so changing either value applies at
  once. The mod now governs the PERMANENT carry weight (base plus permanent modifiers);
  enchantments and spells are temporary modifiers and are left alone. Enabled toggle and
  cap removed.

## 1.0.0 - 2026-09-01 - working

### Added
- First release. Every character level above 1 permanently adds carry weight
  (fPerLevel, default 5.0; optional fMaxBonus cap). The bonus is a pure formula of the
  CURRENT level, so it is inherently retroactive on an existing save and re-computes
  cleanly when the settings change or the mod is disabled (bEnabled=0 removes it all).
- State-based core on the AutoDraw pattern: a ~2 Hz main-thread tick compares the formula
  with the amount tracked in the SKSE co-save and applies the difference - idempotent
  across saves, loads and setting changes. No hooks, no relocations, no plugin file.
- In-game settings page (Apocrypha Menu Framework, stock SKSE Menu Framework fallback):
  Enabled toggle, per-level and cap sliders, live bonus readout, Save/Reload/Restore.
- Plain-file INI (redirector-proof), DevBench driving tool `cwlu.control`, .pdb debug
  symbols in the main download.
