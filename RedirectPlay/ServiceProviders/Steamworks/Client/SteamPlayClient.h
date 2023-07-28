#pragma once

#include "../SteamTypes.h"
#include "Utils/fstring.h"

#include "DirectX/dplay.h"
#include "Steam/isteamnetworkingsockets.h"
#include "Steam/isteamnetworkingutils.h"
#include "Steam/steamclientpublic.h"
#include "Steam/steamnetworkingtypes.h"

#include <deque>
#include <functional>
#include <memory>

class CSteamPlayClient
{
public:
	enum EState
	{
		Disconnected,
		Connecting,
		PendingAuth,
		Connected,
	};

	struct SCreatePlayerData
	{
		char const* szShortName;
		char const* szLongName;
		bool        serverPlayer;
		bool        spectator;
		void const* pData;
		size_t      dataSize;
	};
	using TCreatePlayerCallback = std::function<void(DPID id)>;

protected:
	struct SPlayerData
	{
		fstring<DPSHORTNAMELEN> shortName;
		fstring<DPLONGNAMELEN>  longName;
		bool                    local;

		// @fixme: DirectPlay seems to keep a player array PER player.
		// Thus the sysmsgs also need to be send for local player creations
		// and to _players_ that don't know about it yet, instead of the _client_ that doesn't know about it yet.
		// std::unordered_set<DPID> players;
	};
	using TPlayers = std::unordered_map<DPID, SPlayerData>;
	using TPlayer  = std::pair<const DPID, SPlayerData>;

	struct SDataMessageCache
	{
		DPID                   from;
		DPID                   to;
		TSteamMessageSharedPtr pMsg;
	};
	using TDataMessages = std::deque<SDataMessageCache>;

public:
	CSteamPlayClient();
	~CSteamPlayClient();

	EState  GetState() const              { return m_state; }
	bool    IsConnected() const           { return m_state == Connected; }
	bool    IsDisconnected() const        { return m_state == Disconnected; }
	bool    IsConnectingOrPending() const { return m_state == Connecting || m_state == PendingAuth; }

	bool    Join(CSteamID serverID, char const* szPassword = nullptr);
	void    Disconnect(EDisconnectReason reason);
	bool    CreatePlayer(SCreatePlayerData const& input, TCreatePlayerCallback callback = nullptr);
	bool    SendData(DPID from, DPID to, void* pData, size_t len, bool reliable, bool sameThread = false);
	bool    DestroyPlayer(DPID dpid);
	
	HRESULT ReceiveData(LPDPID pFrom, LPDPID pTo, DWORD flags, LPVOID pData, LPDWORD pSize);

	void    ReceiveNetworkData();

protected:	
	STEAM_CALLBACK(CSteamPlayClient, OnNetConnectionStatusChanged, SteamNetConnectionStatusChangedCallback_t);

	void ProcessNetworkingMessage(TSteamMessageUniquePtr pSteamMessage);
	void OnReceiveInfo(TSteamMessageUniquePtr pSteamMessage);
	void OnReceiveAuthPassed(TSteamMessageUniquePtr pSteamMessage);
	void OnReceiveCreatePlayerResponse(TSteamMessageUniquePtr pSteamMessage);
	void OnReceivePlayerCreated(TSteamMessageUniquePtr pSteamMessage);
	void OnReceivePlayerDestroyed(TSteamMessageUniquePtr pSteamMessage);
	void OnReceiveData(TSteamMessageUniquePtr pSteamMessage);

	TPlayer* FindPlayer(DPID dpid);

protected:
	EState                 m_state;

	CSteamID               m_serverID;
	HSteamNetConnection    m_serverConnection;
	HAuthTicket            m_authTicket;
	fstring<DPPASSWORDLEN> m_password;

	TPlayers               m_players;

	TCreatePlayerCallback  m_createPlayerCallback;

	TDataMessages          m_dataMessages;
};

inline CSteamPlayClient::TPlayer* CSteamPlayClient::FindPlayer(DPID dpid)
{
	TPlayers::iterator it = m_players.find(dpid);
	return it != m_players.end() ? &(*it) : nullptr;
}
