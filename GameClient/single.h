#pragma once


template<typename T>
class singleton
{
private:
	static T*	m_This;

public:
	static T* GetInst()
	{
		if (nullptr == m_This)
		{
			m_This = new T;
		}

		return m_This;
	}

	static void Destroy()
	{
		if (nullptr != m_This)
		{
			delete m_This;
			m_This = nullptr;
		}
	}

	// 생성자는 자식에서만 호출 가능하게
protected:
	singleton()
	{		
		// 프로그램 종료될때, 등록시킨 함수들을 호출시켜주는 기능
		atexit(Destroy);
	}
};

template<typename T>
T* singleton<T>::m_This = nullptr;