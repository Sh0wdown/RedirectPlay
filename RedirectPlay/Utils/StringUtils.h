#pragma once

#include <cstddef>

template<class T, size_t n>
static constexpr size_t ArrayCount(T(&)[n])
{ 
	return n;
}

