#include "Utils.h"

#include <random>

DPID GenerateDPID()
{
	static std::random_device dev;
	static std::mt19937 rng(dev());
	static auto generator = std::uniform_int_distribution<std::mt19937::result_type>(DPID_RESERVEDRANGE + 1, DPID_UNKNOWN - 1);
	return generator(rng);
}

char* ToCharArray(char const* ca)
{
	if (ca)
	{
		size_t const size = strlen(ca) + 1;
		char* result = new char[size];
		return strncpy(result, ca, size);
	}
	return nullptr;
}

char* ToCharArray(wchar_t const* wca)
{
	if (wca)
	{
		size_t const size = wcslen(wca) + 1;
		char* result = new char[size];
		wcstombs(result, wca, size);
		return result;
	}
	return nullptr;
}

wchar_t* ToWCharArray(char const* ca)
{
	if (ca)
	{
		size_t const size = strlen(ca) + 1;
		wchar_t* result = new wchar_t[size];
		mbstowcs(result, ca, size);
		return result;
	}
	return nullptr;
}

wchar_t* ToWCharArray(wchar_t const* wca)
{
	if (wca)
	{
		size_t const size = wcslen(wca) + 1;
		wchar_t* result = new wchar_t[size];
		return wcsncpy(result, wca, size);
	}
	return nullptr;
}
