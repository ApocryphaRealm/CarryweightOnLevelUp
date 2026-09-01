#include "PCH.h"

#include "DevBenchTool.h"

#include "Carryweight.h"
#include "DevBench/DevBenchAPI.h"
#include "Settings.h"
#include "utils/Logger.h"

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

			const auto s = Carryweight::GetState();
			const std::string json = std::format(
				"{{\"ok\":true,"
				"\"settings\":{{\"enabled\":{},\"perLevel\":{:.1f},\"maxBonus\":{:.1f},\"logLevel\":{},\"iniPath\":\"{}\"}},"
				"\"runtime\":{{\"ticking\":{},\"playerLevel\":{},\"applied\":{:.1f},\"target\":{:.1f},\"carryWeightAV\":{:.1f}}}}}",
				settings::general::enabled, settings::general::perLevel, settings::general::maxBonus,
				settings::debug::logLevel, EscapeJson(settings::GetIniPath()),
				s.ticking, s.playerLevel, s.applied, s.target, s.carryWeightAV);
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
			"applied/target bonus and the carry-weight actor value. op=reload re-reads the INI.\","
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
