#include "LibRelay.h"
#include "Log.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <memory>

std::unique_ptr<SLibRelay> s_pLib = nullptr;

template<typename T>
bool GetProc(HMODULE h, T& proc, LPCSTR name)
{
	proc = reinterpret_cast<T>(GetProcAddress(h, name));
	if (proc == NULL)
	{
		Log::Error("LibRelay: Could not find proc address for '%s' (%i).", name, GetLastError());
	}
	return proc;
}

SLibRelay const* SLibRelay::Load()
{
	if (!s_pLib)
	{
		char systemDirectory[MAX_PATH];
		GetSystemDirectoryA(systemDirectory, MAX_PATH);
		lstrcatA(systemDirectory, "\\dplayx.dll");

		std::unique_ptr<SLibRelay> pLib = std::make_unique<SLibRelay>();
		pLib->Handle = LoadLibraryA(systemDirectory);
		if (pLib->Handle == NULL)
		{
			Log::Error("LibRelay: Could not load module (%i).", GetLastError());
			return nullptr;
		}

		if (!GetProc(pLib->Handle, pLib->gdwDPlaySPRefCount, "gdwDPlaySPRefCount")) return nullptr;
		if (!GetProc(pLib->Handle, pLib->DllGetClassObject,  "DllGetClassObject")) return nullptr;
		if (!GetProc(pLib->Handle, pLib->DirectPlayCreate,   "DirectPlayCreate")) return nullptr;

		s_pLib = std::move(pLib);
	}
	return s_pLib.get();
}

void SLibRelay::Free()
{ 
	if (s_pLib)
	{
		FreeLibrary(s_pLib->Handle);
		s_pLib.reset();
	}
}

SLibRelay const* SLibRelay::Get()
{ 
	return s_pLib.get();
}

