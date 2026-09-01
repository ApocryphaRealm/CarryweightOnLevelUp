#pragma once

// Carryweight on Level Up - core. STATE-BASED (the AutoDraw pattern): a low-rate main-thread
// tick compares the bonus the player SHOULD have - a pure formula of their current level -
// with the bonus this mod has APPLIED (tracked in the SKSE co-save), and applies the
// difference. That makes it idempotent across saves/loads/uninstalls-of-the-bonus
// (bEnabled=0 removes it cleanly) and inherently retroactive on an existing save.

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
		float applied = 0.0F;     // what the co-save says this mod has added
		float target = 0.0F;      // what the formula says it should be
		float carryWeightAV = 0.0F;
	};
	State GetState();
}
