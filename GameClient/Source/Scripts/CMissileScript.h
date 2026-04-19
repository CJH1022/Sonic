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
    void SetMoveDir(Vec3 _Dir)
    {
        m_Dir = _Dir;
        if (m_Dir != Vec3(0.f, 0.f, 0.f))
            m_Dir.Normalize();
    }

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

