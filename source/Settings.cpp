#include "PCH.h"

#include "Settings.h"

#include "RaceShare.h"
#include "utils/INISettingCollection.h"
#include "utils/Logger.h"
#include "utils/Setting.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace settings
{
	namespace
	{
		std::string iniPath;

		struct Defaults
		{
			std::uint32_t logLevel;
			float startingWeight;
			float perLevel;
		} defaults{};

		std::string Lower(std::string a_s)
		{
			for (char& c : a_s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
			return a_s;
		}

		std::string Trim(const std::string& a_s)
		{
			const auto b = a_s.find_first_not_of(" \t\r\n");
			if (b == std::string::npos) { return {}; }
			const auto e = a_s.find_last_not_of(" \t\r\n");
			return a_s.substr(b, e - b + 1);
		}

		bool ParseBool(const std::string& a_text, bool& a_out)
		{
			const std::string v = Lower(Trim(a_text));
			if (v == "1" || v == "true" || v == "yes") { a_out = true; return true; }
			if (v == "0" || v == "false" || v == "no") { a_out = false; return true; }
			return false;
		}

		bool ParseFloat(const std::string& a_text, float& a_out)
		{
			try { a_out = std::stof(Trim(a_text)); return true; } catch (...) { return false; }
		}

		bool ParseUInt(const std::string& a_text, std::uint32_t& a_out)
		{
			// Base auto-detection (0x hex and decimal both work) - the DEM 1.5.8 lesson.
			try { a_out = static_cast<std::uint32_t>(std::stoull(Trim(a_text), nullptr, 0)); return true; } catch (...) { return false; }
		}

		std::map<std::string, std::string> ReadKeys()
		{
			std::map<std::string, std::string> keys;
			std::ifstream in(iniPath);
			if (!in) { return keys; }
			std::string line, section;
			while (std::getline(in, line))
			{
				const std::string t = Trim(line);
				if (t.empty() || t[0] == ';' || t[0] == '#') { continue; }
				if (t.front() == '[' && t.back() == ']') { section = Lower(t.substr(1, t.size() - 2)); continue; }
				const auto eq = t.find('=');
				if (eq == std::string::npos) { continue; }
				keys[Lower(Trim(t.substr(0, eq))) + ":" + section] = Trim(t.substr(eq + 1));
			}
			return keys;
		}

		bool LoadFileValues()
		{
			if (!std::filesystem::exists(iniPath))
			{
				logger::warn("INI not found at {}; keeping compiled defaults", iniPath);
				return false;
			}
			const auto k = ReadKeys();
			auto get = [&](const char* a_key, auto& a_out, auto a_parse, bool* a_saw = nullptr) {
				const auto it = k.find(a_key);
				if (it == k.end()) { logger::debug("INI key {} missing; keeping current value", a_key); return; }
				if (!a_parse(it->second, a_out)) { logger::warn("INI value \"{}\" for {} is not valid; keeping current value", it->second, a_key); return; }
				if (a_saw) { *a_saw = true; }
			};
			get("uloglevel:debug", debug::logLevel, ParseUInt);
			bool sawStarting = false, sawOffset = false;
			get("fstartingweight:general", general::startingWeight, ParseFloat, &sawStarting);
			get("foffset:general", general::offset, ParseFloat, &sawOffset);
			get("fperlevel:general", general::perLevel, ParseFloat);
			get("buseownbasevalues:general", general::ownBase, ParseBool);
			get("bapplystartingvalues:general", general::startingValues, ParseBool);
			// 1.0.8: carry weight goes on top of the race's. A 1.0.7 INI has only the absolute fStartingWeight; its offset is
			// that minus the 300 every such value was set against.
			if (sawStarting && !sawOffset)
			{
				general::offset = raceshare::OffsetFromStarting(general::startingWeight);
				logger::info("fStartingWeight {:.1f} from an older version read as fOffset {:+.1f} on top of the race's carry weight (written that way at the "
							 "next Save; bUseOwnBaseValues=1 keeps the old absolute behaviour)", general::startingWeight, general::offset);
			}
			logger::info("settings loaded from {}: offset={:+.1f} startingWeight={:.1f} perLevel={:.1f} ownBase={} startingValues={} logLevel={}",
						 iniPath, general::offset, general::startingWeight, general::perLevel, general::ownBase, general::startingValues, debug::logLevel);
			return true;
		}

		bool WriteKey(std::vector<std::string>& a_lines, const char* a_section, const char* a_key, const std::string& a_value)
		{
			const std::string wantSection = Lower(a_section);
			const std::string wantKey = Lower(a_key);
			std::string section;
			std::size_t lastInSection = std::string::npos;   // index of the section's last non-blank line
			for (std::size_t i = 0; i < a_lines.size(); ++i)
			{
				auto& line = a_lines[i];
				const std::string t = Trim(line);
				if (!t.empty() && t.front() == '[' && t.back() == ']')
				{
					section = Lower(t.substr(1, t.size() - 2));
					if (section == wantSection) { lastInSection = i; }
					continue;
				}
				if (section == wantSection && !t.empty()) { lastInSection = i; }
				const auto eq = t.find('=');
				if (eq == std::string::npos || section != wantSection) { continue; }
				if (Lower(Trim(t.substr(0, eq))) == wantKey)
				{
					line = std::string(a_key) + "=" + a_value;
					return true;
				}
			}
			// 1.0.8: a key an older INI does not have yet is added, at the end of its section (or in a new section),
			// instead of being lost - fOffset and the two switches would otherwise never reach an upgraded INI.
			const std::string newLine = std::string(a_key) + "=" + a_value;
			if (lastInSection != std::string::npos)
			{
				a_lines.insert(a_lines.begin() + static_cast<std::ptrdiff_t>(lastInSection + 1), newLine);
			}
			else
			{
				if (!a_lines.empty() && !Trim(a_lines.back()).empty()) { a_lines.emplace_back(); }
				a_lines.push_back(std::string("[") + a_section + "]");
				a_lines.push_back(newLine);
			}
			logger::info("Save: {} added to [{}] (it was not in the INI yet)", a_key, a_section);
			return true;
		}

		std::string FormatFloat(float a_v)
		{
			char buf[32];
			std::snprintf(buf, sizeof(buf), "%.1f", a_v);
			return buf;
		}
	}

	void Init(const std::string& a_iniFileName)
	{
		iniPath = (std::filesystem::current_path() / "Data" / "SKSE" / "Plugins" / a_iniFileName).string();

		defaults = { debug::logLevel, general::startingWeight, general::perLevel };

		// Registered for engine-side consistency; NEVER read through the collection - the
		// plain-file parse above is the value of record (redirector-proof).
		auto* collection = utils::INISettingCollection::GetSingleton();
		collection->AddSettings(
			utils::MakeSetting("uLogLevel:Debug", static_cast<unsigned int>(debug::logLevel)),
			utils::MakeSetting("fStartingWeight:General", general::startingWeight),
			utils::MakeSetting("fPerLevel:General", general::perLevel));

		LoadFileValues();
	}

	bool Reload()
	{
		const bool ok = LoadFileValues();
		ApplyLogLevel();
		return ok;
	}

	bool Save()
	{
		std::vector<std::string> lines;
		{
			std::ifstream in(iniPath);
			if (!in) { logger::error("Save: could not open {} for reading", iniPath); return false; }
			std::string line;
			while (std::getline(in, line)) { lines.push_back(line); }
		}

		bool ok = true;
		ok &= WriteKey(lines, "Debug", "uLogLevel", std::to_string(debug::logLevel));
		ok &= WriteKey(lines, "General", "fOffset", FormatFloat(general::offset));
		ok &= WriteKey(lines, "General", "fStartingWeight", FormatFloat(general::startingWeight));
		ok &= WriteKey(lines, "General", "fPerLevel", FormatFloat(general::perLevel));
		ok &= WriteKey(lines, "General", "bUseOwnBaseValues", general::ownBase ? "1" : "0");
		ok &= WriteKey(lines, "General", "bApplyStartingValues", general::startingValues ? "1" : "0");

		std::ofstream out(iniPath, std::ios::trunc);
		if (!out) { logger::error("Save: could not open {} for writing", iniPath); return false; }
		for (const auto& line : lines) { out << line << '\n'; }
		logger::info("settings saved to {}", iniPath);
		return ok;
	}

	void RestoreDefaults()
	{
		debug::logLevel = defaults.logLevel;
		general::offset = 0.0F;
		general::startingWeight = defaults.startingWeight;
		general::perLevel = defaults.perLevel;
		general::ownBase = false;
		general::startingValues = true;
		ApplyLogLevel();
	}

	void ApplyLogLevel()
	{
		const auto lvl = static_cast<spdlog::level::level_enum>(std::clamp<std::uint32_t>(debug::logLevel, 0u, 6u));
		SKSE::log::set_level(lvl, lvl);
	}

	const std::string& GetIniPath() { return iniPath; }
}
