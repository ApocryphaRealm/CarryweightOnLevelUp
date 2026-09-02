#pragma once

// Carryweight on Level Up - core. ON DEMAND, no background work (design decision 2026-09-01:
// "it doesn't need something that updates every second ... just a box to reissue the command").
// Apply() sets the player's PERMANENT carry weight (base plus permanent modifiers; enchantments
// and spells are temporary and left alone) to starting weight + per-level x (level - 1) by
// applying the difference as this mod's own modifier, and it runs exactly when something can
// have changed: a save loads, the player levels up (SKSE LevelIncrease event), a slider on the
// settings page changes, or the page's "Apply now" control is pressed. The net amount applied
// so far is tracked in the SKSE co-save.

#include <cstdint>
#include <string>

namespace Carryweight
{
	// Registers the level-up event sink. Call once at kDataLoaded.
	void Install();

	// Recomputes and applies the formula for the current level (main thread only).
	void Apply();

	// Queues Apply() onto the main thread (safe from any thread - the UI, DevBench, events).
	void RequestApply();

	// SKSE co-save plumbing - call from SKSEPluginLoad via the serialization interface.
	void OnSave(SKSE::SerializationInterface* a_intfc);
	void OnLoad(SKSE::SerializationInterface* a_intfc);
	void OnRevert(SKSE::SerializationInterface* a_intfc);

	struct State
	{
		std::uint64_t applications = 0;  // how many times Apply() ran this session
		std::uint16_t playerLevel = 0;
		float applied = 0.0F;     // net amount this mod has added so far (co-save)
		float target = 0.0F;      // what the formula says carry weight should be
		float carryWeightAV = 0.0F;  // current value incl. temporary modifiers
		float permanentAV = 0.0F;    // base + permanent modifiers - what the formula governs
	};
	State GetState();
}
