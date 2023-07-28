#pragma once

#include "DirectX/dplay.h"
#include "Steam/steamtypes.h"

#include <cstdint>

enum class EMessage : uint8_t
{
	// Shared
	Invalid,
	Data,

	// From client
	ClientBeginAuth,
	ClientCreatePlayer,
	ClientDestroyPlayer,

	// From server
	ServerInfo,
	ServerAuthPassed,
	ServerCreatePlayerResponse, // to the sender
	ServerPlayerCreated,        // to all clients
	ServerPlayerDestroyed,
};

#pragma pack( push, 1 )
#pragma warning( push )
#pragma warning( disable : 4200 )

// maybe an actual byte stream would be nicer than fixed messages?

struct SMessage
{
public:
	constexpr EMessage GetId() const noexcept { return id; }

protected:
	constexpr SMessage(EMessage messageId) noexcept : id(messageId) { }

	EMessage id;
};

template<EMessage messageId>
struct SMessageBase : public SMessage
{
	static constexpr EMessage ID = messageId;

	constexpr SMessageBase() noexcept : SMessage(messageId) {}
};

namespace Messages
{

	namespace Shared
	{

		struct SData : public SMessageBase<EMessage::Data>
		{
			DPID from;
			DPID to;

			char pData[0];
		};

	}

	namespace Client
	{

		struct SBeginAuth : public SMessageBase<EMessage::ClientBeginAuth>
		{
			char   szPassword[DPPASSWORDLEN];
			uint32 tokenLen;
			char   pToken[1024];
		};

		struct SCreatePlayer : public SMessageBase<EMessage::ClientCreatePlayer>
		{
			char szShortName[DPSHORTNAMELEN];
			char szLongName[DPLONGNAMELEN];
			bool serverPlayer;
			bool spectator;

			char pData[0];
		};

		struct SDestroyPlayer : public SMessageBase<EMessage::ClientDestroyPlayer>
		{
			DPID dpid;
		};

	}

	namespace Server
	{

		struct SInfo : public SMessageBase<EMessage::ServerInfo>
		{
			bool auth;
			bool password;
		};

		struct SAuthPassed : public SMessageBase<EMessage::ServerAuthPassed>
		{
		};

		struct SCreatePlayerResponse : public SMessageBase<EMessage::ServerCreatePlayerResponse>
		{
			DPID dpid;

			char szShortName[DPSHORTNAMELEN];
			char szLongName[DPLONGNAMELEN];
		};

		struct SPlayerCreated : public SMessageBase<EMessage::ServerPlayerCreated>
		{
			DPID dpid;

			char szShortName[DPSHORTNAMELEN];
			char szLongName[DPLONGNAMELEN];
		};

		struct SPlayerDestroyed : public SMessageBase<EMessage::ServerPlayerDestroyed>
		{
			DPID dpid;
		};

	}

}

#pragma warning( pop )
#pragma pack( pop )
