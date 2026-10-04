#pragma once

#include "player_state.h"
#include <atomic>
#include <cstdint>
#include <string>

namespace DiscordManager
{
void Init();
void Shutdown();

void Update(
	const std::string& serverName,
	const std::string& serverAddress,
	PlayerState playerState,
	int playerCount,
	int maxPlayers,
	int64_t sessionStart);

void ClearPresence();
void PumpCallbacks();

}
