#include "SteamPlayServer.h"
#include "../Messages/Messages.h"
#include "../Messages/MessageSender.h"
#include "../SteamPlayUtilities.h"
#include "DirectPlay/Utils.h"
#include "Log.h"
#include "Utils/StringUtils.h"

#include "Steam/steamclientpublic.h"

#include <cassert>
#include <thread>
#include <type_traits>

constexpr uint32      s_desiredIP = INADDR_ANY;
constexpr uint16      s_gamePort  = 27015;
constexpr uint16      s_queryPort = 27016;
constexpr char        s_version[] = "1.0.0.0";
constexpr EServerMode s_serverAuthMode = eServerModeAuthenticationAndSecure;

constexpr bool UseAuth() { return s_serverAuthMode >= eServerModeAuthentication; }

constexpr TClock::duration s_clientTimeoutDuration = std::chrono::seconds(50);

using TMessageSender = CMessageSender<SteamGameServerNetworkingSockets, Log::ESource::Server>;

CSteamPlayServer::CSteamPlayServer()
	: m_state(EState::Disconnected)
	, m_listenSocket(k_HSteamListenSocket_Invalid)
	, m_netPollGroup(k_HSteamNetPollGroup_Invalid)
	, m_clients()
	, m_players()
	, m_settings()
	, m_sendDataBuf()
	, m_timeoutDuration(s_clientTimeoutDuration)
	, m_pThread(nullptr)
	, m_quitting(false)
{
}

CSteamPlayServer::~CSteamPlayServer()
{
	Close();
}

CSteamID CSteamPlayServer::GetSteamID() const
{
	ISteamGameServer* pGameServer = SteamGameServer();
	return pGameServer ? pGameServer->GetSteamID() : CSteamID();
}

bool CSteamPlayServer::Start(SSteamServerSettings const& settings)
{
	if (m_state != EState::Disconnected)
	{
		assert(("Server is already connected!", m_state == EState::Disconnected));
		return false;
	}

	if (!SteamGameServer_Init(s_desiredIP, s_gamePort, s_queryPort, s_serverAuthMode, s_version))
	{
		Log::WarnServer("Steam GameServer init failed.");
		return false;
	}

	if (!SteamGameServer() || !SteamNetworkingUtils())
	{
		Log::WarnServer("Steam GameServer interface is invalid.");
		return false;
	}

	char szAppName[64];
	GetProductName(szAppName, sizeof(szAppName));
	char szDescription[80];
	snprintf(szDescription, sizeof(szDescription), "%s Multiplayer", szAppName);
	SteamGameServer()->SetModDir(szAppName);
	SteamGameServer()->SetProduct(szAppName);
	SteamGameServer()->SetGameDescription(szDescription);

	SteamGameServer()->LogOnAnonymous();
	SteamGameServer()->SetAdvertiseServerActive(false);
	//SteamNetworkingUtils()->InitRelayNetworkAccess();

	m_listenSocket = SteamGameServerNetworkingSockets()->CreateListenSocketP2P(0, 0, nullptr);
	m_netPollGroup = SteamGameServerNetworkingSockets()->CreatePollGroup();

	m_settings = settings;
	m_state = EState::Connecting;

	m_quitting = false;
	m_pThread = new std::thread([this]() { this->UpdateLoop(); });

	Log::InfoServer("Connecting...");
	return true;
}

void CSteamPlayServer::Close()
{
	if (m_state != EState::Disconnected)
	{
		m_quitting = true;
		m_pThread->join();
		delete m_pThread;

		// tell clients we are exiting
		for (TClient& client : m_clients)
		{
			SteamGameServerNetworkingSockets()->CloseConnection(client.first, (int)EDisconnectReason::ServerClosed, nullptr, false);
			SteamGameServer()->EndAuthSession(client.second.steamId);
		}

		SteamGameServerNetworkingSockets()->CloseListenSocket(m_listenSocket);
		SteamGameServerNetworkingSockets()->DestroyPollGroup(m_netPollGroup);

		// Disconnect from the steam servers
		SteamGameServer()->LogOff();

		// release our reference to the steam client library
		SteamGameServer_Shutdown();

		m_state = EState::Disconnected;

		m_settings = SSteamServerSettings();
		m_clients.clear();
		m_players.clear();

		Log::InfoServer("Disconnected.");
	}
}

