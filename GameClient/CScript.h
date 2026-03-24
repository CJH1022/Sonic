#pragma once
#include "Component.h"

#include "Source/ScriptMgr.h"

class CCollider2D;

class CScript :
    public Component
{
private:
    int    m_ScriptType;

public:
    int GetScriptType() { return m_ScriptType; }

public:
    virtual void Tick() = 0;
    virtual void FinalTick() final {};

public:
   //void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider) {}
   //void Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider) {}
   //void EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider) {}

protected:
    void Destroy();

public:
    virtual CScript* Clone() = 0;
public:
    CScript(int _ScriptType);
    virtual ~CScript();
};

#define ADD_DYNAMIC_BEGIN_OVERLAP(MemFunc) Collider2D()->AddDynamicBeginOverlap(this, (COLLISION_EVENT)&MemFunc);
#define ADD_DYNAMIC_OVERLAP(MemFunc) Collider2D()->AddDynamicOverlap(this, (COLLISION_EVENT)&MemFunc);
#define ADD_DYNAMIC_END_OVERLAP(MemFunc) Collider2D()->AddDynamicEndOverlap(this, (COLLISION_EVENT)&MemFunc);
