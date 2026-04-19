#pragma once

#include "CScript.h"

class CCollider2D;
class CPlayerScript;
class GameObject;

class CCylinderScript
    : public CScript
{
private:
    float       fTheta;
    float       m_fRideSpeed;
    bool        bAttachTree;
    bool        m_bPlayerInside;
    bool        m_bBounceStarted;
    bool        m_bFallingInside;
    int         m_SpiralDir;
    Vec2        m_ExitDoorSize;
    Vec2        m_ExitDoorOffset;
    GameObject* m_pRidingPlayer;
    GameObject* m_pIgnoreOverlapPlayer;

public:
    virtual void Begin() override;
    virtual void Tick() override;
    void DrawExitDoorDebug();

    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);

public:
    Vec2 FallingTree();

public:
    virtual void SaveToLevelFile(FILE* _File) override;
    virtual void LoadFromLevelFile(FILE* _File) override;

public:
    CLONE(CCylinderScript);
    CCylinderScript();
    CCylinderScript(const CCylinderScript& _Origin);
    virtual ~CCylinderScript();

private:
    void RegisterScriptParams();
    bool GetCylinderWorldRect(Vec2& _OutCenter, float& _OutHalfWidth, float& _OutHalfHeight);
    bool GetOtherColliderWorldRect(CCollider2D* _OtherCollider, Vec2& _OutCenter,
                                   float& _OutHalfWidth, float& _OutHalfHeight) const;
    bool TryBeginRide(CPlayerScript* _Player, const Vec2& _PlayerCenter,
                      const Vec2& _CylinderCenter, float _HalfWidth, float _HalfHeight);
    void UpdateRide(CPlayerScript* _Player, const Vec2& _CylinderCenter,
                    float _HalfWidth, float _HalfHeight);
    void ReleasePlayer(CPlayerScript* _Player, const Vec2& _Velocity);
    void ResetRideState(bool _RestoreOutsideSprite);
};
