#pragma once

#include "DirectX/dplay.h"

struct SLibRelay
{
	// COM
	using DllGetClassObjectSignature = HRESULT(WINAPI*)(REFCLSID rclsid, REFIID riid, LPVOID* ppv);

	// dplay
	using DirectPlayCreateSignature = HRESULT (WINAPI*)(LPGUID lpGUID, LPDIRECTPLAY* lplpDP, IUnknown* pUnk);

	static SLibRelay const* Load();
	static void             Free();
	static SLibRelay const* Get();

	HMODULE                    Handle;

	DllGetClassObjectSignature DllGetClassObject;
	DirectPlayCreateSignature  DirectPlayCreate;
	DWORD*                     gdwDPlaySPRefCount;
};

