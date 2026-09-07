#include "PCH.h"

#include "UI.h"

#include "SKSEMenuFramework.h"

#include "Carryweight.h"
#include "Settings.h"

#include "utils/Logger.h"
#include "utils/Strings.h"
#include "utils/Toggle.h"

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

namespace UI
{
	namespace
	{
		std::string statusMessage;
		std::string selectedSlider;

		constexpr const char* kLogLevelNames[] = { "Trace", "Debug", "Info", "Warning", "Error", "Critical", "Off" };
		constexpr const char* kLogLevelKeys[] = { "COLU_LogLevel_Trace", "COLU_LogLevel_Debug", "COLU_LogLevel_Info",
													"COLU_LogLevel_Warning", "COLU_LogLevel_Error", "COLU_LogLevel_Critical", "COLU_LogLevel_Off" };
		constexpr int kLogLevelCount = 7;

		void OnMainThread(std::function<void()> a_task)
		{
			if (auto* taskInterface = SKSE::GetTaskInterface())
			{
				taskInterface->AddTask(std::move(a_task));
			}
		}

		bool HasRequiredExports()
		{
			constexpr const char* required[] = {
				"AddSectionItem",
				"igTextV",
				"igTextDisabledV",
				"igTextWrappedV",
				"igSetTooltipV",
				"igSeparatorText",
				"igCombo_Str_arr",
				"igSliderFloat",
				"igIsKeyPressed_Bool",
				"igIsItemClicked",
				"igIsItemActive",
				"igIsItemHovered",
				"igButton",
				"igSameLine",
				"igSpacing",
				"igPushItemWidth",
				"igPopItemWidth",
				"igGetCursorScreenPos",
				"igGetWindowDrawList",
				"igGetFrameHeight",
				"igInvisibleButton",
				"igPushID_Str",
				"igPopID",
				"ImDrawList_AddRectFilled",
				"ImDrawList_AddCircleFilled"
			};

			for (const char* name : required)
			{
				if (!GetMenuFrameworkFunction<void*>(name))
				{
					logger::warn("The menu framework does not export \"{}\"", name);
					return false;
				}
			}
			return true;
		}

		void HelpMarker(const char* a_description)
		{
			ImGuiMCP::SameLine();
			ImGuiMCP::TextDisabled("%s", strings::TR("COLU_HelpMark", "(?)"));
			if (ImGuiMCP::IsItemHovered())
			{
				ImGuiMCP::SetTooltip("%s", a_description);
			}
		}

