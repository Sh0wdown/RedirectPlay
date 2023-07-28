#pragma once

#include "DirectX/dplay.h"

#include <string>

DPID           GenerateDPID();
constexpr bool IsDPIDValidFrom(DPID from) { return (from > DPID_RESERVEDRANGE && from < DPID_UNKNOWN) || from == DPID_SERVERPLAYER; }
constexpr bool IsDPIDValidTo(DPID to)     { return (to > DPID_RESERVEDRANGE && to < DPID_UNKNOWN) || to <= DPID_SERVERPLAYER; }

[[nodiscard]] char*    ToCharArray(char const* ca);
[[nodiscard]] char*    ToCharArray(wchar_t const* wca);
[[nodiscard]] wchar_t* ToWCharArray(char const* ca);
[[nodiscard]] wchar_t* ToWCharArray(wchar_t const* wca);

inline DPNAME ConstructDPName(char* shortName, char* longName)
{
	DPNAME result;
	result.dwSize = sizeof(DPNAME);
	result.dwFlags = 0;
	result.lpszShortNameA = shortName;
	result.lpszLongNameA = longName;
	return result;
}

inline DPNAME ConstructDPName(wchar_t* shortName, wchar_t* longName)
{
	return DPNAME { sizeof(DPNAME), 0, shortName, longName };
}
