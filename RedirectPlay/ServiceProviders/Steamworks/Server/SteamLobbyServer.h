#pragma once

#include "../SteamTypes.h"
#include "Utils/fstring.h"

#include "DirectX/dplay.h"
#include "Steam/isteammatchmaking.h"

struct SSteamServerSettings;

// Helper lobby to forward players to a session since local game servers
// do not work properly when requesting server lists or joining via Steam UI.
class CSteamLobby
{
public:
	enum EState
	{
		None,
		Creating,
		Joining,
		InLobby,
	};

	CSteamLobby();
	~CSteamLobby();

	bool     Create(SSteamServerSettings const& settings);
	bool     Join(CSteamID lobbyID);
	void     Leave();

	EState   GetState() const       { return m_state; }
	bool     IsInLobby() const      { return m_state == InLobby; }
	bool     IsDisconnected() const { return m_state == None; }

	CSteamID GetSteamID() const     { return m_lobbyID; }

	void     SetGameServer(CSteamID serverID);
	CSteamID GetGameServer() const;

	void     UpdateLobbyDetails();

protected:
	void OnLobbyCreated(LobbyCreated_t* pInfo, bool IOFailure);
	CCallResult<CSteamLobby, LobbyCreated_t> m_steamcallresult_OnLobbyCreated;

	void OnLobbyEntered(LobbyEnter_t* pCallback, bool bIOFailure);
	CCallResult<CSteamLobby, LobbyEnter_t> m_steamcallresult_OnLobbyEntered;

	STEAM_CALLBACK(CSteamLobby, OnLobbyKicked, LobbyKicked_t);
	STEAM_CALLBACK(CSteamLobby, OnLobbyGameCreated, LobbyGameCreated_t);

	EState   m_state;
	CSteamID m_lobbyID;

	fstring<DPSESSIONNAMELEN> m_name;
	bool     m_password;
};

