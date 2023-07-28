#include "DirectPlay.h"
#include "Log.h"
#include "CompoundAddress.h"
#include "Utils.h"
#include "ServiceProviders/Registration.h"

#include "DirectX/dplobby.h"

IUnknown* CRedirectPlay::GetInterfaceFromIID(IID const& iid)
{
	RTN_IF_IFACE(iid, IID_IDirectPlay4, IDirectPlay4);
	RTN_IF_IFACE(iid, IID_IDirectPlay3, IDirectPlay3);
	RTN_IF_IFACE(iid, IID_IDirectPlay2, IDirectPlay2);
	return CComCopy<IDirectPlay4>::GetInterfaceFromIID(iid);
}

HRESULT WINAPI CRedirectPlay::EnumConnections(LPCGUID guidApplication, LPDPENUMCONNECTIONSCALLBACK callback, LPVOID context, DWORD flags)
{
	HRESULT result;
	if (!callback)
	{
		result = DPERR_INVALIDPARAM;
	}
	else
	{
		for (auto&& [guid, pFactory] : CServiceProviderRegistry::GetProviders())
		{
			std::unique_ptr<IServiceProviderData> const pData(pFactory->NewData(guidApplication, flags));
			if (!callback(&guid, pData->GetConnection(), pData->GetConnectionSize(), pData->GetName(), pData->GetFlags(), context))
			{
				return S_OK;
			}
		}

		result = GetOriginal().EnumConnections(guidApplication, callback, context, flags);
	}
	Log::Debug("DirectPlay: EnumConnections(%u)", result);
	return result;
}

HRESULT WINAPI CRedirectPlay::Initialize(LPGUID a)
{
	Log::Debug("DirectPlay: Initialize");
	return E_NOTIMPL;
}

IServiceProviderFactory* FindFactoryFromConnection(void* connection, DWORD flags)
{
	std::vector<CompoundAddress::SEntry> entries;
	CompoundAddress::GetEntries(connection, entries);

	const char* pAddress = nullptr;

	// Check if the user actually selected one of the new providers
	for (CompoundAddress::SEntry const& entry : entries)
	{
		if (entry.guid == DPAID_ServiceProvider)
		{
			if (entry.size != sizeof(GUID))
			{
				continue;
			}

			GUID const providerGUID = *(GUID const*)entry.pData;
			if (IServiceProviderFactory* factory = CServiceProviderRegistry::Find(providerGUID))
			{
				return factory;
			}
		}
		else if (entry.guid == DPAID_INet)
		{
			pAddress = entry.pData;
		}
	}

	if (pAddress)
	{
		// check if we mistakenly got the default service provider (e.g. when a "connect" launch parameter was used).
		for (const auto& pair : CServiceProviderRegistry::GetProviders())
		{
			if (pair.second->IsServiceAddress(pAddress))
			{
				return pair.second.get();
			}
		}
	}

	return nullptr;
}

IDirectPlay4* CRedirectPlay::CreatePlayProvider(void* connection, DWORD flags)
{
	if (IServiceProviderFactory* pFactory = FindFactoryFromConnection(connection, flags))
	{
		Log::Debug("Using custom play provider.");
		return pFactory->NewServiceProvider(connection, flags);
	}
	return &GetOriginal();
}

HRESULT WINAPI CRedirectPlay::InitializeConnection(LPVOID lpConnection, DWORD flags)
{
	Log::Debug("DirectPlay: InitializeConnection %i", flags);

	//CompoundAddress::LogAddress(lpConnection);

	if (m_pProvider)
	{
		Log::Info("Server is already initialized");
		return DPERR_ALREADYINITIALIZED;
	}

	m_pProvider.reset(CreatePlayProvider(lpConnection, flags));
	if (!m_pProvider)
	{
		return DPERR_NOSERVICEPROVIDER;
	}

	HRESULT const result = m_pProvider->InitializeConnection(lpConnection, flags);
	if (result != DP_OK)
	{
		m_pProvider.reset();
	}
	return result;
}

