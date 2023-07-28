#include "SteamServersRequest.h"
#include "../SteamPlayUtilities.h"

CSteamSessionsRequest::CSteamSessionsRequest(TServerRequestSignature callback, size_t timeout)
	: m_requestingServers(false)
	, m_requestHandle(NULL)
	, m_endTime()
	, m_callback(std::move(callback))
	, m_timeout(std::chrono::duration<size_t>(timeout))
{
}

bool CSteamSessionsRequest::RequestServers()
{
	if (m_requestingServers || !m_callback)
	{
		return false;
	}

	m_requestingServers = true;
	m_endTime = m_timeout > TClock::duration::zero() ? (TClock::now() + m_timeout) : TTimePoint::max();

	//Steamworks_TestSecret();

	MatchMakingKeyValuePair_t filter;
	MatchMakingKeyValuePair_t* filterPtr = &filter;

	char szAppName[64];
	GetProductName(szAppName, sizeof(szAppName));
	strncpy(filter.m_szKey, "gamedir", sizeof(filter.m_szKey));
	strncpy(filter.m_szValue, szAppName, sizeof(filter.m_szValue)); // needs to be SteamGameServer()->SetModDir or SteamGameServer()->SetProduct ???

	m_requestHandle = SteamMatchmakingServers()->RequestInternetServerList(SteamUtils()->GetAppID(), &filterPtr, 1, this);
	while (m_requestingServers && TClock::now() < m_endTime)
	{
		SteamAPI_RunCallbacks();
	}
	StopRequest();
	return true;
}

void CSteamSessionsRequest::StopRequest()
{
	if (m_requestHandle)
	{
		SteamMatchmakingServers()->ReleaseRequest(m_requestHandle);
		m_requestHandle = NULL;
	}
	m_requestingServers = false;
	m_callback = nullptr;
}

void CSteamSessionsRequest::ServerResponded(HServerListRequest hRequest, int iServer)
{
	if (gameserveritem_t const* pServerInfo = SteamMatchmakingServers()->GetServerDetails(hRequest, iServer))
	{
		if (pServerInfo->m_nAppID == SteamUtils()->GetAppID())
		{
			m_callback(*pServerInfo);
		}
	}
}

void CSteamSessionsRequest::ServerFailedToRespond(HServerListRequest hRequest, int iServer)
{
	if (gameserveritem_t const* pServerInfo = SteamMatchmakingServers()->GetServerDetails(hRequest, iServer))
	{
		if (pServerInfo->m_nAppID == SteamUtils()->GetAppID())
		{
			m_callback(*pServerInfo);
		}
	}
}

void CSteamSessionsRequest::RefreshComplete(HServerListRequest, EMatchMakingServerResponse)
{
	m_requestingServers = false;
}