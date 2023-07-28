#include "Globals.h"

#include "COM/ClassFactory.h"
#include "DirectPlay/DirectPlay.h"
#include "DirectPlay/DirectPlayLobby.h"
#include "Log.h"
#include "LibRelay/LibRelay.h"

#include "MinHook/include/MinHook.h"

HMODULE g_module = NULL;

// Returns Class Factory for CLSID
STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID* ppv)
{
    if (!ppv)
    {
        return E_INVALIDARG;
    }
    *ppv = NULL;

    if (rclsid != CLSID_DirectPlay && rclsid != CLSID_DirectPlayLobby)
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    IClassFactory* pOriginalFactory = nullptr;
    HRESULT result = SLibRelay::Get()->DllGetClassObject(rclsid, riid, (LPVOID*)&pOriginalFactory);
    if (result != S_OK)
    {
        return result;
    }
    if (!pOriginalFactory)
    {
        return E_FAIL;
    }

    IClassFactory* pFactory = nullptr;
    if (rclsid == CLSID_DirectPlay)
    {
        pFactory = new CClassFactory<CRedirectPlay>(*pOriginalFactory);
    }
    else
    {
        pFactory = new CClassFactory<CDirectPlayLobby>(*pOriginalFactory);
    }

    if (!pFactory)
    {
        return E_OUTOFMEMORY;
    }

    result = pFactory->QueryInterface(riid, ppv);
    if (result != S_OK)
    {
        delete pFactory;
    }
    return result;
}

STDAPI DllCanUnloadNow()
{
    return gdwDPlaySPRefCount == 0UL;
}

HRESULT WINAPI DllRegisterServer()
{
    return E_NOTIMPL;
}

HRESULT WINAPI DllUnregisterServer()
{
    return NOERROR;
}

using CoCreateInstanceSignature = HRESULT(WINAPI*)(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv);

static CoCreateInstanceSignature s_pOriginalCreate;

HRESULT WINAPI CoCreateInstance_Relay(
    _In_ REFCLSID rclsid,
    _In_opt_ LPUNKNOWN pUnkOuter,
    _In_ DWORD dwClsContext,
    _In_ REFIID riid,
    _COM_Outptr_ _At_(*ppv, _Post_readable_size_(_Inexpressible_(varies))) LPVOID  FAR * ppv
)
{
    if (rclsid != CLSID_DirectPlay && rclsid != CLSID_DirectPlayLobby)
    {
        return s_pOriginalCreate(rclsid, pUnkOuter, dwClsContext, riid, ppv);
    }

    if (!ppv)
    {
        return E_INVALIDARG;
    }
    *ppv = NULL;

    if (rclsid == CLSID_DirectPlay)
    {
        IClassFactory* pOriginalFactory = nullptr;
        HRESULT result = SLibRelay::Get()->DllGetClassObject(rclsid, IID_IClassFactory, (LPVOID*)&pOriginalFactory);
        if (result != S_OK)
        {
            Log::Error("GetFactory: %i", result);
            return result;
        }
        if (!pOriginalFactory)
        {
            return E_FAIL;
        }

        CClassFactory<CRedirectPlay> factory(*pOriginalFactory);
        return factory.CreateInstance(pUnkOuter, riid, ppv);
    }
    else
    {
        IClassFactory* pOriginalFactory = nullptr;
        HRESULT result = SLibRelay::Get()->DllGetClassObject(rclsid, IID_IClassFactory, (LPVOID*)&pOriginalFactory);
        if (result != S_OK)
        {
            return result;
        }
        if (!pOriginalFactory)
        {
            return E_FAIL;
        }

        CClassFactory<CDirectPlayLobby> factory(*pOriginalFactory);
        return factory.CreateInstance(pUnkOuter, riid, ppv);
    }
}

bool Init()
{
    if (MH_Initialize() != MH_OK)
    {
        Log::Error("MinHook failed!");
        return false;
    }

    HMODULE hModule = GetModuleHandleA("combase.dll");
    if (hModule == NULL)
    {
        Log::Error("Module failed!");
        return false;
    }

    FARPROC pTarget = GetProcAddress(hModule, "CoCreateInstance");
    if (pTarget == NULL)
    {
        Log::Error("ProcAddr failed!");
        return false;
    }

    LPVOID pOriginal;
    MH_STATUS status = MH_CreateHook(pTarget, &CoCreateInstance_Relay, &pOriginal);
    s_pOriginalCreate = (CoCreateInstanceSignature)pOriginal;

    MH_EnableHook(pTarget);
    return status == MH_OK;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_module = hModule;
        Log::Debug("Attach RedirectPlay");
        if (!SLibRelay::Load())
        {
            return FALSE;
        }
        if (!Init())
        {
            return FALSE;
        }
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        SLibRelay::Free();
        Log::Debug("Detach RedirectPlay");
        break;
    }
    return TRUE;
}

