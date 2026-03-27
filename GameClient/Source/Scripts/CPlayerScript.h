#pragma once
#include "CScript.h"
#include "GameObject.h"

class CPlayerScript 
    : public CScript
{
private:
    struct SurfaceContact
    {
        GameObject* Surface = nullptr;
        Vec2 Normal = Vec2(0.f, 1.f);
        float SignedDistance = 0.f;
        float Score = 999999.f;
        bool TransitionSurface = false;
        bool Attachable = false;
        bool WallLike = false;
        bool Circle = false;
        float SeamBlendT = 0.f;
        bool SeamBlendHasValue = false;
        Vec2 SeamStart = Vec2(0.f, 0.f);
        Vec2 SeamEnd = Vec2(0.f, 0.f);
        Vec2 ContactPoint = Vec2(0.f, 0.f);
    };

    Ptr<GameObject> m_Target;

    float m_Limit = 0.f;
    float gravity = -9.8f;

    // 물리 값
    Vec2 vFraction = Vec2(0.f, 0.f);
    Vec2 vAccel = Vec2(500.f, 1000.f);
    Vec2 vVelocity = Vec2(0.f, 0.f);
    Vec3 vDir = Vec3(0.f, 0.f, 0.f);
    Vec2 vNormal = Vec2(0.f, 1.f);
    Vec2 vTangent = Vec2(0.f, 0.f);

    bool IsJump = false;
    bool m_bNeedGravity = false;

    bool bIsBreak = false;

    bool bIsSpringJump = false;
    bool bIsSpringDash = false;

    PoseState   m_Pose = PoseState::None;
    ActionState m_Action = ActionState::None;

    // 타이머들 (기존 m_Curtime 용도 분리)
    float m_IdleTime = 0.f;   // IdleLong 판단
    float m_PoseTime = 0.f;   // LookUp/Down 시작 프레임 연출
    float m_ActionTime = 0.f;   // Attack/Roll 같은 액션 지속시간(필요 시)
    float m_BreakUngroundedTime = 0.f; // 업힐 보정으로 지면이 순간 끊겨도 브레이크 유지

    // 방향(스킬 대시 방향 결정용)
    int m_Facing = 1; // +1: 오른쪽, -1: 왼쪽
    int m_TurnTargetFacing = -1;
   

private:
    // 입력을 한 곳에서만 읽기
    struct PlayerInput
    {
        bool upHeld = false;
        bool downHeld = false;
        bool downReleased = false;

        bool leftHeld = false;
        bool rightHeld = false;
        bool leftTap = false;
        bool rightTap = false;

        bool spacePressed = false; // KEY_TAP
        bool spaceHeld = false; // KEY_PRESSED
        bool spaceReleased = false; // KEY_RELEASED

        bool attackPressed = false; // TODO: 키 지정
        bool rollPressed = false; // TODO: 키 지정
    };

private:
    bool IsGround = false;                 // 최종 확정값
    int m_TileOverlapCount = 0;            // 지면 오브젝트(타일/블록) begin/end 균형으로 지면 이탈 감지
    bool m_bPushContact = false;
    int m_PushContactDir = 0;
    bool m_bPushing = false;
    int m_PushingDir = 0;
    float m_fBreakSpeed = 0.f;
    int m_iBreakDirection = 1;
    bool m_bBreakWallLocked = false;
    float m_LastTickPosX = 0.f;
    float m_LastFrameDeltaX = 0.f;
    GameObject* m_pAttachBlockedSurface = nullptr;
    float m_AttachBlockedTime = 0.f;
    float m_SurfaceGroundHoldTime = 0.f;
    float m_SurfaceResolveFrame = -1.f;
    float m_SurfaceResolveScore = 999999.f;
    GameObject* m_pResolvedSurface = nullptr;

public:
    void SetTarget(Ptr<GameObject> _Target) { m_Target = _Target; }
    void SetFacing(int _Facing) { m_Facing = _Facing; }
    Vec2 GetNormal() { return vNormal; }
    void SetNormal(Vec2 _Normal) { vNormal = _Normal; }

    virtual void Begin();
    virtual void Tick() override;
    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);

