#pragma once
#include "CScript.h"

class CSpringScript :
    public CScript
{
private:
    Vec2 vVelocity;
	SPRING_DIR m_Dir;
public:
    virtual void Begin();
    virtual void Tick() override;

    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);

public:

public:
    CLONE(CSpringScript);
    CSpringScript();
    virtual ~CSpringScript();
};


