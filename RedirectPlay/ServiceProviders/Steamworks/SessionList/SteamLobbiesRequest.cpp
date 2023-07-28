#include "SteamLobbiesRequest.h"
#include "../SteamTypes.h"
#include "Utils/StringUtils.h"
#include "Log.h"

#include "Steam/isteamutils.h"

constexpr ELobbyDistanceFilter s_distanceFilter = k_ELobbyDistanceFilterDefault;

SSteamLobbiesRequest::SSteamLobbiesRequest()
	: m_requestingList(false)
	, m_requestingData(false)
	, m_lobbies()
{
}

bool SSteamLobbiesRequest::Request()
{
	if (IsRequesting())
	{
		return true;
	}

	Log::Debug("Requested lobby list.");

	m_lobbies.clear();

	// Check friends' lobbies first
	if (ISteamFriends* pFriends = SteamFriends())
	{
		uint32 const appID = SteamUtils()->GetAppID();
		for (int i = 0, n = pFriends->GetFriendCount(k_EFriendFlagImmediate); i < n; ++i)
		{
			CSteamID const friendID = pFriends->GetFriendByIndex(i, k_EFriendFlagImmediate);
			if (friendID.IsValid())
			{
				FriendGameInfo_t gameInfo;
				if (pFriends->GetFriendGamePlayed(friendID, &gameInfo)
					&& gameInfo.m_gameID.IsSteamApp() && gameInfo.m_gameID.AppID() == appID)
				{
					AddLobby(gameInfo.m_steamIDLobby, true);
				}
			}
		}
	}

	// now check matchmaking
	SteamMatchmaking()->AddRequestLobbyListDistanceFilter(s_distanceFilter);
	SteamAPICall_t const result = SteamMatchmaking()->RequestLobbyList();
	if (result != k_uAPICallInvalid)
	{
		m_requestingList = true;
		m_steamcallresult_OnRequestResult.Set(result, this, &SSteamLobbiesRequest::OnRequestResult);
	}
	else
	{
		m_requestingList = false;
	}
	return true;
}

void SSteamLobbiesRequest::Cancel()
{
	m_requestingData = 0;
	m_requestingList = false;
	m_steamcallresult_OnRequestResult.Cancel();
}

void SSteamLobbiesRequest::OnRequestResult(LobbyMatchList_t* info, bool)
{
	if (!m_requestingList)
	{
		return;
	}
	m_requestingList = false;

	if (!info || info->m_nLobbiesMatching == 0)
	{
		return;
	}

	// sorted by closeness
	for (uint32 i = 0; i < info->m_nLobbiesMatching; ++i)
	{
		AddLobby(SteamMatchmaking()->GetLobbyByIndex(i), false);
	}
}

void SSteamLobbiesRequest::AddLobby(CSteamID lobbyID, bool steamFriend)
{
	if (!lobbyID.IsValid() && lobbyID.IsLobby())
	{
		return;
	}

	SLobby* pLobby;
	auto it = std::find_if(m_lobbies.begin(), m_lobbies.end(), [lobbyID](const SLobby& other) { return other.id == lobbyID; });
	if (it != m_lobbies.end())
	{
		pLobby = &*it;
	}
	else
	{
		pLobby = &m_lobbies.emplace_back();
		pLobby->id          = lobbyID;
		pLobby->steamFriend = false;
	}

	pLobby->steamFriend |= steamFriend;
	if (!SetLobbyData(*pLobby) && SteamMatchmaking()->RequestLobbyData(pLobby->id))
	{
		++m_requestingData;
	}
}

bool SSteamLobbiesRequest::SetLobbyData(SLobby& lobby)
{
	bool success = true;
	lobby.maxPlayers = (std::max)(SteamMatchmaking()->GetLobbyMemberLimit(lobby.id), 0);
	lobby.currentPlayers = (std::max)(SteamMatchmaking()->GetNumLobbyMembers(lobby.id), 0);

	char const* szName = SteamMatchmaking()->GetLobbyData(lobby.id, g_szLobbyKeyName);
	if (szName && szName[0])
	{
		lobby.name = szName;
	}
	else
	{
		lobby.name = "Lobby";
		success = false;
	}

	char const* szPassword = SteamMatchmaking()->GetLobbyData(lobby.id, g_szLobbyKeyPassword);
	if (szPassword && szPassword[0])
	{
		lobby.password = szPassword[0] == '1';
	}
	else
	{
		lobby.password = false;
		success = false;
	}

	return success;
}

void SSteamLobbiesRequest::OnDataUpdated(LobbyDataUpdate_t* info)
{
	if (m_requestingData == 0)
	{
		return;
	}
	--m_requestingData;

	if (!info)
	{
		return;
	}

	auto it = std::find_if(m_lobbies.begin(), m_lobbies.end(), [info](const SLobby& other) { return other.id == info->m_ulSteamIDLobby; });
	if (it != m_lobbies.end())
	{
		if (!info->m_bSuccess || !SetLobbyData(*it))
		{
			m_lobbies.erase(it);
		}
	}
}
