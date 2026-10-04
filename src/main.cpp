#include "main.h"

#include <sampapi/CGame.h>
#include <sampapi/CNetGame.h>

#include <RakHook/rakhook.hpp>
#include <RakHook/samp.hpp>
#include <RakNet/BitStream.h>
#include <RakNet/PacketEnumerations.h>
#include <RakNet/StringCompressor.h>

#include "discord_manager.h"
#include "discord_rpc.h"
#include "player_state.h"
#include "samp_info.h"

#include <cstdarg>
#include <cstring>

namespace fs = std::filesystem;

bool ASIinitialized = false;
bool pluginReady = false;
bool localPlayerJoined = false;
bool windowFocused = true; // assume focused until a WM_KILLFOCUS
int g_maxPlayers = -1; // filled from InitGame RPC (id=139)
int64_t g_sessionStart = 0; // set on join, cleared on disconnect

std::atomic<bool> shuttingDown { false };

char logsPath[300];
FILE* g_fLog = nullptr;

std::mutex logMutex;

void WriteToLogFile(const char* path, const char* fmt, ...)
{
	std::lock_guard<std::mutex> lock(logMutex);

	SYSTEMTIME time;
	va_list ap;

	if (g_fLog == nullptr)
	{
		g_fLog = fopen(path, "a");
		if (g_fLog == nullptr)
			return;
	}

	GetLocalTime(&time);
	fprintf(g_fLog, "[%02d:%02d:%02d.%03d] ", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
	va_start(ap, fmt);
	vfprintf(g_fLog, fmt, ap);
	va_end(ap);
	fprintf(g_fLog, "\n");
	fflush(g_fLog);
}

static bool CreateLogsFolderIfMissing() { return true; }

const char* GetLocalPlayerName()
{
	rakhook::samp_ver version = rakhook::samp_version();
	const char* plrName = nullptr;
	switch (version)
	{
	case rakhook::samp_ver::v037r1:
	{
		SAMPAPI_EXPORT sampapi::v037r1::CNetGame* pNetGame = sampapi::v037r1::RefNetGame();
		if (pNetGame && pNetGame->m_pPools)
		{
			plrName = pNetGame->m_pPools->m_pPlayer->GetLocalPlayerName();
			if (strlen(plrName) == 0)
			{
				unsigned short localId = pNetGame->m_pPools->m_pPlayer->m_localInfo.m_nId;
				if (pNetGame->m_pPools->m_pPlayer->IsConnected(localId))
					return pNetGame->m_pPools->m_pPlayer->GetName(localId);
			}
		}
		break;
	}
	case rakhook::samp_ver::v037r31:
	{
		SAMPAPI_EXPORT sampapi::v037r3::CNetGame* pNetGame = sampapi::v037r3::RefNetGame();
		if (pNetGame && pNetGame->m_pPools)
		{
			plrName = pNetGame->m_pPools->m_pPlayer->GetLocalPlayerName();
			if (strlen(plrName) == 0)
			{
				unsigned short localId = pNetGame->m_pPools->m_pPlayer->m_localInfo.m_nId;
				if (pNetGame->m_pPools->m_pPlayer->IsConnected(localId))
					return pNetGame->m_pPools->m_pPlayer->GetName(localId);
			}
		}
		break;
	}
	case rakhook::samp_ver::v037r5:
	{
		SAMPAPI_EXPORT sampapi::v037r5::CNetGame* pNetGame = sampapi::v037r5::RefNetGame();
		if (pNetGame && pNetGame->m_pPools)
		{
			plrName = pNetGame->m_pPools->m_pPlayer->GetLocalPlayerName();
			if (strlen(plrName) == 0)
			{
				unsigned short localId = pNetGame->m_pPools->m_pPlayer->m_localInfo.m_nId;
				if (pNetGame->m_pPools->m_pPlayer->IsConnected(localId))
					return pNetGame->m_pPools->m_pPlayer->GetName(localId);
			}
		}
		break;
	}
	case rakhook::samp_ver::v03dlr1:
	{
		SAMPAPI_EXPORT sampapi::v03dl::CNetGame* pNetGame = sampapi::v03dl::RefNetGame();
		if (pNetGame && pNetGame->m_pPools)
		{
			plrName = pNetGame->m_pPools->m_pPlayer->GetLocalPlayerName();
			if (strlen(plrName) == 0)
			{
				unsigned short localId = pNetGame->m_pPools->m_pPlayer->m_nLocalPlayerId;
				if (pNetGame->m_pPools->m_pPlayer->IsConnected(localId))
					return pNetGame->m_pPools->m_pPlayer->GetName(localId);
			}
		}
		break;
	}
	default:
		plrName = nullptr;
		break;
	}
	return plrName;
}

std::string GetServerAddressPort()
{
	rakhook::samp_ver version = rakhook::samp_version();
	std::string srvAddrPrt;
	switch (version)
	{
	case rakhook::samp_ver::v037r1:
	{
		SAMPAPI_EXPORT sampapi::v037r1::CNetGame* pNetGame = sampapi::v037r1::RefNetGame();
		if (pNetGame)
			srvAddrPrt = std::string(pNetGame->m_szHostAddress) + ':' + std::to_string(pNetGame->m_nPort);
		break;
	}
	case rakhook::samp_ver::v037r31:
	{
		SAMPAPI_EXPORT sampapi::v037r3::CNetGame* pNetGame = sampapi::v037r3::RefNetGame();
		if (pNetGame)
			srvAddrPrt = std::string(pNetGame->m_szHostAddress) + ':' + std::to_string(pNetGame->m_nPort);
		break;
	}
	case rakhook::samp_ver::v037r5:
	{
		SAMPAPI_EXPORT sampapi::v037r5::CNetGame* pNetGame = sampapi::v037r5::RefNetGame();
		if (pNetGame)
			srvAddrPrt = std::string(pNetGame->m_szHostAddress) + ':' + std::to_string(pNetGame->m_nPort);
		break;
	}
	case rakhook::samp_ver::v03dlr1:
	{
		SAMPAPI_EXPORT sampapi::v03dl::CNetGame* pNetGame = sampapi::v03dl::RefNetGame();
		if (pNetGame)
			srvAddrPrt = std::string(pNetGame->m_szHostAddress) + ':' + std::to_string(pNetGame->m_nPort);
		break;
	}
	}
	return srvAddrPrt;
}

bool IsGameInitialized()
{
	rakhook::samp_ver version = rakhook::samp_version();
	bool _initialized = false;
	switch (version)
	{
	case rakhook::samp_ver::v037r1:
		_initialized = (sampapi::v037r1::RefGame() != nullptr);
		break;
	case rakhook::samp_ver::v037r31:
		_initialized = (sampapi::v037r3::RefGame() != nullptr);
		break;
	case rakhook::samp_ver::v037r5:
		_initialized = (sampapi::v037r5::RefGame() != nullptr);
		break;
	case rakhook::samp_ver::v03dlr1:
		_initialized = (sampapi::v03dl::RefGame() != nullptr);
		break;
	default:
		_initialized = false;
		break;
	}
	return _initialized;
}

static WNDPROC g_origWndProc = nullptr;

static LRESULT CALLBACK WndProcHook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_ACTIVATEAPP)
	{
		windowFocused = (wParam == true);
	}
	return CallWindowProcA(g_origWndProc, hWnd, uMsg, wParam, lParam);
}

