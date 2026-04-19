#pragma once
#include "CScript.h"

class CCoinScript :
    public CScript
{
private:
    bool m_bgravity = false;
    int   m_CoinValue = 1;

    Vec2  m_Velocity = Vec2(0.f, 0.f);
    bool  m_IsGround = false;
    bool  m_bCachedLineGround = false;
    int   m_PhysicalGroundOverlapCount = 0;
    int   m_LineGroundProbeFrame = 0;
    float m_Gravity = 900.f;
    float m_LifeTime = 0.f;

private:
    bool CountsAsCoinPhysicalGround(GameObject* _Obj) const;
    bool ResolveLineGround();
    bool CanPickupByPlayer() const;
    void PickupByPlayer();

public:
    GET_SET(bool, bgravity);
    GET_SET(Vec2, Velocity);
    GET_SET(int, CoinValue);
    void SetInitialLineGroundProbeFrame(int _Frame) { m_LineGroundProbeFrame = (_Frame < 0) ? 0 : _Frame; }

public:
    virtual void Begin();
    virtual void Tick() override;

    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);

public:

public:
    CLONE(CCoinScript);
    CCoinScript();
    ~CCoinScript();

};
