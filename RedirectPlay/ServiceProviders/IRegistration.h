#pragma once

#include "DirectX/dplay.h"

struct IServiceProviderData
{
	virtual ~IServiceProviderData() = default;

	virtual LPVOID    GetConnection() = 0;
	virtual DWORD     GetConnectionSize() const = 0;
	virtual LPCDPNAME GetName() const = 0;
	virtual DWORD     GetFlags() const = 0;
};

struct IServiceProviderFactory
{
	virtual ~IServiceProviderFactory() = default;

	virtual IDirectPlay4*         NewServiceProvider(void* pConnection, DWORD flags) = 0;
	virtual IServiceProviderData* NewData(GUID const* pApplicationGuid, DWORD flags) = 0;
	virtual bool                  IsServiceAddress(char const* pCompoundAddress) const = 0;
};
