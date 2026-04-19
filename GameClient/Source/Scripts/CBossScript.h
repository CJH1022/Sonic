#pragma once
#include "CScript.h"

class CCollider2D;

enum class BOSS_STATE
{
    WAIT_FOR_CAMERA,
    ENTER,      // 등장 연출 
    IDLE,
    ATTACK1,
    ATTACK2,
    ATTACK3,
    INVINCIBLE,
    MOVE,
    DEAD,
};

enum class BOSS_DIR
{
    LEFT,
    RIGHT,
    UP,
    DOWN,
};

class CBossScript :
    public CScript
{
private:

private:
    bool  m_bActivated;      // 보스 활성화 여부
    float m_TargetCameraX = 16900.f;  // 보스전이 시작될 카메라의 X 좌표
    float m_EnterStartOffsetY = 900.f;
    bool        m_bEnterInitialized = false;
    Vec3        m_EnterTargetPos = Vec3(0.f, 0.f, 0.f);
    int         m_Life = 5;

    BOSS_STATE  m_State;
    float       m_MoveFlipFPS;
    float       m_IdleTime;
    float       m_AttackStateTime;
    float       m_AttackBurstTime;
    int         m_AttackBurstCount = 2;
    bool        m_AttackBurstActive;
    float       m_HitFlashTime;
    float       m_BodyBlinkTime;

    float       m_MoveSpeed;
    Vec2        Target_1;
    Vec2        Target_2;
    Vec2        Target_3;
    Vec2        Target_4;
    bool        m_bDeadPiecesSpawned = false;

    int         m_Dir;
    int         m_PatternPhase;
    int         m_MoveStep;
    
public:
    virtual void Begin();
    virtual void Tick() override;

    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);

public:
    int GetLife() { return m_Life; }
    void SetLife(int _Life) { m_Life = _Life; }
    float GetTargetCameraX() const { return m_TargetCameraX; }
    void SetTargetCameraX(float _TargetCameraX) { m_TargetCameraX = _TargetCameraX; }
    void SetEnterStartOffsetY(float _OffsetY) { m_EnterStartOffsetY = _OffsetY; }
    void ApplyDamage(int _Damage, Vec3 _HitWorldPos = Vec3(0.f, 0.f, 0.f));
    void SetState(BOSS_STATE _State);
    BOSS_STATE GetState() const { return m_State; }
    int GetStateIndex() const { return (int)m_State; }
    void SetStateByIndex(int _StateIndex);
    void MoveUp();
    void MoveDown();
    void MoveLeft();
    void MoveRight();
    void TickEnter();
public:
    virtual void SaveToLevelFile(FILE* _File) override;
    virtual void LoadFromLevelFile(FILE* _File) override;

private:
    void ResetAttackSequence();
    void StartAttackBurst();
    void StopAttackBurst();
    void UpdateFacing();
    void TickIdle();
    void TickMove();
    void TickAttack1();
    void TickAttack2();
    void TickBodyBlink();
    void TriggerHitFlash(Vec3 _HitWorldPos);
    void TickHitFlash();
    void DestroyHitFlash();
    bool TryGetMovementBounds(float _Margin,
                              float& _OutLeftTargetX,
                              float& _OutRightTargetX,
                              float& _OutUpTargetY,
                              float& _OutDownTargetY) const;

public:
    CLONE(CBossScript);
    CBossScript();
    virtual ~CBossScript();
};
