#include "SteamPlayProvider.h"
#include "Client/Dialogs.h"
#include "Client/SteamPlayClient.h"
#include "DirectPlay/CompoundAddress.h"
#include "DirectPlay/Utils.h"
#include "Log.h"
#include "Server/SteamLobbyServer.h"
#include "Server/SteamPlayServer.h"
#include "ServiceProviders/Registration.h"
#include "SessionList/SteamLobbiesRequest.h"
#include "SteamPlayUtilities.h"
#include "Utils/StringUtils.h"

#include "DirectX/dplobby.h"
#include "Steam/steam_gameserver.h"
#include "Steam/steamclientpublic.h"

#include <cassert>

TClock::duration s_connectionTimeout = std::chrono::seconds(10);

CSteamID StringToSteamID(char const* szText)
{
	//return CSteamID(str);
	return strtoull(szText, nullptr, 10);
}

CSteamPlayProvider::CSteamPlayProvider(void*, DWORD)
{
}

CSteamPlayProvider::~CSteamPlayProvider()
{
	Close();
}

extern "C" void __cdecl SteamAPIDebugTextHook(int nSeverity, const char* szDebugText)
{
	Log::Debug("(SteamAPIDebug %i) %s", nSeverity, szDebugText);
}

void CSteamPlayProvider::Update()
{
	SteamAPI_RunCallbacks();
	if (m_pClient && !m_pClient->IsDisconnected())
	{
		m_pClient->ReceiveNetworkData();
	}
}

HRESULT CSteamPlayProvider::Join(const DPSESSIONDESC2& description)
{
	CSteamID const lobbyID = GUIDToSteamID(description.guidInstance);

	fstring<DPPASSWORDLEN> password;
	if (description.lpszPassword)
	{
		wcstombs(password.data(), description.lpszPassword, password.max_size());
	}

	return JoinLobby(lobbyID, password);
}

HRESULT CSteamPlayProvider::JoinLobby(CSteamID lobbyID, char const* szPassword)
{
	if (m_pLobby)
	{
		Log::Debug("Last lobby session was not closed properly!");
		m_pLobby.reset();
	}

	m_pLobby = std::make_unique<CSteamLobby>();
	if (!m_pLobby->Join(lobbyID))
	{
		m_pLobby.reset();
		return DPERR_GENERIC;
	}

	TClock::time_point const deadline = TClock::now() + s_connectionTimeout;
	while (m_pLobby->GetState() == CSteamLobby::Joining ||
		  !m_pLobby->GetGameServer().IsValid())
	{
		SteamAPI_RunCallbacks();

		if (TClock::now() > deadline)
		{
			m_pLobby.reset();
			return DPERR_TIMEOUT;
		}
	}

	if (!m_pLobby->IsInLobby() ||
		!m_pLobby->GetGameServer().IsValid())
	{
		m_pLobby.reset();
		return DPERR_GENERIC;
	}

	CSteamID const serverID    = m_pLobby->GetGameServer();
	HRESULT const clientResult = JoinServer(serverID, szPassword);
	if (clientResult != DP_OK)
	{
		m_pLobby.reset();
		return clientResult;
	}

	return DP_OK;
}

HRESULT CSteamPlayProvider::JoinServer(CSteamID serverID, char const* szPassword)
{
	if (m_pClient)
	{
		Log::Debug("Last client session was not closed properly!");
		m_pClient.reset();
	}

	m_pClient = std::make_unique<CSteamPlayClient>();
	if (!m_pClient->Join(serverID, szPassword))
	{
		return DPERR_GENERIC;
	}

	TClock::time_point const deadline = TClock::now() + s_connectionTimeout;
	while (m_pClient->IsConnectingOrPending())
	{
		Update();

		if (TClock::now() > deadline)
		{
			m_pClient.reset();
			return DPERR_TIMEOUT;
		}
	}

	if (!m_pClient->IsConnected())
	{
		m_pClient.reset();
		return DPERR_GENERIC;
	}
	return DP_OK;
}

void DPDescToSettings(SSteamServerSettings& settings, DPSESSIONDESC2 const& description)
{
	wcstombs(settings.name.data(), description.lpszSessionName, settings.name.max_size());
	if (description.dwFlags & DPSESSION_PASSWORDREQUIRED)
	{
		wcstombs(settings.password.data(), description.lpszPassword, settings.password.max_size());
	}
	else
	{
		settings.password.clear();
	}
	settings.lobbyType  = k_ELobbyTypeFriendsOnly;
	settings.maxPlayers = description.dwMaxPlayers;
}

