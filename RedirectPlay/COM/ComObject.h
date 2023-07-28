#pragma once

#include "LibRelay/LibRelay.h"
#include "IPtr.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <memory>

extern DWORD gdwDPlaySPRefCount;

#define RTN_IF_IFACE(arg, iid, iface) if (arg == iid) return static_cast<iface*>(this)

template<typename T>
class CComObject : public T
{
public:
	using TInterface = T;
public:
	CComObject() : m_refCount(0)
	{
		++gdwDPlaySPRefCount;
	}

	CComObject(CComObject const&) = delete;
	CComObject(CComObject&&)      = delete;
	CComObject& operator=(CComObject const&) = delete;
	CComObject& operator=(CComObject&&)      = delete;

	virtual ~CComObject()
	{
		--gdwDPlaySPRefCount;
	}

	virtual HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObject) override
	{
		if (!ppvObject)
		{
			return E_INVALIDARG;
		}

		*ppvObject = GetInterfaceFromIID(riid);
		if (!*ppvObject)
		{
			return E_NOINTERFACE;
		}

		AddRef();
		return NOERROR;
	}

	virtual ULONG WINAPI AddRef() override { return InterlockedIncrement(&m_refCount); }
	virtual ULONG WINAPI Release() override
	{
		size_t const count = InterlockedDecrement(&m_refCount);
		if (!count)
		{
			delete this;
		}
		return count;
	}

protected:
	virtual IUnknown* GetInterfaceFromIID(IID const& iid)
	{
		RTN_IF_IFACE(iid, IID_IUnknown, IUnknown);
		return nullptr;
	}

private:
	volatile ULONG m_refCount;
};

template<typename T>
class CComCopy : public CComObject<T>
{
public:
	CComCopy(T& original) : m_pOriginal(&original)
	{
	}

	T& GetOriginal() const { return *m_pOriginal; }
private:
	IPtr<T> m_pOriginal;
};

#define COMCOPY_NOCTOR(className, iid)\
public:\
static IID const& GetIID() { return iid; }\

#define COMCOPY(className, iid)\
COMCOPY_NOCTOR(className, iid)\
className(TInterface& original) : CComCopy(original) {}\
