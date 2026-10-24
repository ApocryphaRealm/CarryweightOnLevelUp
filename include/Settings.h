#pragma once

// Carryweight on Level Up - settings. Plain-file INI (never the Win32 profile API, so
// PrivateProfileRedirector can neither serve stale values nor overwrite the file).

#include <cstdint>
#include <string>

namespace settings
{
	namespace debug
	{
		inline std::uint32_t logLevel = 2;  // uLogLevel:Debug - 2 = info (project default since 2026-09-26)
	}

	namespace general
	{
		// Carry weight grows with the CURRENT level (design decision 2026-09-01), and since 1.0.8 it goes ON TOP of the
		// race's carry weight and other mods' (the owner, 2026-10-24 - include/RaceShare.h):
		//     this mod adds  fOffset + fPerLevel x (level - 1)
		// With bUseOwnBaseValues the total is  fStartingWeight + fPerLevel x (level - 1)  (the 1.0.7 behaviour); with
		// bApplyStartingValues off only  fPerLevel x (level - 1)  is added.
		inline float offset = 0.0F;            // fOffset:General (1.0.8) - added at level 1 on top of the race (a 1.0.7 INI's fStartingWeight - 300)
		inline float startingWeight = 300.0F;  // fStartingWeight:General - with bUseOwnBaseValues: the whole carry weight at level 1
		inline float perLevel = 5.0F;          // fPerLevel:General - added per level above 1
		inline bool ownBase = false;           // bUseOwnBaseValues:General (1.0.8) - this mod's values replace the race's
		inline bool startingValues = true;     // bApplyStartingValues:General (1.0.8) - off: only the per-level gain
	}

	void Init(const std::string& a_iniFileName);
	bool Reload();
	bool Save();
	void RestoreDefaults();
	void ApplyLogLevel();
	const std::string& GetIniPath();
}
