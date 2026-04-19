#pragma once
#include "CScript.h"
#include "CTransform.h"

class CCamMoveScript :
    public CScript
{
private:
    Ptr<GameObject> m_Target;
    wstring m_TargetName;

    float m_fMinLimitX = -7255.f;
    float m_fMinLimitY = -6000.f;
    float m_fMaxLimitX = 16735.f;
    float m_fMaxLimitY = 6000.f;
    float m_fFollowOffsetY = 260.f;
    bool m_bLockPosition = false;
    Vec3 m_LockedPosition = Vec3(0.f, 0.f, 0.f);
    bool m_bStageUILockActive = false;
    int m_StageUILockState = -1;
    Vec3 m_StageUILockedPosition = Vec3(0.f, 0.f, 0.f);

public:
    virtual void Begin() override;
    virtual void Tick() override;

    void SetTarget(Ptr<GameObject> _Target) { m_Target = _Target; }
    void SetTargetName(const wstring& _Name) { m_TargetName = _Name; }
    float GetMaxLimitX() const { return m_fMaxLimitX; }
    bool IsPositionLocked() const { return m_bLockPosition; }
    void LockPosition(const Vec3& _WorldPos)
    {
        m_bLockPosition = true;
        m_LockedPosition = _WorldPos;
    }
    void LockCurrentPosition()
    {
        if (Transform() != nullptr)
            LockPosition(Transform()->GetRelativePos());
    }
    void UnlockPosition() { m_bLockPosition = false; }

private:
    Ptr<GameObject> FindTarget();
    void MovePerspective();
    void MoveOrthographic();
    void ClampToCameraBounds(Vec3& _InOutCamPos) const;

private:
    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);

    CLONE(CCamMoveScript);
public:
    CCamMoveScript();
    virtual ~CCamMoveScript();
};

