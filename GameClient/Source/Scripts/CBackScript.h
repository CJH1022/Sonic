#pragma once
#include "CScript.h"

class CCollider2D;
class GameObject;

class CBackScript :
    public CScript
{
private:
    Vec2 m_vBackPower;

public:
    virtual void Begin();
    virtual void Tick() override;

    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void ExecuteItemBounce(GameObject* _Target);

public:
    Vec2 GetBackPower() const { return m_vBackPower; }
    void SetBackPower(Vec2 _Power) { m_vBackPower = _Power; }

public:
    CLONE(CBackScript);
    CBackScript();
    virtual ~CBackScript();
};