constexpr size_t TicksPerSecond = 60;

void CSteamPlayServer::UpdateLoop()
{
	static constexpr TClock::duration s_tickDuration(TClock::duration::period::den / TicksPerSecond);

	while (!m_quitting)
	{
		TClock::time_point const start = TClock::now();
		SteamGameServer_RunCallbacks();
		ReceiveNetworkData();
		TClock::time_point const end = TClock::now();

		std::chrono::microseconds const diff = std::chrono::duration_cast<std::chrono::microseconds>(s_tickDuration - (end - start));
		if (diff.count() > 0)
		{
			std::this_thread::sleep_for(diff);
		}
	}
}

void CSteamPlayServer::ReceiveNetworkData()
{
	static constexpr size_t s_maxMessages = 128;

	if (!SteamGameServerNetworkingSockets())
	{
		return;
	}

	SteamNetworkingMessage_t* messages[s_maxMessages];
	int const count = SteamGameServerNetworkingSockets()->ReceiveMessagesOnPollGroup(m_netPollGroup, messages, s_maxMessages);

	if (m_state != EState::Connected)
	{
		for (int i = 0; i < count; ++i)
		{
			assert(messages[i] != nullptr);
			messages[i]->Release();
		}
		return;
	}

	for (int i = 0; i < count; ++i)
	{
		ProcessNetworkingMessage(messages[i]);
	}
}

void CSteamPlayServer::UpdateSteamServerDetails()
{
	ISteamGameServer* pGameServer = SteamGameServer();
	if (!pGameServer)
	{
		return;
	}

	pGameServer->SetServerName(m_settings.name);
	pGameServer->SetMaxPlayerCount(m_settings.maxPlayers);
	pGameServer->SetPasswordProtected(HasPassword());
}

void CSteamPlayServer::OnSteamServersConnected(SteamServersConnected_t*)
{
	UpdateSteamServerDetails();
	m_state = EState::Connected;
	Log::InfoServer("Connected to SteamServers.");
}

void CSteamPlayServer::OnSteamServersConnectFailure(SteamServerConnectFailure_t*)
{
	Log::InfoServer("Failure when connecting to SteamServers.");
	Close();
}

void CSteamPlayServer::OnSteamServersDisconnected(SteamServersDisconnected_t*)
{
	Log::InfoServer("Disconnected from SteamServers.");
	Close(); // todo: could this be the source of the callback?
}

void CSteamPlayServer::OnNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pCallback)
{
	if (!pCallback)
	{
		assert(pCallback != nullptr);
		return;
	}

	SteamNetConnectionInfo_t const& info = pCallback->m_info;
	ESteamNetworkingConnectionState const oldState = pCallback->m_eOldState;
	ESteamNetworkingConnectionState const newState = info.m_eState;

	HSteamNetConnection const connection = pCallback->m_hConn;
	CSteamID const steamID = info.m_identityRemote.GetSteamID();

	if (oldState == k_ESteamNetworkingConnectionState_None &&
		newState == k_ESteamNetworkingConnectionState_Connecting)
	{
		AddClient(connection, steamID);
	}
	else if ((oldState == k_ESteamNetworkingConnectionState_Connecting || oldState == k_ESteamNetworkingConnectionState_Connected) &&
		newState == k_ESteamNetworkingConnectionState_ClosedByPeer)
	{
		Log::DebugServer("Client lost %i %i", oldState, newState);
		RemoveClient(connection, EDisconnectReason::ClientDisconnect);
	}
}

