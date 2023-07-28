#pragma once

#include <vector>

struct _GUID;
typedef _GUID GUID;

// Compound Address Structure:
// DPADDRESS DPAID_TotalSize
// var       totalSize
// DPADDRESS DPAID_...
// var       data
// DPADDRESS DPAID_...
// var       data
// ...

namespace CompoundAddress
{

struct SEntry
{
	const GUID& guid;
	size_t      size;
	char const* pData;
};

void GetEntries(void const* pAddress, std::vector<SEntry>& outEntries);
void LogAddress(void const* pAddress);
bool ReadProviderGUID(void const* pAddress, GUID const& providerDPAID, GUID& providerGUID);

};

