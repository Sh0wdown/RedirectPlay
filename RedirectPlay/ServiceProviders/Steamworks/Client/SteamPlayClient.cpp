#include "SteamPlayClient.h"
#include "../Messages/Messages.h"
#include "../Messages/MessageSender.h"
#include "../SteamTypes.h"
#include "Log.h"
#include "DirectPlay/Utils.h"
#include "Dialogs.h"
#include "Utils/StringUtils.h"

#include "Steam/isteammatchmaking.h"
#include "Steam/isteamnetworkingsockets.h"
#include "Steam/isteamuser.h"

#include <unordered_map>
#include <cassert>

using TMessageSender = CMessageSender<SteamNetworkingSockets, Log::ESource::Client>;

CSteamPlayClient::CSteamPlayClient()
	: m_state(EState::Disconnected)
	, m_serverID()
	, m_serverConnection()
	, m_authTicket()
	, m_password()
	, m_players()
	, m_createPlayerCallback()
	, m_dataMessages()
{
}

CSteamPlayClient::~CSteamPlayClient()
{ 
	Disconnect(EDisconnectReason::ClientDisconnect);
}

bool CSteamPlayClient::Join(CSteamID serverID, char const* szPassword)
{
	if (!serverID.IsValid())
	{
		return false;
	}

	SteamNetworkingIdentity identity{ };
	identity.SetSteamID(serverID);
	m_serverConnection = SteamNetworkingSockets()->ConnectP2P(identity, 0, 0, nullptr);
	if (m_serverConnection == k_HSteamNetConnection_Invalid)
	{
		return false;
	}

	//if (m_pP2PAuthedGame)
	//	m_pP2PAuthedGame->m_hConnServer = m_hConnServer;

	m_state = Connecting;
	m_serverID = serverID;
	m_password.assign(szPassword);
	
	Log::InfoClient("Connecting to server...");
	return true;
}

void CSteamPlayClient::Disconnect(EDisconnectReason reason)
{
	if (m_serverConnection != k_HSteamNetConnection_Invalid)
	{
		//if (m_pP2PAuthedGame)
		//	m_pP2PAuthedGame->EndGame();

		if (m_authTicket != k_HAuthTicketInvalid)
		{
			SteamUser()->CancelAuthTicket(m_authTicket);
			m_authTicket = k_HAuthTicketInvalid;
		}
		else
		{
			SteamUser()->AdvertiseGame(k_steamIDNil, 0, 0);
		}

		SteamNetworkingSockets()->CloseConnection(m_serverConnection, (int)reason, nullptr, false);
		m_state = Disconnected;
		m_serverID = CSteamID();
		m_serverConnection = k_HSteamNetConnection_Invalid;
		m_players.clear();
		m_password.clear();

		m_dataMessages.clear();

		Log::InfoClient("Disconnected from server %u.", reason);
	}
}

bool CSteamPlayClient::CreatePlayer(SCreatePlayerData const& input, TCreatePlayerCallback callback)
{
	bool result = TMessageSender::TrySend<Messages::Client::SCreatePlayer>(
		m_serverConnection,
		k_nSteamNetworkingSend_Reliable,
		input.dataSize,
		[&input](Messages::Client::SCreatePlayer& message)
		{
			strncpy(message.szShortName, input.szShortName, ArrayCount(message.szShortName));
			strncpy(message.szLongName, input.szLongName, ArrayCount(message.szLongName));
			message.serverPlayer = input.serverPlayer;
			message.spectator    = input.spectator;
		});

	if (!result)
	{
		return false;
	}

	m_createPlayerCallback = std::move(callback);
	return true;
}

bool CSteamPlayClient::SendData(DPID from, DPID to, void* pData, size_t len, bool reliable, bool sameThread)
{
	int flags = 0;
	if (reliable)
	{
		flags |= k_nSteamNetworkingSend_Reliable;
	}
	if (sameThread)
	{
		flags |= k_nSteamNetworkingSend_UseCurrentThread;
	}

	// todo: return HRESULT / pending etc.?
	return TMessageSender::TrySend<Messages::Shared::SData>(
		m_serverConnection,
		flags,
		len,
		[from, to, pData, len](Messages::Shared::SData& message)
		{
			message.from = from;
			message.to = to;
			memcpy(message.pData, pData, len);
		});
}

