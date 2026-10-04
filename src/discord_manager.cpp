#include "discord_manager.h"
#include "defs.h"
#include "main.h"

#include "discord_rpc.h"

#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>

namespace
{

std::mutex g_mutex;
bool g_initialized = false;

void OnReady(const DiscordUser* user)
{
	WriteToLogFile(logsPath, "[Discord] Ready - user: %s#%s (id=%s)",
		user->username ? user->username : "?",
		user->discriminator ? user->discriminator : "?",
		user->userId ? user->userId : "?");
}

void OnDisconnected(int errorCode, const char* message)
{
	WriteToLogFile(logsPath, "[Discord] Disconnected (code=%d): %s",
		errorCode, message ? message : "");
}

void OnErrored(int errorCode, const char* message)
{
	WriteToLogFile(logsPath, "[Discord] Error (code=%d): %s",
		errorCode, message ? message : "");
}
}

namespace DiscordManager
{
void Init()
{
	std::lock_guard<std::mutex> lock(g_mutex);
	if (g_initialized)
		return;

	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	handlers.ready = OnReady;
	handlers.disconnected = OnDisconnected;
	handlers.errored = OnErrored;
	handlers.joinGame = nullptr; // SA-MP handles join flow
	handlers.spectateGame = nullptr; // SA-MP handles spectate flow
	handlers.joinRequest = nullptr; // not applicable

	Discord_Initialize(DISCORD_APP_ID, &handlers, 0, nullptr);
	g_initialized = true;

	WriteToLogFile(logsPath, "[Discord] Initialized (appId=%s)", DISCORD_APP_ID);
}

void Shutdown()
{
	std::lock_guard<std::mutex> lock(g_mutex);
	if (!g_initialized)
		return;

	Discord_Shutdown();
	g_initialized = false;

	WriteToLogFile(logsPath, "[Discord] Shutdown");
}

void Update(
	const std::string& serverName,
	const std::string& serverAddress,
	PlayerState playerState,
	int playerCount,
	int maxPlayers,
	int64_t sessionStart)
{
	std::lock_guard<std::mutex> lock(g_mutex);
	if (!g_initialized)
		return;

	// ---- details line (bold, max 128 bytes per SDK) -----------------------
	char details[128];
	memset(details, 0, sizeof(details));

	if (!serverName.empty() && !serverAddress.empty())
		snprintf(details, sizeof(details), "%.60s  [%.50s]",
			serverName.c_str(), serverAddress.c_str());
	else if (!serverName.empty())
		snprintf(details, sizeof(details), "%.120s", serverName.c_str());
	else if (!serverAddress.empty())
		snprintf(details, sizeof(details), "%.120s", serverAddress.c_str());
	else
		snprintf(details, sizeof(details), "SA-MP");

	// ---- state line (lighter text, max 128 bytes) -------------------------
	const char* stateLabel = PlayerStateLabel(playerState);

	// ---- party size (online / max) ----------------------------------------
	int partySize = 0, partyMax = 0;
	if (playerState != PlayerState::NotConnected && playerState != PlayerState::Connecting && playerCount > 0)
	{
		partySize = playerCount;
		partyMax = (maxPlayers > 0) ? maxPlayers : playerCount;
	}

	// ---- image keys -------------------------------------------------------
	const char* largeImageKey = "samp_logo"; // upload in Dev Portal
	const char* largeImageText = "SA-MP / open.mp";
	const char* smallImageKey = nullptr;
	const char* smallImageText = nullptr;

	switch (playerState)
	{
	case PlayerState::OnFoot:
		smallImageKey = "state_onfoot";
		smallImageText = "On foot";
		break;
	case PlayerState::InVehicle:
		smallImageKey = "state_vehicle";
		smallImageText = "In a vehicle";
		break;
	case PlayerState::Spectating:
		smallImageKey = "state_spectate";
		smallImageText = "Spectating";
		break;
	case PlayerState::Wasted:
		smallImageKey = "state_wasted";
		smallImageText = "Wasted";
		break;
	case PlayerState::Paused:
		smallImageKey = "state_pause";
		smallImageText = "Paused";
		break;
	case PlayerState::InMenu:
		smallImageKey = "state_menu";
		smallImageText = "In menu";
		break;
	case PlayerState::Connecting:
		smallImageKey = "state_connecting";
		smallImageText = "Connecting";
		break;
	default:
		break;
	}

	// ---- build presence struct (memset like the official example) ---------
	DiscordRichPresence presence;
	memset(&presence, 0, sizeof(presence));

	presence.details = details[0] ? details : nullptr;
	presence.state = stateLabel;
	presence.startTimestamp = (sessionStart > 0) ? sessionStart : 0;
	presence.largeImageKey = largeImageKey;
	presence.largeImageText = largeImageText;
	presence.smallImageKey = smallImageKey;
	presence.smallImageText = smallImageText;
	presence.partySize = partySize;
	presence.partyMax = partyMax;
	presence.instance = 0;

	Discord_UpdatePresence(&presence);
}

void ClearPresence()
{
	std::lock_guard<std::mutex> lock(g_mutex);
	if (!g_initialized)
		return;

	Discord_ClearPresence();

	WriteToLogFile(logsPath, "[Discord] Presence cleared");
}

void PumpCallbacks()
{
	Discord_RunCallbacks();
}

}