HRESULT CSteamPlayProvider::Create(DPSESSIONDESC2& description)
{
	if (m_pServer || m_pLobby)
	{
		Log::Debug("Last server session was not closed properly!");
		m_pServer.reset();
		m_pLobby.reset();
	}

	SSteamServerSettings settings;
	DPDescToSettings(settings, description);
	if (!ShowServerSettings(settings))
	{
		return DPERR_CANCELLED;
	}

	m_pServer = std::make_unique<CSteamPlayServer>();
	m_pLobby = std::make_unique<CSteamLobby>();

	if (!m_pServer->Start(settings) ||
		!m_pLobby->Create(settings))
	{
		m_pServer.reset();
		m_pLobby.reset();
		return DPERR_GENERIC;
	}	

	TClock::time_point const deadline = TClock::now() + s_connectionTimeout;
	while (m_pServer->GetState() == CSteamPlayServer::Connecting ||
		   m_pLobby->GetState() == CSteamLobby::Creating)
	{
		SteamAPI_RunCallbacks();
		SteamGameServer_RunCallbacks();

		if (TClock::now() > deadline)
		{
			m_pServer.reset();
			m_pLobby.reset();
			return DPERR_TIMEOUT;
		}
	}

	if (!m_pServer->IsConnected() ||
		!m_pLobby->IsInLobby())
	{
		m_pServer.reset();
		m_pLobby.reset();
		return DPERR_GENERIC;
	}

	CSteamID const serverID = m_pServer->GetSteamID();
	CSteamID const lobbyID = m_pLobby->GetSteamID();

	HRESULT const clientResult = JoinServer(serverID, settings.password);
	if (clientResult != DP_OK)
	{
		m_pServer.reset();
		m_pLobby.reset();
		return clientResult;
	}

	m_pLobby->SetGameServer(serverID);
	description.guidInstance = SteamIDToGUID(serverID);//SteamIDToGUID(m_lobby.GetSteamID());
	return DP_OK;
}

HRESULT CSteamPlayProvider::InitializeConnection(void* pConnection, DWORD)
{
	if (!SteamUser()->BLoggedOn())
	{
		return DPERR_GENERIC;
	}

	SteamClient()->SetWarningMessageHook(&SteamAPIDebugTextHook);

	// Initialize the peer to peer connection process
	SteamNetworkingUtils()->InitRelayNetworkAccess();

	// let's see if we already have a connection address
	std::vector<CompoundAddress::SEntry> entries;
	CompoundAddress::GetEntries(pConnection, entries);
	for (CompoundAddress::SEntry const& entry : entries)
	{
		if (entry.guid == DPAID_INet)
		{
			CSteamID const steamID = StringToSteamID(entry.pData);
			if (steamID.IsValid())
			{
				if (steamID.IsLobby())
				{
					return JoinLobby(steamID, nullptr);
				}
				else if (steamID.BGameServerAccount())
				{
					return JoinServer(steamID, nullptr);
				}
			}
			return DPERR_GENERIC;
		}
	}

	return DP_OK;
}

HRESULT CSteamPlayProvider::Open(DPSESSIONDESC2* pDescription, DWORD flags)
{
	DPSESSION_NEWPLAYERSDISABLED;
	DPSESSION_MIGRATEHOST;
	DPSESSION_NOMESSAGEID;
	DPSESSION_JOINDISABLED;
	DPSESSION_KEEPALIVE;
	DPSESSION_NODATAMESSAGES;
	DPSESSION_SECURESERVER;
	DPSESSION_PRIVATE;
	DPSESSION_MULTICASTSERVER; // currently it is always multicast
	DPSESSION_CLIENTSERVER;
	DPSESSION_DIRECTPLAYPROTOCOL;
	DPSESSION_NOPRESERVEORDER;
	DPSESSION_OPTIMIZELATENCY;
	DPSESSION_ALLOWVOICERETRO;
	DPSESSION_NOSESSIONDESCMESSAGES;

	if (!pDescription)
	{
		return DPERR_INVALIDPARAM;
	}

	if (flags & DPOPEN_RETURNSTATUS)
	{
		return DPERR_UNSUPPORTED;
	}

	if (flags & DPOPEN_CREATE)
	{
		return Create(*pDescription);
	}
	else if (flags & DPOPEN_JOIN)
	{
		return Join(*pDescription);
	}

	return DPERR_INVALIDPARAM;
}

