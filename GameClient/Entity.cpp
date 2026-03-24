#include "pch.h"
#include "Entity.h"


UINT Entity::g_NextID = 0;


Entity::Entity()
	: m_InstID(g_NextID++)
	, m_RefCount(0)
{
}

// ID 가 겹치지 않게 하기 위해서, 직접 복사생성자 구현
Entity::Entity(const Entity& _Other)
	: m_InstID(g_NextID++)
	, m_Name(_Other.m_Name)
	, m_RefCount(0)
{
}

Entity::~Entity()
{
}
