# CarryweightOnLevelUp - changelog

Rule 61: this mod's own history, kept beside the code it describes.

<!-- VERSIONING-RULES -->
> **Versioning rules (CLAUDE.md rules 6 and 48):** `X.Y.Z`; a change increments the THIRD
> number; at `.9` the MINOR rolls. The next number is LAST WORKING + 1; failed/scratch/
> untested numbers are reused. Numbers come from version-ledger.ps1 + set-version.ps1.

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
