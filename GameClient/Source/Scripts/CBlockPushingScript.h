#pragma once
#include "CScript.h"
class CPlayerScript;
class CBlockPushingScript :
    public CScript
{
private:
    Vec2 vNormal;
    bool bIsMovePossible;
    int m_iFlatTileOverlapCount;
    float m_fVerticalVelocity;
    bool m_bBlockGrounded;
    bool m_bBlockGroundFlat;
    int IsPushing;
    CPlayerScript* m_pOnPlayer;

public:
    virtual void Begin();
    virtual void Tick() override;

    int GetIsPushing() { return IsPushing; }
    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);

public:
    CLONE(CBlockPushingScript);
    CBlockPushingScript();
    virtual ~CBlockPushingScript();
};

