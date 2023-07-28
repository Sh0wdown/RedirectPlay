#pragma once

#include "../SteamTypes.h"
#include "Utils/fstring.h"

#include "DirectX/dplay.h"
#include "Steam/isteammatchmaking.h"

#include <vector>

class SSteamLobbiesRequest
{
public:
	struct SLobby
	{
		CSteamID id;
		bool     password;
		bool     steamFriend;
		size_t   maxPlayers;
		size_t   currentPlayers;
		fstring<DPSESSIONNAMELEN> name;
	};

public:
	SSteamLobbiesRequest();
	~SSteamLobbiesRequest() { Cancel(); }

	bool Request();
	void Cancel();
	bool IsRequesting() const { return m_requestingList || m_requestingData > 0; }

	auto begin() const { return m_lobbies.cbegin(); }
	auto end()   const { return m_lobbies.cend(); }

protected:
	CCallResult<SSteamLobbiesRequest, LobbyMatchList_t> m_steamcallresult_OnRequestResult;
	void OnRequestResult(LobbyMatchList_t* pLobbyMatchList, bool IOFailure);
	void AddLobby(CSteamID lobbyID, bool steamFriend);
	static bool SetLobbyData(SLobby& lobby);

	STEAM_CALLBACK(SSteamLobbiesRequest, OnDataUpdated, LobbyDataUpdate_t);

	bool                m_requestingList;
	size_t              m_requestingData;
	std::vector<SLobby> m_lobbies;
};

