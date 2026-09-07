#include "PCH.h"

#include "DevBenchTool.h"

#include "Carryweight.h"
#include "DevBench/DevBenchAPI.h"
#include "Settings.h"
#include "utils/Logger.h"
#include "utils/Strings.h"

#include <cstdlib>
#include <format>
#include <string>
#include <string_view>

namespace DevBenchTool
{
	namespace
	{
		std::string EscapeJson(std::string_view a_in)
		{
			std::string out;
			out.reserve(a_in.size() + 8);
			for (const char c : a_in)
			{
				switch (c)
				{
				case '\\': out += "\\\\"; break;
				case '"': out += "\\\""; break;
				case '\n': out += "\\n"; break;
				default: out += c; break;
				}
			}
			return out;
		}

		void ControlTool(void*, const char* a_argsJson, void* a_sink, DevBenchAPI::WriteFn a_write)
		{
			const std::string_view args = a_argsJson ? a_argsJson : "";
			if (args.find("\"reload\"") != std::string_view::npos)
			{
				const bool ok = settings::Reload();
				a_write(a_sink, std::format(R"({{"ok":{},"op":"reload"}})", ok ? "true" : "false").c_str());
				return;
			}
			if (args.find("\"strings\"") != std::string_view::npos)
			{
				a_write(a_sink, std::format(R"({{"ok":true,"op":"strings","strings":{}}})", strings::StatusJson()).c_str());
				return;
			}
			if (args.find("\"apply\"") != std::string_view::npos)
			{
				Carryweight::RequestApply();
				a_write(a_sink, R"({"ok":true,"op":"apply"})");
				return;
			}
			// op=starting:<n> / op=perlevel:<n> - a setting change; follow with op=apply (the page's sliders do that themselves).
			for (const auto& [key, target] : { std::pair{ "starting:", &settings::general::startingWeight }, std::pair{ "perlevel:", &settings::general::perLevel } })
			{
				const auto at = args.find(key);
				if (at != std::string_view::npos)
				{
					const float v = std::strtof(std::string(args.substr(at + std::string_view(key).size(), 16)).c_str(), nullptr);
					*target = v;
					a_write(a_sink, std::format(R"({{"ok":true,"op":"{}","value":{:.1f}}})", key, v).c_str());
					return;
				}
			}

			const auto s = Carryweight::GetState();
			const std::string json = std::format(
				"{{\"ok\":true,"
				"\"settings\":{{\"startingWeight\":{:.1f},\"perLevel\":{:.1f},\"logLevel\":{},\"iniPath\":\"{}\"}},"
				"\"runtime\":{{\"applications\":{},\"playerLevel\":{},\"applied\":{:.1f},\"target\":{:.1f},\"carryWeightAV\":{:.1f},\"permanentAV\":{:.1f}}}}}",
				settings::general::startingWeight, settings::general::perLevel,
				settings::debug::logLevel, EscapeJson(settings::GetIniPath()),
				s.applications, s.playerLevel, s.applied, s.target, s.carryWeightAV, s.permanentAV);
			a_write(a_sink, json.c_str());
		}
	}

	void Init(bool a_lastAttempt)
	{
		static bool registered = false;
		if (registered) { return; }

		DevBenchAPI::IDevBenchInterface001* devBench = DevBenchAPI::GetDevBenchInterface001();
		if (!devBench)
		{
			if (a_lastAttempt)
			{
				logger::info("DevBench not detected; skipping the \"cwlu.control\" tool");
			}
			else
			{
				logger::debug("DevBench not detected yet; will retry at the next message");
			}
			return;
		}

		constexpr const char* descriptor =
			"{"
			"\"description\":\"Carryweight on Level Up live state: settings, player level, the "
			"applied/target bonus and the carry-weight actor value. op=reload re-reads the INI. "
			"op=strings reports the active language, source and loaded translation count.\","
			"\"inputSchema\":{\"type\":\"object\",\"properties\":{\"op\":{\"type\":\"string\"}}},"
			"\"readOnly\":false"
			"}";

		if (devBench->RegisterTool("cwlu.control", descriptor, &ControlTool, nullptr))
		{
			logger::info("Registered \"cwlu.control\" with DevBench (build {})", devBench->GetBuildNumber());
			registered = true;
		}
	}
}
