#pragma once

#include "Steam/steam_gameserver.h"

#include <chrono>
#include <functional>

class CSteamSessionsRequest : public ISteamMatchmakingServerListResponse
{
public:
	using TServerRequestSignature = std::function<void(gameserveritem_t const& info)>;

	CSteamSessionsRequest(TServerRequestSignature callback, size_t timeout = 0);

	bool RequestServers();
	void StopRequest();

protected:
	using TClock = std::chrono::steady_clock;
	using TTimePoint = TClock::time_point;

	// Inherited via ISteamMatchmakingServerListResponse
	virtual void ServerResponded(HServerListRequest hRequest, int iServer) override;
	virtual void ServerFailedToRespond(HServerListRequest hRequest, int iServer) override;
	virtual void RefreshComplete(HServerListRequest hRequest, EMatchMakingServerResponse response) override;

	// Track whether we are in the middle of a refresh or not
	bool                    m_requestingServers;

	// Track what server list request is currently running
	HServerListRequest      m_requestHandle;
	TTimePoint              m_endTime;

	TServerRequestSignature m_callback;
	TClock::duration        m_timeout;
};

