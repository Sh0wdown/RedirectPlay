#pragma once

#include <utility>

template<typename T>
static void DefaultRelease(T* ptr)
{
	delete ptr;
}

// Unique pointer that stays default constructible with a custom deleter
template<typename T, void(*deleter)(T*) = DefaultRelease<T>>
class CUniquePtr
{
public:
	CUniquePtr(T* ptr = nullptr) noexcept
		: m_ptr(ptr)
	{
	}

	CUniquePtr(CUniquePtr&& other) noexcept
		: m_ptr(std::exchange(other.m_ptr, nullptr))
	{
	}

	CUniquePtr& operator=(T* ptr)
	{
		reset(ptr);
		return *this;
	}

	CUniquePtr& operator=(CUniquePtr&& other) noexcept
	{
		reset(other.release());
		return *this;
	}

	CUniquePtr(CUniquePtr const& other) = delete;
	CUniquePtr& operator=(CUniquePtr const& other) = delete;

	~CUniquePtr()
	{
		if (m_ptr)
		{
			deleter(m_ptr);
		}
	}

	T* get() noexcept { return m_ptr; }
	T const* get() const noexcept { return m_ptr; }

	T* operator->() noexcept { return get(); }
	T const* operator->() const noexcept { return get(); }

	T& operator*() noexcept { return *get(); }
	T const& operator*() const noexcept { return *get(); }

	explicit operator bool() const noexcept
	{
		return m_ptr != nullptr;
	}

	T* release() noexcept
	{
		return std::exchange(m_ptr, nullptr);
	}

	void reset(T* ptr)
	{
		if (T* pOld = std::exchange(m_ptr, ptr))
		{
			deleter(pOld);
		}
	}

	bool operator==(nullptr_t) noexcept { return m_ptr == nullptr; }
	bool operator!=(nullptr_t) noexcept { return m_ptr != nullptr; }

private:
	T* m_ptr;
};

