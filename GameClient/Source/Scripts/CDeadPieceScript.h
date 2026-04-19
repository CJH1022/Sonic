#pragma once
#include "CScript.h"

class GameObject;

struct DeadPieceSpawnDesc
{
    int LayerIdx;
    float HorizontalSpeed;
    float TopUpwardSpeed;
    float BottomUpwardSpeed;
    float Gravity;
    float LifeTime;
    float SpinSpeed;

    DeadPieceSpawnDesc()
        : LayerIdx(-1)
        , HorizontalSpeed(30.f)
        , TopUpwardSpeed(280.f)
        , BottomUpwardSpeed(220.f)
        , Gravity(900.f)
        , LifeTime(0.9f)
        , SpinSpeed(5.f)
    {
    }
};

class CDeadPieceScript
    : public CScript
{
private:
    Vec2 m_Velocity;
    float m_Gravity;
    float m_LifeTime;
    float m_ElapsedTime;
    float m_AngularVelocity;

public:
    virtual void Begin() override;
    virtual void Tick() override;

    void SetVelocity(const Vec2& _Velocity) { m_Velocity = _Velocity; }
    void SetGravity(float _Gravity) { m_Gravity = _Gravity; }
    void SetLifeTime(float _LifeTime) { m_LifeTime = _LifeTime; }
    void SetAngularVelocity(float _AngularVelocity) { m_AngularVelocity = _AngularVelocity; }

    static bool SpawnSplitPieces(GameObject* _Source, const DeadPieceSpawnDesc& _Desc = DeadPieceSpawnDesc());

public:
    CLONE(CDeadPieceScript);
    CDeadPieceScript();
    virtual ~CDeadPieceScript();
};
