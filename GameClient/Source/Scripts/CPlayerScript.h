#pragma once

#include "CScript.h"
#include "GameObject.h"

#include <vector>

class CCollider2D;

class CPlayerScript
    : public CScript
{
private:
    static constexpr float kKnockBackInvincibleDuration = 3.f;

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
        bool InwardCircle = false;
        float SeamBlendT = 0.f;
        bool SeamBlendHasValue = false;
        Vec2 SeamStart = Vec2(0.f, 0.f);
        Vec2 SeamEnd = Vec2(0.f, 0.f);
        Vec2 ContactPoint = Vec2(0.f, 0.f);
    };

    struct PlayerInput
    {
        bool upHeld = false;
        bool downHeld = false;
        bool downReleased = false;

        bool leftHeld = false;
        bool rightHeld = false;
        bool leftTap = false;
        bool rightTap = false;

        bool spacePressed = false;
        bool spaceHeld = false;
        bool spaceReleased = false;

        bool attackPressed = false;
        bool rollPressed = false;
    };

private:
    Ptr<GameObject> m_Target;

    float m_Limit = 0.f;
    float gravity = -9.8f;

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

    PoseState m_Pose = PoseState::None;
    ActionState m_Action = ActionState::None;
    bool m_bBackReaction = false;

    float m_IdleTime = 0.f;
    float m_PoseTime = 0.f;
    float m_ActionTime = 0.f;
    float m_BreakUngroundedTime = 0.f;
    float m_KnockBackInvincibleTime = 0.f;

    int m_Facing = 1;
    int m_TurnTargetFacing = -1;

    bool IsGround = false;
    int m_TileOverlapCount = 0;
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
    bool m_bInwardCircleLoopTracked = false;
    bool m_bInwardCirclePassedLowerHalf = false;
    Vec2 m_InwardCircleLoopCenter = Vec2(0.f, 0.f);
    float m_InwardCircleLoopRadius = 0.f;
    float m_InwardCircleLoopAccumulatedAngle = 0.f;
    float m_InwardCircleLoopLastAngle = 0.f;
    float m_InwardCircleAttachBlockTime = 0.f;
    Vec2 m_InwardCircleAttachBlockCenter = Vec2(0.f, 0.f);
    float m_InwardCircleAttachBlockRadius = 0.f;
    bool m_bInwardCircleHalfCheckerTracked = false;
    Vec2 m_InwardCircleHalfCheckerCenter = Vec2(0.f, 0.f);
    float m_InwardCircleHalfCheckerRadius = 0.f;
    bool m_bInwardCircleLineOwnsTopRight = false;

    bool m_bHasPendingSurfaceContact = false;
    SurfaceContact m_PendingSurfaceContact = {};
    bool m_bHasCurrentSurfaceContact = false;
    SurfaceContact m_CurrentSurfaceContact = {};
    vector<GameObject*> m_vecActiveSurfaceObjects;

    bool m_bAutoplayInitialized = false;
    bool m_bAutoplayFinished = false;
    bool m_bAutoplayWasGround = false;
    float m_AutoplayTime = 0.f;
    float m_AutoplaySampleAccum = 0.f;
    float m_AutoplayAirTime = 0.f;
    float m_AutoplayLongestAirTime = 0.f;
    float m_AutoplayMaxX = 0.f;
    float m_AutoplayMinY = 0.f;
    float m_AutoplayMaxLineContactError = 0.f;
    float m_AutoplayMaxCircleContactError = 0.f;
    float m_AutoplayCircleGroundTime = 0.f;
    int m_AutoplayGroundLossCount = 0;
    int m_AutoplayInwardCircleReleaseCount = 0;
    int m_AutoplayJumpStartCount = 0;
    int m_AutoplayJumpDetachCount = 0;
    bool m_bAutoplayTouchedCircle = false;
    bool m_bAutoplayPendingJumpDetach = false;
    PlayerInput m_AutoplayInput = {};
    FILE* m_pAutoplayLog = nullptr;

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
        if (m_Action != ActionState::Break && m_Action != ActionState::Roll && m_Action != ActionState::SkillDash)
            return;

        if (m_Action == ActionState::Roll && m_bBackReaction)
            return;

        m_Action = ActionState::None;
        m_bBackReaction = false;
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
    bool IsKnockBackInvincible() const { return m_KnockBackInvincibleTime > 0.f; }
    float GetKnockBackInvincibleTime() const { return m_KnockBackInvincibleTime; }
    bool IsBackReaction() const { return m_bBackReaction; }
    void SetIsGround(bool _IsGround) { IsGround = _IsGround; }
    void SetIsJump(bool _IsJump) { IsJump = _IsJump; }
    bool GetIsJump() const { return IsJump; }
    void ForceFlatGroundContact(float _HoldTime = 0.12f);
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
    void ResetInwardCircleLoopState()
    {
        m_bInwardCircleLoopTracked = false;
        m_bInwardCirclePassedLowerHalf = false;
        m_InwardCircleLoopCenter = Vec2(0.f, 0.f);
        m_InwardCircleLoopRadius = 0.f;
        m_InwardCircleLoopAccumulatedAngle = 0.f;
        m_InwardCircleLoopLastAngle = 0.f;
    }
    void ResetInwardCircleHalfCheckerState()
    {
        m_bInwardCircleHalfCheckerTracked = false;
        m_InwardCircleHalfCheckerCenter = Vec2(0.f, 0.f);
        m_InwardCircleHalfCheckerRadius = 0.f;
        m_bInwardCircleLineOwnsTopRight = false;
    }
    bool IsSameInwardCircleLoop(const Vec2& _Center, float _Radius) const;
    bool IsSameInwardCircleHalfChecker(const Vec2& _Center, float _Radius) const;
    void NoteInwardCircleLoopProgress(const Vec2& _Center, float _Radius, const Vec2& _ContactPoint);
    void UpdateInwardCircleHalfChecker(const Vec2& _Center, float _Radius, const Vec2& _ContactPoint);
    void PrimeInwardCircleHalfCheckerFromLineContext(const SurfaceContact& _Contact);
    bool ShouldLineOwnTopRightInwardCircleHalf(const Vec2& _Center, float _Radius, const Vec2& _ContactPoint) const;
    bool ShouldReleaseInwardCircle(const Vec2& _Center, float _Radius, const Vec2& _ContactPoint) const;
    void BlockInwardCircleAttach(const Vec2& _Center, float _Radius, float _Time);
    bool IsInwardCircleAttachBlocked(const Vec2& _Center, float _Radius) const;
    bool HasInwardCircleLineContext() const;
    bool TryEvaluateGuidedTopHalfInwardCircleContact(const SurfaceContact& _Contact, bool& _OutOnActiveArc, bool& _OutBeforeHalf, bool& _OutBeforeHalfUsesTopRight) const;
    bool IsGuideLinkedCorrectionLine(const SurfaceContact& _LineContact, const SurfaceContact& _CircleContact) const;
    bool IsTopHalfInwardCircleContact(const SurfaceContact& _Contact);
    bool IsChordLineForInwardCircle(const SurfaceContact& _LineContact, const SurfaceContact& _CircleContact) const;
    bool ShouldIgnoreTopHalfInwardCircleContact(const SurfaceContact& _Contact);
    bool ShouldPreferLineOverTopHalfInwardCircle(const SurfaceContact& _LineContact, const SurfaceContact& _CircleContact);
    bool ShouldPreferTopHalfInwardCircleOverLine(const SurfaceContact& _CircleContact, const SurfaceContact& _LineContact);
    void SubmitSurfaceContact(GameObject* _Surface, const Vec2& _Normal, float _SignedDistance,
                              bool _TransitionSurface, bool _Attachable, bool _WallLike, bool _Circle, bool _InwardCircle, float _Score,
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
        m_bBackReaction = false;
        m_Facing = _Dir;
        bIsSpringJump = true;
        IsGround = false;
        m_ActionTime = 0.f;
    }

    void SetGroundTangent(Vec2 _Tangent)
    {
        vTangent = _Tangent;
    }

    void SetKnockBackState(ActionState _Action)
    {
        if (m_bHasCurrentSurfaceContact && m_CurrentSurfaceContact.Surface != nullptr)
            BlockSurfaceAttach(m_CurrentSurfaceContact.Surface, 0.12f);

        m_bHasPendingSurfaceContact = false;
        m_PendingSurfaceContact = SurfaceContact{};
        m_SurfaceGroundHoldTime = 0.f;
        ResetGroundContact();
        ResetInwardCircleLoopState();
        ResetInwardCircleHalfCheckerState();

        IsJump = true;
        m_Action = _Action;
        m_bBackReaction = false;
        bIsSpringJump = false;
        bIsSpringDash = false;
        IsGround = false;
        m_Pose = PoseState::None;
        m_bPushContact = false;
        m_PushContactDir = 0;
        m_bPushing = false;
        m_PushingDir = 0;
        m_fBreakSpeed = 0.f;
        m_bBreakWallLocked = false;
        m_BreakUngroundedTime = 0.f;
        m_IdleTime = 0.f;
        m_ActionTime = 0.f;

        if (_Action == ActionState::KnockBack)
            m_KnockBackInvincibleTime = kKnockBackInvincibleDuration;
    }

    void SetBackState()
    {
        if (m_bHasCurrentSurfaceContact && m_CurrentSurfaceContact.Surface != nullptr)
            BlockSurfaceAttach(m_CurrentSurfaceContact.Surface, 0.12f);

        m_bHasPendingSurfaceContact = false;
        m_PendingSurfaceContact = SurfaceContact{};
        m_SurfaceGroundHoldTime = 0.f;
        ResetGroundContact();
        ResetInwardCircleLoopState();
        ResetInwardCircleHalfCheckerState();

        IsJump = true;
        m_Action = ActionState::Roll;
        m_bBackReaction = true;
        bIsSpringJump = false;
        bIsSpringDash = false;
        IsGround = false;
        m_Pose = PoseState::None;
        m_bPushContact = false;
        m_PushContactDir = 0;
        m_bPushing = false;
        m_PushingDir = 0;
        m_fBreakSpeed = 0.f;
        m_bBreakWallLocked = false;
        m_BreakUngroundedTime = 0.f;
        m_IdleTime = 0.f;
        m_ActionTime = 0.f;
        m_KnockBackInvincibleTime = 0.f;
    }

