// Carryweight on Level Up - own code, MIT (2026-09-01). On-demand core (apply on load,
// level-up, setting change, or the page's Apply now - no background tick), plain-file INI,
// AMF-aware settings page, DevBench driving tool, co-save serialization so the applied amount
// is idempotent across saves and loads.
#include "PCH.h"

#include "Carryweight.h"
#include "DevBenchTool.h"
#include "Settings.h"
#include "UI.h"

#include "utils/Logger.h"

namespace
{
	void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type)
		{
		case SKSE::MessagingInterface::kPostLoad:
			DevBenchTool::Init(false);
			break;
		case SKSE::MessagingInterface::kDataLoaded:
			UI::Register();
			Carryweight::Install();
			DevBenchTool::Init(true);
			break;
		case SKSE::MessagingInterface::kPostLoadGame:
		case SKSE::MessagingInterface::kNewGame:
			Carryweight::RequestApply();
			break;
		default:
			break;
		}
	}

	void SaveCallback(SKSE::SerializationInterface* a_intfc) { Carryweight::OnSave(a_intfc); }
	void LoadCallback(SKSE::SerializationInterface* a_intfc) { Carryweight::OnLoad(a_intfc); }
	void RevertCallback(SKSE::SerializationInterface* a_intfc) { Carryweight::OnRevert(a_intfc); }
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	SKSE::log::init("CarryweightOnLevelUp");

	settings::Init("CarryweightOnLevelUp.ini");
	settings::ApplyLogLevel();

	logger::info("Carryweight on Level Up {} loading",
				 SKSE::PluginDeclaration::GetSingleton()->GetVersion().string("."));

	SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);

	if (auto* serialization = SKSE::GetSerializationInterface())
	{
		serialization->SetUniqueID('CWLU');
		serialization->SetSaveCallback(SaveCallback);
		serialization->SetLoadCallback(LoadCallback);
		serialization->SetRevertCallback(RevertCallback);
	}

	return true;
}
