#include "DirectX/dplay.h"

HRESULT WINAPI DirectPlayEnumerate_Implementation(LPDPENUMDPCALLBACK callback, LPVOID arg)
{
	return DirectPlayEnumerate(callback, arg);
}

HRESULT WINAPI DirectPlayEnumerateA(LPDPENUMDPCALLBACKA, LPVOID)
{
	return E_NOTIMPL;
}

HRESULT WINAPI DirectPlayEnumerateW(LPDPENUMDPCALLBACK, LPVOID)
{
	return E_NOTIMPL;
}

HRESULT WINAPI DirectPlayCreate(LPGUID lpGUID, LPDIRECTPLAY* lplpDP, IUnknown* pUnk)
{
	return E_NOTIMPL;
}