public:
    virtual void SaveToLevelFile(FILE* _File) override;
    virtual void LoadFromLevelFile(FILE* _File) override;

    CLONE(CPlayerScript);
    CPlayerScript();
    virtual ~CPlayerScript();

private:
    PlayerInput ReadInput() const;

    void InitAutoplay();
    void UpdateAutoplay(float dt);
    void LogAutoplayState(const wchar_t* _Tag);
    void FinishAutoplay();

    void ResolveTransitions(const PlayerInput& in, float dt);
    void ResolvePose(const PlayerInput& in, float dt);
    void ResolveAction(const PlayerInput& in, float dt);
    bool ApplySurfaceContact(const SurfaceContact& _Contact);

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

    void ResetGroundContact();
    void RegisterActiveSurface(GameObject* _SurfaceObject);
    void UnregisterActiveSurface(GameObject* _SurfaceObject);
    float ComputeSurfaceCandidateScore(const SurfaceContact& _Contact, GameObject* _CurrentBestSurface) const;
    bool TrySelectSurfaceContact(SurfaceContact _Contact, SurfaceContact& _BestContact,
                                 float& _BestScore, GameObject*& _BestSurface, bool& _HasBest);
    bool ProbeSurfaceObject(GameObject* _SurfaceObject, bool _WasGround,
                            SurfaceContact& _BestContact, float& _BestScore,
                            GameObject*& _BestSurface, bool& _HasBest);
    void ProbeSceneSurfaceContacts(bool _WasGround, SurfaceContact& _BestContact, float& _BestScore,
                                   GameObject*& _BestSurface, bool& _HasBest);
    void ResolveBufferedSurfaceContacts();

    float Lerp(float _Start, float _End, float _Ratio)
    {
        return _Start + (_End - _Start) * _Ratio;
    }
};
