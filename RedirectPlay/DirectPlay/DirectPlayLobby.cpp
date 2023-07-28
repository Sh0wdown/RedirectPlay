#include "DirectPlayLobby.h"
#include "Log.h"

HRESULT WINAPI CDirectPlayLobby::Connect(DWORD, LPDIRECTPLAY2*, IUnknown*)
{
	Log::Debug("Lobby: Connect");
	return E_NOTIMPL;
}

HRESULT WINAPI CDirectPlayLobby::CreateAddress(REFGUID, REFGUID, LPCVOID, DWORD, LPVOID, LPDWORD)
{
	Log::Debug("Lobby: CreateAddress");
	return E_NOTIMPL;
}

HRESULT WINAPI CDirectPlayLobby::EnumAddress(LPDPENUMADDRESSCALLBACK, LPCVOID, DWORD, LPVOID)
{
	Log::Debug("Lobby: EnumAddress");
	return E_NOTIMPL;
}

HRESULT WINAPI CDirectPlayLobby::EnumAddressTypes(LPDPLENUMADDRESSTYPESCALLBACK, REFGUID, LPVOID, DWORD)
{
	Log::Debug("Lobby: EnumAddressTypes");
	return E_NOTIMPL;
}

HRESULT WINAPI CDirectPlayLobby::EnumLocalApplications(LPDPLENUMLOCALAPPLICATIONSCALLBACK, LPVOID, DWORD)
{
	Log::Debug("Lobby: EnumLocalApplications");
	return E_NOTIMPL;
}

HRESULT WINAPI CDirectPlayLobby::GetConnectionSettings(DWORD, LPVOID, LPDWORD)
{
	Log::Debug("Lobby: GetConnectionSettings");
	return E_NOTIMPL;
}

HRESULT WINAPI CDirectPlayLobby::ReceiveLobbyMessage(DWORD, DWORD, LPDWORD, LPVOID, LPDWORD)
{
	Log::Debug("Lobby: ReceiveLobbyMessage");
	return E_NOTIMPL;
}

HRESULT WINAPI CDirectPlayLobby::RunApplication(DWORD, LPDWORD, LPDPLCONNECTION, HANDLE)
{
	Log::Debug("Lobby: RunApplication");
	return E_NOTIMPL;
}

HRESULT WINAPI CDirectPlayLobby::SendLobbyMessage(DWORD, DWORD, LPVOID, DWORD)
{
	Log::Debug("Lobby: SendLobbyMessage");
	return E_NOTIMPL;
}

HRESULT WINAPI CDirectPlayLobby::SetConnectionSettings(DWORD, DWORD, LPDPLCONNECTION)
{
	Log::Debug("Lobby: SetConnectionSettings");
	return E_NOTIMPL;
}

HRESULT WINAPI CDirectPlayLobby::SetLobbyMessageEvent(DWORD, DWORD, HANDLE)
{
	Log::Debug("Lobby: SetLobbyMessageEvent");
	return E_NOTIMPL;
}

HRESULT WINAPI CDirectPlayLobby::CreateCompoundAddress(LPCDPCOMPOUNDADDRESSELEMENT lpElements, DWORD dwElementCount, LPVOID lpAddress, LPDWORD lpdwAddressSize)
{
	Log::Debug("Lobby: CreateCompoundAddress");
	return GetOriginal().CreateCompoundAddress(lpElements, dwElementCount, lpAddress, lpdwAddressSize);
}

HRESULT WINAPI CDirectPlayLobby::ConnectEx(DWORD, REFIID, LPVOID*, IUnknown*)
{
	Log::Debug("Lobby: ConnectEx");
	return E_NOTIMPL;
}

HRESULT WINAPI CDirectPlayLobby::RegisterApplication(DWORD, LPVOID)
{
	Log::Debug("Lobby: RegisterApplication");
	return E_NOTIMPL;
}

HRESULT WINAPI CDirectPlayLobby::UnregisterApplication(DWORD, REFGUID)
{
	Log::Debug("Lobby: UnregisterApplication");
	return E_NOTIMPL;
}

HRESULT WINAPI CDirectPlayLobby::WaitForConnectionSettings(DWORD)
{
	Log::Debug("Lobby: WaitForConnectionSettings");
	return E_NOTIMPL;
}

IUnknown* CDirectPlayLobby::GetInterfaceFromIID(IID const& iid)
{
	RTN_IF_IFACE(iid, IID_IDirectPlayLobby3, IDirectPlayLobby3);
	RTN_IF_IFACE(iid, IID_IDirectPlayLobby2, IDirectPlayLobby2);
	RTN_IF_IFACE(iid, IID_IDirectPlayLobby,  IDirectPlayLobby);
	return CComCopy<IDirectPlayLobby3>::GetInterfaceFromIID(iid);
}