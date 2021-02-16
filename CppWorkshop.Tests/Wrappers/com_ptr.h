#pragma once

#include <utility>
#include <stdexcept>

// Wrapper around COM managed resources, similar to WRL's ComPtr
template <class IUnknown>
class com_ptr final
{
public:
	com_ptr() : m_pUnknown(nullptr) {}
	com_ptr(IUnknown* pResource) : m_pUnknown(pResource) { }
	com_ptr(com_ptr<IUnknown>&& rhs) noexcept : m_pUnknown(std::exchange(rhs.m_pUnknown, nullptr)) {}

	com_ptr(const com_ptr<IUnknown>& rhs) : m_pUnknown(rhs.m_pUnknown)
	{
		Inc();
	}

	~com_ptr()
	{
		Dec();
	}

	explicit operator bool() const
	{
		return m_pUnknown != nullptr;
	}

	IUnknown* operator-> () const
	{
		return Get();
	}

	void operator= (std::nullptr_t)
	{
		Dec();
		m_pUnknown = nullptr;
	}

	void operator= (const com_ptr<IUnknown>& rhs)
	{
		Dec();
		m_pUnknown = rhs.m_pUnknown;
		Inc();
	}

	void operator= (com_ptr<IUnknown>&& rhs) noexcept
	{
		Dec();
		m_pUnknown = std::exchange(rhs.m_pUnknown, nullptr);
	}

	bool operator== (const com_ptr<IUnknown>& rhs) const
	{
		return m_pUnknown == rhs.m_pUnknown;
	}

	bool operator== (std::nullptr_t) const
	{
		return m_pUnknown == nullptr;
	}

	bool operator!= (const com_ptr<IUnknown>& rhs) const
	{
		return m_pUnknown != rhs.m_pUnknown;
	}

	bool operator!= (std::nullptr_t) const
	{
		return m_pUnknown != nullptr;
	}

	IUnknown* Get() const
	{
		return m_pUnknown;
	}

	// Accessor to raw interface pointer for initialisation
	IUnknown** GetPP()
	{
		if (m_pUnknown != nullptr) throw  std::logic_error("Unsafe access to resource");
		return &m_pUnknown;
	}

private:
	void Inc()
	{
		if (m_pUnknown)
		{
			m_pUnknown->AddRef();
		}
	}

	void Dec()
	{
		if (m_pUnknown)
		{
			m_pUnknown->Release();
		}
		// We don't need to explicitly clear m_pUnknown here as every call to Dec() is followed by either
		// destruction or the setting of m_pUnknown to another value
	}

	IUnknown* m_pUnknown;
};
