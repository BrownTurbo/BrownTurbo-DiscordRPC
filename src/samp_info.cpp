#include "samp_info.h"

#include <sampapi/CGame.h>
#include <sampapi/CLocalPlayer.h>
#include <sampapi/CNetGame.h>

#include <RakHook/samp.hpp>

static inline bool HasText(const char* s)
{
	return s != nullptr && s[0] != '\0';
}

namespace SampInfo
{
std::string GetServerName()
{
	rakhook::samp_ver ver = rakhook::samp_version();
	switch (ver)
	{
	case rakhook::samp_ver::v037r1:
	{
		auto* ng = sampapi::v037r1::RefNetGame();
		if (ng && HasText(ng->m_szHostname))
			return ng->m_szHostname;
		break;
	}
	case rakhook::samp_ver::v037r31:
	{
		auto* ng = sampapi::v037r3::RefNetGame();
		if (ng && HasText(ng->m_szHostname))
			return ng->m_szHostname;
		break;
	}
	case rakhook::samp_ver::v037r5:
	{
		auto* ng = sampapi::v037r5::RefNetGame();
		if (ng && HasText(ng->m_szHostname))
			return ng->m_szHostname;
		break;
	}
	case rakhook::samp_ver::v03dlr1:
	{
		auto* ng = sampapi::v03dl::RefNetGame();
		if (ng && HasText(ng->m_szHostname))
			return ng->m_szHostname;
		break;
	}
	default:
		break;
	}
	return {};
}

std::string GetServerAddress()
{
	rakhook::samp_ver ver = rakhook::samp_version();
	switch (ver)
	{
	case rakhook::samp_ver::v037r1:
	{
		auto* ng = sampapi::v037r1::RefNetGame();
		if (ng && HasText(ng->m_szHostAddress))
			return std::string(ng->m_szHostAddress) + ':' + std::to_string(ng->m_nPort);
		break;
	}
	case rakhook::samp_ver::v037r31:
	{
		auto* ng = sampapi::v037r3::RefNetGame();
		if (ng && HasText(ng->m_szHostAddress))
			return std::string(ng->m_szHostAddress) + ':' + std::to_string(ng->m_nPort);
		break;
	}
	case rakhook::samp_ver::v037r5:
	{
		auto* ng = sampapi::v037r5::RefNetGame();
		if (ng && HasText(ng->m_szHostAddress))
			return std::string(ng->m_szHostAddress) + ':' + std::to_string(ng->m_nPort);
		break;
	}
	case rakhook::samp_ver::v03dlr1:
	{
		auto* ng = sampapi::v03dl::RefNetGame();
		if (ng && HasText(ng->m_szHostAddress))
			return std::string(ng->m_szHostAddress) + ':' + std::to_string(ng->m_nPort);
		break;
	}
	default:
		break;
	}
	return {};
}

int GetPlayerCount()
{
	rakhook::samp_ver ver = rakhook::samp_version();
	switch (ver)
	{
	case rakhook::samp_ver::v037r1:
	{
		auto* ng = sampapi::v037r1::RefNetGame();
		if (ng && ng->m_pPools && ng->m_pPools->m_pPlayer)
			return ng->m_pPools->m_pPlayer->GetCount(/*bIncludeNPC=*/false);
		break;
	}
	case rakhook::samp_ver::v037r31:
	{
		auto* ng = sampapi::v037r3::RefNetGame();
		if (ng && ng->m_pPools && ng->m_pPools->m_pPlayer)
			return ng->m_pPools->m_pPlayer->GetCount(false);
		break;
	}
	case rakhook::samp_ver::v037r5:
	{
		auto* ng = sampapi::v037r5::RefNetGame();
		if (ng && ng->m_pPools && ng->m_pPools->m_pPlayer)
			return ng->m_pPools->m_pPlayer->GetCount(false);
		break;
	}
	case rakhook::samp_ver::v03dlr1:
	{
		auto* ng = sampapi::v03dl::RefNetGame();
		if (ng && ng->m_pPools && ng->m_pPools->m_pPlayer)
			return ng->m_pPools->m_pPlayer->GetCount(false);
		break;
	}
	default:
		break;
	}
	return -1;
}

int GetGameState()
{
	rakhook::samp_ver ver = rakhook::samp_version();
	switch (ver)
	{
	case rakhook::samp_ver::v037r1:
	{
		auto* ng = sampapi::v037r1::RefNetGame();
		if (ng)
			return ng->m_nGameState;
		break;
	}
	case rakhook::samp_ver::v037r31:
	{
		auto* ng = sampapi::v037r3::RefNetGame();
		if (ng)
			return ng->m_nGameState;
		break;
	}
	case rakhook::samp_ver::v037r5:
	{
		auto* ng = sampapi::v037r5::RefNetGame();
		if (ng)
			return ng->m_nGameState;
		break;
	}
	case rakhook::samp_ver::v03dlr1:
	{
		auto* ng = sampapi::v03dl::RefNetGame();
		if (ng)
			return ng->m_nGameState;
		break;
	}
	default:
		break;
	}
	return -1;
}

bool IsDebugMode()
{
	if (GetModuleHandleA("samp.dll") != nullptr)
	{
		auto* cfg = sampapi::v037r1::RefConfig();
		if (cfg)
		{
			if (cfg->GetIntValue("debug") == 1)
				return true;
		}
	}

	const char* cmdLine = GetCommandLineA();
	if (cmdLine)
	{
		const char* p = cmdLine;
		while (*p != '\0')
		{
			// Skip whitespace
			while (*p == ' ' || *p == '\t')
				++p;

			// Check for "-d" followed by whitespace or end-of-string
			if (p[0] == '-' && p[1] == 'd' && (p[2] == '\0' || p[2] == ' ' || p[2] == '\t'))
				return true;

			// Advance past current token
			while (*p != '\0' && *p != ' ' && *p != '\t')
				++p;
		}
	}

	return false;
}

PlayerState GetPlayerState(bool localPlayerJoined, bool windowFocused)
{
	// 1. Game window unfocused → always "Paused".
	if (!windowFocused)
		return PlayerState::Paused;

	// 2. Not yet joined any server.
	if (!localPlayerJoined)
	{
		// If NetGame exists we are at least in the handshake / spawn screen.
		int gs = GetGameState();
		if (gs == -1)
			return PlayerState::NotConnected;
		return PlayerState::Connecting;
	}

	// 3. Joined - inspect local player through sampapi.
	rakhook::samp_ver ver = rakhook::samp_version();
	switch (ver)
	{
	case rakhook::samp_ver::v037r1:
	{
		auto* ng = sampapi::v037r1::RefNetGame();
		if (!ng || !ng->m_pPools || !ng->m_pPools->m_pPlayer)
			break;

		auto* local = ng->m_pPools->m_pPlayer->GetLocalPlayer();
		if (!local)
			break;

		auto* game = sampapi::v037r1::RefGame();
		if (game && game->IsMenuVisible())
			return PlayerState::InMenu;
		if (local->m_bIsWasted)
			return PlayerState::Wasted;
		if (local->m_bDoesSpectating)
			return PlayerState::Spectating;
		if (local->m_nCurrentVehicle != 0xFFFF)
			return PlayerState::InVehicle;
		return PlayerState::OnFoot;
	}
	case rakhook::samp_ver::v037r31:
	{
		auto* ng = sampapi::v037r3::RefNetGame();
		if (!ng || !ng->m_pPools || !ng->m_pPools->m_pPlayer)
			break;

		auto* local = ng->m_pPools->m_pPlayer->GetLocalPlayer();
		if (!local)
			break;

		auto* game = sampapi::v037r3::RefGame();
		if (game && game->IsMenuVisible())
			return PlayerState::InMenu;
		if (local->m_bIsWasted)
			return PlayerState::Wasted;
		if (local->m_bDoesSpectating)
			return PlayerState::Spectating;
		if (local->m_nCurrentVehicle != 0xFFFF)
			return PlayerState::InVehicle;
		return PlayerState::OnFoot;
	}
	case rakhook::samp_ver::v037r5:
	{
		auto* ng = sampapi::v037r5::RefNetGame();
		if (!ng || !ng->m_pPools || !ng->m_pPools->m_pPlayer)
			break;

		auto* local = ng->m_pPools->m_pPlayer->GetLocalPlayer();
		if (!local)
			break;

		auto* game = sampapi::v037r5::RefGame();
		if (game && game->IsMenuVisible())
			return PlayerState::InMenu;
		if (local->m_bIsWasted)
			return PlayerState::Wasted;
		if (local->m_bDoesSpectating)
			return PlayerState::Spectating;
		if (local->m_nCurrentVehicle != 0xFFFF)
			return PlayerState::InVehicle;
		return PlayerState::OnFoot;
	}
	case rakhook::samp_ver::v03dlr1:
	{
		auto* ng = sampapi::v03dl::RefNetGame();
		if (!ng || !ng->m_pPools || !ng->m_pPools->m_pPlayer)
			break;

		auto* local = ng->m_pPools->m_pPlayer->GetLocalPlayer();
		if (!local)
			break;

		auto* game = sampapi::v03dl::RefGame();
		if (game && game->IsMenuVisible())
			return PlayerState::InMenu;
		if (local->m_bIsWasted)
			return PlayerState::Wasted;
		if (local->m_bDoesSpectating)
			return PlayerState::Spectating;
		if (local->m_nCurrentVehicle != 0xFFFF)
			return PlayerState::InVehicle;
		return PlayerState::OnFoot;
	}
	default:
		break;
	}

	return PlayerState::OnFoot;
}

}
