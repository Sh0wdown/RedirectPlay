#include "SteamPlayUtilities.h"

#include "Steam/isteamutils.h"

#include <stdio.h>

static unsigned char s_steamGuidIdentifier[8] = "STEAMID";

GUID SteamIDToGUID(CSteamID id)
{
	GUID guid;
	guid.Data1 = (unsigned long)(id.ConvertToUint64() >> 32ULL);
	guid.Data2 = (unsigned short)(id.ConvertToUint64() >> 16ULL);
	guid.Data3 = (unsigned short)id.ConvertToUint64();
	for (size_t i = 0; i < 8; ++i)
		guid.Data4[i] = s_steamGuidIdentifier[i];
	return guid;
}

bool IsGUIDSteamID(GUID const& guid)
{
	for (size_t i = 0; i < 8; ++i)
	{
		if (guid.Data4[i] != s_steamGuidIdentifier[i])
		{
			return false;
		}
	}
	return true;
}

CSteamID GUIDToSteamID(GUID const& guid)
{
	uint64 id = 0;
	if (IsGUIDSteamID(guid))
	{
		id = (uint64)guid.Data1 << 32;
		id |= (uint64)guid.Data2 << 16;
		id |= (uint64)guid.Data3;
	}
	return id;
}

void GetProductName(char* szBuffer, size_t bufferSize)
{
	uint32 const appID = SteamUtils()->GetAppID();
	snprintf(szBuffer, bufferSize, "RedirectPlay%u", appID);
}
