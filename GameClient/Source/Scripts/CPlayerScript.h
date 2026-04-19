#pragma once

#include "CScript.h"
#include "GameObject.h"

#include <unordered_set>
#include <vector>

class CCollider2D;
class CSurfaceCircleGuideScript;

class CPlayerScript
    : public CScript
{
public:
    enum class CylinderState
    {
        None,
        Bounce,
    };

    enum class ITEM_STATE
    {
        NONE,
        FIRE,
        WATER,
        ELECTRIC,
        STAR,
    };

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
        Vec2 Tangent = Vec2(1.f, 0.f);
        float SlopeAngle = 0.f;
    };

    struct SurfaceAttachBlock
    {
        GameObject* Surface = nullptr;
        float Time = 0.f;
    };

    struct WallPushContact
    {
        GameObject* Surface = nullptr;
        int Dir = 0;
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
    float m_MaxMoveSpeed = 1000.f;
    float m_JumpForce = 600.f;
    float m_SkillDashSpeed = 600.f;
    float m_PushSpeed = 30.f;
    float m_AirAccelScale = 0.5f;
    float m_WaterAccelScale = 1.f;
    int m_WaterVolumeCount = 0;
    ITEM_STATE m_eShieldType = ITEM_STATE::NONE;
    bool m_bIsFirebouncing = false;
    bool m_bIsWaterbouncing = false;
    bool m_bIsElectricbouncing = false;
    float m_StarShieldTime = 0.f;
    float m_StoredMaxMoveSpeed = 1000.f;

    bool IsJump = false;
    bool m_bNeedGravity = false;

    bool bIsBreak = false;

    bool bIsSpringJump = false;
    bool bIsSpringDash = false;

    PoseState m_Pose = PoseState::None;
    ActionState m_Action = ActionState::None;
    CylinderState m_CylinderState = CylinderState::None;
    bool m_bBackReaction = false;

    float m_IdleTime = 0.f;
    float m_PoseTime = 0.f;
    float m_ActionTime = 0.f;
    float m_BreakUngroundedTime = 0.f;
    float m_KnockBackInvincibleTime = 0.f;

    int m_Facing = 1;
    int m_TurnTargetFacing = -1;

    bool IsGround = false;
    bool m_bVerticalRoleLineAttached = false;
    int m_PhysicalGroundOverlapCount = 0;
    int m_AttachableSurfaceOverlapCount = 0;
    bool m_bHasNearbyAttachableSurface = false;
    bool m_bPushContact = false;
    int m_PushContactDir = 0;
    bool m_bPushing = false;
    int m_PushingDir = 0;
    vector<WallPushContact> m_vecWallPushContacts;
    float m_fBreakSpeed = 0.f;
    int m_iBreakDirection = 1;
    bool m_bBreakWallLocked = false;
    float m_LastTickPosX = 0.f;
    float m_LastFrameDeltaX = 0.f;
    float m_PhysicalGroundAnimGraceTime = 0.f;
    vector<SurfaceAttachBlock> m_vecAttachBlockedSurfaces;
    float m_ForcedFlatGroundLockTime = 0.f;
    float m_SurfaceGroundHoldTime = 0.f;
    float m_ItemBoxBounceAttachIgnoreTime = 0.f;
    float m_SurfaceResolveFrame = -1.f;
    float m_SurfaceResolveScore = 999999.f;
    float m_LastSceneSurfaceProbeFrame = -1.f;
    Vec2 m_LastSceneSurfaceQueryMin = Vec2(0.f, 0.f);
    Vec2 m_LastSceneSurfaceQueryMax = Vec2(0.f, 0.f);
    bool m_bHasLastSceneSurfaceQueryBounds = false;
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
    bool m_bGuidedInwardCircleEntryTracked = false;
    Vec2 m_GuidedInwardCircleEntryCenter = Vec2(0.f, 0.f);
    float m_GuidedInwardCircleEntryRadius = 0.f;
    bool m_bGuidedInwardCircleEntryFromRight = false;
    bool m_bGuidedInwardCircleLineOwnsRight = false;
    bool m_bGuidedInwardCirclePassedHalf = false;
    bool m_bGuidedInwardCircleHalfZoneTracked = false;
    int m_GuidedInwardCircleHalfZoneSign = 0;
    bool m_bGuidedInwardCircleHalfDeltaTracked = false;
    float m_GuidedInwardCircleLastHalfDelta = 0.f;
    bool m_bHasRecentReleasedInwardCircleContact = false;
    SurfaceContact m_RecentReleasedInwardCircleContact = {};
    float m_RecentReleasedInwardCircleTime = 0.f;
    float m_RecentGuidedCorrectionLineTime = 0.f;
    wstring m_RecentGuidedCorrectionLineName;
    float m_SurfaceRotationSnapTime = 0.f;

    bool m_bHasPendingSurfaceContact = false;
    SurfaceContact m_PendingSurfaceContact = {};
    bool m_bHasCurrentSurfaceContact = false;
    SurfaceContact m_CurrentSurfaceContact = {};
    vector<GameObject*> m_vecActiveSurfaceObjects;
    std::unordered_set<GameObject*> m_setActiveSurfaceObjects;

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

private:
    static int m_PlayerLife;

public:
    static void AddLife()
    {
        m_PlayerLife += 1;
        if (m_PlayerLife < 0)
            m_PlayerLife = -1;
        // 여기서 UI 업데이트 함수를 호출하면 편리합니다!
    }

    static void SubLife()
    {
        m_PlayerLife -= 1;
        if (m_PlayerLife < 0)
            m_PlayerLife = -1;
        // 여기서 UI 업데이트 함수를 호출하면 편리합니다!
    }

    static int GetPlayerLife()
    {
        return m_PlayerLife;
    }

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
    float GetMaxMoveSpeed() const { return m_MaxMoveSpeed; }
    float GetBreakSpeed() const { return m_fBreakSpeed; }
    ActionState GetAction() const { return m_Action; }
    CylinderState GetCylinderState() const { return m_CylinderState; }
    bool GetIsGround() const { return IsGround; }
    bool HasCurrentCircleSurfaceContact() const
    {
        return m_bHasCurrentSurfaceContact &&
            m_CurrentSurfaceContact.Surface != nullptr &&
            m_CurrentSurfaceContact.Circle;
    }
    bool ShouldFreezeInwardCircleHalfChecker() const
    {
        return m_InwardCircleAttachBlockTime > 0.f ||
            (m_bHasRecentReleasedInwardCircleContact && m_RecentReleasedInwardCircleTime > 0.f);
    }
    bool IsBreakOrRollAction() const { return m_Action == ActionState::Break || m_Action == ActionState::Roll || m_Action == ActionState::SkillDash; }
    bool IsKnockBackInvincible() const { return m_KnockBackInvincibleTime > 0.f; }
    float GetKnockBackInvincibleTime() const { return m_KnockBackInvincibleTime; }
    bool IsBackReaction() const { return m_bBackReaction; }
    bool IsCylinderBounceState() const { return m_CylinderState == CylinderState::Bounce; }
    void ClearStableAttachJumpState()
    {
        IsJump = false;
        bIsSpringJump = false;
        if (m_Action == ActionState::Spring)
        {
            m_Action = ActionState::None;
            m_ActionTime = 0.f;
        }
    }
    void SetIsGround(bool _IsGround)
    {
        IsGround = _IsGround;
        if (_IsGround)
            ClearStableAttachJumpState();

        m_bVerticalRoleLineAttached = false;
    }
    void SetVerticalRoleLineAttached(bool _Attached)
    {
        m_bVerticalRoleLineAttached = _Attached;
        if (_Attached)
        {
            IsGround = true;
            ClearStableAttachJumpState();
        }
    }
    bool IsVerticalRoleLineAttached() const { return m_bVerticalRoleLineAttached; }
    void SetIsJump(bool _IsJump) { IsJump = _IsJump; }
    bool GetIsJump() const { return IsJump; }
    void SetCylinderState(CylinderState _State) { m_CylinderState = _State; }
    bool HasPhysicalGroundOverlap() const { return m_PhysicalGroundOverlapCount > 0; }
    bool HasAttachableSurfaceOverlap() const { return m_AttachableSurfaceOverlapCount > 0 || m_bHasNearbyAttachableSurface; }
    bool HasAnyGroundOverlap() const { return HasPhysicalGroundOverlap() || HasAttachableSurfaceOverlap(); }
    bool HasPushContactDir(int _Dir) const;
    void RegisterWallPushContact(GameObject* _Surface, int _Dir);
    void UnregisterWallPushContact(GameObject* _Surface);
    void ForceFlatGroundContact(float _HoldTime = 0.12f);
    void BlockSurfaceAttach(GameObject* _Surface, float _Time);
    bool IsSurfaceAttachBlocked(GameObject* _Surface) const;
    void ClearPlainSurfaceAttachBlocks();
    void BeginItemBoxBounceDetach(float _BlockTime = 0.14f);
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
    void ResetGuidedInwardCircleState()
    {
        m_bGuidedInwardCircleEntryTracked = false;
        m_GuidedInwardCircleEntryCenter = Vec2(0.f, 0.f);
        m_GuidedInwardCircleEntryRadius = 0.f;
        m_bGuidedInwardCircleEntryFromRight = false;
        m_bGuidedInwardCircleLineOwnsRight = false;
        m_bGuidedInwardCirclePassedHalf = false;
        m_bGuidedInwardCircleHalfZoneTracked = false;
        m_GuidedInwardCircleHalfZoneSign = 0;
        m_bGuidedInwardCircleHalfDeltaTracked = false;
        m_GuidedInwardCircleLastHalfDelta = 0.f;
    }
    void ResetInwardCircleHalfCheckerState()
    {
        m_bInwardCircleHalfCheckerTracked = false;
        m_InwardCircleHalfCheckerCenter = Vec2(0.f, 0.f);
        m_InwardCircleHalfCheckerRadius = 0.f;
        m_bInwardCircleLineOwnsTopRight = false;
        ResetGuidedInwardCircleState();
    }
    bool IsSameInwardCircleLoop(const Vec2& _Center, float _Radius) const;
    bool IsSameInwardCircleHalfChecker(const Vec2& _Center, float _Radius) const;
    bool TryInferInwardCircleHalfOwnership(const Vec2& _Center, float _Radius, bool& _OutLineOwnsTopRight) const;
    bool IsSameGuidedInwardCircleEntryState(const Vec2& _Center, float _Radius) const;
    bool IsAttachableLineSurfaceObject(GameObject* _SurfaceObject) const;
    bool IsVerticalRoleLineSurfaceObject(GameObject* _SurfaceObject) const;
    const SurfaceContact* GetReferenceAttachableLineContact() const;
    GameObject* GetReferenceAttachableLineSurface() const;
    bool AreLineSurfaceEndpointsConnected(GameObject* _SurfaceA, GameObject* _SurfaceB) const;
    bool TryGetTransitionLineSeamEndpoint(const SurfaceContact& _Contact, bool _UseExitEndpoint, Vec2& _OutEndpoint) const;
    float GetTransitionLineEndpointGap(const SurfaceContact& _ReferenceContact, const SurfaceContact& _CandidateContact) const;
    bool AreTransitionLineContactsSeamCompatible(const SurfaceContact& _ReferenceContact, const SurfaceContact& _CandidateContact) const;
    bool IsConnectedTransitionLineContact(const SurfaceContact& _Contact) const;
    bool IsRelaxedDownwardTransitionLineContact(const SurfaceContact& _Contact) const;
    float ComputeVerticalRoleLineNormalAlignment(const SurfaceContact& _Contact) const;
    float ComputeVerticalRoleLineTransferAlignment(const SurfaceContact& _Contact) const;
    bool IsVerticalRoleLineTransferCandidate(const SurfaceContact& _Contact) const;
    bool InferGuidedInwardCircleEntryFromRight(const Vec2& _Center);
    bool TryGetInwardCircleSurfaceData(const SurfaceContact& _Contact, Vec2& _OutCenter, float& _OutRadius) const;
    bool TryGetInwardCircleGuideData(const SurfaceContact& _Contact, Vec2& _OutCenter, float& _OutRadius, const CSurfaceCircleGuideScript*& _OutGuide) const;
    bool TryGetGuidedInwardCircleABPoints(const Vec2& _Center, float _Radius, const CSurfaceCircleGuideScript* _Guide,
                                          Vec2& _OutLeftPoint, Vec2& _OutRightPoint) const;
    bool TryGetGuidedInwardCircleRuntimeState(const Vec2& _Center, float _Radius, const CSurfaceCircleGuideScript* _Guide,
                                              Vec2& _OutLeftPoint, Vec2& _OutRightPoint,
                                              bool& _OutEntryFromRight, bool& _OutPassedHalf,
                                              bool& _OutLineOwnsRight);
    bool TryGetGuidedInwardCirclePassState(const Vec2& _Center, float _Radius, const CSurfaceCircleGuideScript* _Guide,
                                           Vec2& _OutLeftPoint, Vec2& _OutRightPoint,
                                           bool& _OutPassedHalf, bool& _OutOpenRight);
    void NoteInwardCircleLoopProgress(const Vec2& _Center, float _Radius, const Vec2& _ContactPoint);
    void UpdateInwardCircleHalfChecker(const Vec2& _Center, float _Radius, const Vec2& _ContactPoint);
    void PrimeInwardCircleHalfCheckerFromLineContext(const SurfaceContact& _Contact);
    bool ShouldLineOwnTopRightInwardCircleHalf(const Vec2& _Center, float _Radius, const Vec2& _ContactPoint) const;
    bool ShouldReleaseInwardCircle(const Vec2& _Center, float _Radius, const Vec2& _ContactPoint) const;
    void BlockInwardCircleAttach(const Vec2& _Center, float _Radius, float _Time);
    bool IsInwardCircleAttachBlocked(const Vec2& _Center, float _Radius) const;
    bool HasInwardCircleLineContext() const;
    bool ShouldIgnoreGuidedCircleWallContact(GameObject* _Surface, const Vec2& _ContactPoint);
    bool IsGuidedInwardCircleActiveArcPoint(const Vec2& _Center, float _Radius, const CSurfaceCircleGuideScript* _Guide,
                                            const Vec2& _Point);
    bool ShouldIgnoreGuidedInwardCirclePoint(const Vec2& _Center, float _Radius, const CSurfaceCircleGuideScript* _Guide,
                                             const Vec2& _Point);
    bool TryResolveGuidedInwardCircleEntryExit(const Vec2& _Center, float _Radius, const CSurfaceCircleGuideScript* _Guide,
                                               Vec2& _OutEntryPoint, Vec2& _OutExitPoint,
                                               float& _OutEntryAngle, float& _OutExitAngle,
                                               bool _CommitEntryState = false);
    bool TryGetGuidedInwardCircleHalfDelta(const SurfaceContact& _Contact, bool& _OutOnActiveArc,
                                           float& _OutHalfDelta, bool& _OutEntryOnRight);
    void UpdateGuidedInwardCircleHalfPhase(const SurfaceContact& _Contact, bool _ContinuingCurrentInwardCircle);
    bool ShouldIgnoreGuidedInwardCircleContact(const SurfaceContact& _Contact);
    bool ShouldReleaseGuidedInwardCircle(const SurfaceContact& _Contact, const Vec2& _Center, float _Radius);
    bool TryEvaluateGuidedTopHalfInwardCircleContact(const SurfaceContact& _Contact, bool& _OutOnActiveArc, bool& _OutBeforeHalf, bool& _OutBeforeHalfUsesTopRight);
    bool IsGuideLinkedCorrectionLine(const SurfaceContact& _LineContact, const SurfaceContact& _CircleContact) const;
    bool IsGuidedCorrectionLineContact(const SurfaceContact& _Contact) const;
    bool IsRecentGuidedCorrectionLineSurface(GameObject* _Surface) const;
    void CacheRecentGuidedCorrectionLineFromSurface(GameObject* _Surface, float _Time);
    bool IsTopHalfInwardCircleContact(const SurfaceContact& _Contact);
    bool IsChordLineForInwardCircle(const SurfaceContact& _LineContact, const SurfaceContact& _CircleContact) const;
    bool IsPreservedInwardCircleContact(const SurfaceContact& _Contact) const;
    bool ShouldIgnoreTopHalfInwardCircleContact(const SurfaceContact& _Contact);
    bool ShouldPreferLineOverTopHalfInwardCircle(const SurfaceContact& _LineContact, const SurfaceContact& _CircleContact);
    bool ShouldPreferTopHalfInwardCircleOverLine(const SurfaceContact& _CircleContact, const SurfaceContact& _LineContact);
    void DrawGuidedInwardCircleDebug();
    void SubmitSurfaceContact(GameObject* _Surface, const Vec2& _Normal, float _SignedDistance,
                              bool _TransitionSurface, bool _Attachable, bool _WallLike, bool _Circle, bool _InwardCircle, float _Score,
                              float _SeamBlendT = 0.f, bool _SeamBlendHasValue = false,
                              const Vec2& _SeamStart = Vec2(0.f, 0.f), const Vec2& _SeamEnd = Vec2(0.f, 0.f),
                              const Vec2& _ContactPoint = Vec2(0.f, 0.f),
                              const Vec2& _Tangent = Vec2(1.f, 0.f), float _SlopeAngle = 0.f);

    void SetNeedGravity(bool _Value) { m_bNeedGravity = _Value; }
    bool GetNeedGravity() const { return m_bNeedGravity; }
    void EnterWaterVolume(float _AccelScale);
    void ExitWaterVolume();
    float GetWaterAccelScale() const { return m_WaterAccelScale; }
    ITEM_STATE GetItemState() const { return m_eShieldType; }
    void SetItemState(ITEM_STATE _State);
    void Fire();
    void Water();
    void Electric();
    void Star();
    void CreateFireShield();
    void CreateWaterShield();
    void CreateElectricShield();
    void CreateStarShield();
    void CreateElectricShieldEffect();
    void OnEatShield();

public:
    void SetSpringJumpState(ActionState _Action, int _Dir)
    {
        IsJump = true;
        m_Action = _Action;
        m_bBackReaction = false;
        m_Facing = _Dir;
        bIsSpringJump = true;
        IsGround = false;
        m_bVerticalRoleLineAttached = false;
        m_ActionTime = 0.f;
    }

    void SetGroundTangent(Vec2 _Tangent)
    {
        vTangent = _Tangent;
    }
    Vec2 GetGroundTangent() const { return vTangent; }

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
        m_CylinderState = CylinderState::None;
        m_bBackReaction = false;
        bIsSpringJump = false;
        bIsSpringDash = false;
        IsGround = false;
        m_bVerticalRoleLineAttached = false;
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
        m_CylinderState = CylinderState::None;
        m_bBackReaction = true;
        bIsSpringJump = false;
        bIsSpringDash = false;
        IsGround = false;
        m_bVerticalRoleLineAttached = false;
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

    void SetItemBoxBounceState();

public:
    virtual void SaveToLevelFile(FILE* _File) override;
    virtual void LoadFromLevelFile(FILE* _File) override;

    void Dead();

    CLONE(CPlayerScript);
    CPlayerScript();
    CPlayerScript(const CPlayerScript& _Origin);
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
    bool HandleCirclePhysics(const SurfaceContact& _Contact, const Vec2& _CurrentNormal,
                             bool _JumpPressed, bool _BlockedByWallLikeSurface,
                             bool _SuppressSlopeSlip,
                             float& _InOutTangentSpeed, bool& _OutForceFall);

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

    void NormalizeStableSurfaceAttachmentState();
    void ResetGroundContact();
    void RegisterActiveSurface(GameObject* _SurfaceObject);
    void UnregisterActiveSurface(GameObject* _SurfaceObject);
    void RefreshNearbyAttachableSurfaces();
    bool IsActiveSurfaceObject(GameObject* _SurfaceObject) const;
    float ComputeSurfaceCandidateScore(const SurfaceContact& _Contact, GameObject* _CurrentBestSurface) const;
    bool TrySelectSurfaceContact(SurfaceContact _Contact, SurfaceContact& _BestContact,
                                 float& _BestScore, GameObject*& _BestSurface, bool& _HasBest);
    bool ProbeSurfaceObject(GameObject* _SurfaceObject, bool _WasGround,
                            SurfaceContact& _BestContact, float& _BestScore,
                            GameObject*& _BestSurface, bool& _HasBest);
    void ResolveBufferedSurfaceContacts();

    float Lerp(float _Start, float _End, float _Ratio)
    {
        return _Start + (_End - _Start) * _Ratio;
    }
};
