#include "PCH.h"

#include "Carryweight.h"

#include "Settings.h"
#include "utils/Logger.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <mutex>

namespace Carryweight
{
	namespace
	{
		constexpr std::uint32_t kRecordType = 'BONS';
		constexpr std::uint32_t kRecordVersion = 1;

		std::atomic<bool> g_installed{ false };

		std::mutex g_stateLock;
		State g_state;

		// The net amount this mod has applied to the player's carry weight, mirrored in the
		// co-save. Only main-thread Apply() writes it.
		float g_applied = 0.0F;

		// SKSE raises this when the player's level goes up - the one moment the formula's input
		// changes on its own.
		class LevelSink : public RE::BSTEventSink<RE::LevelIncrease::Event>
		{
		public:
			static LevelSink* GetSingleton()
			{
				static LevelSink singleton;
				return &singleton;
			}
			RE::BSEventNotifyControl ProcessEvent(const RE::LevelIncrease::Event* a_event, RE::BSTEventSource<RE::LevelIncrease::Event>*) override
			{
				if (a_event) { logger::debug("level-up event (level {}) - applying", a_event->newLevel); RequestApply(); }
				return RE::BSEventNotifyControl::kContinue;
			}
		};
	}

	void Apply()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player || !player->Is3DLoaded()) { logger::debug("Apply: no loaded player yet"); return; }

		State s;
		auto* avOwner = player->AsActorValueOwner();
		s.playerLevel = player->GetLevel();
		s.carryWeightAV = avOwner->GetActorValue(RE::ActorValue::kCarryWeight);
		s.permanentAV = avOwner->GetPermanentActorValue(RE::ActorValue::kCarryWeight);

		// carry weight = starting + perLevel x (level - 1), for the CURRENT level
		const float target = settings::general::startingWeight
			+ static_cast<float>(std::max<int>(0, s.playerLevel - 1)) * settings::general::perLevel;
		s.target = target;

		const float delta = target - s.permanentAV;
		if (std::fabs(delta) > 0.01F)
		{
			avOwner->ModActorValue(RE::ActorValue::kCarryWeight, delta);
			g_applied += delta;
			logger::info("carry weight {:.1f} -> {:.1f} (level {}, starting {:.1f}, perLevel {:.1f}; net applied {:.1f})",
						 s.permanentAV, target, s.playerLevel, settings::general::startingWeight, settings::general::perLevel, g_applied);
			s.carryWeightAV = avOwner->GetActorValue(RE::ActorValue::kCarryWeight);
			s.permanentAV = avOwner->GetPermanentActorValue(RE::ActorValue::kCarryWeight);
		}
		else
		{
			logger::debug("Apply: carry weight already {:.1f} at level {}", target, s.playerLevel);
		}
		s.applied = g_applied;

		std::scoped_lock l(g_stateLock);
		s.applications = g_state.applications + 1;
		g_state = s;
	}

	void RequestApply()
	{
		if (auto* tasks = SKSE::GetTaskInterface()) { tasks->AddTask(Apply); }
	}

	void Install()
	{
		if (g_installed.exchange(true)) { return; }
		if (auto* source = RE::LevelIncrease::GetEventSource())
		{
			source->AddEventSink(LevelSink::GetSingleton());
			logger::info("level-up event sink registered (apply on load, level-up, setting change, or Apply now - no background tick)");
		}
		else
		{
			logger::warn("SKSE LevelIncrease event source unavailable - carry weight applies on load, setting change and Apply now only");
		}
	}

	void OnSave(SKSE::SerializationInterface* a_intfc)
	{
		if (a_intfc->OpenRecord(kRecordType, kRecordVersion))
		{
			a_intfc->WriteRecordData(&g_applied, sizeof(g_applied));
		}
	}

	void OnLoad(SKSE::SerializationInterface* a_intfc)
	{
		g_applied = 0.0F;
		std::uint32_t type = 0, version = 0, length = 0;
		while (a_intfc->GetNextRecordInfo(type, version, length))
		{
			if (type == kRecordType && length == sizeof(float))
			{
				float v = 0.0F;
				if (a_intfc->ReadRecordData(&v, sizeof(v)) == sizeof(v)) { g_applied = v; }
			}
		}
		logger::debug("co-save loaded: applied bonus {}", g_applied);
		RequestApply();  // a save just loaded - apply the formula for its level once
	}

	void OnRevert(SKSE::SerializationInterface*)
	{
		g_applied = 0.0F;
	}

	State GetState()
	{
		std::scoped_lock l(g_stateLock);
		return g_state;
	}
}
