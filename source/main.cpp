// Carryweight on Level Up - own code, GPL-3.0-or-later (2026-09-01). On-demand core (apply on load,
// level-up, setting change, or the page's Apply now - no background tick), plain-file INI,
// AMF-aware settings page, DevBench driving tool, co-save serialization so the applied amount
// is idempotent across saves and loads.
#include "PCH.h"

#include "Carryweight.h"
#include "DevBenchTool.h"
#include "Settings.h"
#include "UI.h"

#include "utils/AddressLibraryGuard.h"
#include "utils/Logger.h"
#include "utils/Strings.h"

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
			strings::Configure("CarryweightOnLevelUp");
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
	SKSE::log::init("CarryweightOnLevelUp");
	// Address Library pre-check (the guard every mod of ours carries), BEFORE SKSE::Init, which opens the
	// Address Library itself (logic library 6026): a missing file gets a message naming it and the plugin
	// loads inert instead of CommonLibSSE-NG's bare failure line.
	if (!AddressLibraryGuard::Guard("Carryweight on Level Up"))
	{
		return true;
	}
	SKSE::Init(a_skse);

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
