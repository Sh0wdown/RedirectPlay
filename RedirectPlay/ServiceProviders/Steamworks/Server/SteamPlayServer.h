#pragma once

#include "../SteamTypes.h"
#include "SteamServerSettings.h"

#include "Steam/steam_gameserver.h"
#include "DirectX/dplay.h"

#include <functional> // needed for callbacks
#include <unordered_map>

class CSteamPlayServer
{
public:
	enum EState
	{
		Disconnected,
		Connecting,
		Connected,
	};

private:
	struct SClientData
	{
		CSteamID           steamId;
		bool               authorized;
		TClock::time_point lastMessage;
	};
	using TClients = std::unordered_map<HSteamNetConnection, SClientData>;
	using TClient  = std::pair<const HSteamNetConnection, SClientData>;

	struct SPlayerData
	{
		HSteamNetConnection     connection;
		fstring<DPSHORTNAMELEN> shortName;
		fstring<DPLONGNAMELEN>  longName;
	};
	using TPlayers = std::unordered_map<DPID, SPlayerData>;
	using TPlayer  = std::pair<const DPID, SPlayerData>;

public:
	CSteamPlayServer();
	~CSteamPlayServer();

	CSteamID         GetSteamID() const;

	EState           GetState() const       { return m_state; }
	bool             IsConnected() const    { return m_state == Connected; }
	bool             IsDisconnected() const { return m_state == Disconnected; }

	bool             Start(SSteamServerSettings const& settings);
	void             Close();

private:
	STEAM_GAMESERVER_CALLBACK(CSteamPlayServer, OnSteamServersConnected,      SteamServersConnected_t);
	STEAM_GAMESERVER_CALLBACK(CSteamPlayServer, OnSteamServersConnectFailure, SteamServerConnectFailure_t);
	STEAM_GAMESERVER_CALLBACK(CSteamPlayServer, OnSteamServersDisconnected,   SteamServersDisconnected_t);

	STEAM_GAMESERVER_CALLBACK(CSteamPlayServer, OnValidateAuthTicketResponse, ValidateAuthTicketResponse_t);
	STEAM_GAMESERVER_CALLBACK(CSteamPlayServer, OnNetConnectionStatusChanged, SteamNetConnectionStatusChangedCallback_t);

	bool               HasPassword() const { return m_settings.HasPassword(); }

	void               UpdateLoop();
	void               ReceiveNetworkData();
	void               UpdateSteamServerDetails();

	void               AddClient(HSteamNetConnection connection, CSteamID steamID);
	bool               RemoveClient(HSteamNetConnection connection, EDisconnectReason reason);
	TClients::iterator RemoveClient(TClients::iterator entry, EDisconnectReason reason);

	void               ProcessNetworkingMessage(TSteamMessageUniquePtr pSteamMessage);
	void               OnReceiveBeginAuth(TClient& client, TSteamMessageUniquePtr pSteamMessage);
	void               OnReceiveCreatePlayer(TClient& client, TSteamMessageUniquePtr pSteamMessage);
	void               OnReceiveDestroyPlayer(TClient& client, TSteamMessageUniquePtr pSteamMessage);
	void               OnReceiveData(TClient& client, TSteamMessageUniquePtr pSteamMessage);

	void               OnAuthCompleted(TClient& client, bool success);

	DPID               FindEmptyId() const;

	TPlayers::iterator DestroyPlayer(TPlayers::iterator validEntry);

private:
	std::atomic<EState>  m_state;

	HSteamListenSocket   m_listenSocket;
	HSteamNetPollGroup   m_netPollGroup;

	TClients             m_clients;
	TPlayers             m_players;
	 
	SSteamServerSettings m_settings;

	std::vector<char>    m_sendDataBuf;

	TClock::duration     m_timeoutDuration;

	std::thread*         m_pThread;
	std::atomic_bool     m_quitting;
};