HRESULT WINAPI CRedirectPlay::Open(LPDPSESSIONDESC2 sdesc, DWORD flags)
{
	HRESULT const result = m_pProvider ? m_pProvider->Open(sdesc, flags) : DPERR_UNINITIALIZED;
	Log::Debug("DirectPlay: Open(%u) f %u %s sf %u", result, flags, (flags & DPOPEN_CREATE) ? "Create" : (flags & DPOPEN_JOIN ? "Create" : ""), sdesc->dwFlags);
	return result;
}

HRESULT WINAPI CRedirectPlay::CreatePlayer(LPDPID dpid, LPDPNAME name, HANDLE event, LPVOID data, DWORD size, DWORD flags)
{
	HRESULT const result = m_pProvider ? m_pProvider->CreatePlayer(dpid, name, event, data, size, flags) : DPERR_UNINITIALIZED;
	Log::Debug("DirectPlay: CreatePlayer(%u) id %u f %u", result, *dpid, flags);
	return result;
}

HRESULT WINAPI CRedirectPlay::SendEx(DPID from, DPID to, DWORD flags, LPVOID data, DWORD size, DWORD priority, DWORD timeout, LPVOID context, DWORD_PTR* msgid)
{
	HRESULT const result = m_pProvider ? m_pProvider->SendEx(from, to, flags, data, size, priority, timeout, context, msgid) : DPERR_UNINITIALIZED;
	//Log::Debug("DirectPlay: SendEx(%u) from %i to %i f %i s %i (p%i, mid%i)", result, from, to, flags, size, priority, msgid);
	return result;
}

HRESULT WINAPI CRedirectPlay::Receive(LPDPID from, LPDPID to, DWORD flags, LPVOID data, LPDWORD size)
{
	HRESULT result = m_pProvider ? m_pProvider->Receive(from, to, flags, data, size) : DPERR_UNINITIALIZED;
#ifndef NDEBUG
	if (result == DP_OK)
	{
		//Log::Debug("DirectPlay: Receive(%u) from %i to %i f %i s %i", result, *from, *to, flags, *size);
		if (from && *from == DPID_SYSMSG && data && size && *size >= sizeof(DPMSG_GENERIC))
		{
			DPMSG_GENERIC* genMsg = (DPMSG_GENERIC*)data;
			switch (genMsg->dwType)
			{
			case DPSYS_CREATEPLAYERORGROUP:
			{
				DPMSG_CREATEPLAYERORGROUP* msg = (DPMSG_CREATEPLAYERORGROUP*)data;
				Log::Debug("DPSYS_CREATEPLAYERORGROUP PlayerType: %u ID: %u Players: %u Data: %u Size: %u IdParent: %u Flags: %u",
					msg->dwPlayerType, msg->dpId, msg->dwCurrentPlayers, msg->lpData ? 1 : 0, msg->dwDataSize, msg->dpIdParent, msg->dwFlags);
			}
			break;
			default:
				Log::Debug("DPSYS MESSAGE %u", genMsg->dwType);
				break;
			}
		}
	}
#endif
	return result;
}

#define REDIRECT_CALL(func, ...) { \
	HRESULT const result = m_pProvider ? m_pProvider->func(__VA_ARGS__) : DPERR_UNINITIALIZED; \
	Log::Debug("DirectPlay: "#func"(%u)", result); \
	return result; \
} \

HRESULT WINAPI CRedirectPlay::EnumSessions(LPDPSESSIONDESC2 desc, DWORD timeout, LPDPENUMSESSIONSCALLBACK2 callback, LPVOID context, DWORD flags)
REDIRECT_CALL(EnumSessions, desc, timeout, callback, context, flags)

HRESULT WINAPI CRedirectPlay::SetSessionDesc(LPDPSESSIONDESC2 a, DWORD b)
REDIRECT_CALL(SetSessionDesc, a, b)

HRESULT WINAPI CRedirectPlay::CancelMessage(DWORD msgid, DWORD flags)
REDIRECT_CALL(CancelMessage, msgid, flags)

