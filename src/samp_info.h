#pragma once

#include "main.h"
#include "player_state.h"

#include <sampapi/CConfig.h>
#include <sampapi/CGame.h>
#include <sampapi/CLocalPlayer.h>
#include <sampapi/CNetGame.h>

#include <RakHook/samp.hpp>

namespace SampInfo
{
std::string GetServerName();
std::string GetServerAddress();
int GetPlayerCount();
int GetGameState();
bool IsDebugMode();

PlayerState GetPlayerState(bool localPlayerJoined, bool windowFocused);
}
