#pragma once
#include "Component.h"

#include "CScript.h"

class CCollider2D;
typedef void(CScript::* COLLISION_EVENT)(CCollider2D*, CCollider2D*);

struct COLLISION_DELEGATE
{
    CScript*        Inst;
    COLLISION_EVENT MemFunc;
};

class CCollider2D :
    public Component
{
private:
    Vec2        m_Offset;
    Vec2        m_Scale;

    int         m_OverlapCount;
    Matrix      m_matWorld;
       
    vector<COLLISION_DELEGATE> m_vecBeginDel;
    vector<COLLISION_DELEGATE> m_vecOverDel;
    vector<COLLISION_DELEGATE> m_vecEndDel;

public:
    void AddDynamicBeginOverlap(CScript* _Inst, COLLISION_EVENT _MemFunc);    
    void AddDynamicOverlap(CScript* _Inst, COLLISION_EVENT _MemFunc);
    void AddDynamicEndOverlap(CScript* _Inst, COLLISION_EVENT _MemFunc);        

public:
    GET_SET(Vec2, Offset);
    GET_SET(Vec2, Scale);

    const Matrix& GetWorldMat() { return m_matWorld; }

private:
    void BeginOverlap(Ptr<CCollider2D> _Other);
    void Overlap(Ptr<CCollider2D> _Other);
    void EndOverlap(Ptr<CCollider2D> _Other);


public:
    virtual void FinalTick() override;
    
    CLONE(CCollider2D);
public:
    CCollider2D();
    CCollider2D(const CCollider2D& _Origin);
    virtual ~CCollider2D();
    friend class CollisionMgr;
};

