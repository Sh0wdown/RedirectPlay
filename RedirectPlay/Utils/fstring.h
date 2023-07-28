#pragma once

#include <string.h>
#include <utility>

// A null-terminated fixed size char array. TSize is the _total_ size of the array, including the terminating null-char.
template<typename TChar, size_t TSize>
class fstringbase
{
protected:
	static_assert(TSize > 0);

	static constexpr size_t s_arraySize = TSize;
	static constexpr size_t s_maxSize   = s_arraySize - 1;
	static constexpr size_t s_lastIndex = s_maxSize;

public:
	fstringbase() noexcept
	{
		Init();
		clear();
	}

	fstringbase(TChar const* src)
	{
		Init();
		assign(src);
	}

	fstringbase(TChar const* src, size_t srcSize)
	{
		Init();
		assign(src, srcSize);
	}

	template<size_t srcSize>
	fstringbase(TChar(&src)[srcSize])
	{
		Init();
		assign(src);
	}

	fstringbase(fstringbase const& src)
	{
		Init();
		assign(src);
	}

	template<size_t srcSize>
	fstringbase(fstringbase<TChar, srcSize> const& src)
	{
		Init();
		assign(src);
	}

	fstringbase& operator=(TChar const* src)
	{
		assign(src);
		return *this;
	}

	template<size_t srcSize>
	fstringbase& operator=(TChar(&src)[srcSize])
	{
		assign(src);
		return *this;
	}

	fstringbase& operator=(fstringbase const& src)
	{
		assign(src);
		return *this;
	}

	template<size_t srcSize>
	fstringbase& operator=(fstringbase<TChar, srcSize> const& src)
	{
		assign(src);
		return *this;
	}

	fstringbase(fstringbase&& other) = delete;
	fstringbase& operator=(fstringbase&& other) = delete;

	// src must be null-terminated
	void assign(TChar const* src)                 { copyTerminated(m_data, s_maxSize, src); }
	void assign(TChar const* src, size_t srcSize) { copyUnterminated(m_data, s_maxSize, src, srcSize); }
	void assign(fstringbase const& src)           { assign(src.data(), src.max_size()); }
	template<size_t srcSize>
	void assign(TChar(&src)[srcSize])             { assign(src, srcSize); }
	template<size_t srcSize>
	void assign(fstringbase<TChar, srcSize> const& src)  { assign(src.data(), src.max_size()); }

	void copyTo(TChar* dest, size_t destSize) const
	{
		copyUnterminated(dest, destSize, m_data, s_maxSize);
	}

	template<size_t destSize>
	void copyTo(TChar(&dest)[destSize]) const
	{
		copyTo(dest, destSize);
	}

	void clear() noexcept       { m_data[0] = 0; }
	bool empty() const noexcept { return m_data[0] == 0; }

	// Returns the full array size (including the terminating null-character).
	constexpr size_t array_size() const noexcept { return s_arraySize; }
	// Returns the max size that should be read & written to (excluding the terminating null-character).
	constexpr size_t max_size() const noexcept { return s_maxSize; }
	// Returns the actual string length.
	size_t size() const noexcept { return strnlen(m_data, s_maxSize); }

	// If you modify this make sure the last character stays null-terminated!
	TChar*       data() noexcept           { return m_data; }
	TChar const* data() const noexcept     { return m_data; }

	operator TChar const*() const noexcept { return m_data; }

protected:
	void Init()
	{
		m_data[s_lastIndex] = 0; // should never be edited
	}

	// assumes src is null-terminated
	static void copyTerminated(TChar* dest, size_t destSize, TChar const* src)
	{
		if (!dest || destSize == 0)
		{
			return;
		}

		if (!src)
		{
			dest[0] = 0;
			return;
		}

		copyImpl(dest, src, destSize - 1);
	}

	// src might not be null-terminated
	static void copyUnterminated(TChar* dest, size_t destSize, TChar const* src, size_t srcSize)
	{
		if (!dest || destSize == 0)
		{
			return;
		}

		if (!src || srcSize == 0)
		{
			dest[0] = 0;
			return;
		}

		copyImpl(dest, src, (std::min)(destSize - 1, srcSize));
	}

	static void copyImpl(TChar* dest, TChar const* src, size_t lastIndex)
	{
		const size_t len = strnlen(src, lastIndex);
		memcpy(dest, src, len);
		dest[len] = 0;
	}

	TChar m_data[s_arraySize];
};

template<size_t TSize>
class fstring final : public fstringbase<char, TSize>
{
	using TBase = fstringbase<char, TSize>;
public:
	using TBase::TBase;

	template<typename ...TArgs>
	size_t format(char const* fmt, TArgs... args)
	{
		// snprintf always adds a null-character, so pass the full size
		return snprintf(TBase::m_data, TBase::s_arraySize, fmt, args...);
	}
};

template<size_t TSize>
class fwstring final : public fstringbase<wchar_t, TSize>
{
	using TBase = fstringbase<wchar_t, TSize>;
public:
	using TBase::TBase;

	template<typename ...TArgs>
	size_t format(wchar_t const* fmt, TArgs... args)
	{
		// snprintf always adds a null-character, so pass the full size
		return swprintf(TBase::m_data, TBase::s_arraySize, fmt, args...);
	}
};
