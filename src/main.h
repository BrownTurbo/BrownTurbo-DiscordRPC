#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <windows.h>

#include <sampapi/sampapi.h>

extern bool ASIinitialized;
void WriteToLogFile(const char* path, const char* fmt, ...);

const char* GetLocalPlayerName();
std::string GetServerAddressPort();
bool IsGameInitialized();

void InitializeHooks();

extern char logsPath[300];
extern FILE* g_fLog;

extern std::atomic<bool> localPlayerJoined;
extern std::atomic<bool> windowFocused;
extern std::atomic<int> g_maxPlayers;
extern std::atomic<int64_t> g_sessionStart;
