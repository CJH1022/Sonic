#pragma once
#include "CScript.h"
class CKnockbackScript :
    public CScript
{
private:
    Vec2 m_vKnockBackPower;
    void TryApplyKnockback(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);

public:
    virtual void Begin();
    virtual void Tick() override;

    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);

public:
    Vec2 GetKnockBackPower() const { return m_vKnockBackPower; }
    void SetKnockBackPower(Vec2 _Power) { m_vKnockBackPower = _Power; }

public:
    CLONE(CKnockbackScript);
    CKnockbackScript();
    virtual ~CKnockbackScript();
};


