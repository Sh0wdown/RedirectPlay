#pragma once

#include "COM/ComObject.h"

#include "DirectX/dplay.h"
#include "Steam/steam_api.h"

#include <memory>

class CSteamPlayClient;
class CSteamPlayServer;
class CSteamLobby;

class CSteamPlayProvider final : public CComObject<IDirectPlay4>
{
public:
	CSteamPlayProvider(void*, DWORD);
	virtual ~CSteamPlayProvider() override;

protected:
	void    Update();
	HRESULT Join(const DPSESSIONDESC2& description);
	HRESULT JoinLobby(CSteamID lobbyID, char const* password);
	HRESULT JoinServer(CSteamID serverID, char const* password);
	HRESULT Create(DPSESSIONDESC2& description);

protected:
	std::unique_ptr<CSteamPlayClient> m_pClient;
	std::unique_ptr<CSteamPlayServer> m_pServer;
	std::unique_ptr<CSteamLobby>      m_pLobby;

public:
	// Inherited via IDirectPlay4
	virtual HRESULT WINAPI InitializeConnection(void* connection, DWORD flags) override;
	virtual HRESULT WINAPI Open(DPSESSIONDESC2* description, DWORD flags) override;
	virtual HRESULT WINAPI CreatePlayer(LPDPID lpidPlayer, LPDPNAME name, HANDLE event, LPVOID data, DWORD size, DWORD flags) override;
	virtual HRESULT WINAPI EnumSessions(DPSESSIONDESC2* description, DWORD timeout, LPDPENUMSESSIONSCALLBACK2 callback, void* context, DWORD flags) override;
	virtual HRESULT WINAPI SendEx(DPID from, DPID to, DWORD flags, LPVOID data, DWORD size, DWORD priority, DWORD timeout, LPVOID context, DWORD_PTR* msgid) override;
	virtual HRESULT WINAPI Receive(LPDPID from, LPDPID to, DWORD flags, LPVOID data, LPDWORD size) override;
	virtual HRESULT WINAPI Send(DPID from, DPID to, DWORD flags, LPVOID data, DWORD size) override;
	virtual HRESULT WINAPI SetSessionDesc(LPDPSESSIONDESC2 description, DWORD flags) override;
	virtual HRESULT WINAPI CancelMessage(DWORD msgid, DWORD flags) override;
	virtual HRESULT WINAPI DestroyPlayer(DPID dpid) override;
	virtual HRESULT WINAPI Close(void) override;

	virtual HRESULT WINAPI AddPlayerToGroup(DPID, DPID) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI CreateGroup(LPDPID, LPDPNAME, LPVOID, DWORD, DWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI DeletePlayerFromGroup(DPID, DPID) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI DestroyGroup(DPID) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI EnumGroupPlayers(DPID, LPGUID, LPDPENUMPLAYERSCALLBACK2, LPVOID, DWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI EnumGroups(LPGUID, LPDPENUMPLAYERSCALLBACK2, LPVOID, DWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI EnumPlayers(LPGUID, LPDPENUMPLAYERSCALLBACK2, LPVOID, DWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI GetCaps(LPDPCAPS, DWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI GetGroupData(DPID, LPVOID, LPDWORD, DWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI GetGroupName(DPID, LPVOID, LPDWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI GetMessageCount(DPID, LPDWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI GetPlayerAddress(DPID, LPVOID, LPDWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI GetPlayerCaps(DPID, LPDPCAPS, DWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI GetPlayerData(DPID, LPVOID, LPDWORD, DWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI GetPlayerName(DPID, LPVOID, LPDWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI GetSessionDesc(LPVOID, LPDWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI Initialize(LPGUID) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI SetGroupData(DPID, LPVOID, DWORD, DWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI SetGroupName(DPID, LPDPNAME, DWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI SetPlayerData(DPID, LPVOID, DWORD, DWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI SetPlayerName(DPID, LPDPNAME, DWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI AddGroupToGroup(DPID, DPID) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI CreateGroupInGroup(DPID, LPDPID, LPDPNAME, LPVOID, DWORD, DWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI DeleteGroupFromGroup(DPID, DPID) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI EnumConnections(LPCGUID, LPDPENUMCONNECTIONSCALLBACK, LPVOID, DWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI EnumGroupsInGroup(DPID, LPGUID, LPDPENUMPLAYERSCALLBACK2, LPVOID, DWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI GetGroupConnectionSettings(DWORD, DPID, LPVOID, LPDWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI SecureOpen(LPCDPSESSIONDESC2, DWORD, LPCDPSECURITYDESC, LPCDPCREDENTIALS) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI SendChatMessage(DPID, DPID, DWORD, LPDPCHAT) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI SetGroupConnectionSettings(DWORD, DPID, LPDPLCONNECTION) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI StartSession(DWORD, DPID) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI GetGroupFlags(DPID, LPDWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI GetGroupParent(DPID, LPDPID) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI GetPlayerAccount(DPID, DWORD, LPVOID, LPDWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI GetPlayerFlags(DPID, LPDWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI GetGroupOwner(DPID, LPDPID) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI SetGroupOwner(DPID, DPID) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI GetMessageQueue(DPID, DPID, DWORD, LPDWORD, LPDWORD) override { return E_NOTIMPL; }
	virtual HRESULT WINAPI CancelPriority(DWORD, DWORD, DWORD) override { return E_NOTIMPL; }
};