bool CSteamPlayClient::DestroyPlayer(DPID dpid)
{
	if (dpid == DPID_ALLPLAYERS)
	{
		TPlayers::iterator it = m_players.begin();
		while (it != m_players.end())
		{
			if (it->second.local)
			{
				it = m_players.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
	else
	{
		TPlayers::iterator const it = m_players.find(dpid);
		if (it == m_players.end() || !it->second.local)
		{
			return false;
		}

		m_players.erase(it);
	}

	return TMessageSender::TrySend<Messages::Client::SDestroyPlayer>(
		m_serverConnection,
		k_nSteamNetworkingSend_Reliable,
		[dpid](Messages::Client::SDestroyPlayer& message)
		{
			message.dpid = dpid;
		});
}

HRESULT CSteamPlayClient::ReceiveData(LPDPID pFrom, LPDPID pTo, DWORD flags, LPVOID pData, LPDWORD pSize)
{
	if (!pFrom || !pTo || !pSize)
	{
		return DPERR_INVALIDPARAM;
	}

	TDataMessages::iterator it = m_dataMessages.begin();
	if (!(flags & DPRECEIVE_ALL))
	{
		while (it != m_dataMessages.end())
		{
			bool const fromValid = !(flags & DPRECEIVE_FROMPLAYER) || it->from == *pFrom;
			bool const toValid = !(flags & DPRECEIVE_TOPLAYER) || it->to == *pTo;
			if (fromValid && toValid)
			{
				break;
			}

			++it;
		}
	}

	if (it == m_dataMessages.end())
	{
		return DPERR_NOMESSAGES;
	}

	*pFrom = it->from;
	*pTo = it->to;
	void const* pSourceData;
	size_t      sourceSize;
	if (it->from == DPID_SYSMSG)
	{
		pSourceData = it->pMsg->GetData();
		sourceSize = it->pMsg->GetSize();
	}
	else
	{
		pSourceData = static_cast<Messages::Shared::SData const*>(it->pMsg->GetData())->pData;
		sourceSize = it->pMsg->GetSize() - sizeof(Messages::Shared::SData);
	}

	if (!pData || *pSize < sourceSize)
	{
		return DPERR_BUFFERTOOSMALL;
	}

	memcpy(pData, pSourceData, sourceSize);

	if (!(flags & DPRECEIVE_PEEK))
	{
		m_dataMessages.erase(it);
	}
	return DP_OK;
}

void CSteamPlayClient::ReceiveNetworkData()
{
	static constexpr size_t s_maxMessages = 64;

	if (!SteamNetworkingSockets() || m_serverConnection == k_HSteamNetConnection_Invalid)
		return;

	SteamNetworkingMessage_t* messages[s_maxMessages];
	int const count = SteamNetworkingSockets()->ReceiveMessagesOnConnection(m_serverConnection, messages, s_maxMessages);

	if (m_state == Disconnected)
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

void CSteamPlayClient::ProcessNetworkingMessage(TSteamMessageUniquePtr pSteamMessage)
{
	assert(pSteamMessage != nullptr);
	assert(pSteamMessage->GetData() != nullptr);
	assert(pSteamMessage->GetSize() > 0);

	if (pSteamMessage->GetSize() < sizeof(SMessage))
	{
		Log::WarnClient("Server message was too short.");
		return;
	}

	size_t receiveSize;
	void(CSteamPlayClient::*pReceive)(TSteamMessageUniquePtr);

	SMessage const& message = *static_cast<SMessage*>(pSteamMessage->m_pData);
	switch (message.GetId())
	{
	case Messages::Server::SInfo::ID:
		pReceive = &CSteamPlayClient::OnReceiveInfo;
		receiveSize = sizeof(Messages::Server::SInfo);
		break;
	case Messages::Server::SAuthPassed::ID:
		pReceive = &CSteamPlayClient::OnReceiveAuthPassed;
		receiveSize = sizeof(Messages::Server::SAuthPassed);
		break;
	case Messages::Server::SCreatePlayerResponse::ID:
		pReceive = &CSteamPlayClient::OnReceiveCreatePlayerResponse;
		receiveSize = sizeof(Messages::Server::SCreatePlayerResponse);
		break;
	case Messages::Server::SPlayerCreated::ID:
		pReceive = &CSteamPlayClient::OnReceivePlayerCreated;
		receiveSize = sizeof(Messages::Server::SPlayerCreated);
		break;
	case Messages::Server::SPlayerDestroyed::ID:
		pReceive = &CSteamPlayClient::OnReceivePlayerDestroyed;
		receiveSize = sizeof(Messages::Server::SPlayerDestroyed);
		break;
	case Messages::Shared::SData::ID:
		pReceive = &CSteamPlayClient::OnReceiveData;
		receiveSize = sizeof(Messages::Shared::SData);
		break;
	default:
		Log::WarnClient("Server sent unregistered message %u.", message.GetId());
		return;
	}

	if (pSteamMessage->GetSize() < receiveSize)
	{
		Log::WarnClient("Server message %u was too short. Got %u but expected at least %u.", message.GetId(), pSteamMessage->GetSize(), receiveSize);
		return;
	}

	(this->*pReceive)(std::move(pSteamMessage));
}

void CSteamPlayClient::OnReceiveInfo(TSteamMessageUniquePtr pSteamMessage)
{
	assert(pSteamMessage != nullptr);
	Messages::Server::SInfo const& message = *static_cast<Messages::Server::SInfo*>(pSteamMessage->m_pData);

	if (!message.auth && !message.password)
	{
		SteamNetConnectionInfo_t info;
		SteamNetworkingSockets()->GetConnectionInfo(m_serverConnection, &info);
		SteamUser()->AdvertiseGame(k_steamIDNonSteamGS, info.m_addrRemote.GetIPv4(), info.m_addrRemote.m_port);

		m_state = Connected;

		Log::InfoClient("Connected to server!");
		return;
	}

	//Steamworks_TestSecret();

	Messages::Client::SBeginAuth response;
	if (message.password)
	{
		if (m_password.empty())
		{
			if (!ShowPasswordRequest(m_password.data(), m_password.array_size()))
			{
				Disconnect(EDisconnectReason::ClientDisconnect); // todo? test, DPERR_INVALIDPASSWORD
				return;
			}
		}

		m_password.copyTo(response.szPassword);
	}

	if (message.auth)
	{
		m_authTicket = SteamUser()->GetAuthSessionTicket(response.pToken, sizeof(response.pToken), &response.tokenLen);
		if (response.tokenLen < 1)
		{
			Log::WarnClient("Got invalid auth session ticket!");
		}
	}

	TMessageSender::Send(response, m_serverConnection, k_nSteamNetworkingSend_Reliable);

	m_state = PendingAuth;
	Log::InfoClient("Pending Auth with server!");
}

void CSteamPlayClient::OnReceiveAuthPassed(TSteamMessageUniquePtr)
{
	m_state = Connected;
	Log::InfoClient("Passed Auth with server!");
}

void CSteamPlayClient::OnReceiveCreatePlayerResponse(TSteamMessageUniquePtr pSteamMessage)
{
	assert(pSteamMessage != nullptr);
	Messages::Server::SCreatePlayerResponse& message = *static_cast<Messages::Server::SCreatePlayerResponse*>(pSteamMessage->m_pData);

	if (message.dpid != DPID_UNKNOWN)
	{
		if (m_players.contains(message.dpid))
		{
			Log::InfoClient("Player with id %u already exists.", message.dpid);
		}

		SPlayerData& playerData = m_players[message.dpid] = SPlayerData{ message.szShortName, message.szLongName , true };
		Log::DebugClient("Created local player '%s' '%s'", playerData.shortName.data(), playerData.longName.data());
	}
	else
	{
		Log::InfoClient("Server failed to create player.");
	}

	if (m_createPlayerCallback)
	{
		TCreatePlayerCallback callback = nullptr;
		std::swap(callback, m_createPlayerCallback);
		callback(message.dpid);
	}
}

void CSteamPlayClient::OnReceivePlayerCreated(TSteamMessageUniquePtr pSteamMessage)
{
	assert(pSteamMessage != nullptr);
	Messages::Server::SPlayerCreated& message = *static_cast<Messages::Server::SPlayerCreated*>(pSteamMessage->m_pData);

	if (message.dpid == DPID_UNKNOWN)
	{
		return;
	}

	SPlayerData& playerData = m_players[message.dpid] = SPlayerData{ message.szShortName, message.szLongName , false };

	size_t const shortNameSize = playerData.shortName.size() + 1;
	size_t const longNameSize  = playerData.longName.size() + 1;
	size_t const totalSize = sizeof(DPMSG_CREATEPLAYERORGROUP) + sizeof(wchar_t) * shortNameSize + sizeof(wchar_t) * longNameSize;
	TSteamMessageSharedPtr sysMsg(SteamNetworkingUtils()->AllocateMessage(totalSize), &ReleaseSteamMessage);

	DPMSG_CREATEPLAYERORGROUP* pDPMessage = static_cast<DPMSG_CREATEPLAYERORGROUP*>(sysMsg->m_pData);
	pDPMessage->dwType           = DPSYS_CREATEPLAYERORGROUP;
	pDPMessage->dwPlayerType     = DPPLAYERTYPE_PLAYER;
	pDPMessage->dpId             = message.dpid;
	pDPMessage->dwCurrentPlayers = m_players.size();
	pDPMessage->lpData           = nullptr;
	pDPMessage->dwDataSize       = 0;

	wchar_t* szShortName = reinterpret_cast<wchar_t*>(pDPMessage + 1);
	wchar_t* szLongName = szShortName + shortNameSize;
	mbstowcs(szShortName, playerData.shortName, shortNameSize);
	mbstowcs(szLongName, playerData.longName, longNameSize);
	pDPMessage->dpnName = ConstructDPName(szShortName, szLongName);

	pDPMessage->dpIdParent = 0;
	pDPMessage->dwFlags    = 0;

	for (TPlayer const& player : m_players)
	{
		if (player.second.local)
		{
			m_dataMessages.emplace_back(DPID_SYSMSG, player.first, sysMsg);
		}
	}

	Log::DebugClient("Created remote player %u '%s' '%s'", message.dpid, playerData.shortName.data(), playerData.longName.data());
}

void CSteamPlayClient::OnReceivePlayerDestroyed(TSteamMessageUniquePtr pSteamMessage)
{
	assert(pSteamMessage != nullptr);
	Messages::Server::SPlayerDestroyed const& message = *static_cast<Messages::Server::SPlayerDestroyed*>(pSteamMessage->m_pData);

	TPlayers::iterator const playerIt = m_players.find(message.dpid);
	if (playerIt == m_players.end())
	{
		return;
	}

	SPlayerData const& playerData = playerIt->second;

	size_t const shortNameSize = playerData.shortName.size() + 1;
	size_t const longNameSize = playerData.longName.size() + 1;
	size_t const totalSize = sizeof(DPMSG_DESTROYPLAYERORGROUP) + sizeof(wchar_t) * shortNameSize + sizeof(wchar_t) * longNameSize;
	TSteamMessageSharedPtr sysMsg(SteamNetworkingUtils()->AllocateMessage(totalSize), &ReleaseSteamMessage);

	DPMSG_DESTROYPLAYERORGROUP* pDPMessage = static_cast<DPMSG_DESTROYPLAYERORGROUP*>(sysMsg->m_pData);
	pDPMessage->dwType           = DPSYS_DESTROYPLAYERORGROUP;
	pDPMessage->dwPlayerType     = DPPLAYERTYPE_PLAYER;
	pDPMessage->dpId             = message.dpid;
	pDPMessage->lpLocalData      = nullptr;
	pDPMessage->dwLocalDataSize  = 0;
	pDPMessage->lpRemoteData     = nullptr;
	pDPMessage->dwRemoteDataSize = 0;

	wchar_t* szShortName = reinterpret_cast<wchar_t*>(pDPMessage + 1);
	wchar_t* szLongName  = szShortName + shortNameSize;
	mbstowcs(szShortName, playerData.shortName.data(), shortNameSize);
	mbstowcs(szLongName, playerData.longName.data(), longNameSize);
	pDPMessage->dpnName = ConstructDPName(szShortName, szLongName);

	pDPMessage->dpIdParent = 0;
	pDPMessage->dwFlags    = 0;

	m_players.erase(playerIt);
	for (TPlayer const& player : m_players)
	{
		if (player.second.local)
		{
			m_dataMessages.emplace_back(DPID_SYSMSG, player.first, sysMsg);
		}
	}
}

void CSteamPlayClient::OnReceiveData(TSteamMessageUniquePtr pSteamMessage)
{
	assert(pSteamMessage != nullptr);
	Messages::Shared::SData const& message = *static_cast<Messages::Shared::SData*>(pSteamMessage->m_pData);

	if (!IsDPIDValidFrom(message.from))
	{
		Log::WarnClient("Server data message sender is invalid.");
		return;
	}

	if (!IsDPIDValidTo(message.to))
	{
		Log::WarnClient("Server data message recipient is invalid.");
		return;
	}

	if (message.to == DPID_ALLPLAYERS)
	{
		TSteamMessageSharedPtr ptr(pSteamMessage.release(), &ReleaseSteamMessage);
		for (TPlayer const& player : m_players)
		{
			if (player.second.local)
			{
				m_dataMessages.emplace_back(message.from, player.first, ptr);
			}
		}
	}
	else if (TPlayer* recipient = FindPlayer(message.to))
	{
		if (recipient->second.local)
		{
			m_dataMessages.emplace_back(message.from, message.to, TSteamMessageSharedPtr(pSteamMessage.release(), &ReleaseSteamMessage));
		}
	}
	else
	{
		Log::InfoClient("Server data message recipient %u not found!", message.to);
	}
}

static bool IsSteamNetworkingDisconnected(ESteamNetworkingConnectionState state)
{
	return state == k_ESteamNetworkingConnectionState_None
		|| state == k_ESteamNetworkingConnectionState_ClosedByPeer
		|| state == k_ESteamNetworkingConnectionState_ProblemDetectedLocally;
}

void CSteamPlayClient::OnNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pCallback)
{
	SteamNetConnectionInfo_t const& info = pCallback->m_info;
	ESteamNetworkingConnectionState const oldState = pCallback->m_eOldState;
	ESteamNetworkingConnectionState const newState = info.m_eState;

	if (!IsSteamNetworkingDisconnected(oldState) &&
		 IsSteamNetworkingDisconnected(newState))
	{
		Log::DebugClient("SessionLost %i %i", oldState, newState);
		//Disconnect((EDisconnectReason)info.m_eEndReason);

		TSteamMessageSharedPtr sysMsg(SteamNetworkingUtils()->AllocateMessage(sizeof(DPMSG_SESSIONLOST)), &ReleaseSteamMessage);

		static_cast<DPMSG_SESSIONLOST*>(sysMsg->m_pData)->dwType = DPSYS_SESSIONLOST;
		for (TPlayer const& player : m_players)
		{
			if (player.second.local)
			{
				m_dataMessages.emplace_back(DPID_SYSMSG, player.first, sysMsg);
			}
		}

		// The game might not notify the player.
		MessageBoxA(GetMainWindow(), "Session has been closed.", "Steamworks Connection", MB_OK);
	}
}