		bool NudgeableSlider(const char* a_label, float* a_value, float a_min, float a_max,
							 const char* a_format, float a_step)
		{
			bool changed = ImGuiMCP::SliderFloat(a_label, a_value, a_min, a_max, a_format);
			if (ImGuiMCP::IsItemClicked() || ImGuiMCP::IsItemActive()) { selectedSlider = a_label; }
			if (selectedSlider == a_label)
			{
				float nudge = 0.0F;
				if (ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_LeftArrow) || ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_DownArrow)) { nudge -= a_step; }
				if (ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_RightArrow) || ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_UpArrow)) { nudge += a_step; }
				if (nudge != 0.0F)
				{
					*a_value = std::clamp(*a_value + nudge, a_min, a_max);
					changed = true;
				}
				ImGuiMCP::SameLine();
				ImGuiMCP::TextDisabled("%s", strings::TR("COLU_NudgeArrows", "<-->"));
			}
			return changed;
		}

		void RenderGeneralSection()
		{
			using namespace settings;

			ImGuiMCP::SeparatorText(strings::TR("COLU_CarryWeight", "Carry weight"));

			bool changed = false;
			changed |= NudgeableSlider(strings::TR("COLU_StartingWeight", "Starting weight"), &general::startingWeight, 0.0F, 1000.0F, "%.0f", 5.0F);
			HelpMarker(strings::TR("COLU_HelpStartingWeight", "Carry weight at level 1. Vanilla Skyrim starts at 300."));

			changed |= NudgeableSlider(strings::TR("COLU_PerLevel", "Per level"), &general::perLevel, 0.0F, 25.0F, "%.1f", 0.5F);
			HelpMarker(strings::TR("COLU_HelpPerLevel", "Carry weight added for every level above 1."));

			if (changed) { Carryweight::RequestApply(); }

			if (ImGuiMCP::Button(strings::TR("COLU_ApplyNowBtn", "Apply now")))
			{
				Carryweight::RequestApply();
				statusMessage = strings::TR("COLU_StatusApplied", "Applied to your current level.");
			}
			HelpMarker(strings::TR("COLU_HelpApplyNow", "Reissues the formula for your current level with the values above. It also runs by itself when a save loads, when you level up, and when you move a slider - nothing runs in the background."));

			const auto s = Carryweight::GetState();
			ImGuiMCP::Text(strings::TR("COLU_LevelFormula", "Level %u: %.0f + %.1f x %u = %.0f carry weight"), s.playerLevel, general::startingWeight, general::perLevel,
						   s.playerLevel > 0 ? s.playerLevel - 1 : 0, s.target);
		}

		void RenderDebugSection()
		{
			using namespace settings;

			ImGuiMCP::SeparatorText(strings::TR("COLU_Debug", "Debug"));

			int level = static_cast<int>(debug::logLevel);
			level = std::clamp(level, 0, kLogLevelCount - 1);
			// Rebuilt from TR'd entries every frame (plan 2.2); labelStore owns the translated
			// bytes for this call so the const char* pointers handed to Combo stay valid.
			std::vector<std::string> logLevelLabelStore;
			logLevelLabelStore.reserve(kLogLevelCount);
			for (int i = 0; i < kLogLevelCount; ++i)
			{
				logLevelLabelStore.push_back(strings::TR(kLogLevelKeys[i], kLogLevelNames[i]));
			}
			std::vector<const char*> logLevelLabels;
			logLevelLabels.reserve(logLevelLabelStore.size());
			for (const auto& s : logLevelLabelStore) { logLevelLabels.push_back(s.c_str()); }
			if (ImGuiMCP::Combo(strings::TR("COLU_LogLevel", "Log level"), &level, logLevelLabels.data(), kLogLevelCount))
			{
				debug::logLevel = static_cast<std::uint32_t>(level);
				ApplyLogLevel();
			}
			HelpMarker(strings::TR("COLU_HelpLogLevel", "Applies immediately. The log is at Documents\\My Games\\Skyrim Special Edition\\SKSE\\CarryweightOnLevelUp.log."));
		}

		void RenderButtons()
		{
			ImGuiMCP::SeparatorText("");

			if (ImGuiMCP::Button(strings::TR("COLU_SaveBtn", "Save")))
			{
				statusMessage = strings::TR("COLU_StatusSaving", "Saving...");
				OnMainThread([]() {
					statusMessage = settings::Save() ? strings::TR("COLU_StatusSaved", "Settings saved.")
													   : strings::TR("COLU_StatusSaveFail", "Could not write the INI. See the log for why.");
				});
			}
			HelpMarker(strings::TR("COLU_HelpSave", "Writes every setting on this page to the plugin's INI so it survives a restart."));

			ImGuiMCP::SameLine();

			if (ImGuiMCP::Button(strings::TR("COLU_ReloadBtn", "Reload from INI")))
			{
				statusMessage = strings::TR("COLU_StatusReloading", "Reloading...");
				OnMainThread([]() {
					statusMessage = settings::Reload() ? strings::TR("COLU_StatusReloaded", "Settings reloaded from the INI.")
													   : strings::TR("COLU_StatusReloadFail", "Could not read the INI. See the log for why.");
				});
			}
			HelpMarker(strings::TR("COLU_HelpReload", "Throws away any change made here since the last save and re-reads the INI from disk."));

			ImGuiMCP::SameLine();

			if (ImGuiMCP::Button(strings::TR("COLU_RestoreBtn", "Restore defaults")))
			{
				OnMainThread([]() {
					settings::RestoreDefaults();
					logger::debug("Restored default settings");
				});
				statusMessage = strings::TR("COLU_StatusRestored", "Defaults restored. Press Save to keep them.");
			}
			HelpMarker(strings::TR("COLU_HelpRestore", "Puts every setting back to its fresh-install value. Nothing is written until you press Save."));

			if (!statusMessage.empty())
			{
				ImGuiMCP::TextWrapped("%s", statusMessage.c_str());
			}

			ImGuiMCP::Spacing();
			ImGuiMCP::Text("%s", settings::GetIniPath().c_str());
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled())
		{
			logger::info("No menu framework is installed; settings will be read from the INI only");
			return;
		}
		if (!HasRequiredExports())
		{
			logger::warn("The installed menu framework is older than this plugin's settings "
						 "menu needs. Update it (Apocrypha Menu Framework, or SKSE Menu "
						 "Framework version 3 or newer).");
			return;
		}

		SKSEMenuFramework::SetSection("Carryweight on Level Up");
		SKSEMenuFramework::AddSectionItem("Settings", SettingsPanel::Render);
		logger::info("Registered the settings page with the menu framework");
	}

	void __stdcall SettingsPanel::Render()
	{
		strings::Tick();

		ImGuiMCP::TextWrapped("%s", strings::TR("COLU_Intro", "Changes apply as soon as you make them. Press Save to keep them for the next time you play."));
		ImGuiMCP::Spacing();

		ImGuiMCP::PushItemWidth(260.0F);

		RenderGeneralSection();
		ImGuiMCP::Spacing();

		RenderDebugSection();
		ImGuiMCP::Spacing();

		ImGuiMCP::PopItemWidth();

		RenderButtons();
	}
}
