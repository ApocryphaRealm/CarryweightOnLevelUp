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
		constexpr std::uint32_t kRecordVersion = 1;

		std::atomic<bool> g_installed{ false };
		std::atomic<bool> g_tickPending{ false };

		std::mutex g_stateLock;
		State g_state;

		// The amount this mod has applied to the player's carry weight, mirrored in the
		// co-save. Only the main-thread tick writes it.
		float g_applied = 0.0F;

		void Tick()
		{
			g_tickPending.store(false, std::memory_order_release);

			State s;
			s.ticking = true;

			auto* ui = RE::UI::GetSingleton();
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!ui || ui->GameIsPaused() || !player || !player->Is3DLoaded())
			{
				std::scoped_lock l(g_stateLock);
				s.applied = g_applied;
				g_state = s;
				return;
			}

			auto* avOwner = player->AsActorValueOwner();
			s.playerLevel = player->GetLevel();
			s.applied = g_applied;
			s.carryWeightAV = avOwner->GetActorValue(RE::ActorValue::kCarryWeight);
			s.permanentAV = avOwner->GetPermanentActorValue(RE::ActorValue::kCarryWeight);

			// carry weight = starting + perLevel x (level - 1), recalculated for the CURRENT level
			// every tick, so a setting change applies at once.
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
				s.applied = g_applied;
				s.carryWeightAV = avOwner->GetActorValue(RE::ActorValue::kCarryWeight);
				s.permanentAV = avOwner->GetPermanentActorValue(RE::ActorValue::kCarryWeight);
			}

			std::scoped_lock l(g_stateLock);
			g_state = s;
		}
	}

	void Install()
	{
		if (g_installed.exchange(true)) { return; }
		if (!SKSE::GetTaskInterface())
		{
			g_installed = false;
			logger::error("SKSE task interface unavailable; the mod cannot run");
			return;
		}

		// Poster thread hands one tick at a time to the main thread (~2 Hz - level-ups are
		// rare; the AutoDraw pattern, incl. the lesson that a task must NEVER re-queue
		// itself: SKSE drains its queue within one frame).
		std::thread([]() {
			while (g_installed.load(std::memory_order_relaxed))
			{
				if (!g_tickPending.exchange(true, std::memory_order_acq_rel))
				{
					if (auto* tasks = SKSE::GetTaskInterface()) { tasks->AddTask(Tick); }
					else { g_tickPending.store(false, std::memory_order_release); }
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}
		}).detach();

		logger::info("tick poster installed (state-based, ~2 Hz)");
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