void CSteamPlayServer::AddClient(HSteamNetConnection connection, CSteamID steamID)
{
	if (connection == k_HSteamNetConnection_Invalid)
	{
		return;
	}

	if (UseAuth() && !steamID.IsValid())
	{
		SteamGameServerNetworkingSockets()->CloseConnection(connection, (int)EDisconnectReason::ServerReject, "Invalid Steam ID!", false);
		Log::InfoServer("Rejecting client connection, invalid Steam ID.");
		return;
	}

	if (m_clients.find(connection) != m_clients.end())
	{
		Log::WarnServer("Added client connection already exists.");
		return;
	}

	if (m_players.size() >= m_settings.maxPlayers)
	{
		// No empty slots. Server full!
		SteamGameServerNetworkingSockets()->CloseConnection(connection, (int)EDisconnectReason::ServerFull, "Server full!", false);
		Log::InfoServer("Rejecting client connection, server is full.");
		return;
	}

	EResult const result = SteamGameServerNetworkingSockets()->AcceptConnection(connection);
	if (result != k_EResultOK)
	{
		SteamGameServerNetworkingSockets()->CloseConnection(connection, k_ESteamNetConnectionEnd_AppException_Generic, "Failed to accept connection.", false);
		Log::InfoServer("Accepting connection failed: %u.", result);
		return;
	}

	SteamGameServerNetworkingSockets()->SetConnectionPollGroup(connection, m_netPollGroup);

	m_clients[connection] = { steamID, false, TClock::now() };

	TMessageSender::TrySend<Messages::Server::SInfo>(
		connection,
		k_nSteamNetworkingSend_Reliable,
		[this](Messages::Server::SInfo& message)
		{
			message.auth     = UseAuth();
			message.password = HasPassword();
		});

	Log::InfoServer("Accepted Client %u.", connection);
}

bool CSteamPlayServer::RemoveClient(HSteamNetConnection connection, EDisconnectReason reason)
{
	TClients::iterator entry = m_clients.find(connection);
	if (entry != m_clients.end())
	{
		RemoveClient(entry, reason);
		return true;
	}
	return false;
}

CSteamPlayServer::TClients::iterator CSteamPlayServer::RemoveClient(TClients::iterator entry, EDisconnectReason reason)
{
	assert(entry != m_clients.end());

	if (UseAuth())
	{
		SteamGameServer()->EndAuthSession(entry->second.steamId);
	}

	HSteamNetConnection connection = entry->first;
	SteamGameServerNetworkingSockets()->CloseConnection(connection, (int)reason, nullptr, false);
	TClients::iterator const it = m_clients.erase(entry);

	for (auto pit = m_players.begin(); pit != m_players.end();)
	{
		if (pit->second.connection == connection)
		{
			pit = DestroyPlayer(pit);
		}
		else
		{
			++pit;
		}
	}


	Log::InfoServer("Removed Client %u.", connection);
	return it;
}

