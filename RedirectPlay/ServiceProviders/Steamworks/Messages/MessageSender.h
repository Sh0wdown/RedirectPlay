#pragma once

#include "../SteamTypes.h"
#include "Messages.h"
#include "Log.h"

#include "Steam/isteamnetworkingutils.h"
#include "Steam/isteamnetworkingsockets.h"

#include <cassert>

template<ISteamNetworkingSockets*(*pGetSockets)(), Log::ESource logSource>
class CMessageSender
{
public:
	template<typename TMessage, std::enable_if_t<std::is_base_of_v<SMessage, TMessage>, bool> = true>
	static SteamNetworkingMessage_t* Allocate(size_t attachedDataSize = 0)
	{
		SteamNetworkingMessage_t* pSteamMessage = Allocate(sizeof(TMessage) + attachedDataSize);
		if (pSteamMessage)
		{
			*reinterpret_cast<TMessage*>(pSteamMessage->m_pData) = TMessage();
		}
		return pSteamMessage;
	}

	static SteamNetworkingMessage_t* Allocate(size_t totalDataSize)
	{
		ISteamNetworkingUtils* pUtils = SteamNetworkingUtils();
		assert(pUtils != nullptr);

		SteamNetworkingMessage_t* pSteamMessage = pUtils->AllocateMessage(totalDataSize);
		if (pSteamMessage)
		{
			assert(pSteamMessage->GetData());
			assert(pSteamMessage->GetSize() == totalDataSize);
		}
		else
		{
			Log::Write(Log::ELevel::Error, logSource, "Failed to allocate message of size %u.", totalDataSize);
		}
		return pSteamMessage;
	}

	static SteamNetworkingMessage_t* Copy(SteamNetworkingMessage_t const& steamMessage)
	{
		assert(steamMessage.GetData() != nullptr);
		assert(steamMessage.GetSize() > 0);

		SteamNetworkingMessage_t* pCopy = Allocate(steamMessage.GetSize());
		if (pCopy)
		{
			memcpy(pCopy->m_pData, steamMessage.GetData(), steamMessage.GetSize());
		}
		return pCopy;
	}

	template<typename TMessage, std::enable_if_t<std::is_base_of_v<SMessage, TMessage>, bool> = true>
	static bool Send(TMessage const& message, HSteamNetConnection connection, int flags)
	{
		SteamNetworkingMessage_t* pSteamMessage = Allocate(sizeof(TMessage));
		if (!pSteamMessage)
		{
			return false;
		}

		memcpy(pSteamMessage->m_pData, &message, sizeof(TMessage));
		return Send(pSteamMessage, connection, flags);
	}

	template<typename TMessage, std::enable_if_t<std::is_base_of_v<SMessage, TMessage>, bool> = true>
	static bool Send(HSteamNetConnection connection, int flags)
	{
		SteamNetworkingMessage_t* pSteamMessage = Allocate<TMessage>(0);
		return pSteamMessage && Send(pSteamMessage, connection, flags);
	}

	static bool Send(TSteamMessageUniquePtr pSteamMessage, HSteamNetConnection connection, int flags)
	{
		ISteamNetworkingSockets* pSockets = pGetSockets();
		assert(pSockets != nullptr);
		assert(pSteamMessage != nullptr);
		assert(pSteamMessage->GetData() != nullptr);
		assert(pSteamMessage->GetSize() > 0);

		pSteamMessage->m_conn = connection;
		pSteamMessage->m_nFlags = flags;

		int64 messageNumberOrResult;
		SteamNetworkingMessage_t* ptr = pSteamMessage.release();
		pSockets->SendMessages(1, &ptr, &messageNumberOrResult);

		if (messageNumberOrResult < 0)
		{
			EResult  result = static_cast<EResult>(-messageNumberOrResult);
			Log::Write(Log::ELevel::Info, logSource, "Failed to send message to %u with error code %u.", connection, result);
			return false;
		}
		return true;
	}

	template<typename TMessage, typename TWrite, std::enable_if_t<std::is_base_of_v<SMessage, TMessage>, bool> = true>
	static bool TrySend(HSteamNetConnection connection, int flags, TWrite&& write)
	{
		return TrySend<TMessage>(connection, flags, 0, std::forward<TWrite>(write));
	}

	template<typename TMessage, typename TWrite, std::enable_if_t<std::is_base_of_v<SMessage, TMessage>, bool> = true>
	static bool TrySend(HSteamNetConnection connection, int flags, size_t attachedDataSize, TWrite&& write)
	{
		TSteamMessageUniquePtr pSteamMessage = Allocate<TMessage>(attachedDataSize);
		if (!pSteamMessage)
		{
			return false;
		}

		TMessage& message = *static_cast<TMessage*>(pSteamMessage->m_pData);
		write(message);
		return Send(std::move(pSteamMessage), connection, flags);
	}
};
