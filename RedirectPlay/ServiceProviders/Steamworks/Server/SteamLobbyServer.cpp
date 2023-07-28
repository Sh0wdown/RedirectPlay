#include "SteamLobbyServer.h"
#include "Log.h"
#include "SteamPlayServer.h"
#include "SteamServerSettings.h"
#include "Utils/StringUtils.h"

#include <cassert>

CSteamLobby::CSteamLobby()
	: m_state(EState::None)
	, m_lobbyID()
	, m_name()
	, m_password(false)
{
}

CSteamLobby::~CSteamLobby()
{
	Leave();
}

bool CSteamLobby::Create(SSteamServerSettings const& settings)
{
	SteamAPICall_t steamAPICall = SteamMatchmaking()->CreateLobby(settings.lobbyType, settings.maxPlayers);
	if (steamAPICall == k_uAPICallInvalid)
	{
		return false;
	}

	m_steamcallresult_OnLobbyCreated.Set(steamAPICall, this, &CSteamLobby::OnLobbyCreated);
	m_state    = Creating;
	m_password = settings.HasPassword();
	m_name     = settings.name;
	Log::Debug("Creating Lobby...");
	return true;
}

bool CSteamLobby::Join(CSteamID lobbyID)
{
	SteamAPICall_t steamAPICall = SteamMatchmaking()->JoinLobby(lobbyID);
	if (steamAPICall == k_uAPICallInvalid)
	{
		return false;
	}

	m_steamcallresult_OnLobbyEntered.Set(steamAPICall, this, &CSteamLobby::OnLobbyEntered);
	m_state = Joining;
	Log::Debug("Joining Lobby...");
	return true;
}

void CSteamLobby::Leave()
{
	if (m_lobbyID.IsValid())
	{
		SteamMatchmaking()->LeaveLobby(m_lobbyID);
	}
	m_steamcallresult_OnLobbyCreated.Cancel();
	m_state = None;
	UpdateLobbyDetails();
	Log::Debug("Left lobby.");
}

void CSteamLobby::SetGameServer(CSteamID serverID)
{
	SteamMatchmaking()->SetLobbyGameServer(m_lobbyID, 0, 0, serverID);
	UpdateLobbyDetails();
	Log::Debug("Set lobby game server: %llu", serverID.ConvertToUint64());
}

CSteamID CSteamLobby::GetGameServer() const
{
	if (m_lobbyID.IsValid())
	{
		CSteamID serverID;
		if (SteamMatchmaking()->GetLobbyGameServer(m_lobbyID, nullptr, nullptr, &serverID))
		{
			return serverID;
		}
	}
	return CSteamID();
}

void CSteamLobby::UpdateLobbyDetails()
{
	ISteamFriends* pFriends = SteamFriends();
	assert(pFriends != nullptr);

	if (IsInLobby() && m_lobbyID.IsValid())
	{
		pFriends->SetRichPresence("status", "In Multiplayer Game");

		char szBuf[128];
		snprintf(szBuf, sizeof(szBuf), "+connect %llu", m_lobbyID.ConvertToUint64());
		pFriends->SetRichPresence("connect", szBuf);

		if (CSteamID gameServer = GetGameServer(); gameServer.IsValid())
		{
			SteamUser()->AdvertiseGame(gameServer, 0, 0);
			snprintf(szBuf, sizeof(szBuf), "%llu", gameServer.ConvertToUint64());
		}
		else
		{
			szBuf[0] = 0;
		}
		pFriends->SetRichPresence("steam_player_group", szBuf);
	}
	else
	{
		pFriends->ClearRichPresence();
	}
}

void CSteamLobby::OnLobbyCreated(LobbyCreated_t* pInfo, bool)
{
	if (pInfo->m_eResult != k_EResultOK || !CSteamID(pInfo->m_ulSteamIDLobby).IsValid())
	{
		m_state = None;
		return;
	}

	m_state = InLobby;
	m_lobbyID = pInfo->m_ulSteamIDLobby;
	SteamMatchmaking()->SetLobbyData(m_lobbyID, g_szLobbyKeyName, m_name);
	SteamMatchmaking()->SetLobbyData(m_lobbyID, g_szLobbyKeyPassword, m_password ? "1" : "0");
	UpdateLobbyDetails();
	Log::Debug("Created lobby %llu.", m_lobbyID.ConvertToUint64());
}

void CSteamLobby::OnLobbyEntered(LobbyEnter_t* pInfo, bool bIOFailure)
{
	if (!CSteamID(pInfo->m_ulSteamIDLobby).IsValid())
	{
		m_state = None;
		return;
	}

	m_state = InLobby;
	m_lobbyID = pInfo->m_ulSteamIDLobby;
	UpdateLobbyDetails();
	Log::Debug("Entered lobby %llu.", m_lobbyID.ConvertToUint64());
}

void CSteamLobby::OnLobbyKicked(LobbyKicked_t* pInfo)
{
	if (m_lobbyID == CSteamID() ||
		m_lobbyID != CSteamID(pInfo->m_ulSteamIDLobby))
	{
		return;
	}

	m_lobbyID = CSteamID();
	m_state = None;
	UpdateLobbyDetails();
	Log::Debug("Kicked from lobby.");
}

void CSteamLobby::OnLobbyGameCreated(LobbyGameCreated_t*)
{
	UpdateLobbyDetails();
}

