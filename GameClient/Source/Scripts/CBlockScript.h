#pragma once
#include "CScript.h"
class CBlockScript :
    public CScript
{
private:

public:
    virtual void Begin();
    virtual void Tick() override;

    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);

public:
    CLONE(CBlockScript);
    CBlockScript();
    virtual ~CBlockScript();
};

