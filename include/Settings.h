#pragma once

// Carryweight on Level Up - settings. Plain-file INI (never the Win32 profile API, so
// PrivateProfileRedirector can neither serve stale values nor overwrite the file).

#include <cstdint>
#include <string>

namespace settings
{
	namespace debug
	{
		inline std::uint32_t logLevel = 0;  // uLogLevel:Debug - 0 = trace (project default)
	}

	namespace general
	{
		// Carry weight is a plain formula of the CURRENT level (design decision 2026-09-01: the
		// page just sets the starting weight and the weight per level, and changing either
		// recalculates what the player should have right now):
		//     carry weight = fStartingWeight + fPerLevel x (level - 1)
		inline float startingWeight = 300.0F;  // fStartingWeight:General - carry weight at level 1 (vanilla 300)
		inline float perLevel = 5.0F;          // fPerLevel:General - added per level above 1
	}

	void Init(const std::string& a_iniFileName);
	bool Reload();
	bool Save();
	void RestoreDefaults();
	void ApplyLogLevel();
	const std::string& GetIniPath();
}
