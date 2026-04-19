#pragma once
#include "CScript.h"

class CEffectScript :
    public CScript
{
private:
    bool  m_bDestroyAtFinish; // 애니메이션 종료 시 파괴 여부 (기본값 true)
    float m_fGravity = 0.f; // 중력 세기
    Vec2  m_vVelocity = Vec2(0.f, 0.f); // 초기 점프 속도 (위쪽 방향)
    Vec2 m_vAccel = Vec2(0.f, 0.f);
    float m_LifeTime = 0.f;
    float m_ElapsedTime = 0.f;
public:
    virtual void Begin() override;
    virtual void Tick() override;

    void TickMove(Vec3 _Pos);

public:
    // 옵션: 애니메이션이 끝나도 안 지워지게 하고 싶을 때를 위해
    void SetDestroyAtFinish(bool _b) { m_bDestroyAtFinish = _b; }
    GET_SET(float, fGravity);
    GET_SET(Vec2, vVelocity);
    GET_SET(Vec2, vAccel);
    GET_SET(float, LifeTime);

public:
    CLONE(CEffectScript);
    CEffectScript();
    ~CEffectScript();
};
