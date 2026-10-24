#include "PCH.h"

#include "Carryweight.h"

#include "RaceShare.h"
#include "Settings.h"
#include "utils/Logger.h"

#include "REX/W32/KERNEL32.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <format>
#include <mutex>
#include <thread>

namespace Carryweight
{
	namespace
	{
		constexpr std::uint32_t kRecordType = 'BONS';
		// 3 (1.0.8): applied + the permanent value after the last apply; the share is a PERMANENT MODIFIER. A 1 or 2 record's
		// share went into the base, which the game rebuilds from the race at every load, so nothing of it is left: read as 0.
		constexpr std::uint32_t kRecordVersion = 3;

		std::atomic<bool> g_installed{ false };

		std::mutex g_stateLock;
		State g_state;

		// The net amount this mod has applied to the player's carry weight, mirrored in the
		// co-save. Only main-thread Apply() writes it.
		float g_applied = 0.0F;

		// The permanent carry weight as it stood after the last apply (record v3). Up to 1.0.7 the share went into the BASE,
		// which the game does not keep across a save (logic library 36; Main Agent's 1.0.7 test, 2026-10-08: base 400
		// saved, 300 after the load) - and the base is the race mod's number. 1.0.8 holds the share as a permanent modifier
		// instead; should a load still come back without it (the permanent value is the saved one minus exactly the
		// share), the first apply after the load starts the share from 0 rather than trusting the co-save.
		float g_lastPermanent = 0.0F;
		bool g_checkAfterLoad = false;   // set by a v3 co-save load, cleared by the first apply after it
		bool g_toldAboutCpc = false;

		// Slot 05 (SE's ModActorValue, 7.x's ModBaseActorValue) changes the base; slot 06 with kPermanent (SE's
		// RestoreActorValue, 7.x's ModActorValue(modifier, ...)) moves the permanent modifier. To be proven in game on each
		// line (the 1.0.8 test request).
		void Modify(RE::ActorValueOwner* a_owner, float a_delta)
		{
#if RUNTIME_LINE == 17
			a_owner->ModActorValue(RE::ACTOR_VALUE_MODIFIERS::kPermanent, RE::ActorValue::kCarryWeight, a_delta);
#else
			a_owner->RestoreActorValue(RE::ACTOR_VALUE_MODIFIERS::kPermanent, RE::ActorValue::kCarryWeight, a_delta);
#endif
		}

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

		if (auto* race = player->GetRace()) { s.raceBase = race->data.baseCarryWeight; }
		s.cpcLoaded = REX::W32::GetModuleHandleA("CharacterProgressionControl.dll") != nullptr;
		if (s.cpcLoaded && !g_toldAboutCpc)
		{
			g_toldAboutCpc = true;
			logger::info("Character Progression Control is loaded too: its Carry Weight tab stands down while this mod is loaded, so carry weight has one owner - this one");
		}

		if (g_checkAfterLoad)
		{
			g_checkAfterLoad = false;
			if (std::fabs(g_applied) > 0.01F && std::fabs(s.permanentAV - (g_lastPermanent - g_applied)) < 0.5F && std::fabs(s.permanentAV - g_lastPermanent) > 0.5F)
			{
				logger::info("Apply: the game did not keep this mod's {:.1f} across the load (permanent {:.1f}, was {:.1f}) - starting from 0", g_applied, s.permanentAV,
							 g_lastPermanent);
				g_applied = 0.0F;
			}
		}

		// 1.0.8 (include/RaceShare.h): this mod holds a SHARE of the permanent carry weight and moves only that.
		//   additive (default): offset + perLevel x (level - 1) on top of the race's carry weight and every other mod's;
		//   bUseOwnBaseValues:  the total is fStartingWeight + perLevel x (level - 1), whatever the race and others set;
		//   bApplyStartingValues off: perLevel x (level - 1) only.
		raceshare::Carry c;
		c.on = true;
		c.startingValues = settings::general::startingValues;
		c.ownBase = settings::general::ownBase;
		c.offset = settings::general::offset;
		c.starting = settings::general::startingWeight;
		c.perLevel = settings::general::perLevel;
		c.level = s.playerLevel;
		const float share = raceshare::CarryShare(c, s.permanentAV, g_applied);
		const float delta = share - g_applied;
		s.target = s.permanentAV + delta;
		if (std::fabs(delta) > 0.01F)
		{
			Modify(avOwner, delta);
			g_applied = share;
			const float before = s.permanentAV;
			s.carryWeightAV = avOwner->GetActorValue(RE::ActorValue::kCarryWeight);
			s.permanentAV = avOwner->GetPermanentActorValue(RE::ActorValue::kCarryWeight);
			s.baseAV = avOwner->GetBaseActorValue(RE::ActorValue::kCarryWeight);
			logger::info("carry weight {:.1f} -> {:.1f} (level {}, {}; this mod's share {:.1f}; race {:.1f}, base {:.1f})", before, s.permanentAV, s.playerLevel,
						 !c.startingValues ? std::format("starting values off: per level {:.1f} only", c.perLevel)
						 : c.ownBase ? std::format("our base values: starting {:.1f}, per level {:.1f}", c.starting, c.perLevel)
									 : std::format("on top of the race: offset {:+.1f}, per level {:.1f}", c.offset, c.perLevel),
						 g_applied, s.raceBase, s.baseAV);
		}
		else
		{
			logger::debug("Apply: carry weight already {:.1f} at level {} (this mod's share {:.1f})", s.permanentAV, s.playerLevel, g_applied);
		}
		s.applied = g_applied;
		g_lastPermanent = s.permanentAV;

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
			a_intfc->WriteRecordData(&g_lastPermanent, sizeof(g_lastPermanent));
		}
	}

	void OnLoad(SKSE::SerializationInterface* a_intfc)
	{
		g_applied = 0.0F;
		g_lastPermanent = 0.0F;
		g_checkAfterLoad = false;
		std::uint32_t type = 0, version = 0, length = 0;
		while (a_intfc->GetNextRecordInfo(type, version, length))
		{
			if (type != kRecordType) { continue; }
			if (version < 3)
			{
				// 1.0.7 and older put the share into the base carry weight, which the game rebuilt from the race at this load:
				// nothing of it is left, so the share starts from 0 and is applied once as a permanent modifier.
				logger::info("co-save record v{} from 1.0.7 or older: its share was in the base, which this load rebuilt - starting from 0", version);
				continue;
			}
			float v[2]{};
			if (length >= sizeof(v) && a_intfc->ReadRecordData(v, sizeof(v)) == sizeof(v))
			{
				g_applied = v[0];
				g_lastPermanent = v[1];
				g_checkAfterLoad = true;
			}
		}
		logger::debug("co-save loaded: this mod's share {}, permanent after the last apply {}", g_applied, g_lastPermanent);
		RequestApply();  // a save just loaded - apply the formula for its level once
	}

	void OnRevert(SKSE::SerializationInterface*)
	{
		g_applied = 0.0F;
		g_lastPermanent = 0.0F;
		g_checkAfterLoad = false;
	}

	State GetState()
	{
		std::scoped_lock l(g_stateLock);
		return g_state;
	}
}