public:
    Vec2 GetVelocity() { return vVelocity; }
    void SetVelocity(Vec2 _vVelocity) 
    {
        vVelocity.x = _vVelocity.x; 
        vVelocity.y = _vVelocity.y;
    }
    void SetVelocityY(float _vVelocity)
    {
        vVelocity.y = _vVelocity;
    }
    void StopBlockedAction()
    {
        if (m_Action != ActionState::Break && m_Action != ActionState::Roll && m_Action !=ActionState::SkillDash)
            return;

        m_Action = ActionState::None;
        vVelocity.x = 0.f;
        m_fBreakSpeed = 0.f;
        m_iBreakDirection = 1;
        m_bBreakWallLocked = false;
        m_BreakUngroundedTime = 0.f;
        m_ActionTime = 0.f;
    }

    void RequestBreakWallStop()
    {
        if (m_Action != ActionState::Break)
            return;

        m_bBreakWallLocked = true;
        m_fBreakSpeed = 0.f;
        vVelocity.x = 0.f;
        vVelocity.y = 0.f;
        IsJump = false;
        bIsSpringJump = false;
    }

    int GetFacing() { return m_Facing; }
    float GetBreakSpeed() const { return m_fBreakSpeed; }
    ActionState GetAction() const { return m_Action; }
    bool GetIsGround() const { return IsGround; }
    bool IsBreakOrRollAction() const { return m_Action == ActionState::Break || m_Action == ActionState::Roll || m_Action == ActionState::SkillDash; }
    void SetIsGround(bool _IsGround){ IsGround = _IsGround; }
    void SetIsJump(bool _IsJump) { IsJump = _IsJump; }
    void BlockSurfaceAttach(GameObject* _Surface, float _Time)
    {
        m_pAttachBlockedSurface = _Surface;
        m_AttachBlockedTime = _Time;
    }
    bool IsSurfaceAttachBlocked(GameObject* _Surface) const
    {
        return (m_pAttachBlockedSurface == _Surface && m_AttachBlockedTime > 0.f);
    }
    void RefreshSurfaceGroundHold(float _Time = 0.08f)
    {
        if (_Time > m_SurfaceGroundHoldTime)
            m_SurfaceGroundHoldTime = _Time;
    }
    void SubmitSurfaceContact(GameObject* _Surface, const Vec2& _Normal, float _SignedDistance,
                              bool _TransitionSurface, bool _Attachable, bool _WallLike, bool _Circle, float _Score,
                              float _SeamBlendT = 0.f, bool _SeamBlendHasValue = false,
                              const Vec2& _SeamStart = Vec2(0.f, 0.f), const Vec2& _SeamEnd = Vec2(0.f, 0.f),
                              const Vec2& _ContactPoint = Vec2(0.f, 0.f));

    void SetNeedGravity(bool _Value) { m_bNeedGravity = _Value; }
    bool GetNeedGravity() const { return m_bNeedGravity; }

public:

    void SetSpringJumpState(ActionState _Action, int _Dir)
    {
        IsJump = true;
        m_Action = _Action;
        m_Facing = _Dir;
        bIsSpringJump = true;
        IsGround = false;
        m_ActionTime = 0.f;
    }


    void SetGroundTangent(Vec2 _Tangent) {
        vTangent = _Tangent;
    }

public:
    virtual void SaveToLevelFile(FILE* _File) override;
    virtual void LoadFromLevelFile(FILE* _File) override;

    CLONE(CPlayerScript);
    CPlayerScript();
    virtual ~CPlayerScript();

private:
    PlayerInput ReadInput() const;

    void ResolveTransitions(const PlayerInput& in, float dt);
    void ResolvePose(const PlayerInput& in, float dt);
    void ResolveAction(const PlayerInput& in, float dt);
    void ApplySurfaceContact(const SurfaceContact& _Contact);

    bool CanStartJump(const PlayerInput& in) const;
    void StartJump();

    void StartSkillDash();
    void Rolling();
    bool CanMoveInput() const;

    void Simulate(const PlayerInput& in, float dt);
    void ApplyFacing(const PlayerInput& in, Vec3& vScale);
    void SimulateHorizontal(const PlayerInput& in, float dt, Vec3& vPos);
    void SimulateVertical(float dt, Vec3& vPos);

    void UpdateTimers(float dt);
    void UpdateGroundRotation();
    void UpdateAnimation(float dt);

    float Lerp(float _Start, float _End, float _Ratio)
    {
        return _Start + (_End - _Start) * _Ratio;
    }
};
