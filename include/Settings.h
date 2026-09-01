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
		inline bool enabled = true;      // bEnabled:General
		// Carry weight gained per character level above 1. The bonus is a pure formula of the
		// CURRENT level - (level - 1) * fPerLevel, capped - so it is inherently retroactive on
		// an existing save and always self-consistent.
		inline float perLevel = 5.0F;    // fPerLevel:General
		inline float maxBonus = 0.0F;    // fMaxBonus:General - 0 = no cap
	}

	void Init(const std::string& a_iniFileName);
	bool Reload();
	bool Save();
	void RestoreDefaults();
	void ApplyLogLevel();
	const std::string& GetIniPath();
}
