#pragma once

template<typename T>
class Ptr
{
private:
	T*	m_Ptr;

public:
	T* Get() const { return m_Ptr; }
	T** GetAdressOf() { return &m_Ptr; }

public:
	Ptr& operator =(T* _Ptr)
	{
		if (nullptr != m_Ptr)
			m_Ptr->Release();

		m_Ptr = _Ptr;

		if (nullptr != m_Ptr)
			m_Ptr->AddRef();

		return *this;
	}

	Ptr& operator =(const Ptr<T>& _Ptr)
	{
		if (nullptr != m_Ptr)
			m_Ptr->Release();

		m_Ptr = _Ptr.m_Ptr;

		if (nullptr != m_Ptr)
			m_Ptr->AddRef();

		return *this;
	}

	T* operator->() const { return m_Ptr; }

	bool operator == (T* _Ptr) const { return m_Ptr == _Ptr; }
	bool operator == (const Ptr<T>& _Ptr) const { return m_Ptr == _Ptr.m_Ptr; }
	
	bool operator != (T* _Ptr) const { return m_Ptr != _Ptr; }
	bool operator != (const Ptr<T>& _Ptr) const { return m_Ptr != _Ptr.m_Ptr; }

public:
	Ptr()
		: m_Ptr(nullptr)
	{}

	Ptr(T* _Ptr)
		: m_Ptr(_Ptr)
	{
		if (nullptr != m_Ptr)
			m_Ptr->AddRef();
	}

	Ptr(const Ptr<T>& _Ptr)
		: m_Ptr(_Ptr.m_Ptr)
	{
		if (nullptr != m_Ptr)
			m_Ptr->AddRef();
	}

	~Ptr()
	{
		if (nullptr != m_Ptr)
			m_Ptr->Release();
	}
};

template<typename T>
bool operator == (void* _Pointer, const Ptr<T>& _Ptr)
{
	return _Pointer == _Ptr.Get();
}

template<typename T>
bool operator != (void* _Pointer, const Ptr<T>& _Ptr)
{
	return _Pointer != _Ptr.Get();
}