void CSteamPlayServer::ProcessNetworkingMessage(TSteamMessageUniquePtr pSteamMessage)
{
	assert(pSteamMessage != nullptr);
	assert(pSteamMessage->GetData() != nullptr);
	assert(pSteamMessage->GetSize() > 0);

	TClients::iterator clientIt = m_clients.find(pSteamMessage->GetConnection());
	if (clientIt == m_clients.end())
	{
		Log::InfoServer("Client message sender %u is unknown.", pSteamMessage->GetConnection());
		return;
	}

	if (pSteamMessage->GetSize() < sizeof(SMessage))
	{
		Log::WarnServer("Client message was too short.");
		return;
	}

	size_t receiveSize;
	void(CSteamPlayServer::*pReceive)(TClient&, TSteamMessageUniquePtr);

	SMessage const& message = *static_cast<SMessage*>(pSteamMessage->m_pData);
	switch (message.GetId())
	{
	case Messages::Client::SBeginAuth::ID:
		pReceive = &CSteamPlayServer::OnReceiveBeginAuth;
		receiveSize = sizeof(Messages::Client::SBeginAuth);
		break;
	case Messages::Client::SCreatePlayer::ID:
		pReceive = &CSteamPlayServer::OnReceiveCreatePlayer;
		receiveSize = sizeof(Messages::Client::SCreatePlayer);
		break;
	case Messages::Client::SDestroyPlayer::ID:
		pReceive = &CSteamPlayServer::OnReceiveDestroyPlayer;
		receiveSize = sizeof(Messages::Client::SDestroyPlayer);
		break;
	case Messages::Shared::SData::ID:
		pReceive = &CSteamPlayServer::OnReceiveData;
		receiveSize = sizeof(Messages::Shared::SData);
		break;
	default:
		Log::WarnServer("Client sent unregistered message %u.", message.GetId());
		return;
	}

	if (pSteamMessage->GetSize() < receiveSize)
	{
		Log::WarnServer("Client message %u was too short. Got %u but expected at least %u.", message.GetId(), pSteamMessage->GetSize(), receiveSize);
		return;
	}

	(this->*pReceive)(*clientIt, std::move(pSteamMessage));
}

void CSteamPlayServer::OnReceiveBeginAuth(TClient& client, TSteamMessageUniquePtr pSteamMessage)
{
	assert(pSteamMessage != nullptr);
	Messages::Client::SBeginAuth const& message = *static_cast<Messages::Client::SBeginAuth*>(pSteamMessage->m_pData);

	if (HasPassword() && strncmp(m_settings.password, message.szPassword, ArrayCount(message.szPassword)) != 0)
	{
		RemoveClient(client.first, EDisconnectReason::ServerReject);
	}
	else if (UseAuth())
	{
		// authenticate the user with the Steam back-end servers
		EBeginAuthSessionResult const result = SteamGameServer()->BeginAuthSession(message.pToken, message.tokenLen, client.second.steamId);
		if (result != k_EBeginAuthSessionResultOK)
		{
			RemoveClient(client.first, EDisconnectReason::ServerReject);
		}
	}
	else
	{
		OnAuthCompleted(client, true);
	}
}

void CSteamPlayServer::OnValidateAuthTicketResponse(ValidateAuthTicketResponse_t* pInfo)
{
	for (TClients::iterator it = m_clients.begin(), end = m_clients.end(); it != end; ++it)
	{
		if (pInfo->m_SteamID == it->second.steamId)
		{
			OnAuthCompleted(*it, pInfo->m_eAuthSessionResponse == k_EAuthSessionResponseOK);
			return;
		}
	}
}

void CSteamPlayServer::OnAuthCompleted(TClient& client, bool success)
{
	if (!success)
	{
		RemoveClient(client.first, EDisconnectReason::ServerReject);
		return;
	}
	client.second.authorized = true;

	TMessageSender::Send<Messages::Server::SAuthPassed>(client.first, k_nSteamNetworkingSend_Reliable);
}

DPID CSteamPlayServer::FindEmptyId() const
{
	for (size_t i = 0; i < 1000; ++i)
	{
		DPID newId = GenerateDPID();
		if (m_players.find(newId) == m_players.end())
		{
			return newId;
		}
	}
	return DPID_UNKNOWN;
}

