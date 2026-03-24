#pragma once
#include "Entity.h"

#define GET_OTHER_COMPONENT(COM_NAME) class C##COM_NAME* COM_NAME();

class GameObject;

class Component :
    public Entity
{
private:
    const COMPONENT_TYPE    m_Type;     // 컴포넌트 타입
    GameObject*             m_Owner;    // 컴포넌트를 소유한 게임오브젝트
    
public:
    COMPONENT_TYPE GetType() { return m_Type; }
    GameObject* GetOwner() { return m_Owner; }
    GET_OTHER_COMPONENT(Light2D);
    GET_OTHER_COMPONENT(TileRender);
    GET_OTHER_COMPONENT(FlipbookRender);
    GET_OTHER_COMPONENT(SpriteRender);

    GET_OTHER_COMPONENT(Transform);
    GET_OTHER_COMPONENT(MeshRender);
    GET_OTHER_COMPONENT(BillboardRender);
    GET_OTHER_COMPONENT(Camera);
    GET_OTHER_COMPONENT(Collider2D);

    virtual void Init() {};
    virtual void Begin() {}
    virtual void FinalTick() = 0;

    virtual void SaveToLevelFile(FILE* _File) {}
    virtual void LoadFromLevelFile(FILE* _File) {}

    virtual Component* Clone() = 0;
public:
    Component(COMPONENT_TYPE _Type);
    Component(const Component& _Origin);
    virtual ~Component();

    friend class GameObject;
};
