#pragma once

// Carryweight on Level Up - core. ON DEMAND, no background work (design decision 2026-09-01:
// "it doesn't need something that updates every second ... just a box to reissue the command").
// Apply() sets the player's BASE carry weight to starting weight + per-level x (level - 1) by
// applying the difference to the base value. Everything else stays on top of it: constant-effect
// enchantments (a +50 backpack, rings), abilities (the Steed Stone, Extra Pockets) and spells.
// 1.0.7: those count as PERMANENT modifiers in the engine, and up to 1.0.6 the formula was measured
// against base + permanent modifiers, so every load cancelled them (wolf1438's report,
// 2026-10-08: a +50 backpack read 300 after loading, then 250 once taken off). It runs exactly when something can
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
		float permanentAV = 0.0F;    // base + permanent modifiers (enchantments, abilities)
		float baseAV = 0.0F;         // base value only - what the formula governs (1.0.7)
	};
	State GetState();
}