HRESULT CSteamPlayProvider::CreatePlayer(LPDPID pPlayerId, LPDPNAME pName, HANDLE event, LPVOID pData, DWORD size, DWORD flags)
{
	if (!pPlayerId)
	{
		return DPERR_INVALIDPARAM;
	}
	*pPlayerId = DPID_UNKNOWN;

	if (!m_pClient || !m_pClient->IsConnected())
	{
		return DPERR_NOCONNECTION;
	}

	fstring<DPSHORTNAMELEN> shortName;
	if (pName->lpszShortName)
	{
		wcstombs(shortName.data(), pName->lpszShortName, shortName.max_size());
	}
	fstring<DPLONGNAMELEN> longName;
	if (pName->lpszLongName)
	{
		wcstombs(longName.data(), pName->lpszLongName, longName.max_size());
	}

	bool gotResponse = false;
	m_pClient->CreatePlayer(
		{
			shortName,
			longName,
			(flags & DPPLAYER_SERVERPLAYER) != 0,
			(flags & DPPLAYER_SPECTATOR) != 0,
			pData,
			size,
		},
		[pPlayerId, &gotResponse](DPID id)	
		{
			*pPlayerId = id;
			gotResponse = true;
		}
	);

	TClock::time_point const deadline = TClock::now() + s_connectionTimeout;
	while (!gotResponse)
	{
		Update();

		if (std::chrono::steady_clock::now() > deadline)
			return DPERR_TIMEOUT;
	}

	return *pPlayerId != DPID_UNKNOWN ? DP_OK : DPERR_GENERIC;
}

void GetSessionName(SSteamLobbiesRequest::SLobby const& lobby, wchar_t* szBuffer, size_t bufferSize)
{
	static std::string const s_friendIdentifier = "[Friend]";

	std::string name = lobby.name.data();
	while (true)
	{
		size_t pos = name.find(s_friendIdentifier);
		if (pos == std::string::npos)
			break;

		name.erase(pos, s_friendIdentifier.length());
	}

	if (lobby.steamFriend)
	{
		name = s_friendIdentifier + " " + name;
	}

	mbstowcs(szBuffer, name.c_str(), bufferSize - 1);
	szBuffer[bufferSize - 1] = '\0';
}

HRESULT CSteamPlayProvider::EnumSessions(DPSESSIONDESC2* enumDesc, DWORD timeout, LPDPENUMSESSIONSCALLBACK2 callback, void* context, DWORD flags)
{
	DPENUMSESSIONS_AVAILABLE;
	DPENUMSESSIONS_ALL;
	DPENUMSESSIONS_ASYNC;
	DPENUMSESSIONS_STOPASYNC;
	DPENUMSESSIONS_PASSWORDREQUIRED;
	DPENUMSESSIONS_RETURNSTATUS;

	if (!enumDesc || !callback)
	{
		return DPERR_INVALIDPARAM;
	}

	//if (description->guidApplication != appGuid) return DPERR_GENERIC;

	SSteamLobbiesRequest lobbies;
	if (!lobbies.Request())
	{
		return DPERR_GENERIC;
	}

	TClock::time_point const deadline = TClock::now() + ((timeout > 0) ? std::chrono::milliseconds(timeout) : s_connectionTimeout);
	while (lobbies.IsRequesting())
	{
		SteamAPI_RunCallbacks();
		if (TClock::now() > deadline)
		{
			lobbies.Cancel();
			return DPERR_TIMEOUT;
		}
	}

	DPSESSIONDESC2 desc = DPSESSIONDESC2();
	desc.dwSize = sizeof(DPSESSIONDESC2);
	desc.guidApplication = enumDesc->guidApplication;
	wchar_t szName[128]{ 0 };
	desc.lpszSessionName = szName;
	
	for (auto const& lobby : lobbies)
	{
		if (lobby.maxPlayers == 0)
		{
			// we have no data, e.g. when the lobby is private
			continue;
		}

		GetSessionName(lobby, szName, ArrayCount(szName));

		desc.dwCurrentPlayers = lobby.currentPlayers;
		desc.dwMaxPlayers = lobby.maxPlayers;
		desc.guidInstance = SteamIDToGUID(lobby.id);
		desc.dwFlags = 0;
		if (lobby.password)
		{
			desc.dwFlags |= DPSESSION_PASSWORDREQUIRED;
		}
		// if (lobby.secure)
		{
			desc.dwFlags |= DPSESSION_SECURESERVER;
		}		

		DPESC_TIMEDOUT;
		if (!callback(&desc, nullptr, 0, context)) // todo, break on true or false?
		{
			break;
		}
	}

	return DP_OK;
}

