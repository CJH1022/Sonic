#pragma once
#include "CScript.h"

#include "GameObject.h"

class CMissileScript :
    public CScript
{
private:
    Ptr<GameObject> m_Target;
    Vec3            m_Dir;

public:
    void SetTarget(Ptr<GameObject> _Target) { m_Target = _Target; }

public:
    virtual void Begin();
    virtual void Tick();

    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);

    virtual void SaveToLevelFile(FILE* _File) override;
    virtual void LoadFromLevelFile(FILE* _File) override;

    CLONE(CMissileScript);
public:
    CMissileScript();
    virtual ~CMissileScript();
};