void CSteamPlayServer::OnReceiveCreatePlayer(TClient& client, TSteamMessageUniquePtr pSteamMessage)
{
	assert(pSteamMessage != nullptr);
	Messages::Client::SCreatePlayer& message = *static_cast<Messages::Client::SCreatePlayer*>(pSteamMessage->m_pData);

	SPlayerData* pPlayerData = nullptr;

	DPID const id = m_players.size() < m_settings.maxPlayers ? FindEmptyId() : DPID_UNKNOWN;
	if (id != DPID_UNKNOWN)
	{
		pPlayerData = &m_players[id];
		*pPlayerData = SPlayerData{ client.first, message.szShortName, message.szLongName };

		for (TClient const& other : m_clients)
		{
			if (other.first != client.first)
			{
				// should we also send message.pData?

				TMessageSender::TrySend<Messages::Server::SPlayerCreated>(
					other.first,
					k_nSteamNetworkingSend_Reliable,
					[id, pPlayerData](Messages::Server::SPlayerCreated& message)
					{
						message.dpid = id;
						pPlayerData->shortName.copyTo(message.szShortName);
						pPlayerData->longName.copyTo(message.szLongName);
					});
			}
		}
	}

	TMessageSender::TrySend<Messages::Server::SCreatePlayerResponse>(
		client.first,
		k_nSteamNetworkingSend_Reliable,
		[id, pPlayerData](Messages::Server::SCreatePlayerResponse& message)
		{
			message.dpid = id;
			pPlayerData->shortName.copyTo(message.szShortName);
			pPlayerData->longName.copyTo(message.szLongName);
		});
}

void CSteamPlayServer::OnReceiveDestroyPlayer(TClient& client, TSteamMessageUniquePtr pSteamMessage)
{
	assert(pSteamMessage != nullptr);
	Messages::Client::SDestroyPlayer& message = *static_cast<Messages::Client::SDestroyPlayer*>(pSteamMessage->m_pData);

	if (message.dpid == DPID_ALLPLAYERS)
	{
		TPlayers::iterator it = m_players.begin();
		while (it != m_players.end())
		{
			if (it->second.connection == client.first)
			{
				it = DestroyPlayer(it);
			}
			else
			{
				++it;
			}
		}
	}
	else
	{
		auto const it = m_players.find(message.dpid);
		if (it != m_players.end() && client.first == it->second.connection)
		{
			DestroyPlayer(it);
		}
	}
}

CSteamPlayServer::TPlayers::iterator CSteamPlayServer::DestroyPlayer(TPlayers::iterator validEntry)
{
	assert(validEntry != m_players.end());

	DPID const id = validEntry->first;
	for (TClient const& client : m_clients)
	{
		if (client.first != validEntry->second.connection)
		{
			TMessageSender::TrySend<Messages::Server::SPlayerDestroyed>(
				client.first,
				k_nSteamNetworkingSend_Reliable,
				[id](Messages::Server::SPlayerDestroyed& message)
				{
					message.dpid = id;
				});
		}
	}

	Log::DebugServer("Destroy Player '%u'", validEntry->first);
	return m_players.erase(validEntry);
}

void CSteamPlayServer::OnReceiveData(TClient& client, TSteamMessageUniquePtr pSteamMessage)
{
	assert(pSteamMessage != nullptr);
	Messages::Shared::SData& message = *static_cast<Messages::Shared::SData*>(pSteamMessage->m_pData);

	TPlayers::iterator fromIt = m_players.find(message.from);
	if (fromIt == m_players.end() || fromIt->second.connection != client.first)
	{
		Log::InfoServer("Got client data with invalid sender id.");
		return;
	}

	if (message.to == DPID_ALLPLAYERS)
	{
		for (TClient& recipient : m_clients)
		{
			if (recipient.first != client.first)
			{
				if (TSteamMessageUniquePtr pSteamMessageCopy = TMessageSender::Copy(*pSteamMessage))
				{
					TMessageSender::Send(std::move(pSteamMessageCopy), recipient.first, pSteamMessage->m_nFlags);
				}
			}
		}
	}
	else
	{
		TPlayers::iterator toIt = m_players.find(message.to);
		if (toIt != m_players.end())
		{
			TMessageSender::Send(std::move(pSteamMessage), toIt->second.connection, pSteamMessage->m_nFlags);
		}
		else
		{
			Log::InfoServer("Could not find client data recipient with player id %u.", message.to);
		}
	}
}



