#pragma once

namespace DevBenchTool
{
	// Registers "cwlu.control" with DevBench when present (the driving-tool standard).
	// Call with false at kPostLoad and true at kDataLoaded.
	void Init(bool a_lastAttempt);
}
