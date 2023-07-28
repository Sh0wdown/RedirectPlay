#include "Registration.h"
#include "DirectPlay/Utils.h"

CServiceProviderRegistry::TProviders CServiceProviderRegistry::s_providers = TProviders();

CServiceProviderData::CServiceProviderData(DWORD flags, wchar_t const* szShortName, wchar_t const* szLongName)
	: m_name(ConstructDPName(ToWCharArray(szShortName), ToWCharArray(szLongName)))
	, m_flags(flags)
{
}

CServiceProviderData::~CServiceProviderData()
{
	delete m_name.lpszShortName;
	delete m_name.lpszLongName;
}
