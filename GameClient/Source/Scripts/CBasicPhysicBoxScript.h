#pragma once
#include "CScript.h"
class CBasicPhysicBoxScript :
    public CScript
{

private:
    Vec2 vVelocity;

public:
    virtual void Begin();
    virtual void Tick() override;

public:
    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);

public:
    CLONE(CBasicPhysicBoxScript);
    CBasicPhysicBoxScript();
    virtual ~CBasicPhysicBoxScript();
};

