#pragma once
#include "CScript.h"
class CBasicPhysicBox :
    public CScript
{

private:
    Vec2 vVelocity;

public:
    virtual void Begin();
    virtual void Tick() override;

    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);

public:

public:
    CLONE(CBasicPhysicBox);
    CBasicPhysicBox();
    virtual ~CBasicPhysicBox();
};

};