static void HookWindowFocus()
{
	HWND hWnd = FindWindowA("Grand theft auto San Andreas", nullptr);
	if (!hWnd)
		return;

	g_origWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcHook)));

	WriteToLogFile(logsPath, "[Focus] Window subclassed (hwnd=%p)", hWnd);
}

static void DiscordPollingThread()
{
	while (!ASIinitialized)
	{
		if (shuttingDown.load(std::memory_order_relaxed))
			return;
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	constexpr auto INTERVAL = std::chrono::seconds(5);

	PlayerState lastState = PlayerState::NotConnected;
	bool wasDebug = false;

	while (!shuttingDown.load(std::memory_order_relaxed))
	{
		bool debugMode = SampInfo::IsDebugMode();

		if (debugMode)
		{
			if (!wasDebug)
			{
				DiscordManager::ClearPresence();
				WriteToLogFile(logsPath, "[Discord] Debug mode detected - presence suppressed");
				wasDebug = true;
			}
			// Still pump callbacks so Discord stays connected even while suppressed.
			DiscordManager::PumpCallbacks();
			std::this_thread::sleep_for(INTERVAL);
			continue;
		}

		if (wasDebug)
		{
			WriteToLogFile(logsPath, "[Discord] Debug mode ended - resuming presence");
			wasDebug = false;
		}

		std::string serverName = SampInfo::GetServerName();
		std::string serverAddress = SampInfo::GetServerAddress();
		int playerCount = SampInfo::GetPlayerCount();
		int maxPlayers = (g_maxPlayers > 0) ? g_maxPlayers : -1;
		PlayerState playerState = SampInfo::GetPlayerState(localPlayerJoined, windowFocused);
		int64_t sessionStart = (playerState != PlayerState::NotConnected && playerState != PlayerState::Connecting)
			? g_sessionStart
			: 0;

		DiscordManager::Update(serverName, serverAddress, playerState, playerCount, maxPlayers, sessionStart);

		// Pump callbacks OUTSIDE the Update mutex, exactly as the official
		// Discord example calls Discord_RunCallbacks() in its game loop.
		DiscordManager::PumpCallbacks();

		lastState = playerState;

		std::this_thread::sleep_for(INTERVAL);
	}
}

void InitializeHooks()
{
	while (GetModuleHandleA("samp.dll") == nullptr)
	{
		if (shuttingDown.load(std::memory_order_relaxed))
			return;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	while (!ASIinitialized)
	{
		if (shuttingDown.load(std::memory_order_relaxed))
			return;

		if (rakhook::samp_addr() && rakhook::samp_version() != rakhook::samp_ver::unknown)
		{
			if (IsGameInitialized())
			{
				if (rakhook::initialize())
				{
					ASIinitialized = true;
					break;
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	HookWindowFocus();

	rakhook::on_receive_rpc += [](unsigned char& id, RakNet::BitStream* bs) -> bool
	{
		if (id != 139)
			return true;

		localPlayerJoined = true;
		g_sessionStart = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());

		std::string addr = GetServerAddressPort();
		std::string name = SampInfo::GetServerName();

		WriteToLogFile(logsPath, "[JOIN] Connected  addr=%s  name=%s", addr.c_str(), name.c_str());

		DiscordManager::Update(name, addr, SampInfo::GetPlayerState(true, windowFocused), SampInfo::GetPlayerCount(), g_maxPlayers, g_sessionStart);

		bs->ResetReadPointer();
		return true;
	};

	rakhook::on_receive_packet += [](auto* packet) -> bool
	{
		unsigned char pid = packet->data[0];

		if (pid == ID_DISCONNECTION_NOTIFICATION || pid == ID_CONNECTION_LOST)
		{
			if (localPlayerJoined)
			{
				const char* reason = (pid == ID_DISCONNECTION_NOTIFICATION) ? "graceful" : "timeout";
				WriteToLogFile(logsPath, "[QUIT] Disconnected (%s)", reason);

				localPlayerJoined = false;
				g_sessionStart = 0;
				g_maxPlayers = -1;

				DiscordManager::ClearPresence();
			}
		}
		else if (pid == ID_CONNECTION_BANNED || pid == ID_INVALID_PASSWORD || pid == ID_NO_FREE_INCOMING_CONNECTIONS || pid == ID_CONNECTION_ATTEMPT_FAILED)
		{
			if (!localPlayerJoined)
				WriteToLogFile(logsPath, "[CONNECT_FAILED] reason id=%d", (int)pid);
		}
		return true;
	};
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		DisableThreadLibraryCalls(hModule);

		if (CreateLogsFolderIfMissing())
		{
			snprintf(logsPath, sizeof(logsPath), "brownturbo-discordrpc.log");
			WriteToLogFile(logsPath, "DiscordRPC - initialized");
			pluginReady = true;
		}
		else
		{
			pluginReady = false;
		}

		if (pluginReady)
		{
			DiscordManager::Init();

			static bool threadsSpawned = false;
			if (!threadsSpawned)
			{
				threadsSpawned = true;
				std::thread(InitializeHooks).detach();
				std::thread(DiscordPollingThread).detach();
			}
		}
		break;
	}

	case DLL_PROCESS_DETACH:
	{
		shuttingDown.store(true, std::memory_order_relaxed);

		if (g_origWndProc)
		{
			HWND hWnd = FindWindowA("Grand theft auto San Andreas", nullptr);
			if (hWnd)
				SetWindowLongPtrA(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_origWndProc));
			g_origWndProc = nullptr;
		}

		rakhook::on_receive_rpc.clear();
		rakhook::on_send_rpc.clear();
		rakhook::on_receive_packet.clear();
		rakhook::on_send_packet.clear();

		if (GetModuleHandleA("samp.dll") != nullptr)
			rakhook::destroy();

		DiscordManager::Shutdown();

		std::lock_guard<std::mutex> lock(logMutex);
		if (g_fLog)
		{
			fclose(g_fLog);
			g_fLog = nullptr;
		}
		break;
	}
	}
	return true;
}
