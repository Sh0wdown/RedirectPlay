#pragma once
#include "COM/ComObject.h"

#include "DirectX/dplobby.h"

class CDirectPlayLobby final : public CComCopy<IDirectPlayLobby3>
{
	COMCOPY(CDirectPlayLobby, IID_IDirectPlayLobby3)
public:
	// Inherited via IDirectPlayLobby
	virtual HRESULT WINAPI Connect(DWORD, LPDIRECTPLAY2*, IUnknown*) override;
	virtual HRESULT WINAPI CreateAddress(REFGUID, REFGUID, LPCVOID, DWORD, LPVOID, LPDWORD) override;
	virtual HRESULT WINAPI EnumAddress(LPDPENUMADDRESSCALLBACK, LPCVOID, DWORD, LPVOID) override;
	virtual HRESULT WINAPI EnumAddressTypes(LPDPLENUMADDRESSTYPESCALLBACK, REFGUID, LPVOID, DWORD) override;
	virtual HRESULT WINAPI EnumLocalApplications(LPDPLENUMLOCALAPPLICATIONSCALLBACK, LPVOID, DWORD) override;
	virtual HRESULT WINAPI GetConnectionSettings(DWORD, LPVOID, LPDWORD) override;
	virtual HRESULT WINAPI ReceiveLobbyMessage(DWORD, DWORD, LPDWORD, LPVOID, LPDWORD) override;
	virtual HRESULT WINAPI RunApplication(DWORD, LPDWORD, LPDPLCONNECTION, HANDLE) override;
	virtual HRESULT WINAPI SendLobbyMessage(DWORD, DWORD, LPVOID, DWORD) override;
	virtual HRESULT WINAPI SetConnectionSettings(DWORD, DWORD, LPDPLCONNECTION) override;
	virtual HRESULT WINAPI SetLobbyMessageEvent(DWORD, DWORD, HANDLE) override;

	// Inherited via IDirectPlayLobby2
	virtual HRESULT WINAPI CreateCompoundAddress(LPCDPCOMPOUNDADDRESSELEMENT, DWORD, LPVOID, LPDWORD) override;

	// Inherited via IDirectPlayLobby3
	virtual HRESULT WINAPI ConnectEx(DWORD, REFIID, LPVOID*, IUnknown*) override;
	virtual HRESULT WINAPI RegisterApplication(DWORD, LPVOID) override;
	virtual HRESULT WINAPI UnregisterApplication(DWORD, REFGUID) override;
	virtual HRESULT WINAPI WaitForConnectionSettings(DWORD) override;

protected:
	// Inherited via CComCopy
	virtual IUnknown* GetInterfaceFromIID(IID const& iid) override;
};

