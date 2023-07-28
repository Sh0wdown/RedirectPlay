#pragma once

#include "Steam/steamclientpublic.h"

#include <guiddef.h>

GUID     SteamIDToGUID(CSteamID id);
bool     IsGUIDSteamID(GUID const& guid);
CSteamID GUIDToSteamID(GUID const& guid);

void GetProductName(char* szBuffer, size_t bufferSize);

