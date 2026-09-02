#pragma once

// Carryweight on Level Up - core. STATE-BASED (the AutoDraw pattern): a low-rate main-thread
// tick compares the carry weight the player SHOULD have - a pure formula of their current
// level, starting weight + per-level x (level - 1) - with their PERMANENT carry weight
// (base plus permanent modifiers; enchantments and spells are temporary and left alone), and
// applies the difference as this mod's own modifier. The amount applied so far is tracked
// in the SKSE co-save. Changing either setting recalculates on the next tick.

#include <cstdint>
#include <string>

namespace Carryweight
{
	// Starts the tick (poster thread + SKSE task, ~2 Hz). Call once at kDataLoaded.
	void Install();

	// SKSE co-save plumbing - call from SKSEPluginLoad via the serialization interface.
	void OnSave(SKSE::SerializationInterface* a_intfc);
	void OnLoad(SKSE::SerializationInterface* a_intfc);
	void OnRevert(SKSE::SerializationInterface* a_intfc);

	struct State
	{
		bool ticking = false;
		std::uint16_t playerLevel = 0;
		float applied = 0.0F;     // net amount this mod has added so far (co-save)
		float target = 0.0F;      // what the formula says carry weight should be
		float carryWeightAV = 0.0F;  // current value incl. temporary modifiers
		float permanentAV = 0.0F;    // base + permanent modifiers - what the formula governs
	};
	State GetState();
}
