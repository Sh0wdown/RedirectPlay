#include "CompoundAddress.h"
#include "Log.h"
#include "Utils/GUIDUtils.h"

#include "DirectX/dplobby.h"

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <ostream>

namespace CompoundAddress
{

void GetEntries(void const* pAddress, std::vector<SEntry>& outEntries)
{
	char const* ptr = (char const*)pAddress;
	if (!ptr)
	{
		return;
	}

	DPADDRESS const* pSizeEntry = (DPADDRESS*)ptr;
	if (pSizeEntry->guidDataType != DPAID_TotalSize || pSizeEntry->dwDataSize == 0)
	{
		return;
	}

	uint64_t totalSize = 0;
	memcpy(&totalSize, ptr + sizeof(DPADDRESS), min(pSizeEntry->dwDataSize, sizeof(totalSize)));

	char const* pEnd = ptr + totalSize;
	ptr += sizeof(DPADDRESS) + pSizeEntry->dwDataSize;
	while (ptr < pEnd)
	{
		DPADDRESS const* pEntry = (DPADDRESS*)ptr;
		ptr += sizeof(DPADDRESS);
		if (ptr > pEnd)
		{
			break;
		}

		outEntries.emplace_back(pEntry->guidDataType, pEntry->dwDataSize, ptr);
		ptr += pEntry->dwDataSize;
	}
}

bool ReadProviderGUID(void const* pAddress, GUID const& providerDPAID, GUID& providerGUID)
{
	std::vector<SEntry> entries;
	GetEntries(pAddress, entries);
	for (SEntry const& entry : entries)
	{
		if (entry.guid == providerDPAID)
		{
			if (entry.size == sizeof(GUID))
			{
				providerGUID = *(GUID const*)entry.pData;
				return true;
			}
			else
			{
				Log::Warn("CompoundAddress ServiceProviderGUID is malformed.");
			}
		}
	}
	return false;
}

std::ostream& operator<<(std::ostream& os, const GUID& guid)
{
	os << std::uppercase;
	os.width(8);
	os << std::setfill('0');
	os << std::hex << guid.Data1 << '-';

	os.width(4);
	os << std::hex << guid.Data2 << '-';

	os.width(4);
	os << std::hex << guid.Data3 << '-';

	os.width(2);
	os << std::hex
		<< static_cast<int>(guid.Data4[0])
		<< static_cast<int>(guid.Data4[1])
		<< '-'
		<< static_cast<int>(guid.Data4[2])
		<< static_cast<int>(guid.Data4[3])
		<< static_cast<int>(guid.Data4[4])
		<< static_cast<int>(guid.Data4[5])
		<< static_cast<int>(guid.Data4[6])
		<< static_cast<int>(guid.Data4[7]);
	os << std::nouppercase;
	return os;
}

void LogAddress(void const* pAddress)
{
	std::ofstream f("compoundaddress.log");

	std::vector<SEntry> entries;
	GetEntries(pAddress, entries);
	for (SEntry const& entry : entries)
	{
		f << entry.guid << " (" << entry.size << "): ";
		for (auto ptr = entry.pData, end = entry.pData + entry.size; ptr < end; ++ptr)
		{
			f << std::hex << (byte)*ptr << " ";
		}
		f << std::endl;
	}

	f.close();
}

}
