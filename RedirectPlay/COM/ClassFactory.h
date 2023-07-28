#pragma once
#include "ComObject.h"

template<typename TComCopy>
class CClassFactory final : public CComCopy<IClassFactory>
{
	COMCOPY(CClassFactory, IID_IClassFactory)
public:
	virtual HRESULT WINAPI CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject) override
	{
		if (!ppvObject)
		{
			return E_INVALIDARG;
		}

		if (pUnkOuter)
		{
			*ppvObject = NULL;
			return CLASS_E_NOAGGREGATION;
		}

		// improveme: check iids first?

		// create original object
		typename TComCopy::TInterface* pOriginal = nullptr;
		HRESULT result = GetOriginal().CreateInstance(pUnkOuter, TComCopy::GetIID(), (LPVOID*)&pOriginal);
		if (result != S_OK)
		{
			return result;
		}

		if (!pOriginal)
		{
			return E_FAIL;
		}

		TComCopy* pObject = new TComCopy(*pOriginal);
		if (!pObject)
		{
			return E_OUTOFMEMORY;
		}

		result = pObject->QueryInterface(riid, ppvObject);
		if (result != S_OK)
		{
			delete pObject;
		}
		return result;
	}

	virtual HRESULT WINAPI LockServer(BOOL fLock) override
	{
		return E_NOTIMPL;
	}

protected:
	// Inherited via CComCopy
	virtual IUnknown* GetInterfaceFromIID(IID const& iid) override
	{
		RTN_IF_IFACE(iid, IID_IClassFactory, IClassFactory);
		return CComCopy<IClassFactory>::GetInterfaceFromIID(iid);
	}
};

