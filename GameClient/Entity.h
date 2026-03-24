#pragma once

class Entity
{
private:
	static	UINT g_NextID; 

private:
	const UINT	m_InstID;	// 객체마다 가지는 고유 ID 값
	wstring		m_Name;		// 객체마다 이름을 지정할 수 있다.
	int         m_RefCount; // 참조 카운팅

public:
	void AddRef() 	{ ++m_RefCount; }
	void Release()  {  --m_RefCount;  if (m_RefCount <= 0) delete this; }

public:
	UINT GetID() { return m_InstID; }
	void SetName(const wstring& _Name) { m_Name = _Name; }
	const wstring& GetName() { return m_Name; }

public:
	Entity();
	Entity(const Entity& _Other); // ID 가 겹치지 않게 하기 위해서, 직접 복사생성자 구현
	//Entity(const Entity& _Other) = delete;	
	virtual ~Entity();


	template<typename T>
	friend class Ptr;
};

