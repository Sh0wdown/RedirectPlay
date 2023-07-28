#pragma once

#include "Utils/Memory.h"

#include "Steam/steamnetworkingtypes.h"

#include <chrono>

using TClock = std::chrono::steady_clock;

static constexpr char const* g_szLobbyKeyName     = "name";
static constexpr char const* g_szLobbyKeyPassword = "pw";

enum class EDisconnectReason
{
	ClientDisconnect = ESteamNetConnectionEnd::k_ESteamNetConnectionEnd_App_Min + 1,
	ServerClosed,
	ServerReject,
	ServerFull,
	ClientKicked
};

static void ReleaseSteamMessage(SteamNetworkingMessage_t* pMessage)
{
	pMessage->Release();
}

using TSteamMessageSharedPtr = std::shared_ptr<SteamNetworkingMessage_t>;
using TSteamMessageUniquePtr = CUniquePtr<SteamNetworkingMessage_t, ReleaseSteamMessage>;