HRESULT WINAPI CRedirectPlay::DestroyPlayer(DPID dpid)
REDIRECT_CALL(DestroyPlayer, dpid)

HRESULT WINAPI CRedirectPlay::Close(void)
REDIRECT_CALL(Close)

HRESULT WINAPI CRedirectPlay::AddPlayerToGroup(DPID a, DPID b)                               REDIRECT_CALL(AddPlayerToGroup, a, b)
HRESULT WINAPI CRedirectPlay::CreateGroup(LPDPID a, LPDPNAME b, LPVOID c, DWORD d, DWORD e)  REDIRECT_CALL(CreateGroup, a, b, c, d, e)
HRESULT WINAPI CRedirectPlay::DeletePlayerFromGroup(DPID a, DPID b)                          REDIRECT_CALL(DeletePlayerFromGroup, a, b)
HRESULT WINAPI CRedirectPlay::DestroyGroup(DPID dpid)                                        REDIRECT_CALL(DestroyGroup, dpid)
HRESULT WINAPI CRedirectPlay::EnumGroupPlayers(DPID a, LPGUID b, LPDPENUMPLAYERSCALLBACK2 c, LPVOID d, DWORD e) REDIRECT_CALL(EnumGroupPlayers, a, b, c, d, e)
HRESULT WINAPI CRedirectPlay::EnumGroups(LPGUID a, LPDPENUMPLAYERSCALLBACK2 b, LPVOID c, DWORD d)               REDIRECT_CALL(EnumGroups, a, b, c, d)
HRESULT WINAPI CRedirectPlay::EnumPlayers(LPGUID a, LPDPENUMPLAYERSCALLBACK2 b, LPVOID c, DWORD d)              REDIRECT_CALL(EnumPlayers, a, b, c, d)
HRESULT WINAPI CRedirectPlay::GetCaps(LPDPCAPS a, DWORD b)                                   REDIRECT_CALL(GetCaps, a, b)
HRESULT WINAPI CRedirectPlay::GetGroupData(DPID a, LPVOID b, LPDWORD c, DWORD d)             REDIRECT_CALL(GetGroupData, a, b, c, d)
HRESULT WINAPI CRedirectPlay::GetGroupName(DPID a, LPVOID b, LPDWORD c)                      REDIRECT_CALL(GetGroupName, a, b, c)
HRESULT WINAPI CRedirectPlay::GetMessageCount(DPID a, LPDWORD b)                             REDIRECT_CALL(GetMessageCount, a, b)
HRESULT WINAPI CRedirectPlay::GetPlayerAddress(DPID a, LPVOID b, LPDWORD c)                  REDIRECT_CALL(GetPlayerAddress, a, b, c)
HRESULT WINAPI CRedirectPlay::GetPlayerCaps(DPID a, LPDPCAPS b, DWORD c)                     REDIRECT_CALL(GetPlayerCaps, a, b, c)
HRESULT WINAPI CRedirectPlay::GetPlayerData(DPID a, LPVOID b, LPDWORD c, DWORD d)            REDIRECT_CALL(GetPlayerData, a, b, c, d)
HRESULT WINAPI CRedirectPlay::GetPlayerName(DPID a, LPVOID b, LPDWORD c)                     REDIRECT_CALL(GetPlayerName, a, b, c)
HRESULT WINAPI CRedirectPlay::GetSessionDesc(LPVOID a, LPDWORD b)                            REDIRECT_CALL(GetSessionDesc, a, b)
HRESULT WINAPI CRedirectPlay::Send(DPID a, DPID b, DWORD c, LPVOID d, DWORD e)               REDIRECT_CALL(Send, a, b, c, d, e)
HRESULT WINAPI CRedirectPlay::SetGroupData(DPID a, LPVOID b, DWORD c, DWORD d)               REDIRECT_CALL(SetGroupData, a, b, c, d)
HRESULT WINAPI CRedirectPlay::SetGroupName(DPID a, LPDPNAME b, DWORD c)                      REDIRECT_CALL(SetGroupName, a, b, c)
HRESULT WINAPI CRedirectPlay::SetPlayerData(DPID a, LPVOID b, DWORD c, DWORD d)              REDIRECT_CALL(SetPlayerData, a, b, c, d)
HRESULT WINAPI CRedirectPlay::SetPlayerName(DPID a, LPDPNAME b, DWORD c)                     REDIRECT_CALL(SetPlayerName, a, b, c)
HRESULT WINAPI CRedirectPlay::AddGroupToGroup(DPID a, DPID b)                                REDIRECT_CALL(AddGroupToGroup, a, b)
HRESULT WINAPI CRedirectPlay::CreateGroupInGroup(DPID a, LPDPID b, LPDPNAME c, LPVOID d, DWORD e, DWORD f)       REDIRECT_CALL(CreateGroupInGroup, a, b, c, d, e, f)
HRESULT WINAPI CRedirectPlay::DeleteGroupFromGroup(DPID a, DPID b)                                               REDIRECT_CALL(DeleteGroupFromGroup, a, b)
HRESULT WINAPI CRedirectPlay::EnumGroupsInGroup(DPID a, LPGUID b, LPDPENUMPLAYERSCALLBACK2 c, LPVOID d, DWORD e) REDIRECT_CALL(EnumGroupsInGroup, a, b, c, d, e)
HRESULT WINAPI CRedirectPlay::GetGroupConnectionSettings(DWORD a, DPID b, LPVOID c, LPDWORD d)                   REDIRECT_CALL(GetGroupConnectionSettings, a, b, c, d)
HRESULT WINAPI CRedirectPlay::SecureOpen(LPCDPSESSIONDESC2 a, DWORD b, LPCDPSECURITYDESC c, LPCDPCREDENTIALS d)  REDIRECT_CALL(SecureOpen, a, b, c, d)
HRESULT WINAPI CRedirectPlay::SendChatMessage(DPID a, DPID b, DWORD c, LPDPCHAT d)           REDIRECT_CALL(SendChatMessage, a, b, c, d)
HRESULT WINAPI CRedirectPlay::SetGroupConnectionSettings(DWORD a, DPID b, LPDPLCONNECTION c) REDIRECT_CALL(SetGroupConnectionSettings, a, b, c)
HRESULT WINAPI CRedirectPlay::StartSession(DWORD a, DPID b)                                  REDIRECT_CALL(StartSession, a, b)
HRESULT WINAPI CRedirectPlay::GetGroupFlags(DPID a, LPDWORD b)                               REDIRECT_CALL(GetGroupFlags, a, b)
HRESULT WINAPI CRedirectPlay::GetGroupParent(DPID a, LPDPID b)                               REDIRECT_CALL(GetGroupParent, a, b)
HRESULT WINAPI CRedirectPlay::GetPlayerAccount(DPID a, DWORD b, LPVOID c, LPDWORD d)         REDIRECT_CALL(GetPlayerAccount, a, b, c, d)
HRESULT WINAPI CRedirectPlay::GetPlayerFlags(DPID a, LPDWORD b)                              REDIRECT_CALL(GetPlayerFlags, a, b)
HRESULT WINAPI CRedirectPlay::GetGroupOwner(DPID a, LPDPID b)                                REDIRECT_CALL(GetGroupOwner, a, b)
HRESULT WINAPI CRedirectPlay::SetGroupOwner(DPID a, DPID b)                                  REDIRECT_CALL(SetGroupOwner, a, b)
HRESULT WINAPI CRedirectPlay::GetMessageQueue(DPID a, DPID b, DWORD c, LPDWORD d, LPDWORD e) REDIRECT_CALL(GetMessageQueue, a, b, c, d, e)
HRESULT WINAPI CRedirectPlay::CancelPriority(DWORD a, DWORD b, DWORD c)                      REDIRECT_CALL(CancelPriority, a, b, c)


