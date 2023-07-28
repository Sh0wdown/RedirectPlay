#pragma once

#include "COM/ComObject.h"

#include "DirectX/dplay.h"

#include <memory>

class CRedirectPlay final : public CComCopy<IDirectPlay4>
{
	COMCOPY(CRedirectPlay, IID_IDirectPlay4)
public:
	// Inherited via IDirectPlay2
	virtual HRESULT WINAPI AddPlayerToGroup(DPID player, DPID) override;
	virtual HRESULT WINAPI Close(void) override;
	virtual HRESULT WINAPI CreateGroup(LPDPID, LPDPNAME, LPVOID, DWORD, DWORD) override;
	virtual HRESULT WINAPI CreatePlayer(LPDPID, LPDPNAME, HANDLE, LPVOID, DWORD, DWORD) override;
	virtual HRESULT WINAPI DeletePlayerFromGroup(DPID, DPID) override;
	virtual HRESULT WINAPI DestroyGroup(DPID) override;
	virtual HRESULT WINAPI DestroyPlayer(DPID) override;
	virtual HRESULT WINAPI EnumGroupPlayers(DPID, LPGUID, LPDPENUMPLAYERSCALLBACK2, LPVOID, DWORD) override;
	virtual HRESULT WINAPI EnumGroups(LPGUID, LPDPENUMPLAYERSCALLBACK2, LPVOID, DWORD) override;
	virtual HRESULT WINAPI EnumPlayers(LPGUID, LPDPENUMPLAYERSCALLBACK2, LPVOID, DWORD) override;
	virtual HRESULT WINAPI EnumSessions(LPDPSESSIONDESC2, DWORD, LPDPENUMSESSIONSCALLBACK2, LPVOID, DWORD) override;
	virtual HRESULT WINAPI GetCaps(LPDPCAPS, DWORD) override;
	virtual HRESULT WINAPI GetGroupData(DPID, LPVOID, LPDWORD, DWORD) override;
	virtual HRESULT WINAPI GetGroupName(DPID, LPVOID, LPDWORD) override;
	virtual HRESULT WINAPI GetMessageCount(DPID, LPDWORD) override;
	virtual HRESULT WINAPI GetPlayerAddress(DPID, LPVOID, LPDWORD) override;
	virtual HRESULT WINAPI GetPlayerCaps(DPID, LPDPCAPS, DWORD) override;
	virtual HRESULT WINAPI GetPlayerData(DPID, LPVOID, LPDWORD, DWORD) override;
	virtual HRESULT WINAPI GetPlayerName(DPID, LPVOID, LPDWORD) override;
	virtual HRESULT WINAPI GetSessionDesc(LPVOID, LPDWORD) override;
	virtual HRESULT WINAPI Initialize(LPGUID) override;
	virtual HRESULT WINAPI Open(LPDPSESSIONDESC2, DWORD) override;
	virtual HRESULT WINAPI Receive(LPDPID, LPDPID, DWORD, LPVOID, LPDWORD) override;
	virtual HRESULT WINAPI Send(DPID, DPID, DWORD, LPVOID, DWORD) override;
	virtual HRESULT WINAPI SetGroupData(DPID, LPVOID, DWORD, DWORD) override;
	virtual HRESULT WINAPI SetGroupName(DPID, LPDPNAME, DWORD) override;
	virtual HRESULT WINAPI SetPlayerData(DPID, LPVOID, DWORD, DWORD) override;
	virtual HRESULT WINAPI SetPlayerName(DPID, LPDPNAME, DWORD) override;
	virtual HRESULT WINAPI SetSessionDesc(LPDPSESSIONDESC2, DWORD) override;

	// Inherited via IDirectPlay3
	virtual HRESULT WINAPI AddGroupToGroup(DPID, DPID) override;
	virtual HRESULT WINAPI CreateGroupInGroup(DPID, LPDPID, LPDPNAME, LPVOID, DWORD, DWORD) override;
	virtual HRESULT WINAPI DeleteGroupFromGroup(DPID, DPID) override;

	// \param lpguidApplication	Pointer to an application's globally unique identifier (GUID). Only service providers and lobby providers that this application can use will be returned. If this parameter is set to a NULL pointer, a list of all the connections is enumerated regardless of the application GUID.
	// \param lpEnumCallback		Pointer to a user-supplied EnumConnectionsCallback function that will be called for each available connection.
	// \param lpContext	Pointer to a user-defined context that is passed to the callback function.
	// \param dwFlags	Flags that specify the type of connections to be enumerated. The default (0) will enumerate DirectPlay service providers only.
	virtual HRESULT WINAPI EnumConnections(LPCGUID lpguidApplication, LPDPENUMCONNECTIONSCALLBACK lpEnumCallback, LPVOID lpContext, DWORD dwFlags) override;
	virtual HRESULT WINAPI EnumGroupsInGroup(DPID, LPGUID, LPDPENUMPLAYERSCALLBACK2, LPVOID, DWORD) override;
	virtual HRESULT WINAPI GetGroupConnectionSettings(DWORD, DPID, LPVOID, LPDWORD) override;
	virtual HRESULT WINAPI InitializeConnection(LPVOID, DWORD) override;
	virtual HRESULT WINAPI SecureOpen(LPCDPSESSIONDESC2, DWORD, LPCDPSECURITYDESC, LPCDPCREDENTIALS) override;
	virtual HRESULT WINAPI SendChatMessage(DPID, DPID, DWORD, LPDPCHAT) override;
	virtual HRESULT WINAPI SetGroupConnectionSettings(DWORD, DPID, LPDPLCONNECTION) override;
	virtual HRESULT WINAPI StartSession(DWORD, DPID) override;
	virtual HRESULT WINAPI GetGroupFlags(DPID, LPDWORD) override;
	virtual HRESULT WINAPI GetGroupParent(DPID, LPDPID) override;
	virtual HRESULT WINAPI GetPlayerAccount(DPID, DWORD, LPVOID, LPDWORD) override;
	virtual HRESULT WINAPI GetPlayerFlags(DPID, LPDWORD) override;

	// Inherited via IDirectPlay4
	virtual HRESULT WINAPI GetGroupOwner(DPID, LPDPID) override;
	virtual HRESULT WINAPI SetGroupOwner(DPID, DPID) override;
	virtual HRESULT WINAPI SendEx(DPID, DPID, DWORD, LPVOID, DWORD, DWORD, DWORD, LPVOID, DWORD_PTR*) override;
	virtual HRESULT WINAPI GetMessageQueue(DPID, DPID, DWORD, LPDWORD, LPDWORD) override;
	virtual HRESULT WINAPI CancelMessage(DWORD, DWORD) override;
	virtual HRESULT WINAPI CancelPriority(DWORD, DWORD, DWORD) override;

protected:
	// Inherited via CComCopy
	virtual IUnknown* GetInterfaceFromIID(IID const& iid) override;

	IDirectPlay4* CreatePlayProvider(void* connection, DWORD flags);

	IPtr<IDirectPlay4> m_pProvider;
};


