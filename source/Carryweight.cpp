#include "PCH.h"

#include "Carryweight.h"

#include "Settings.h"
#include "utils/Logger.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <mutex>
#include <thread>

namespace Carryweight
{
	namespace
	{
		constexpr std::uint32_t kRecordType = 'BONS';
		constexpr std::uint32_t kRecordVersion = 2;   // 2 (1.0.7): applied + the base value after the last apply

		std::atomic<bool> g_installed{ false };

		std::mutex g_stateLock;
		State g_state;

		// The net amount this mod has applied to the player's carry weight, mirrored in the
		// co-save. Only main-thread Apply() writes it.
		float g_applied = 0.0F;

		// The base carry weight as it stood after the last apply, saved beside g_applied (record v2). The game does not
		// keep this change to the player's carry-weight base across a save (logic library 36; Main Agent's 1.0.7 test,
		// 2026-10-08: base 400 saved, 300 after the load), and adding the re-application on top of the saved amount
		// made "applied" climb by the bonus on every load. The first apply after a load takes off what the game dropped.
		float g_lastBase = 0.0F;
		bool g_reconcile = false;   // set by a v2 co-save load, cleared by the first apply after it

		// One waiting thread at most: set while Apply() is waiting for the player to be placed (new game).
		std::atomic<bool> g_waitingForPlayer{ false };

		void RetryUntilPlaced()
		{
			if (g_waitingForPlayer.exchange(true)) { return; }   // already waiting
			logger::debug("Apply: no placed player yet - asking again every 2 s until there is one (at most 20 min)");
			std::thread([] {
				for (int i = 0; i < 600 && g_waitingForPlayer.load(); ++i)
				{
					std::this_thread::sleep_for(std::chrono::seconds(2));
					if (!g_waitingForPlayer.load()) { return; }
					RequestApply();   // Apply clears g_waitingForPlayer once the player is placed
				}
				if (g_waitingForPlayer.exchange(false)) { logger::warn("Apply: no placed player after 20 min - waiting for a load, level-up or Apply now"); }
			}).detach();
		}

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
		// The player must be placed in a cell before its actor values mean anything (gate rule
		// or-player-actor-values-need-a-placed-player). On a NEW GAME SKSE's kNewGame arrives before character
		// creation has placed anyone, and nothing else asks again until a save loads or the player levels up -
		// so a custom Starting weight never reached a new character (Main Agent's 1.0.7 test, 2026-10-08).
		// Ask again every 2 s until the player is placed, then stop: a bounded wait, not a background tick.
		if (!player || !player->Is3DLoaded() || !player->parentCell)
		{
			RetryUntilPlaced();
			return;
		}
		g_waitingForPlayer.store(false);

		State s;
		auto* avOwner = player->AsActorValueOwner();
		s.playerLevel = player->GetLevel();
		s.carryWeightAV = avOwner->GetActorValue(RE::ActorValue::kCarryWeight);
		s.permanentAV = avOwner->GetPermanentActorValue(RE::ActorValue::kCarryWeight);
		s.baseAV = avOwner->GetBaseActorValue(RE::ActorValue::kCarryWeight);

		// carry weight = starting + perLevel x (level - 1), for the CURRENT level
		const float target = settings::general::startingWeight
			+ static_cast<float>(std::max<int>(0, s.playerLevel - 1)) * settings::general::perLevel;
		s.target = target;

		if (g_reconcile)
		{
			g_reconcile = false;
			const float dropped = g_lastBase - s.baseAV;
			if (std::fabs(dropped) > 0.01F)
			{
				g_applied -= dropped;
				logger::debug("Apply: the save dropped {:.1f} of this mod's base change (saved base {:.1f}, now {:.1f}) - applied now {:.1f}",
							  dropped, g_lastBase, s.baseAV, g_applied);
			}
		}

		// Measured against the BASE value only. Constant enchantments and abilities are permanent
		// modifiers, so measuring base + permanent (up to 1.0.6) cancelled them on every load.
		const float delta = target - s.baseAV;
		if (std::fabs(delta) > 0.01F)
		{
			
#if RUNTIME_LINE == 17
			avOwner->ModBaseActorValue(RE::ActorValue::kCarryWeight, delta);
#else
			avOwner->ModActorValue(RE::ActorValue::kCarryWeight, delta);
#endif
			g_applied += delta;
			s.carryWeightAV = avOwner->GetActorValue(RE::ActorValue::kCarryWeight);
			const float before = s.permanentAV;
			s.permanentAV = avOwner->GetPermanentActorValue(RE::ActorValue::kCarryWeight);
			logger::info("base carry weight {:.1f} -> {:.1f} (level {}, starting {:.1f}, perLevel {:.1f}; net applied {:.1f}); "
						 "with enchantments and abilities {:.1f} -> {:.1f}",
						 s.baseAV, target, s.playerLevel, settings::general::startingWeight, settings::general::perLevel, g_applied,
						 before, s.permanentAV);
			s.baseAV = avOwner->GetBaseActorValue(RE::ActorValue::kCarryWeight);
		}
		else
		{
			logger::debug("Apply: base carry weight already {:.1f} at level {} (with enchantments and abilities {:.1f})",
						  target, s.playerLevel, s.permanentAV);
		}
		s.applied = g_applied;
		g_lastBase = s.baseAV;

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
			a_intfc->WriteRecordData(&g_lastBase, sizeof(g_lastBase));
		}
	}

	void OnLoad(SKSE::SerializationInterface* a_intfc)
	{
		g_applied = 0.0F;
		g_lastBase = 0.0F;
		g_reconcile = false;
		std::uint32_t type = 0, version = 0, length = 0;
		while (a_intfc->GetNextRecordInfo(type, version, length))
		{
			if (type != kRecordType) { continue; }
			float v = 0.0F;
			if (length >= sizeof(float) && a_intfc->ReadRecordData(&v, sizeof(v)) == sizeof(v)) { g_applied = v; }
			// v1 (1.0.6 and older) holds only the applied amount, which may already have drifted; it is kept as it was.
			if (version >= 2 && length >= 2 * sizeof(float) && a_intfc->ReadRecordData(&v, sizeof(v)) == sizeof(v))
			{
				g_lastBase = v;
				g_reconcile = true;
			}
		}
		logger::debug("co-save loaded: applied bonus {}, base after the last apply {}", g_applied, g_lastBase);
		RequestApply();  // a save just loaded - apply the formula for its level once
	}

	void OnRevert(SKSE::SerializationInterface*)
	{
		g_applied = 0.0F;
		g_lastBase = 0.0F;
		g_reconcile = false;
	}

	State GetState()
	{
		std::scoped_lock l(g_stateLock);
		return g_state;
	}
}
