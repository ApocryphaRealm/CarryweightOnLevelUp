Carryweight on Level Up 1.0.8

Dual-line build (SKSE plugin). The installer asks which Skyrim you have and installs
the matching DLL:
  - Skyrim SE 1.5.97 / AE 1.6.1170  -> the SE/AE build (CommonLibSSE-NG 3.7.0)
  - Skyrim 1.7.99 / 1.7.104         -> the GPL build (CommonLibSSE-NG 7.2.0), licence texts included

This shared folder holds the INI and licence. The .pdb ships beside each DLL for Crash Logger.

Log: Documents\My Games\Skyrim Special Edition\SKSE\CarryweightOnLevelUp.log
Source: https://github.com/ApocryphaRealm/CarryweightOnLevelUp

WHAT CHANGED
------------

Version 1.0.8
Carry weight is added on top of your race's, so race mods such as Imperious keep their numbers: an Altmer's 250 or an Argonian's 325 stays, and this mod adds an Offset plus its per-level gain on top. A Starting weight set in an older version is read as an offset of (that - 300). The page shows your race's carry weight and your base carry weight.
New switch: Use our base values instead of the race's - your carry weight is set to Starting weight plus per level times (level minus 1), replacing what the race and other mods set; switching it off gives the race's value back exactly.
New switch: Set starting values (on by default) - off, your starting carry weight stays the race's and only the per-level gain is added.
This mod's share is now kept as its own permanent modifier rather than written into the base carry weight, which the game rebuilds from your race at every load.
Version 1.0.7
Carry weight from enchantments and abilities is no longer cancelled when a save loads. A backpack with Fortify Carry Weight +50 now reads 350 after loading and 300 once taken off; the Steed Stone, Extra Pockets and carry weight rings stack on top the same way. A save already affected corrects itself on the next load.
A new character now gets your Starting weight as soon as it is placed in the world, not only after the first save and load.
Version 1.0.4
The settings page is shown in the game's language: Japanese, Korean, Chinese, Russian, German, French, Spanish, Italian, Polish and Czech translation files ship beside the DLL, and the page follows the Apocrypha Menu Framework's Language setting; English is the fallback.
Version 1.0.3
Added a Skyrim 1.7.99 / 1.7.104 build; the mod now installs as a FOMOD that picks the build for your game version (SE 1.5.97 / AE 1.6.1170, or Skyrim 1.7.x).
Version 1.0.2
Removed the background tick. Carry weight is now recalculated only when a save loads, on level up, when a slider changes, or when you press the new Apply now button.
Version 1.0.1
Replaced the Enabled toggle and cap with two sliders: Starting weight (carry weight at level 1) and Per level.
Carry weight is now starting weight plus per level times (level minus 1), so changing either slider updates it immediately for your current level.
The mod now governs your permanent carry weight only; enchantments and spells stay temporary modifiers and are left alone.
Version 1.0.0
First release. Every character level above 1 permanently adds carry weight, configurable per level with an optional cap.
Added an in-game settings page through Apocrypha Menu Framework, or SKSE Menu Framework as a fallback.
Uses no hooks, memory patches or plugin file.
Debug symbols ship inside the main download so Crash Logger can resolve this mod's crash frames.

Version 1.0.5
Relicensed under GPL-3.0-or-later, as the libraries it is built on require; no gameplay change.

LICENCE

GPL-3.0-or-later: LICENSE-CarryweightOnLevelUp-GPL-3.0.txt and NOTICE.md. Components under other licences, with their
notices: THIRD_PARTY_NOTICES.md. Source: https://github.com/ApocryphaRealm/CarryweightOnLevelUp
