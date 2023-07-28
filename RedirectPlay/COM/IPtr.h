#pragma once

#include <utility>

template<typename TInterface>
class IPtr
{
public:
	IPtr(TInterface* ptr = nullptr) : m_ptr(ptr)
	{
		IncRef();
	}

	~IPtr()
	{
		DecRef();
	}

	IPtr(IPtr const& other) : IPtr(other.m_ptr)
	{
	}
	IPtr(IPtr&& other) : m_ptr(std::exchange(other.m_ptr, nullptr))
	{
	}

	IPtr& operator=(IPtr const& other) { reset(other.m_ptr); return *this; }
	IPtr& operator=(IPtr&& other)
	{		
		std::swap(m_ptr, other.m_ptr);
		if (other.m_ptr)
		{
			other.m_ptr->Release();
			other.m_ptr = nullptr;
		}
	}

	void reset(TInterface* ptr = nullptr)
	{
		if (ptr)
		{
			ptr->AddRef();
		}
		DecRef();
		m_ptr = ptr;
	}

	TInterface* get() const noexcept { return m_ptr; }
	TInterface& operator*() const noexcept { return *m_ptr; }
	TInterface* operator->() const noexcept { return m_ptr; }
	explicit operator bool() const noexcept { return m_ptr != nullptr; }

private:
	void IncRef() { if (m_ptr) m_ptr->AddRef(); }
	void DecRef() { if (m_ptr) m_ptr->Release(); }

private:
	TInterface* m_ptr;
};