HRESULT CSteamPlayProvider::SendEx(DPID from, DPID to, DWORD flags, LPVOID data, DWORD size, DWORD priority, DWORD timeout, LPVOID context, DWORD_PTR* msgid)
{
	Update();
	return Send(from, to, flags, data, size);
}

HRESULT CSteamPlayProvider::Receive(LPDPID from, LPDPID to, DWORD flags, LPVOID data, LPDWORD size)
{
	Update();

	if (!m_pClient || !m_pClient->IsConnected())
	{
		return DPERR_NOCONNECTION;
	}

	return m_pClient->ReceiveData(from, to, flags, data, size);
}

HRESULT CSteamPlayProvider::Send(DPID from, DPID to, DWORD flags, LPVOID data, DWORD size)
{
	Update();

	if (!data || size == 0)
	{
		return DPERR_INVALIDPARAM;
	}

	if (!IsDPIDValidFrom(from) || !IsDPIDValidTo(to))
	{
		return DPERR_INVALIDPLAYER;
	}

	if (!m_pClient || !m_pClient->IsConnected())
	{
		return DPERR_NOCONNECTION;
	}

	if (flags & (DPSEND_SIGNED | DPSEND_ENCRYPTED | DPSEND_LOBBYSYSTEMMESSAGE))
	{
		return DPERR_UNSUPPORTED;
	}

	if (!(flags & DPSEND_NOSENDCOMPLETEMSG))
	{
		return DPERR_UNSUPPORTED;
	}

	return m_pClient->SendData(from, to, data, size, flags & DPSEND_GUARANTEED, !(flags & DPSEND_ASYNC)) ? DPERR_PENDING : DPERR_GENERIC;
}

HRESULT CSteamPlayProvider::SetSessionDesc(LPDPSESSIONDESC2 description, DWORD flags)
{
	return DP_OK;
}

HRESULT CSteamPlayProvider::CancelMessage(DWORD msgid, DWORD flags)
{
	return DP_OK;
}

HRESULT CSteamPlayProvider::DestroyPlayer(DPID dpid)
{
	if (m_pClient && m_pClient->IsConnected())
	{
		return m_pClient->DestroyPlayer(dpid) ? DP_OK : DPERR_GENERIC;
	}
	return DPERR_NOCONNECTION;
}

HRESULT CSteamPlayProvider::Close(void)
{
	if (m_pClient)
	{
		m_pClient->Disconnect(EDisconnectReason::ClientDisconnect);
		m_pClient.reset();
	}
	if (m_pServer)
	{
		m_pServer->Close();
		m_pServer.reset();
	}
	if (m_pLobby)
	{
		m_pLobby->Leave();
		m_pLobby.reset();
	}
	return DP_OK;
}



class CSteamPlayProviderFactory final : public CServiceProviderFactory<CSteamPlayProvider>
{
public:
	using CServiceProviderFactory::CServiceProviderFactory;

	static bool IsAvailable()
	{
		if (!SteamAPI_Init())
		{
			return false;
		}
		return true;
	}

	virtual ~CSteamPlayProviderFactory() override
	{
		SteamAPI_Shutdown();
	}

	virtual bool IsServiceAddress(char const* address) const override
	{
		const CSteamID id = StringToSteamID(address);
		return id.IsValid() && (id.IsLobby() || id.BGameServerAccount());
	}
};

// {F7EF59FB-FA02-45CE-BC36-7BF1D0F6BCE5}
DEFINE_GUID(DPSPGUID_STEAM, 0xf7ef59fb, 0xfa02, 0x45ce, 0xbc, 0x36, 0x7b, 0xf1, 0xd0, 0xf6, 0xbc, 0xe5);
REGISTER_SERVICEPROVIDER_FACTORY_IFAVAILABLE(CSteamPlayProviderFactory, DPSPGUID_STEAM, DPCONNECTION_DIRECTPLAY, L"Steamworks Connection");
