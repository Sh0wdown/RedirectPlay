#pragma once

#include "IRegistration.h"
#include "Utils/GUIDUtils.h"

#include <unordered_map>
#include <memory>

class CServiceProviderRegistry
{
public:
	using TFactoryPtr = std::unique_ptr<IServiceProviderFactory>;
	using TProviders  = std::unordered_map<GUID, TFactoryPtr>;

	static void Register(GUID const& guid, TFactoryPtr pFactory)
	{
		if (pFactory)
		{
			s_providers.emplace(guid, std::move(pFactory));
		}
	}

	static IServiceProviderFactory* Find(GUID const& guid)
	{
		auto it = s_providers.find(guid);
		return it != s_providers.cend() ? it->second.get() : nullptr;
	}

	static TProviders const& GetProviders() { return s_providers; }

private:
	static TProviders s_providers;
};

class CServiceProviderData : public IServiceProviderData
{
public:
	CServiceProviderData(DWORD flags, wchar_t const* szName) : CServiceProviderData(flags, szName, szName) { }
	CServiceProviderData(DWORD flags, wchar_t const* szShortName, wchar_t const* szLongName);

	virtual ~CServiceProviderData() override;

	virtual LPVOID    GetConnection() override           { return nullptr; }
	virtual DWORD     GetConnectionSize() const override { return 0; }
	virtual LPCDPNAME GetName() const override           { return &m_name; }
	virtual DWORD     GetFlags() const override          { return m_flags; }

private:
	DPNAME m_name;
	DWORD  m_flags;
};

template<typename T>
class CServiceProviderFactory : public IServiceProviderFactory
{
public:
	CServiceProviderFactory(DWORD flags, std::wstring name)
		: CServiceProviderFactory(flags, std::move(name), name)
	{
	}

	CServiceProviderFactory(DWORD flags, std::wstring shortName, std::wstring longName)
		: m_flags(flags)
		, m_shortName(std::move(shortName))
		, m_longName(std::move(longName))
	{
	}

	virtual IDirectPlay4* NewServiceProvider(void* pConnection, DWORD flags) override
	{
		return new T(pConnection, flags);
	}

	virtual CServiceProviderData* NewData(GUID const*, DWORD) override
	{
		return new CServiceProviderData(m_flags, m_shortName.c_str(), m_longName.c_str());
	}

	virtual bool IsServiceAddress(char const* address) const override { return false; }

private:
	std::wstring m_shortName;
	std::wstring m_longName;
	DWORD        m_flags;
};

struct CServiceProviderRegistrar
{
	CServiceProviderRegistrar() = default;
	CServiceProviderRegistrar(GUID const& guid, CServiceProviderRegistry::TFactoryPtr pFactory)
	{
		CServiceProviderRegistry::Register(guid, std::move(pFactory));
	}
};

#define REGISTER_SERVICEPROVIDER(providerType, guid, ...) \
static CServiceProviderRegistrar const s_serviceProviderRegistrar_##providerType = CServiceProviderRegistrar(guid, std::make_unique<CServiceProviderFactory<providerType>>(__VA_ARGS__));

#define REGISTER_SERVICEPROVIDER_FACTORY(factoryType, guid, ...) \
static CServiceProviderRegistrar const s_serviceProviderRegistrar_##factoryType  = CServiceProviderRegistrar(guid, std::make_unique<factoryType>(__VA_ARGS__));

#define REGISTER_SERVICEPROVIDER_FACTORY_IFAVAILABLE(factoryType, guid, ...) \
static CServiceProviderRegistrar const s_serviceProviderRegistrar_##factoryType  = factoryType::IsAvailable() ? CServiceProviderRegistrar(guid, std::make_unique<factoryType>(__VA_ARGS__)) : CServiceProviderRegistrar();