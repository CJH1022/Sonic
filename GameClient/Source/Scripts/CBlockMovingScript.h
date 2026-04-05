#pragma once
#include "CScript.h"
#include "CPlayerScript.h"
class CBlockMovingScript :
    public CScript
{
private:
    enum class MOVESTATE
    {
        START,
        BACK,
    };

    MOVESTATE curState;
    Vec3 vstartPos;
    Vec3 vendPos;
    Vec2 vVelocity;
    Vec2 vDir;
    Vec2 curvDir;
    float m_speed;
    float Dist;
    float curDist;

    Vec3 m_vPrevPos;                 // 이전 프레임의 위치
    Ptr<CPlayerScript> m_pOnPlayer; // 발판에 올라탄 플레이어 포인터
    void RegisterScriptParams();
public:
    virtual void Begin();
    virtual void Tick() override;

    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void SetStartPos(Vec3 _StartPos) { vstartPos = _StartPos; }
    void SetEndPos(Vec3 _EndPos) { vendPos = _EndPos; }
    void SetVelocity(Vec2 _Velocity) { vVelocity = _Velocity; }

public:
    CLONE(CBlockMovingScript);
    CBlockMovingScript();
    CBlockMovingScript(const CBlockMovingScript& _Origin);
    virtual ~CBlockMovingScript();
};

