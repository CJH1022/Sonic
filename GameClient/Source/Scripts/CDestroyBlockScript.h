#pragma once
#include "CScript.h"
class CDestroyBlockScript :
    public CScript
{
private:
    int m_life;

public:
    virtual void Begin() override;
    virtual void Tick() override;

    void SetBlockLife(int _life) { m_life = _life; }
    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
public:
    CLONE(CDestroyBlockScript);
    CDestroyBlockScript();
    virtual ~CDestroyBlockScript();
};
