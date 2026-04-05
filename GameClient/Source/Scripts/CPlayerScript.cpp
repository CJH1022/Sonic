#include "pch.h"
#include "CPlayerScript.h"

#include "KeyMgr.h"
#include "TimeMgr.h"
#include "RenderMgr.h"
#include "CTransform.h"
#include "LevelMgr.h"
#include "GameObject.h"
#include "CCollider2D.h"
#include "TaskMgr.h"
#include "CTileScript.h"
#include "CSurfaceCircleGuideScript.h"
#include "CSurfaceScript.h"
#include "CBlockScript.h"
#include "CBlockMovingScript.h"
#include "func.h"

#include <cmath> // fabsf
#include "CBlockPushingScript.h"
#include "AssetMgr.h"

namespace
{
    constexpr int kPushingFlipbookIndex = 20;
    constexpr int kKnockBackFlipbookIndex = 21;
    constexpr float kKnockBackBlinkInterval = 0.08f;
    constexpr float kKnockBackMinGroundRecoverTime = 0.25f;
    constexpr float kKnockBackMaxDuration = 0.45f;
    constexpr float kKnockBackGroundFriction = 8.f;
    constexpr float kKnockBackAirDrag = 0.75f;
    constexpr float kGroundRotationLerpSpeed = 14.f;
    constexpr float kSurfaceResolveFrameEpsilon = 0.0001f;
    constexpr float kSurfaceResolveScoreEpsilon = 0.01f;
    constexpr float kSurfaceLineSeamKeepDot = 0.90f;
    constexpr float kSurfaceLineSeamTransitionDepth = 0.03f;
    constexpr float kSurfaceLineSeamDownwardDelta = 0.025f;
    constexpr float kSurfaceLineSeamDirectionBias = 0.40f;
    constexpr float kSurfaceLineSeamTransitionScoreFavor = 0.30f;
    constexpr float kSurfaceLineSeamFlatToSlopePushLimit = 0.02f;
    constexpr float kSurfaceLineSeamAngleShiftDot = 0.90f;
    constexpr float kSurfaceLineSeamAngleShiftScorePenalty = 0.28f;
    constexpr float kSurfaceLineSeamAngleShiftPushLimit = 0.015f;
    constexpr float kSurfaceLineSeamEntryBlendFloor = 0.25f;
    constexpr float kSurfaceLineSeamEnterDownGradeBonus = 0.6f;
    constexpr float kSurfaceLineSeamMinY = 0.08f;
    constexpr float kSurfaceLineSeamMoveSpeedGate = 2200.f;
    constexpr float kSurfaceLineSeamHoldSpeedMin = 70.f;      // 낮은 속도에서 이전 접선 고착을 더 억제
    constexpr float kSurfaceLineSeamFlatToSlopeBlendFloor = 0.55f;
    constexpr float kSurfaceLineSeamAngleShiftBlendFloor = 0.60f;
    constexpr float kSurfaceLineSeamDownwardBlendFloor = 0.80f;
    constexpr float kSurfaceLineSeamSteepDownBlendFloor = 0.85f;
    constexpr float kSurfaceLineSeamRisingSlopeScoreFavor = 0.45f;
    constexpr float kSurfaceLineSeamRisingSlopeBlendFloor = 0.72f;
constexpr float kSurfaceLineSeamRisingSlopePushLimit = 1.25f;
constexpr float kSurfaceLineSeamRisingSlopeSnapMaxDistance = 10.f;
constexpr float kSurfaceLineSeamRisingSlopeMoveMin = 5.f;
constexpr float kSurfaceLineSeamRisingSlopeSpeedMin = 70.f;
    constexpr float kSurfaceLineSeamRisingSlopeEntryBlendMin = 0.12f;
    constexpr float kSurfaceLineSeamConnectionMaxDistance = 6.f;
    constexpr float kSurfaceLineSeamEndpointMatchDistance = 4.f;
    constexpr float kSurfaceLineSeamEntryProjectionMinDistance = 0.75f;
    constexpr float kSurfaceLineSeamEntrySnapMinDistance = 1.0f;
    constexpr float kSurfaceLineUnconnectedPositiveRejectDistance = 0.25f;
    constexpr float kSurfaceLineRelaxedDownwardSeamBiasMin = 0.15f;
    constexpr float kSurfaceSceneProbeMargin = 32.f;
    constexpr float kSurfaceDetachedLinePositiveThreshold = 0.05f;
    constexpr float kSurfaceDetachedLineFallSpeedMin = 10.f;

    constexpr float kSurfaceLineAttachSnapMaxDistance = 2.5f;  // line 표면은 현재/전이 라인일 때만 짧게 스냅한다
    constexpr float kSurfaceLineStableProjectionMaxDistance = 2.5f;
    constexpr float kSurfaceLinePenetrationPushMaxDistance = 1.5f;
    constexpr float kSurfaceGuidedLineProjectionMaxDistance = 18.f;
    constexpr float kSurfaceGuidedLineDeepProjectionMaxDistance = 64.f;
    constexpr float kSurfaceGuidedLineForceProjectionMaxDistance = 96.f;
    constexpr float kSurfaceContactPushBias = 0.015f;         // 미세 침투 복귀량을 더 낮춰 점프/튕김을 완화
    constexpr float kSurfaceContactMaxPush = 1.5f;
    constexpr float kSurfaceContactDownTransitionPushLimit = 0.08f;
    constexpr float kSurfaceStableProjectionMaxDistance = 6.f;
    constexpr float kSurfaceCircleProjectionMaxDistance = 6.f;
    constexpr float kSurfaceCircleAirAcquireMaxDistance = 1.25f;
    constexpr float kSurfaceCircleNormalBlend = 0.82f;
    constexpr float kSurfaceCircleGroundHoldTime = 0.14f;
    constexpr float kSurfaceCircleStickMinY = 0.02f;
    constexpr float kSurfaceCircleStickSpeedMin = 40.f;
    constexpr float kSurfaceCircleLowSpeedFallMinNormalY = 0.22f;
    constexpr float kSurfaceCircleLowSpeedFallSpeedMin = 180.f;
    constexpr float kSurfaceJumpDetachSpeedMin = 60.f;
    constexpr float kSurfaceInwardCircleAttachBlockTime = 1.20f;
    constexpr float kSurfaceInwardCircleLoopReleaseAngle = XM_2PI - 0.75f;
    constexpr float kSurfaceInwardCircleUpperHalfMin = 0.f;
    constexpr float kSurfaceInwardCircleMaxAngleStep = XM_PIDIV2;
    constexpr float kSurfaceInwardCircleReturnPullMaxDistance = 4.f;
    constexpr float kSurfaceInwardCirclePreReleaseUpperHalfMin = -0.05f;
    constexpr float kSurfaceInwardCirclePreReleaseAngleMargin = 0.20f;
    constexpr float kSurfaceGuidedHalfCheckTopAngleDeg = 90.f;
    constexpr float kSurfaceGuidedEndpointSwitchToleranceMin = 12.f;
    constexpr float kSurfaceGuidedEndpointSwitchToleranceScale = 0.10f;
    constexpr float kSurfaceGuidedEndpointHoldToleranceMin = 18.f;
    constexpr float kSurfaceGuidedEndpointHoldToleranceScale = 0.14f;
    constexpr float kSurfaceGuidedEndpointBlendRadiusMin = 20.f;
    constexpr float kSurfaceGuidedEndpointBlendRadiusScale = 0.16f;
    constexpr float kSurfaceGuidedEndpointBlendMin = 0.22f;
    constexpr float kSurfaceGuidedHalfCrossAngleEpsilon = XMConvertToRadians(3.f);
    constexpr float kSurfaceGuidedHalfTouchRadiusMin = 10.f;
    constexpr float kSurfaceGuidedHalfTouchRadiusScale = 0.06f;
    constexpr float kSurfaceGuidedCorrectionLineTransferHoldTime = 0.18f;
    constexpr float kSurfaceGuidedLineRotationSnapTime = 0.10f;
    constexpr float kSurfaceInwardCircleTopHalfSwitchEpsilon = 1.5f;
    constexpr float kSurfaceInwardCircleHalfCheckerSwitchUpperHalfMin = 0.75f;
    constexpr float kSurfaceInwardCircleLowSpeedDetachUpperHalfMin = 0.35f;
    constexpr float kSurfaceInwardCircleLowSpeedDetachSpeedMin = 110.f;
    constexpr float kSurfaceInwardCircleLowSpeedDetachMinAngle = 0.35f;
    constexpr float kSurfaceInwardCircleLowSpeedDetachBlockTime = 0.22f;
    constexpr float kSurfaceInwardCircleLowSpeedFallUpperHalfMin = 0.58f;
    constexpr float kSurfaceInwardCircleLowSpeedFallMaxNormalY = 0.30f;
    constexpr float kSurfaceInwardCircleBackSlipMaxNormalY = 0.55f;
    constexpr float kSurfaceInwardCircleBackSlipMinGravityTangent = 0.15f;
    constexpr float kSurfaceInwardCircleBackSlipWalkSpeedMin = 55.f;
    constexpr float kSurfaceInwardCircleBackSlipWalkSpeedMax = 130.f;
    constexpr float kSurfaceInwardCircleBackSlipAccelScale = 0.55f;
    constexpr float kSurfaceLineOverTopHalfInwardCircleDistanceMargin = 1.5f;
    constexpr float kAutoplaySampleInterval = 0.10f;
    constexpr float kAutoplayDefaultTotalDuration = 15.35f;
    constexpr float kAutoplaySurfaceTotalDuration = 9.20f;
    constexpr float kAutoplaySurfaceChordTotalDuration = 3.40f;
    constexpr float kAutoplaySurfaceLoopTotalDuration = 7.20f;
    constexpr float kAutoplaySurfaceJumpTotalDuration = 2.60f;
    constexpr float kAutoplaySurfaceTopJumpTotalDuration = 2.20f;
    constexpr float kAutoplaySurfaceSlowFallTotalDuration = 2.20f;
    constexpr float kAutoplaySurfaceLineUnderTotalDuration = 2.20f;

    float WrapAngleRad(float _Angle)
    {
        while (_Angle > XM_PI)
            _Angle -= XM_2PI;

        while (_Angle < -XM_PI)
            _Angle += XM_2PI;

        return _Angle;
    }

    float LerpAngleRad(float _Current, float _Target, float _Speed, float _Dt)
    {
        float delta = WrapAngleRad(_Target - _Current);
        float t = _Speed * _Dt;
        if (t > 1.f)
            t = 1.f;

        return WrapAngleRad(_Current + delta * t);
    }

    float Clamp01f(float _Value)
    {
        if (_Value < 0.f)
            return 0.f;
        if (_Value > 1.f)
            return 1.f;
        return _Value;
    }

    bool GetColliderQueryBounds(CCollider2D* _Collider, Vec2& _OutMin, Vec2& _OutMax)
    {
        if (_Collider == nullptr || _Collider->GetOwner() == nullptr)
            return false;

        GameObject* pOwner = _Collider->GetOwner();
        Vec3 worldPos = pOwner->Transform()->GetWorldPos();
        Vec3 worldScale = pOwner->Transform()->GetWorldScale();
        Vec2 offset = _Collider->GetOffset();
        Vec2 scale = _Collider->GetScale();

        Vec2 center = Vec2(worldPos.x + offset.x, worldPos.y + offset.y);
        Vec2 half = Vec2(fabsf(worldScale.x * scale.x) * 0.5f,
                         fabsf(worldScale.y * scale.y) * 0.5f);

        _OutMin = center - half;
        _OutMax = center + half;
        return true;
    }

    float NormalizePositiveAngleRad(float _Angle)
    {
        while (_Angle < 0.f)
            _Angle += XM_2PI;

        while (_Angle >= XM_2PI)
            _Angle -= XM_2PI;

        return _Angle;
    }

    float DeltaAngleCCW(float _From, float _To)
    {
        return NormalizePositiveAngleRad(_To - _From);
    }

    float DeltaAngleAlongOrientation(bool _CounterClockwise, float _From, float _To)
    {
        return _CounterClockwise ? DeltaAngleCCW(_From, _To) : DeltaAngleCCW(_To, _From);
    }

    bool TryResolveArcOrientationViaHalf(float _StartAngle, float _EndAngle, float _HalfAngle, bool& _OutCounterClockwise)
    {
        if (DeltaAngleCCW(_StartAngle, _HalfAngle) <= DeltaAngleCCW(_StartAngle, _EndAngle) + 0.0001f)
        {
            _OutCounterClockwise = true;
            return true;
        }

        if (DeltaAngleCCW(_EndAngle, _HalfAngle) <= DeltaAngleCCW(_EndAngle, _StartAngle) + 0.0001f)
        {
            _OutCounterClockwise = false;
            return true;
        }

        return false;
    }

    bool IsCircleGeometry(CSurfaceScript::SURFACE_GEOMETRY _Geometry)
    {
        return _Geometry == CSurfaceScript::SURFACE_GEOMETRY::CIRCLE ||
               _Geometry == CSurfaceScript::SURFACE_GEOMETRY::FULL_CIRCLE;
    }

    float DegreesToRadians(float _AngleDeg)
    {
        return _AngleDeg * XM_PI / 180.f;
    }

    Vec2 MakeCirclePointRad(const Vec2& _Center, float _Radius, float _AngleRad)
    {
        return _Center + Vec2(cosf(_AngleRad) * _Radius, sinf(_AngleRad) * _Radius);
    }

    void DrawDebugLine2D(const Vec2& _Start, const Vec2& _End, float _Thickness, const Vec4& _Color,
                         float _Duration, bool _DepthTest, float _Z)
    {
        const Vec2 delta = _End - _Start;
        const float length = sqrtf(delta.x * delta.x + delta.y * delta.y);
        if (length <= 0.001f)
            return;

        const Vec2 center = (_Start + _End) * 0.5f;
        const float angle = atan2f(delta.y, delta.x);
        DrawDebugRect(Vec3(center.x, center.y, _Z),
                      Vec3(length, _Thickness, 1.f),
                      Vec3(0.f, 0.f, angle),
                      _Color,
                      _Duration,
                      _DepthTest);
    }

    void DrawDebugArc2D(const Vec2& _Center, float _Radius,
                        float _StartAngleRad, float _EndAngleRad, bool _CounterClockwise,
                        float _Thickness, const Vec4& _Color,
                        float _Duration, bool _DepthTest, float _Z)
    {
        if (_Radius <= 0.001f)
            return;

        const int segmentCount = 20;
        Vec2 prev = MakeCirclePointRad(_Center, _Radius, _StartAngleRad);
        const float totalArc = DeltaAngleAlongOrientation(_CounterClockwise, _StartAngleRad, _EndAngleRad);
        for (int segment = 1; segment <= segmentCount; ++segment)
        {
            const float t = (float)segment / (float)segmentCount;
            const float angle = _CounterClockwise
                ? (_StartAngleRad + totalArc * t)
                : (_StartAngleRad - totalArc * t);
            const Vec2 next = MakeCirclePointRad(_Center, _Radius, angle);
            DrawDebugLine2D(prev, next, _Thickness, _Color, _Duration, _DepthTest, _Z);
            prev = next;
        }
    }

    void DrawDebugArrow2D(const Vec2& _Start, const Vec2& _End, float _Thickness, const Vec4& _Color,
                          float _Duration, bool _DepthTest, float _Z)
    {
        DrawDebugLine2D(_Start, _End, _Thickness, _Color, _Duration, _DepthTest, _Z);

        Vec2 forward = _End - _Start;
        const float forwardLength = sqrtf(forward.x * forward.x + forward.y * forward.y);
        if (forwardLength <= 0.0001f)
            return;

        forward /= forwardLength;
        const Vec2 side = Vec2(-forward.y, forward.x);
        const float arrowHeadLength = 10.f;
        const float arrowHeadWidth = 5.f;
        const Vec2 headBase = _End - forward * arrowHeadLength;
        DrawDebugLine2D(_End, headBase + side * arrowHeadWidth, _Thickness, _Color, _Duration, _DepthTest, _Z);
        DrawDebugLine2D(_End, headBase - side * arrowHeadWidth, _Thickness, _Color, _Duration, _DepthTest, _Z);
    }

    bool TryResolveGuidedInwardCircleHalfArc(const Vec2& _Center, float _Radius,
                                             const Vec2& _LeftPoint, const Vec2& _RightPoint,
                                             bool _LineOwnsRight,
                                             float& _OutStartAngle, float& _OutEndAngle,
                                             bool& _OutCounterClockwise)
    {
        _OutStartAngle = 0.f;
        _OutEndAngle = 0.f;
        _OutCounterClockwise = false;

        if (_Radius <= 0.0001f)
            return false;

        const float halfAngle = DegreesToRadians(kSurfaceGuidedHalfCheckTopAngleDeg);
        const Vec2 halfPoint = MakeCirclePointRad(_Center, _Radius, halfAngle);
        const Vec2 startPoint = _LineOwnsRight ? halfPoint : _RightPoint;
        const Vec2 endPoint = _LineOwnsRight ? _LeftPoint : halfPoint;
        _OutStartAngle = atan2f(startPoint.y - _Center.y, startPoint.x - _Center.x);
        _OutEndAngle = atan2f(endPoint.y - _Center.y, endPoint.x - _Center.x);
        return TryResolveArcOrientationViaHalf(_OutStartAngle, _OutEndAngle, halfAngle, _OutCounterClockwise);
    }

    bool IsAngleOnGuidedInwardCircleArc(float _PointAngle, float _StartAngle, float _EndAngle,
                                        bool _CounterClockwise, float _AngleTolerance)
    {
        const float arcToEnd = DeltaAngleAlongOrientation(_CounterClockwise, _StartAngle, _EndAngle);
        const float arcToPoint = DeltaAngleAlongOrientation(_CounterClockwise, _StartAngle, _PointAngle);
        return (arcToPoint <= arcToEnd + _AngleTolerance);
    }

    float ComputeLineSeamBlend(float _SeamT, bool _HasValue, bool _IsTransition,
                              const Vec2& _SeamStart, const Vec2& _SeamEnd, const Vec2& _ContactPoint,
                              float _MoveVelocityX)
    {
        if (!_HasValue || !_IsTransition)
            return 0.f;

        float blend = Clamp01f(_SeamT);
        if (_SeamEnd.x != _SeamStart.x || _SeamEnd.y != _SeamStart.y)
        {
            const float dx = _SeamEnd.x - _SeamStart.x;
            const float dy = _SeamEnd.y - _SeamStart.y;
            if (fabsf(dx) >= fabsf(dy))
            {
                if (fabsf(dx) > 0.0001f)
                    blend = Clamp01f((_ContactPoint.x - _SeamStart.x) / dx);
            }
            else if (fabsf(dy) > 0.0001f)
            {
                blend = Clamp01f((_ContactPoint.y - _SeamStart.y) / dy);
            }
        }

        if (_MoveVelocityX > 0.f)
            return blend;

        if (_MoveVelocityX < 0.f)
            return 1.f - blend;

        return 0.f;
    }

    float ComputeSeamDirectionBias(float _SeamT, bool _HasValue, bool _IsTransition, float _MoveVelocityX)
    {
        if (!_HasValue || !_IsTransition)
            return 0.f;

        if (_MoveVelocityX > 1.f)
            return Clamp01f(1.f - Clamp01f(_SeamT));

        if (_MoveVelocityX < -1.f)
            return Clamp01f(_SeamT);

        return 0.f;
    }

    float DotVec2(const Vec2& _A, const Vec2& _B)
    {
        return _A.x * _B.x + _A.y * _B.y;
    }

    float LengthVec2(const Vec2& _V)
    {
        return sqrtf(_V.x * _V.x + _V.y * _V.y);
    }

    void CanonicalizeLineEndpoints(Vec2& _A, Vec2& _B)
    {
        const Vec2 delta = _B - _A;
        if (fabsf(delta.x) >= fabsf(delta.y))
        {
            if (_A.x > _B.x)
                std::swap(_A, _B);
        }
        else
        {
            if (_A.y > _B.y)
                std::swap(_A, _B);
        }
    }

    float CrossVec2(const Vec2& _A, const Vec2& _B)
    {
        return _A.x * _B.y - _A.y * _B.x;
    }

    Vec2 NormalizeSafeVec2(const Vec2& _V, const Vec2& _Fallback = Vec2(0.f, 1.f))
    {
        float len = LengthVec2(_V);
        if (len <= 0.0001f)
            return _Fallback;

        return Vec2(_V.x / len, _V.y / len);
    }

    bool GetColliderSupportDataWorld(CCollider2D* _Collider, const Vec2& _SurfaceNormal,
                                     bool _AlignToSurfaceNormal,
                                     Vec2& _OutCenter, Vec2& _OutSupportPoint, float& _OutSupportDistance)
    {
        if (_Collider == nullptr || _Collider->GetOwner() == nullptr)
            return false;

        GameObject* pOwner = _Collider->GetOwner();
        Vec3 ownerPos = pOwner->Transform()->GetRelativePos();
        Vec3 ownerRot = pOwner->Transform()->GetRelativeRot();
        Vec3 ownerScale = pOwner->Transform()->GetRelativeScale();
        Vec2 colliderOffset = _Collider->GetOffset();
        Vec2 colliderScale = _Collider->GetScale();

        Vec2 n = _SurfaceNormal;
        const float normalLen = sqrtf(n.x * n.x + n.y * n.y);
        if (normalLen <= 0.0001f)
            n = Vec2(0.f, 1.f);
        else
            n /= normalLen;

        if (_AlignToSurfaceNormal)
            ownerRot.z = atan2f(n.y, n.x) + XM_PIDIV2 + XM_PI;

        XMMATRIX matOwner =
            XMMatrixScaling(ownerScale.x, ownerScale.y, ownerScale.z) *
            XMMatrixRotationX(ownerRot.x) *
            XMMatrixRotationY(ownerRot.y) *
            XMMatrixRotationZ(ownerRot.z) *
            XMMatrixTranslation(ownerPos.x, ownerPos.y, ownerPos.z);

        XMMATRIX matWorld =
            XMMatrixScaling(colliderScale.x, colliderScale.y, 1.f) *
            XMMatrixTranslation(colliderOffset.x, colliderOffset.y, 0.f) *
            matOwner;

        const Vec3 center3 = XMVector3TransformCoord(Vec3(0.f, 0.f, 0.f), matWorld);
        const Vec3 halfRight3 = XMVector3TransformNormal(Vec3(0.5f, 0.f, 0.f), matWorld);
        const Vec3 halfUp3 = XMVector3TransformNormal(Vec3(0.f, 0.5f, 0.f), matWorld);

        _OutCenter = Vec2(center3.x, center3.y);

        const Vec2 halfRight = Vec2(halfRight3.x, halfRight3.y);
        const Vec2 halfUp = Vec2(halfUp3.x, halfUp3.y);

        _OutSupportDistance = fabsf(DotVec2(n, halfRight)) + fabsf(DotVec2(n, halfUp));
        _OutSupportPoint = _OutCenter - n * _OutSupportDistance;
        return true;
    }

    bool GetPlayerVisualFootDataWorld(CCollider2D* _Collider, const Vec2& _SurfaceNormal,
                                      Vec2& _OutCenter, Vec2& _OutFootPoint, float& _OutFootDistance)
    {
        if (_Collider == nullptr || _Collider->GetOwner() == nullptr)
            return false;

        Vec2 surfaceNormal = _SurfaceNormal;
        const float normalLen = sqrtf(surfaceNormal.x * surfaceNormal.x + surfaceNormal.y * surfaceNormal.y);
        if (normalLen <= 0.0001f)
            surfaceNormal = Vec2(0.f, 1.f);
        else
            surfaceNormal /= normalLen;

        return GetColliderSupportDataWorld(_Collider,
                                           surfaceNormal,
                                           true,
                                           _OutCenter,
                                           _OutFootPoint,
                                           _OutFootDistance);
    }

    bool IsAutoplayEnabled()
    {
        static const bool enabled = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay"));
        return enabled;
    }

    bool IsSurfaceAutoplayScenario()
    {
        static const bool enabled = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface"));
        return enabled;
    }

    bool IsSurfaceAutoplayLeftScenario()
    {
        static const bool enabled =
            (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_left")) ||
            (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_chord_left")) ||
            (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_loop_left"));
        return enabled;
    }

    bool IsSurfaceAutoplayChordScenario()
    {
        static const bool enabled = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_chord"));
        return enabled;
    }

    bool IsSurfaceAutoplayLoopScenario()
    {
        static const bool enabled = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_loop"));
        return enabled;
    }

    bool IsSurfaceAutoplayJumpScenario()
    {
        static const bool enabled = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_jump"));
        return enabled;
    }

    bool IsSurfaceAutoplayTopJumpScenario()
    {
        static const bool enabled = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_top_jump"));
        return enabled;
    }

    bool IsSurfaceAutoplaySlowFallScenario()
    {
        static const bool enabled = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_slow_fall"));
        return enabled;
    }

    bool IsSurfaceAutoplayLineUnderScenario()
    {
        static const bool enabled = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_line_under"));
        return enabled;
    }

    float GetAutoplayTotalDuration()
    {
        if (IsSurfaceAutoplayTopJumpScenario())
            return kAutoplaySurfaceTopJumpTotalDuration;

        if (IsSurfaceAutoplaySlowFallScenario())
            return kAutoplaySurfaceSlowFallTotalDuration;

        if (IsSurfaceAutoplayLineUnderScenario())
            return kAutoplaySurfaceLineUnderTotalDuration;

        if (IsSurfaceAutoplayLoopScenario())
            return kAutoplaySurfaceLoopTotalDuration;

        if (IsSurfaceAutoplayJumpScenario())
            return kAutoplaySurfaceJumpTotalDuration;

        if (IsSurfaceAutoplayChordScenario())
            return kAutoplaySurfaceChordTotalDuration;

        return IsSurfaceAutoplayScenario() ? kAutoplaySurfaceTotalDuration : kAutoplayDefaultTotalDuration;
    }

    const wchar_t* GetAutoplayPhaseName(float _Time)
    {
        if (IsSurfaceAutoplayScenario())
        {
            if (IsSurfaceAutoplayTopJumpScenario())
            {
                if (_Time < 0.14f)
                    return L"settle";

                if (_Time < 0.24f)
                    return L"top_jump";

                if (_Time < GetAutoplayTotalDuration())
                    return L"cooldown";

                return L"finish";
            }

            if (IsSurfaceAutoplaySlowFallScenario())
            {
                if (_Time < 0.35f)
                    return L"settle";

                if (_Time < 0.85f)
                    return L"slow_circle_fall";

                if (_Time < GetAutoplayTotalDuration())
                    return L"cooldown";

                return L"finish";
            }

            if (IsSurfaceAutoplayLineUnderScenario())
            {
                if (_Time < 0.35f)
                    return L"settle";

                if (_Time < 0.85f)
                    return L"line_under_fall";

                if (_Time < GetAutoplayTotalDuration())
                    return L"cooldown";

                return L"finish";
            }

            if (IsSurfaceAutoplayLoopScenario())
            {
                if (_Time < 0.20f)
                    return L"settle";

                if (_Time < GetAutoplayTotalDuration())
                    return L"surface_circle_loop";

                return L"finish";
            }

            if (IsSurfaceAutoplayJumpScenario())
            {
                if (_Time < 0.35f)
                    return L"settle";

                if (_Time < 0.62f)
                    return IsSurfaceAutoplayLeftScenario() ? L"surface_run_left" : L"surface_run_right";

                if (_Time < 0.72f)
                    return L"jump";

                if (_Time < GetAutoplayTotalDuration())
                    return L"cooldown";

                return L"finish";
            }

            if (_Time < 0.35f)
                return L"settle";

            if (_Time < 8.30f)
                return IsSurfaceAutoplayLeftScenario() ? L"surface_run_left" : L"surface_run_right";

            if (_Time < GetAutoplayTotalDuration())
                return L"cooldown";

            return L"finish";
        }

        if (_Time < 0.35f)
            return L"settle";

        if (_Time < 7.35f)
            return L"run_right";

        if (_Time < 8.00f)
            return L"pause";

        if (_Time < 15.00f)
            return L"run_left";

        if (_Time < GetAutoplayTotalDuration())
            return L"cooldown";

        return L"finish";
    }

    wstring GetAutoplayLogPath()
    {
        wchar_t modulePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, modulePath, MAX_PATH);

        wstring logPath = modulePath;
        const size_t slashPos = logPath.find_last_of(L"\\/");
        if (slashPos != wstring::npos)
            logPath.erase(slashPos + 1);

        if (IsSurfaceAutoplayTopJumpScenario())
            logPath += L"autoplay_surface_top_jump_report.txt";
        else if (IsSurfaceAutoplaySlowFallScenario())
            logPath += L"autoplay_surface_slow_fall_report.txt";
        else if (IsSurfaceAutoplayLineUnderScenario())
            logPath += L"autoplay_surface_line_under_report.txt";
        else if (IsSurfaceAutoplayLoopScenario() && IsSurfaceAutoplayLeftScenario())
            logPath += L"autoplay_surface_loop_left_report.txt";
        else if (IsSurfaceAutoplayLoopScenario())
            logPath += L"autoplay_surface_loop_report.txt";
        else if (IsSurfaceAutoplayJumpScenario())
            logPath += L"autoplay_surface_jump_report.txt";
        else if (IsSurfaceAutoplayChordScenario() && IsSurfaceAutoplayLeftScenario())
            logPath += L"autoplay_surface_chord_left_report.txt";
        else if (IsSurfaceAutoplayChordScenario())
            logPath += L"autoplay_surface_chord_report.txt";
        else if (IsSurfaceAutoplayLeftScenario())
            logPath += L"autoplay_surface_left_report.txt";
        else if (IsSurfaceAutoplayScenario())
            logPath += L"autoplay_surface_suite_report.txt";
        else
            logPath += L"autoplay_surface_report.txt";
        return logPath;
    }

    void AppendSurfaceAutoplayDebug(const wchar_t* _Format, ...)
    {
        if (!IsSurfaceAutoplayScenario())
            return;

        wchar_t modulePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, modulePath, MAX_PATH);

        wstring logPath = modulePath;
        const size_t slashPos = logPath.find_last_of(L"\\/");
        if (slashPos != wstring::npos)
            logPath.erase(slashPos + 1);
        logPath += L"autoplay_surface_debug.txt";

        FILE* pTrace = nullptr;
        _wfopen_s(&pTrace, logPath.c_str(), L"a, ccs=UTF-8");
        if (nullptr == pTrace)
            return;

        va_list args;
        va_start(args, _Format);
        vfwprintf(pTrace, _Format, args);
        va_end(args);
        fwprintf(pTrace, L"\n");
        fclose(pTrace);
    }
}

CPlayerScript::CPlayerScript()
    : CScript(SCRIPT_TYPE::PLAYERSCRIPT)
{
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_MaxMoveSpeed, L"Max Speed", false, 10.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_JumpForce, L"Jump Force", false, 10.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_SkillDashSpeed, L"Skill Dash Speed", false, 10.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_PushSpeed, L"Push Speed", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_AirAccelScale, L"Air Accel Scale", false, 0.05f);
    AddScriptParam(SCRIPT_PARAM::VEC2, &vAccel, L"Acceleration", false, 10.f);
    AddScriptParam(SCRIPT_PARAM::VEC2, &vFraction, L"Friction", false, 0.1f);

    // 네 기존 초기값 스타일 유지
    m_Limit = 0.f;
    
    vAccel = Vec2(500.f, 1000.f);
    vVelocity = Vec2(30.f, 0.f);     // 원래 30이었으면 유지 (원치 않으면 0으로)
    vFraction = Vec2(5.f, 10.f);

    IsGround = false;
    bIsSpringJump = false;

    m_Pose = PoseState::None;
    m_Action = ActionState::None;

    m_IdleTime = 0.f;
    m_PoseTime = 0.f;
    m_ActionTime = 0.f;

    m_Facing = 1;
    m_TurnTargetFacing = 1;
}

CPlayerScript::CPlayerScript(const CPlayerScript& _Origin)
    : CScript(_Origin)
    , m_Target(_Origin.m_Target)
    , m_Limit(_Origin.m_Limit)
    , gravity(_Origin.gravity)
    , vFraction(_Origin.vFraction)
    , vAccel(_Origin.vAccel)
    , vVelocity(_Origin.vVelocity)
    , vDir(_Origin.vDir)
    , vNormal(_Origin.vNormal)
    , vTangent(_Origin.vTangent)
    , m_MaxMoveSpeed(_Origin.m_MaxMoveSpeed)
    , m_JumpForce(_Origin.m_JumpForce)
    , m_SkillDashSpeed(_Origin.m_SkillDashSpeed)
    , m_PushSpeed(_Origin.m_PushSpeed)
    , m_AirAccelScale(_Origin.m_AirAccelScale)
    , IsJump(_Origin.IsJump)
    , m_bNeedGravity(_Origin.m_bNeedGravity)
    , bIsBreak(_Origin.bIsBreak)
    , bIsSpringJump(_Origin.bIsSpringJump)
    , bIsSpringDash(_Origin.bIsSpringDash)
    , m_Pose(_Origin.m_Pose)
    , m_Action(_Origin.m_Action)
    , m_bBackReaction(_Origin.m_bBackReaction)
    , m_IdleTime(_Origin.m_IdleTime)
    , m_PoseTime(_Origin.m_PoseTime)
    , m_ActionTime(_Origin.m_ActionTime)
    , m_BreakUngroundedTime(_Origin.m_BreakUngroundedTime)
    , m_KnockBackInvincibleTime(_Origin.m_KnockBackInvincibleTime)
    , m_Facing(_Origin.m_Facing)
    , m_TurnTargetFacing(_Origin.m_TurnTargetFacing)
    , IsGround(_Origin.IsGround)
    , m_PhysicalGroundOverlapCount(_Origin.m_PhysicalGroundOverlapCount)
    , m_AttachableSurfaceOverlapCount(_Origin.m_AttachableSurfaceOverlapCount)
    , m_bHasNearbyAttachableSurface(_Origin.m_bHasNearbyAttachableSurface)
    , m_bPushContact(_Origin.m_bPushContact)
    , m_PushContactDir(_Origin.m_PushContactDir)
    , m_bPushing(_Origin.m_bPushing)
    , m_PushingDir(_Origin.m_PushingDir)
    , m_vecWallPushContacts(_Origin.m_vecWallPushContacts)
    , m_fBreakSpeed(_Origin.m_fBreakSpeed)
    , m_iBreakDirection(_Origin.m_iBreakDirection)
    , m_bBreakWallLocked(_Origin.m_bBreakWallLocked)
    , m_LastTickPosX(_Origin.m_LastTickPosX)
    , m_LastFrameDeltaX(_Origin.m_LastFrameDeltaX)
    , m_vecAttachBlockedSurfaces(_Origin.m_vecAttachBlockedSurfaces)
    , m_ForcedFlatGroundLockTime(_Origin.m_ForcedFlatGroundLockTime)
    , m_SurfaceGroundHoldTime(_Origin.m_SurfaceGroundHoldTime)
    , m_SurfaceResolveFrame(_Origin.m_SurfaceResolveFrame)
    , m_SurfaceResolveScore(_Origin.m_SurfaceResolveScore)
    , m_LastSceneSurfaceProbeFrame(_Origin.m_LastSceneSurfaceProbeFrame)
    , m_pResolvedSurface(_Origin.m_pResolvedSurface)
    , m_bInwardCircleLoopTracked(_Origin.m_bInwardCircleLoopTracked)
    , m_bInwardCirclePassedLowerHalf(_Origin.m_bInwardCirclePassedLowerHalf)
    , m_InwardCircleLoopCenter(_Origin.m_InwardCircleLoopCenter)
    , m_InwardCircleLoopRadius(_Origin.m_InwardCircleLoopRadius)
    , m_InwardCircleLoopAccumulatedAngle(_Origin.m_InwardCircleLoopAccumulatedAngle)
    , m_InwardCircleLoopLastAngle(_Origin.m_InwardCircleLoopLastAngle)
    , m_InwardCircleAttachBlockTime(_Origin.m_InwardCircleAttachBlockTime)
    , m_InwardCircleAttachBlockCenter(_Origin.m_InwardCircleAttachBlockCenter)
    , m_InwardCircleAttachBlockRadius(_Origin.m_InwardCircleAttachBlockRadius)
    , m_bInwardCircleHalfCheckerTracked(_Origin.m_bInwardCircleHalfCheckerTracked)
    , m_InwardCircleHalfCheckerCenter(_Origin.m_InwardCircleHalfCheckerCenter)
    , m_InwardCircleHalfCheckerRadius(_Origin.m_InwardCircleHalfCheckerRadius)
    , m_bInwardCircleLineOwnsTopRight(_Origin.m_bInwardCircleLineOwnsTopRight)
    , m_bGuidedInwardCircleEntryTracked(_Origin.m_bGuidedInwardCircleEntryTracked)
    , m_GuidedInwardCircleEntryCenter(_Origin.m_GuidedInwardCircleEntryCenter)
    , m_GuidedInwardCircleEntryRadius(_Origin.m_GuidedInwardCircleEntryRadius)
    , m_bGuidedInwardCircleEntryFromRight(_Origin.m_bGuidedInwardCircleEntryFromRight)
    , m_bGuidedInwardCircleLineOwnsRight(_Origin.m_bGuidedInwardCircleLineOwnsRight)
    , m_bGuidedInwardCirclePassedHalf(_Origin.m_bGuidedInwardCirclePassedHalf)
    , m_bGuidedInwardCircleHalfZoneTracked(_Origin.m_bGuidedInwardCircleHalfZoneTracked)
    , m_GuidedInwardCircleHalfZoneSign(_Origin.m_GuidedInwardCircleHalfZoneSign)
    , m_bGuidedInwardCircleHalfDeltaTracked(_Origin.m_bGuidedInwardCircleHalfDeltaTracked)
    , m_GuidedInwardCircleLastHalfDelta(_Origin.m_GuidedInwardCircleLastHalfDelta)
    , m_bHasRecentReleasedInwardCircleContact(_Origin.m_bHasRecentReleasedInwardCircleContact)
    , m_RecentReleasedInwardCircleContact(_Origin.m_RecentReleasedInwardCircleContact)
    , m_RecentReleasedInwardCircleTime(_Origin.m_RecentReleasedInwardCircleTime)
    , m_RecentGuidedCorrectionLineTime(_Origin.m_RecentGuidedCorrectionLineTime)
    , m_RecentGuidedCorrectionLineName(_Origin.m_RecentGuidedCorrectionLineName)
    , m_SurfaceRotationSnapTime(_Origin.m_SurfaceRotationSnapTime)
    , m_bHasPendingSurfaceContact(_Origin.m_bHasPendingSurfaceContact)
    , m_PendingSurfaceContact(_Origin.m_PendingSurfaceContact)
    , m_bHasCurrentSurfaceContact(_Origin.m_bHasCurrentSurfaceContact)
    , m_CurrentSurfaceContact(_Origin.m_CurrentSurfaceContact)
    , m_vecActiveSurfaceObjects(_Origin.m_vecActiveSurfaceObjects)
    , m_bAutoplayInitialized(_Origin.m_bAutoplayInitialized)
    , m_bAutoplayFinished(_Origin.m_bAutoplayFinished)
    , m_bAutoplayWasGround(_Origin.m_bAutoplayWasGround)
    , m_AutoplayTime(_Origin.m_AutoplayTime)
    , m_AutoplaySampleAccum(_Origin.m_AutoplaySampleAccum)
    , m_AutoplayAirTime(_Origin.m_AutoplayAirTime)
    , m_AutoplayLongestAirTime(_Origin.m_AutoplayLongestAirTime)
    , m_AutoplayMaxX(_Origin.m_AutoplayMaxX)
    , m_AutoplayMinY(_Origin.m_AutoplayMinY)
    , m_AutoplayMaxLineContactError(_Origin.m_AutoplayMaxLineContactError)
    , m_AutoplayMaxCircleContactError(_Origin.m_AutoplayMaxCircleContactError)
    , m_AutoplayCircleGroundTime(_Origin.m_AutoplayCircleGroundTime)
    , m_AutoplayGroundLossCount(_Origin.m_AutoplayGroundLossCount)
    , m_AutoplayInwardCircleReleaseCount(_Origin.m_AutoplayInwardCircleReleaseCount)
    , m_AutoplayJumpStartCount(_Origin.m_AutoplayJumpStartCount)
    , m_AutoplayJumpDetachCount(_Origin.m_AutoplayJumpDetachCount)
    , m_bAutoplayTouchedCircle(_Origin.m_bAutoplayTouchedCircle)
    , m_bAutoplayPendingJumpDetach(_Origin.m_bAutoplayPendingJumpDetach)
    , m_AutoplayInput(_Origin.m_AutoplayInput)
    , m_pAutoplayLog(nullptr)
{
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_MaxMoveSpeed, L"Max Speed", false, 10.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_JumpForce, L"Jump Force", false, 10.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_SkillDashSpeed, L"Skill Dash Speed", false, 10.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_PushSpeed, L"Push Speed", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_AirAccelScale, L"Air Accel Scale", false, 0.05f);
    AddScriptParam(SCRIPT_PARAM::VEC2, &vAccel, L"Acceleration", false, 10.f);
    AddScriptParam(SCRIPT_PARAM::VEC2, &vFraction, L"Friction", false, 0.1f);
}

void CPlayerScript::Begin()
{
    Collider2D()->AddDynamicBeginOverlap(this, (COLLISION_EVENT)&CPlayerScript::BeginOverlap);
    Collider2D()->AddDynamicOverlap(this, (COLLISION_EVENT)&CPlayerScript::Overlap);
    Collider2D()->AddDynamicEndOverlap(this, (COLLISION_EVENT)&CPlayerScript::EndOverlap);

    FlipbookRender()->SetFlipbook(kPushingFlipbookIndex, LOAD(AFlipbook, L"Flipbook\\Sonic_Pushing.flip"));
    FlipbookRender()->SetFlipbook(kKnockBackFlipbookIndex, LOAD(AFlipbook, L"Flipbook\\Sonic_Hurt.flip"));
    m_LastTickPosX = Transform()->GetRelativePos().x;
    m_LastFrameDeltaX = 0.f;
}

CPlayerScript::~CPlayerScript()
{
    if (nullptr != m_pAutoplayLog)
    {
        fclose(m_pAutoplayLog);
        m_pAutoplayLog = nullptr;
    }
}

CPlayerScript::PlayerInput CPlayerScript::ReadInput() const
{
    if (IsAutoplayEnabled())
        return m_AutoplayInput;

    PlayerInput in;
    // =======================================================
    // [수정됨] 넉백 상태일 때는 모든 키 입력을 무시하고 빈 입력 반환
    if (m_Action == ActionState::KnockBack)
    {
        return in; // 모든 멤버가 false인 상태로 반환됨
    }
    // =======================================================

    in.upHeld = KEY_PRESSED(KEY::UP);
    in.downHeld = KEY_PRESSED(KEY::DOWN);
    in.downReleased = KEY_RELEASED(KEY::DOWN);

    in.leftHeld = KEY_PRESSED(KEY::LEFT);
    in.rightHeld = KEY_PRESSED(KEY::RIGHT);
    in.leftTap = KEY_TAP(KEY::LEFT);
    in.rightTap = KEY_TAP(KEY::RIGHT);

    in.spacePressed = KEY_TAP(KEY::SPACE);
    in.spaceHeld = KEY_PRESSED(KEY::SPACE);
    in.spaceReleased = KEY_RELEASED(KEY::SPACE);

    // TODO: 공격/구르기 키는 네 프로젝트에 맞게 지정
    //in.attackPressed = KEY_TAP(KEY::Z);
    in.rollPressed = KEY_TAP(KEY::DOWN);

    return in;
}

void CPlayerScript::Tick()
{
    float dt = DT;
    if (IsAutoplayEnabled() && dt > (1.f / 60.f))
        dt = (1.f / 60.f);
    if (IsSurfaceAutoplayScenario() && m_AutoplayTime < 0.5f)
        AppendSurfaceAutoplayDebug(L"tick begin t=%.3f dt=%.6f", m_AutoplayTime, dt);

    if (!m_vecWallPushContacts.empty())
    {
        auto iter = m_vecWallPushContacts.begin();
        while (iter != m_vecWallPushContacts.end())
        {
            if (iter->Surface == nullptr || iter->Surface->IsDead() || (iter->Dir != 1 && iter->Dir != -1))
                iter = m_vecWallPushContacts.erase(iter);
            else
                ++iter;
        }
    }

    InitAutoplay();
    UpdateAutoplay(dt);

    if (!m_vecAttachBlockedSurfaces.empty())
    {
        auto iter = m_vecAttachBlockedSurfaces.begin();
        while (iter != m_vecAttachBlockedSurfaces.end())
        {
            iter->Time -= dt;
            if (iter->Time <= 0.f || iter->Surface == nullptr || iter->Surface->IsDead())
                iter = m_vecAttachBlockedSurfaces.erase(iter);
            else
                ++iter;
        }
    }

    if (m_ForcedFlatGroundLockTime > 0.f)
    {
        m_ForcedFlatGroundLockTime -= dt;
        if (m_ForcedFlatGroundLockTime < 0.f)
            m_ForcedFlatGroundLockTime = 0.f;
    }

    if (m_InwardCircleAttachBlockTime > 0.f)
    {
        m_InwardCircleAttachBlockTime -= dt;
        if (m_InwardCircleAttachBlockTime <= 0.f)
        {
            m_InwardCircleAttachBlockTime = 0.f;
            m_InwardCircleAttachBlockCenter = Vec2(0.f, 0.f);
            m_InwardCircleAttachBlockRadius = 0.f;
        }
    }

    if (m_RecentGuidedCorrectionLineTime > 0.f)
    {
        m_RecentGuidedCorrectionLineTime -= dt;
        if (m_RecentGuidedCorrectionLineTime <= 0.f)
        {
            m_RecentGuidedCorrectionLineTime = 0.f;
            m_RecentGuidedCorrectionLineName.clear();
        }
    }

    if (m_RecentReleasedInwardCircleTime > 0.f)
    {
        m_RecentReleasedInwardCircleTime -= dt;
        if (m_RecentReleasedInwardCircleTime <= 0.f)
        {
            m_RecentReleasedInwardCircleTime = 0.f;
            m_bHasRecentReleasedInwardCircleContact = false;
            m_RecentReleasedInwardCircleContact = SurfaceContact{};
        }
    }

    if (m_SurfaceRotationSnapTime > 0.f)
    {
        m_SurfaceRotationSnapTime -= dt;
        if (m_SurfaceRotationSnapTime < 0.f)
            m_SurfaceRotationSnapTime = 0.f;
    }

    if (m_SurfaceGroundHoldTime > 0.f)
    {
        m_SurfaceGroundHoldTime -= dt;
        if (m_SurfaceGroundHoldTime < 0.f)
            m_SurfaceGroundHoldTime = 0.f;
    }

    if (m_KnockBackInvincibleTime > 0.f)
    {
        m_KnockBackInvincibleTime -= dt;
        if (m_KnockBackInvincibleTime < 0.f)
            m_KnockBackInvincibleTime = 0.f;
    }

    if (FlipbookRender() != nullptr)
    {
        bool bVisible = true;

        if (m_KnockBackInvincibleTime > 0.f)
        {
            const float blinkElapsed = kKnockBackInvincibleDuration - m_KnockBackInvincibleTime;
            const float blinkCycle = kKnockBackBlinkInterval * 2.f;
            const float blinkPhase = fmodf(blinkElapsed, blinkCycle);
            bVisible = (blinkPhase < kKnockBackBlinkInterval);
        }

        FlipbookRender()->SetVisible(bVisible);
    }

    ResolveBufferedSurfaceContacts();
    if (IsSurfaceAutoplayScenario() && m_AutoplayTime < 0.5f)
        AppendSurfaceAutoplayDebug(L"tick after_resolve1 t=%.3f ground=%d", m_AutoplayTime, IsGround ? 1 : 0);

    if (IsGround && !HasAnyGroundOverlap() && m_SurfaceGroundHoldTime <= 0.f && !m_bHasCurrentSurfaceContact)
        ResetGroundContact();

    const float curPosX = Transform()->GetRelativePos().x;
    m_LastFrameDeltaX = curPosX - m_LastTickPosX;
    m_LastTickPosX = curPosX;

    const PlayerInput in = ReadInput();

    // 1) 상태 전이(결정)
    ResolveTransitions(in, dt);

    // 2) 물리/이동(실행)
    Simulate(in, dt);
    if (IsSurfaceAutoplayScenario() && m_AutoplayTime < 0.5f)
        AppendSurfaceAutoplayDebug(L"tick after_simulate t=%.3f pos=(%.2f,%.2f) vel=(%.2f,%.2f)",
                                   m_AutoplayTime, Transform()->GetRelativePos().x, Transform()->GetRelativePos().y, vVelocity.x, vVelocity.y);

    // 이동 직후 현재 위치 기준으로 다시 surface를 잡아, 새 평지/경사 전환이
    // 다음 프레임까지 밀리지 않도록 한다.
    ResolveBufferedSurfaceContacts();
    if (IsSurfaceAutoplayScenario() && m_AutoplayTime < 0.5f)
        AppendSurfaceAutoplayDebug(L"tick after_resolve2 t=%.3f ground=%d", m_AutoplayTime, IsGround ? 1 : 0);

    // 3) 회전 상태 에니메이션 갱신
    UpdateGroundRotation();
    if (IsSurfaceAutoplayScenario() && m_AutoplayTime < 0.5f)
        AppendSurfaceAutoplayDebug(L"tick after_rotation t=%.3f rot=%.3f", m_AutoplayTime, Transform()->GetRelativeRot().z);

    // 4) 타이머 갱신 (속도/상태 기반)
    UpdateTimers(dt);

    // 5) 애니메이션(표현) - 한 곳에서 우선순위로만 결정
    UpdateAnimation(dt);
    DrawGuidedInwardCircleDebug();
    if (IsSurfaceAutoplayScenario() && m_AutoplayTime < 0.5f)
        AppendSurfaceAutoplayDebug(L"tick end t=%.3f action=%d", m_AutoplayTime, (int)m_Action);

    // 디버그는 네 코드처럼 필요하면 여기서
    if (in.spacePressed)
    {
        DrawDebugRect(Transform()->GetWorldMat(), Vec4(1.f, 0.f, 0.f, 1.f), 2.f);
    }

    if (IsAutoplayEnabled() && m_bAutoplayInitialized && !m_bAutoplayFinished)
    {
        const Vec3 pos = Transform()->GetRelativePos();
        if (pos.x > m_AutoplayMaxX)
            m_AutoplayMaxX = pos.x;
        if (pos.y < m_AutoplayMinY)
            m_AutoplayMinY = pos.y;

        if (!IsGround)
        {
            m_AutoplayAirTime += dt;
            if (m_bAutoplayWasGround && m_AutoplayTime > 0.75f)
            {
                ++m_AutoplayGroundLossCount;
                LogAutoplayState(L"ground_lost");
            }
        }
        else
        {
            if (!m_bAutoplayWasGround && m_AutoplayAirTime > 0.f)
                LogAutoplayState(L"ground_regained");

            if (m_AutoplayAirTime > m_AutoplayLongestAirTime)
                m_AutoplayLongestAirTime = m_AutoplayAirTime;

            m_AutoplayAirTime = 0.f;
        }

        m_bAutoplayWasGround = IsGround;
        m_AutoplaySampleAccum += dt;
        if (m_AutoplaySampleAccum >= kAutoplaySampleInterval)
        {
            m_AutoplaySampleAccum = 0.f;
            LogAutoplayState(L"sample");
        }

        if (m_bAutoplayPendingJumpDetach && !IsGround)
        {
            ++m_AutoplayJumpDetachCount;
            m_bAutoplayPendingJumpDetach = false;
            LogAutoplayState(L"jump_detach");
        }

        if (m_bHasCurrentSurfaceContact && m_CurrentSurfaceContact.Surface != nullptr)
        {
            const wstring surfaceName = m_CurrentSurfaceContact.Surface->GetName();
            const float contactError = fabsf(m_CurrentSurfaceContact.SignedDistance);

            if (surfaceName.find(L"SurfaceLine") == 0)
            {
                if (contactError > m_AutoplayMaxLineContactError)
                    m_AutoplayMaxLineContactError = contactError;
            }
            else if (surfaceName.find(L"SurfaceCircle") == 0)
            {
                if (contactError > m_AutoplayMaxCircleContactError)
                    m_AutoplayMaxCircleContactError = contactError;

                if (IsGround)
                {
                    m_bAutoplayTouchedCircle = true;
                    m_AutoplayCircleGroundTime += dt;
                }
            }
        }

        if (m_AutoplayTime >= GetAutoplayTotalDuration())
            FinishAutoplay();
    }
}

void CPlayerScript::InitAutoplay()
{
    if (!IsAutoplayEnabled() || m_bAutoplayInitialized)
        return;

    m_bAutoplayInitialized = true;
    m_bAutoplayWasGround = IsGround;
    m_AutoplayMaxLineContactError = 0.f;
    m_AutoplayMaxCircleContactError = 0.f;
    m_AutoplayCircleGroundTime = 0.f;
    m_AutoplayInwardCircleReleaseCount = 0;
    m_AutoplayJumpStartCount = 0;
    m_AutoplayJumpDetachCount = 0;
    m_bAutoplayPendingJumpDetach = false;
    m_bAutoplayTouchedCircle = false;

    const Vec3 pos = Transform()->GetRelativePos();
    m_AutoplayMaxX = pos.x;
    m_AutoplayMinY = pos.y;

    const wstring logPath = GetAutoplayLogPath();
    _wfopen_s(&m_pAutoplayLog, logPath.c_str(), L"w, ccs=UTF-8");
    if (nullptr != m_pAutoplayLog)
    {
        fwprintf(m_pAutoplayLog, L"autoplay surface report\n");
        fwprintf(m_pAutoplayLog, L"log_path=%ls\n", logPath.c_str());
    }

    LogAutoplayState(L"begin");
}

void CPlayerScript::UpdateAutoplay(float dt)
{
    if (!IsAutoplayEnabled() || !m_bAutoplayInitialized || m_bAutoplayFinished)
        return;

    m_AutoplayTime += dt;
    m_AutoplayInput = {};

    if (IsSurfaceAutoplayScenario())
    {
        if (IsSurfaceAutoplayTopJumpScenario())
        {
            if (m_AutoplayTime < 0.14f)
                return;

            if (m_AutoplayTime < 0.24f)
            {
                m_AutoplayInput.spaceHeld = true;
                m_AutoplayInput.spacePressed = true;
                return;
            }

            return;
        }

        if (m_AutoplayTime < 0.35f)
            return;

        if (IsSurfaceAutoplaySlowFallScenario() || IsSurfaceAutoplayLineUnderScenario())
            return;

        if (IsSurfaceAutoplayLoopScenario())
        {
            if (m_AutoplayTime < 0.20f)
                return;

            if (IsSurfaceAutoplayLeftScenario())
                m_AutoplayInput.leftHeld = true;
            else
                m_AutoplayInput.rightHeld = true;
            return;
        }

        if (IsSurfaceAutoplayJumpScenario())
        {
            if (m_AutoplayTime < 0.62f)
            {
                if (IsSurfaceAutoplayLeftScenario())
                    m_AutoplayInput.leftHeld = true;
                else
                    m_AutoplayInput.rightHeld = true;
                return;
            }

            if (m_AutoplayTime < 0.72f)
            {
                if (IsSurfaceAutoplayLeftScenario())
                    m_AutoplayInput.leftHeld = true;
                else
                    m_AutoplayInput.rightHeld = true;
                m_AutoplayInput.spaceHeld = true;
                m_AutoplayInput.spacePressed = true;
                return;
            }

            return;
        }

        if (m_AutoplayTime < 8.30f)
        {
            if (IsSurfaceAutoplayLeftScenario())
                m_AutoplayInput.leftHeld = true;
            else
                m_AutoplayInput.rightHeld = true;
            return;
        }

        return;
    }

    if (m_AutoplayTime < 0.35f)
        return;

    if (m_AutoplayTime < 7.35f)
    {
        m_AutoplayInput.rightHeld = true;
        return;
    }

    if (m_AutoplayTime < 8.00f)
        return;

    if (m_AutoplayTime < 15.00f)
    {
        m_AutoplayInput.leftHeld = true;
        return;
    }
}

void CPlayerScript::LogAutoplayState(const wchar_t* _Tag)
{
    if (nullptr == m_pAutoplayLog)
        return;

    const Vec3 pos = Transform()->GetRelativePos();
    const wchar_t* phase = GetAutoplayPhaseName(m_AutoplayTime);
    const wstring surfaceName = (m_bHasCurrentSurfaceContact && m_CurrentSurfaceContact.Surface)
        ? m_CurrentSurfaceContact.Surface->GetName()
        : L"-";
    const float contactError = m_bHasCurrentSurfaceContact ? m_CurrentSurfaceContact.SignedDistance : 0.f;

    fwprintf(m_pAutoplayLog,
             L"tag=%ls time=%.3f phase=%ls pos=(%.2f,%.2f) vel=(%.2f,%.2f) ground=%d normal=(%.3f,%.3f) tangent=(%.3f,%.3f) action=%d surface=%ls contact_error=%.3f\n",
             _Tag, m_AutoplayTime, phase, pos.x, pos.y, vVelocity.x, vVelocity.y, IsGround ? 1 : 0,
             vNormal.x, vNormal.y, vTangent.x, vTangent.y, static_cast<int>(m_Action), surfaceName.c_str(), contactError);
    fflush(m_pAutoplayLog);
}

void CPlayerScript::FinishAutoplay()
{
    if (!IsAutoplayEnabled() || m_bAutoplayFinished)
        return;

    m_bAutoplayFinished = true;

    if (m_AutoplayAirTime > m_AutoplayLongestAirTime)
        m_AutoplayLongestAirTime = m_AutoplayAirTime;

    LogAutoplayState(L"finish");

    if (nullptr != m_pAutoplayLog)
    {
        if (IsSurfaceAutoplayScenario())
        {
            fwprintf(m_pAutoplayLog,
                     L"summary max_x=%.2f min_y=%.2f ground_loss_count=%d longest_air=%.3f line_error_max=%.3f circle_error_max=%.3f circle_ground_time=%.3f touched_circle=%d circle_release_count=%d jump_start_count=%d jump_detach_count=%d\n",
                     m_AutoplayMaxX, m_AutoplayMinY, m_AutoplayGroundLossCount, m_AutoplayLongestAirTime,
                     m_AutoplayMaxLineContactError, m_AutoplayMaxCircleContactError,
                     m_AutoplayCircleGroundTime, m_bAutoplayTouchedCircle ? 1 : 0,
                     m_AutoplayInwardCircleReleaseCount, m_AutoplayJumpStartCount, m_AutoplayJumpDetachCount);
        }
        else
        {
            fwprintf(m_pAutoplayLog, L"summary max_x=%.2f min_y=%.2f ground_loss_count=%d longest_air=%.3f\n",
                     m_AutoplayMaxX, m_AutoplayMinY, m_AutoplayGroundLossCount, m_AutoplayLongestAirTime);
        }
        fclose(m_pAutoplayLog);
        m_pAutoplayLog = nullptr;
    }

    PostQuitMessage(0);
}

void CPlayerScript::ResolveTransitions(const PlayerInput& in, float dt)
{
    // 먼저 액션 업데이트(차징 중이면 release 감지 등)
    ResolveAction(in, dt);

    const bool wasPushing = (m_Action == ActionState::Pushing);
    const bool wantsLeft = in.leftHeld && !in.rightHeld;
    const bool wantsRight = in.rightHeld && !in.leftHeld;
    const int inputDir = wantsRight ? 1 : (wantsLeft ? -1 : 0);
    const bool canPush = IsGround && inputDir != 0 && HasPushContactDir(inputDir);
    const Vec2 preferredGroundTangent = vTangent;
    const bool suppressGuidedTransferBreak =
        IsGround &&
        inputDir != 0 &&
        (m_RecentGuidedCorrectionLineTime > 0.f || m_RecentReleasedInwardCircleTime > 0.f);

    if (canPush)
    {
        m_Action = ActionState::Pushing;
        m_bPushing = true;
        m_PushingDir = inputDir;
        m_Facing = inputDir;
        m_Pose = PoseState::None;
    }
    else if (m_Action == ActionState::Pushing)
    {
        m_Action = ActionState::None;
        m_bPushing = false;
        m_PushingDir = 0;
    }

    if (m_Action == ActionState::Break)
    {
        if (suppressGuidedTransferBreak)
        {
            m_Action = ActionState::None;
            m_bBreakWallLocked = false;
            m_fBreakSpeed = 0.f;
            m_BreakUngroundedTime = 0.f;
            m_TurnTargetFacing = inputDir;
            m_Facing = inputDir;
        }
        else if (m_bBreakWallLocked &&
            ((m_iBreakDirection == 1 && wantsLeft) || (m_iBreakDirection == -1 && wantsRight)))
        {
            // 벽에서 정지된 브레이크 상태에서 반대 방향 입력이 오면
            // 즉시 방향을 바꿔서 벽 밀림을 빠져나갈 수 있도록 처리.
            m_bBreakWallLocked = false;
            m_iBreakDirection = -m_iBreakDirection;
            m_TurnTargetFacing = m_iBreakDirection;
            m_BreakUngroundedTime = 0.f;
        }

        if (IsGround)
        {
            m_BreakUngroundedTime = 0.f;
        }
        else
        {
            m_BreakUngroundedTime += dt;

            // 실제 점프/낙하가 확실하면 즉시 해제,
            // 경사 보정 중 지면 판정 끊김은 짧게 허용.
            const bool hardAirborne = IsJump || fabsf(vVelocity.y) > 120.f;
            if ((hardAirborne || m_BreakUngroundedTime > 0.08f) && !m_bBreakWallLocked)
            {
                m_Action = ActionState::None;
                m_BreakUngroundedTime = 0.f;
            }
        }
    }
    else
    {
        m_BreakUngroundedTime = 0.f;
    }

    if ((m_Action == ActionState::Roll) && IsGround && !m_bBackReaction)
    {
        const float blockedMove = fabsf(m_LastFrameDeltaX);
        const float currentSpeed = fabsf(vVelocity.Dot(preferredGroundTangent));
        if (blockedMove < 0.5f && currentSpeed > 80.f)
        {
            StopBlockedAction();
            return;
        }
    }

    // 액션 중이면 자세는 강제 해제(원하는 규칙)
    if (m_Action != ActionState::None
        && m_Action != ActionState::SkillDash
        && m_Action != ActionState::Roll
        && m_Action != ActionState::Pushing)
    {
        m_Pose = PoseState::None;
        return;
    }

    // 액션이 없을 때만 자세(위/아래 보기) 가능
    ResolvePose(in, dt);

    // 점프 시작 조건
    if (CanStartJump(in))
    {
        StartJump();
        return;
    }

    if (canPush)
        return;

    // 스킬 차징 시작 조건:
    // - 지상
    // - 아래보기 자세
    // - 스페이스 누르고 있음
    if (IsGround && m_Pose == PoseState::LookDown && in.spaceHeld)
    {
        m_Action = ActionState::SkillCharge;
        m_ActionTime = 0.f;
        return;
    }

    // TODO: Attack/Roll 시작도 여기서 (액션 시작하면 Pose는 해제)
    // if (IsGround && in.attackPressed) { m_Action = ActionState::Attack; m_ActionTime = 0.f; m_Pose = PoseState::None; }
    const float moveSpeed = IsGround ? vVelocity.Dot(preferredGroundTangent) : vVelocity.x;
    const float absXSpeed = fabsf(vVelocity.x);
    const float absMoveSpeed = fabsf(moveSpeed);
    const float speedForSign =
        (absMoveSpeed >= absXSpeed * 0.25f) ? moveSpeed : vVelocity.x;
    const float absSpeed = fabsf(speedForSign);
    float normalSpeed = 0.f;

    if (IsGround)
        normalSpeed = fabsf(vVelocity.Dot(vNormal));
    else
        normalSpeed = fabsf(vVelocity.y);

    float rollMinSpeed = 150.f;

    if (IsGround && in.rollPressed)
    {
        if (absSpeed >= rollMinSpeed && normalSpeed < 30.f)
        {
            Rolling();
            return;
        }
    }

    bool isOpposite = false;
    // 반대 방향 입력 감지
    if (IsGround)
    {
        isOpposite =
            (speedForSign > 0.f && wantsLeft) ||
            (speedForSign < 0.f && wantsRight);

        if (!isOpposite && fabsf(speedForSign) < 1.f && !suppressGuidedTransferBreak)
            isOpposite = (wantsLeft && m_Facing == 1) || (wantsRight && m_Facing == -1);
    }
    else
    {
        isOpposite =
            (vVelocity.x > 0.f && wantsLeft) ||
            (vVelocity.x < 0.f && wantsRight);
    }

    // 브레이크 모션
    const float breakEntrySpeed = (absSpeed > absXSpeed) ? absSpeed : absXSpeed;

    if (IsGround &&
        m_Action == ActionState::None &&
        !suppressGuidedTransferBreak &&
        isOpposite &&
        breakEntrySpeed > 30.f)
    {
        m_Action = ActionState::Break;
        float breakStartSpeed = IsGround ? fabsf(vVelocity.Dot(preferredGroundTangent)) : fabsf(vVelocity.x);
        if (breakStartSpeed < fabsf(vVelocity.x))
            breakStartSpeed = fabsf(vVelocity.x);

        m_fBreakSpeed = breakStartSpeed;
        m_iBreakDirection = (speedForSign >= 0.f) ? 1 : -1;
        m_bBreakWallLocked = false;
        m_BreakUngroundedTime = 0.f;
        m_TurnTargetFacing = wantsLeft ? -1 : 1;
        m_ActionTime = 0.f;
        return;
    }

    if (wasPushing && m_Action == ActionState::None && inputDir != 0 && absSpeed <= 250.f)
    {
        m_Facing = inputDir;
    }

    // 정지 상태 방향 전환
    if (IsGround && absSpeed < 5.f)
    {
        if (in.rightTap)      m_Facing = 1;
        else if (in.leftTap)  m_Facing = -1;
    }
}

void CPlayerScript::ResolvePose(const PlayerInput& in, float dt)
{
    // ✅ "보면서 달리기" 금지 규칙: 이동 입력이거나 속도가 크면 Pose 불가
    const bool wantsMove = in.leftHeld || in.rightHeld || in.leftTap || in.rightTap;
    const Vec2 preferredGroundTangent = vTangent;

    const float absSpeed = IsGround ? fabsf(vVelocity.Dot(preferredGroundTangent)) : fabsf(vVelocity.x);

    if (!IsGround || wantsMove || absSpeed > 15.f)
    {
        if (m_Pose != PoseState::None)
        {
            m_Pose = PoseState::None;
            m_PoseTime = 0.f;
        }
        return;
    }

    PoseState nextPose = PoseState::None;
    if (in.upHeld && in.downHeld)
    {
        nextPose = PoseState::None;
    }
    else if (in.upHeld) nextPose = PoseState::LookUp;
    else if (in.downHeld) nextPose = PoseState::LookDown;

    if (nextPose != m_Pose)
    {
        m_Pose = nextPose;
        m_PoseTime = 0.f; // 포즈 시작 연출용 타이머 리셋
    }
}

void CPlayerScript::ResolveAction(const PlayerInput& in, float dt)
{
    // 차징 중: 아래키 유지 + 스페이스 유지
    if (m_Action == ActionState::SkillCharge)
    {
        // 아래키를 떼면 대시 발동
        if (in.downReleased)
        {
            StartSkillDash();
            return;
        }

        // 스페이스를 떼면 대시 발동
        if (in.spaceReleased)
        {
            StartSkillDash();
            return;
        }

        // 스페이스를 그냥 안 누르게 되면 취소(원하면 유지로 바꿔도 됨)
        if (!in.spaceHeld)
        {
            StartSkillDash();
            m_Action = ActionState::SkillCharge;
            return;
        }

        return;
    }

    // SkillDash는 이동 시뮬레이션에서 속도 보고 종료 처리
    if (m_Action == ActionState::SkillDash)
        return;

    if (m_Action == ActionState::KnockBack)
    {
        const bool bRecoveredOnGround = IsGround && m_ActionTime >= kKnockBackMinGroundRecoverTime;
        const bool bTimedOut = m_ActionTime >= kKnockBackMaxDuration;

        if (bRecoveredOnGround || bTimedOut)
        {
            m_Action = ActionState::None;
            m_ActionTime = 0.f;
            bIsSpringJump = false;

            if (IsGround)
                IsJump = false;
        }
        return;
    }

    // TODO: Attack/Roll 종료는 m_ActionTime 또는 애니 이벤트로 처리
    // if (m_Action == ActionState::Attack) ...
    if (m_Action == ActionState::Roll)
    {
        if (!m_bBackReaction)
            return;

        const bool bRecoveredOnGround = IsGround;
        const bool bTimedOut = m_ActionTime >= kKnockBackMaxDuration;

        if (bRecoveredOnGround || bTimedOut)
        {
            m_Action = ActionState::None;
            m_ActionTime = 0.f;
            m_bBackReaction = false;
            bIsSpringJump = false;

            if (IsGround)
                IsJump = false;
        }
        return;
    }

    if (m_Action == ActionState::Pushing)
    {
        if (!IsGround)
        {
            m_bPushContact = false;
            m_PushContactDir = 0;
            m_bPushing = false;
            m_PushingDir = 0;
            m_Action = ActionState::None;
        }
        return;
    }
}

bool CPlayerScript::CanStartJump(const PlayerInput& in) const
{
    const bool surfaceCoyoteTime =
        (m_SurfaceGroundHoldTime > 0.f) &&
        ((m_bHasCurrentSurfaceContact && m_CurrentSurfaceContact.Attachable && !m_CurrentSurfaceContact.WallLike) ||
         (m_bHasPendingSurfaceContact && m_PendingSurfaceContact.Attachable && !m_PendingSurfaceContact.WallLike));
    if (!IsGround && !surfaceCoyoteTime) return false;

    // Dash는 예외로 허용
    if (m_bBackReaction)
        return false;

    if (m_Action != ActionState::None
        && m_Action != ActionState::SkillDash
        && m_Action != ActionState::Roll
        && m_Action != ActionState::Break
        && m_Action != ActionState::Pushing)
        return false;

    // ✅ 기존처럼: 아래보기(앉기) 상태에서는 스페이스가 점프가 아니라 스킬에 쓰이므로 점프 금지
    if (m_Pose == PoseState::LookDown) return false;

    return in.spacePressed;
}

bool CPlayerScript::IsSameInwardCircleLoop(const Vec2& _Center, float _Radius) const
{
    return (fabsf(m_InwardCircleLoopCenter.x - _Center.x) <= 0.5f)
        && (fabsf(m_InwardCircleLoopCenter.y - _Center.y) <= 0.5f)
        && (fabsf(m_InwardCircleLoopRadius - _Radius) <= 0.5f);
}

bool CPlayerScript::IsSameInwardCircleHalfChecker(const Vec2& _Center, float _Radius) const
{
    return (fabsf(m_InwardCircleHalfCheckerCenter.x - _Center.x) <= 0.5f)
        && (fabsf(m_InwardCircleHalfCheckerCenter.y - _Center.y) <= 0.5f)
        && (fabsf(m_InwardCircleHalfCheckerRadius - _Radius) <= 0.5f);
}

bool CPlayerScript::TryGetInwardCircleSurfaceData(const SurfaceContact& _Contact, Vec2& _OutCenter, float& _OutRadius) const
{
    _OutCenter = Vec2(0.f, 0.f);
    _OutRadius = 0.f;

    if (!_Contact.Circle || !_Contact.InwardCircle || _Contact.Surface == nullptr)
        return false;

    auto pSurface = _Contact.Surface->GetScript<CSurfaceScript>();
    if (pSurface == nullptr || !IsCircleGeometry(pSurface->GetGeometry()))
        return false;

    Vec2 boxMin = {};
    Vec2 boxMax = {};
    pSurface->GetArcWorldData(_OutCenter, _OutRadius, boxMin, boxMax);
    return (_OutRadius > 0.0001f);
}

bool CPlayerScript::TryGetInwardCircleGuideData(const SurfaceContact& _Contact, Vec2& _OutCenter, float& _OutRadius, const CSurfaceCircleGuideScript*& _OutGuide) const
{
    _OutCenter = Vec2(0.f, 0.f);
    _OutRadius = 0.f;
    _OutGuide = nullptr;

    if (!TryGetInwardCircleSurfaceData(_Contact, _OutCenter, _OutRadius))
        return false;

    const auto tryMatchGuide = [&](GameObject* _SurfaceObject) -> const CSurfaceCircleGuideScript*
    {
        if (_SurfaceObject == nullptr)
            return nullptr;

        auto pSurface = _SurfaceObject->GetScript<CSurfaceScript>();
        auto pGuide = _SurfaceObject->GetScript<CSurfaceCircleGuideScript>();
        if (pSurface == nullptr ||
            pGuide == nullptr ||
            !pGuide->IsEnabled() ||
            !IsCircleGeometry(pSurface->GetGeometry()) ||
            !pSurface->GetFillInside())
        {
            return nullptr;
        }

        Vec2 center = {};
        float radius = 0.f;
        Vec2 boxMin = {};
        Vec2 boxMax = {};
        pSurface->GetArcWorldData(center, radius, boxMin, boxMax);
        if (fabsf(center.x - _OutCenter.x) > 0.5f ||
            fabsf(center.y - _OutCenter.y) > 0.5f ||
            fabsf(radius - _OutRadius) > 0.5f)
        {
            return nullptr;
        }

        return pGuide.Get();
    };

    if (const CSurfaceCircleGuideScript* pGuide = tryMatchGuide(_Contact.Surface))
    {
        _OutGuide = pGuide;
        return true;
    }

    if (const CSurfaceCircleGuideScript* pGuide = tryMatchGuide(m_bHasCurrentSurfaceContact ? m_CurrentSurfaceContact.Surface : nullptr))
    {
        _OutGuide = pGuide;
        return true;
    }

    if (const CSurfaceCircleGuideScript* pGuide = tryMatchGuide(m_bHasPendingSurfaceContact ? m_PendingSurfaceContact.Surface : nullptr))
    {
        _OutGuide = pGuide;
        return true;
    }

    for (GameObject* pSurfaceObject : m_vecActiveSurfaceObjects)
    {
        if (const CSurfaceCircleGuideScript* pGuide = tryMatchGuide(pSurfaceObject))
        {
            _OutGuide = pGuide;
            return true;
        }
    }

    return false;
}

bool CPlayerScript::IsSameGuidedInwardCircleEntryState(const Vec2& _Center, float _Radius) const
{
    return m_bGuidedInwardCircleEntryTracked &&
        (fabsf(m_GuidedInwardCircleEntryCenter.x - _Center.x) <= 0.5f) &&
        (fabsf(m_GuidedInwardCircleEntryCenter.y - _Center.y) <= 0.5f) &&
        (fabsf(m_GuidedInwardCircleEntryRadius - _Radius) <= 0.5f);
}

bool CPlayerScript::InferGuidedInwardCircleEntryFromRight(const Vec2& _Center)
{
    const Vec3 playerPos3 = Transform()->GetRelativePos();

    // Guided inward-circle entry is now committed only on the initial
    // outside-to-inside transition, so the player's side relative to the
    // circle center is the most stable way to decide the entry direction.
    return (playerPos3.x > _Center.x);
}

bool CPlayerScript::TryGetGuidedInwardCircleABPoints(const Vec2& _Center, float _Radius, const CSurfaceCircleGuideScript* _Guide,
                                                     Vec2& _OutLeftPoint, Vec2& _OutRightPoint) const
{
    _OutLeftPoint = _Center;
    _OutRightPoint = _Center;

    if (_Guide == nullptr || !_Guide->IsEnabled() || _Radius <= 0.0001f)
        return false;

    Vec2 pointA = _Center + _Guide->GetCorrectionLineStartLocal();
    Vec2 pointB = _Center + _Guide->GetCorrectionLineEndLocal();
    if (LengthVec2(pointB - pointA) <= 1.f)
    {
        pointA = MakeCirclePointRad(_Center, _Radius, DegreesToRadians(_Guide->GetEntryAngleDeg()));
        pointB = MakeCirclePointRad(_Center, _Radius, DegreesToRadians(_Guide->GetExitAngleDeg()));
    }

    if (pointB.x < pointA.x)
    {
        const Vec2 temp = pointA;
        pointA = pointB;
        pointB = temp;
    }

    _OutLeftPoint = pointA;
    _OutRightPoint = pointB;
    return true;
}

bool CPlayerScript::TryGetGuidedInwardCircleRuntimeState(const Vec2& _Center, float _Radius, const CSurfaceCircleGuideScript* _Guide,
                                                         Vec2& _OutLeftPoint, Vec2& _OutRightPoint,
                                                         bool& _OutEntryFromRight, bool& _OutPassedHalf,
                                                         bool& _OutLineOwnsRight)
{
    _OutLeftPoint = _Center;
    _OutRightPoint = _Center;
    _OutEntryFromRight = false;
    _OutPassedHalf = false;
    _OutLineOwnsRight = false;

    if (!TryGetGuidedInwardCircleABPoints(_Center, _Radius, _Guide, _OutLeftPoint, _OutRightPoint))
        return false;

    if (IsSameGuidedInwardCircleEntryState(_Center, _Radius))
    {
        _OutEntryFromRight = m_bGuidedInwardCircleEntryFromRight;
        _OutLineOwnsRight = m_bGuidedInwardCircleLineOwnsRight;
        _OutPassedHalf = m_bGuidedInwardCirclePassedHalf;
    }
    else
    {
        // Guided inward-circle entry is decided only when we first enter from
        // the outside. Once inside, only the half-check is allowed to swap the
        // entry/exit ownership, so an untracked runtime state must not be
        // reconstructed from the current interior position or input.
        return false;
    }

    return true;
}

bool CPlayerScript::TryGetGuidedInwardCirclePassState(const Vec2& _Center, float _Radius, const CSurfaceCircleGuideScript* _Guide,
                                                      Vec2& _OutLeftPoint, Vec2& _OutRightPoint,
                                                      bool& _OutPassedHalf, bool& _OutOpenRight)
{
    _OutLeftPoint = _Center;
    _OutRightPoint = _Center;
    _OutPassedHalf = false;
    _OutOpenRight = false;

    bool entryFromRight = false;
    return TryGetGuidedInwardCircleRuntimeState(_Center, _Radius, _Guide,
                                                _OutLeftPoint, _OutRightPoint,
                                                entryFromRight, _OutPassedHalf, _OutOpenRight);
}

bool CPlayerScript::ShouldLineOwnTopRightInwardCircleHalf(const Vec2& _Center, float _Radius, const Vec2& _ContactPoint) const
{
    if (m_bInwardCircleHalfCheckerTracked && IsSameInwardCircleHalfChecker(_Center, _Radius))
        return m_bInwardCircleLineOwnsTopRight;

    float travelX = 0.f;
    if (fabsf(vVelocity.x) > 1.f)
        travelX = vVelocity.x;
    else if (fabsf(m_LastFrameDeltaX) > 0.1f)
        travelX = m_LastFrameDeltaX;

    if (travelX > 0.f)
        return true;

    if (travelX < 0.f)
        return false;

    if (_ContactPoint.x < _Center.x - kSurfaceInwardCircleTopHalfSwitchEpsilon)
        return true;

    if (_ContactPoint.x > _Center.x + kSurfaceInwardCircleTopHalfSwitchEpsilon)
        return false;

    return (_ContactPoint.x >= _Center.x);
}

void CPlayerScript::UpdateInwardCircleHalfChecker(const Vec2& _Center, float _Radius, const Vec2& _ContactPoint)
{
    if (!m_bInwardCircleHalfCheckerTracked || !IsSameInwardCircleHalfChecker(_Center, _Radius))
    {
        m_bInwardCircleHalfCheckerTracked = true;
        m_InwardCircleHalfCheckerCenter = _Center;
        m_InwardCircleHalfCheckerRadius = _Radius;
        m_bInwardCircleLineOwnsTopRight = ShouldLineOwnTopRightInwardCircleHalf(_Center, _Radius, _ContactPoint);
    }

    const float safeRadius = max(_Radius, 0.0001f);
    const float upperHalfRatio = (_Center.y - _ContactPoint.y) / safeRadius;
    if (upperHalfRatio < kSurfaceInwardCircleHalfCheckerSwitchUpperHalfMin)
        return;

    float travelX = 0.f;
    if (fabsf(vVelocity.x) > 1.f)
        travelX = vVelocity.x;
    else if (fabsf(m_LastFrameDeltaX) > 0.1f)
        travelX = m_LastFrameDeltaX;

    if (travelX > 0.f && _ContactPoint.x >= _Center.x + kSurfaceInwardCircleTopHalfSwitchEpsilon)
    {
        m_bInwardCircleLineOwnsTopRight = false;
    }
    else if (travelX < 0.f && _ContactPoint.x <= _Center.x - kSurfaceInwardCircleTopHalfSwitchEpsilon)
    {
        m_bInwardCircleLineOwnsTopRight = true;
    }
}

void CPlayerScript::PrimeInwardCircleHalfCheckerFromLineContext(const SurfaceContact& _Contact)
{
    if (_Contact.Circle || _Contact.WallLike || !_Contact.Attachable)
        return;

    const auto primeFromCircleSurface = [&](GameObject* _SurfaceObject)
    {
        if (_SurfaceObject == nullptr)
            return false;

        auto pSurface = _SurfaceObject->GetScript<CSurfaceScript>();
        if (pSurface == nullptr ||
            !IsCircleGeometry(pSurface->GetGeometry()) ||
            !pSurface->GetFillInside())
        {
            return false;
        }

        auto pGuide = _SurfaceObject->GetScript<CSurfaceCircleGuideScript>();
        if (pGuide != nullptr && pGuide->IsEnabled())
            return false;

        if (pSurface->GetGeometry() == CSurfaceScript::SURFACE_GEOMETRY::CIRCLE)
        {
            const CSurfaceScript::ARC_CORNER corner = pSurface->GetArcCorner();
            if (corner != CSurfaceScript::ARC_CORNER::TOP_LEFT &&
                corner != CSurfaceScript::ARC_CORNER::TOP_RIGHT)
            {
                return false;
            }
        }

        Vec2 center = {};
        float radius = 0.f;
        Vec2 boxMin = {};
        Vec2 boxMax = {};
        pSurface->GetArcWorldData(center, radius, boxMin, boxMax);
        UpdateInwardCircleHalfChecker(center, radius, _Contact.ContactPoint);
        return true;
    };

    if (m_bHasCurrentSurfaceContact &&
        m_CurrentSurfaceContact.Circle &&
        m_CurrentSurfaceContact.InwardCircle &&
        primeFromCircleSurface(m_CurrentSurfaceContact.Surface))
    {
        return;
    }

    if (m_bHasPendingSurfaceContact &&
        m_PendingSurfaceContact.Circle &&
        m_PendingSurfaceContact.InwardCircle &&
        primeFromCircleSurface(m_PendingSurfaceContact.Surface))
    {
        return;
    }

    for (GameObject* pSurfaceObject : m_vecActiveSurfaceObjects)
    {
        if (primeFromCircleSurface(pSurfaceObject))
            return;
    }
}

void CPlayerScript::NoteInwardCircleLoopProgress(const Vec2& _Center, float _Radius, const Vec2& _ContactPoint)
{
    const Vec2 radial = _ContactPoint - _Center;
    if (fabsf(radial.x) <= 0.0001f && fabsf(radial.y) <= 0.0001f)
        return;

    const float angle = atan2f(radial.y, radial.x);
    if (!m_bInwardCircleLoopTracked || !IsSameInwardCircleLoop(_Center, _Radius))
    {
        m_bInwardCircleLoopTracked = true;
        m_bInwardCirclePassedLowerHalf = false;
        m_InwardCircleLoopCenter = _Center;
        m_InwardCircleLoopRadius = _Radius;
        m_InwardCircleLoopAccumulatedAngle = 0.f;
        m_InwardCircleLoopLastAngle = angle;
        return;
    }

    const float delta = WrapAngleRad(angle - m_InwardCircleLoopLastAngle);
    m_InwardCircleLoopLastAngle = angle;

    const float stepAngle = min(fabsf(delta), kSurfaceInwardCircleMaxAngleStep);
    if (stepAngle > 0.0001f)
        m_InwardCircleLoopAccumulatedAngle += stepAngle;

    if (radial.y >= 0.f)
        m_bInwardCirclePassedLowerHalf = true;
}

bool CPlayerScript::ShouldReleaseInwardCircle(const Vec2& _Center, float _Radius, const Vec2& _ContactPoint) const
{
    if (!m_bInwardCircleLoopTracked || !IsSameInwardCircleLoop(_Center, _Radius))
        return false;

    const float safeRadius = max(_Radius, 0.0001f);
    const float upperHalfRatio = (_Center.y - _ContactPoint.y) / safeRadius;
    const bool reachedUpperHalf = (upperHalfRatio >= kSurfaceInwardCircleUpperHalfMin);
    const bool completedLoop = (m_InwardCircleLoopAccumulatedAngle >= kSurfaceInwardCircleLoopReleaseAngle);
    return m_bInwardCirclePassedLowerHalf && completedLoop && reachedUpperHalf;
}

void CPlayerScript::BlockInwardCircleAttach(const Vec2& _Center, float _Radius, float _Time)
{
    if (m_bHasCurrentSurfaceContact &&
        m_CurrentSurfaceContact.Surface != nullptr &&
        m_CurrentSurfaceContact.Circle &&
        m_CurrentSurfaceContact.InwardCircle)
    {
        m_bHasRecentReleasedInwardCircleContact = true;
        m_RecentReleasedInwardCircleContact = m_CurrentSurfaceContact;
        m_RecentReleasedInwardCircleTime = kSurfaceGuidedCorrectionLineTransferHoldTime;
        CacheRecentGuidedCorrectionLineFromSurface(m_CurrentSurfaceContact.Surface, kSurfaceGuidedCorrectionLineTransferHoldTime);
    }

    if (m_bHasPendingSurfaceContact &&
        m_PendingSurfaceContact.Surface != nullptr &&
        m_PendingSurfaceContact.Circle &&
        m_PendingSurfaceContact.InwardCircle)
    {
        m_bHasRecentReleasedInwardCircleContact = true;
        m_RecentReleasedInwardCircleContact = m_PendingSurfaceContact;
        m_RecentReleasedInwardCircleTime = kSurfaceGuidedCorrectionLineTransferHoldTime;
        CacheRecentGuidedCorrectionLineFromSurface(m_PendingSurfaceContact.Surface, kSurfaceGuidedCorrectionLineTransferHoldTime);
    }

    m_InwardCircleAttachBlockCenter = _Center;
    m_InwardCircleAttachBlockRadius = _Radius;
    m_InwardCircleAttachBlockTime = _Time;

    if (m_bHasCurrentSurfaceContact && m_CurrentSurfaceContact.Surface != nullptr)
        BlockSurfaceAttach(m_CurrentSurfaceContact.Surface, _Time);

    m_bHasPendingSurfaceContact = false;
    m_PendingSurfaceContact = SurfaceContact{};
    m_SurfaceResolveFrame = E_Time;
    m_SurfaceResolveScore = 999999.f;
    m_pResolvedSurface = nullptr;
    m_SurfaceGroundHoldTime = 0.f;
    ResetGroundContact();
    ResetInwardCircleLoopState();
    ResetGuidedInwardCircleState();

    if (IsSurfaceAutoplayScenario() && !IsJump)
    {
        ++m_AutoplayInwardCircleReleaseCount;
        LogAutoplayState(L"circle_release");
    }
}

bool CPlayerScript::IsInwardCircleAttachBlocked(const Vec2& _Center, float _Radius) const
{
    if (m_InwardCircleAttachBlockTime <= 0.f)
        return false;

    return (fabsf(m_InwardCircleAttachBlockCenter.x - _Center.x) <= 0.5f)
        && (fabsf(m_InwardCircleAttachBlockCenter.y - _Center.y) <= 0.5f)
        && (fabsf(m_InwardCircleAttachBlockRadius - _Radius) <= 0.5f);
}

bool CPlayerScript::HasInwardCircleLineContext() const
{
    if (m_bHasCurrentSurfaceContact &&
        !m_CurrentSurfaceContact.Circle &&
        !m_CurrentSurfaceContact.WallLike &&
        m_CurrentSurfaceContact.Attachable)
    {
        return true;
    }

    if (m_bHasPendingSurfaceContact &&
        !m_PendingSurfaceContact.Circle &&
        !m_PendingSurfaceContact.WallLike &&
        m_PendingSurfaceContact.Attachable)
    {
        return true;
    }

    for (GameObject* pSurfaceObject : m_vecActiveSurfaceObjects)
    {
        if (IsAttachableLineSurfaceObject(pSurfaceObject))
            return true;
    }

    return false;
}

bool CPlayerScript::IsAttachableLineSurfaceObject(GameObject* _SurfaceObject) const
{
    if (_SurfaceObject == nullptr)
        return false;

    auto pSurface = _SurfaceObject->GetScript<CSurfaceScript>();
    if (pSurface == nullptr)
        return false;

    return pSurface->IsAttachable()
        && (pSurface->GetGeometry() == CSurfaceScript::SURFACE_GEOMETRY::LINE)
        && (pSurface->GetRole() != CSurfaceScript::SURFACE_ROLE::WALL);
}

const CPlayerScript::SurfaceContact* CPlayerScript::GetReferenceAttachableLineContact() const
{
    if (m_bHasCurrentSurfaceContact &&
        IsAttachableLineSurfaceObject(m_CurrentSurfaceContact.Surface))
    {
        return &m_CurrentSurfaceContact;
    }

    if (m_bHasPendingSurfaceContact &&
        IsAttachableLineSurfaceObject(m_PendingSurfaceContact.Surface))
    {
        return &m_PendingSurfaceContact;
    }

    return nullptr;
}

GameObject* CPlayerScript::GetReferenceAttachableLineSurface() const
{
    const SurfaceContact* pReferenceContact = GetReferenceAttachableLineContact();
    if (pReferenceContact != nullptr)
        return pReferenceContact->Surface;

    return nullptr;
}

bool CPlayerScript::AreLineSurfaceEndpointsConnected(GameObject* _SurfaceA, GameObject* _SurfaceB) const
{
    if (_SurfaceA == nullptr || _SurfaceB == nullptr)
        return false;

    if (_SurfaceA == _SurfaceB)
        return true;

    if (!IsAttachableLineSurfaceObject(_SurfaceA) || !IsAttachableLineSurfaceObject(_SurfaceB))
        return false;

    auto pSurfaceA = _SurfaceA->GetScript<CSurfaceScript>();
    auto pSurfaceB = _SurfaceB->GetScript<CSurfaceScript>();
    if (pSurfaceA == nullptr || pSurfaceB == nullptr)
        return false;

    Vec2 aStart = {};
    Vec2 aEnd = {};
    Vec2 bStart = {};
    Vec2 bEnd = {};
    pSurfaceA->GetWorldEndpoints(aStart, aEnd);
    pSurfaceB->GetWorldEndpoints(bStart, bEnd);
    CanonicalizeLineEndpoints(aStart, aEnd);
    CanonicalizeLineEndpoints(bStart, bEnd);

    const float endpointDistance =
        min(min(LengthVec2(aStart - bStart), LengthVec2(aStart - bEnd)),
            min(LengthVec2(aEnd - bStart), LengthVec2(aEnd - bEnd)));
    return endpointDistance <= kSurfaceLineSeamConnectionMaxDistance;
}

bool CPlayerScript::TryGetTransitionLineSeamEndpoint(const SurfaceContact& _Contact, bool _UseExitEndpoint, Vec2& _OutEndpoint) const
{
    _OutEndpoint = Vec2(0.f, 0.f);

    if (!_Contact.Attachable ||
        _Contact.WallLike ||
        _Contact.Circle ||
        _Contact.Surface == nullptr ||
        !_Contact.SeamBlendHasValue)
    {
        return false;
    }

    auto pSurface = _Contact.Surface->GetScript<CSurfaceScript>();
    if (pSurface == nullptr || pSurface->GetGeometry() != CSurfaceScript::SURFACE_GEOMETRY::LINE)
        return false;

    Vec2 lineStart = {};
    Vec2 lineEnd = {};
    pSurface->GetWorldEndpoints(lineStart, lineEnd);
    CanonicalizeLineEndpoints(lineStart, lineEnd);

    const float moveX = vVelocity.x;
    bool useForwardSeam = (_Contact.SeamBlendT >= 0.5f);
    if (moveX > 1.f)
        useForwardSeam = true;
    else if (moveX < -1.f)
        useForwardSeam = false;

    if (_UseExitEndpoint)
        _OutEndpoint = useForwardSeam ? lineEnd : lineStart;
    else
        _OutEndpoint = useForwardSeam ? lineStart : lineEnd;

    return true;
}

float CPlayerScript::GetTransitionLineEndpointGap(const SurfaceContact& _ReferenceContact, const SurfaceContact& _CandidateContact) const
{
    Vec2 referenceExitPoint = {};
    Vec2 candidateEntryPoint = {};
    if (!TryGetTransitionLineSeamEndpoint(_ReferenceContact, true, referenceExitPoint) ||
        !TryGetTransitionLineSeamEndpoint(_CandidateContact, false, candidateEntryPoint))
    {
        return -1.f;
    }

    return LengthVec2(referenceExitPoint - candidateEntryPoint);
}

bool CPlayerScript::AreTransitionLineContactsSeamCompatible(const SurfaceContact& _ReferenceContact, const SurfaceContact& _CandidateContact) const
{
    if (!_ReferenceContact.SeamBlendHasValue || !_CandidateContact.SeamBlendHasValue)
        return false;

    const float moveX = vVelocity.x;
    if (moveX < -1.f)
    {
        if (!(_ReferenceContact.SeamBlendT <= 0.5f) || !(_CandidateContact.SeamBlendT >= 0.5f))
            return false;
    }
    else if (moveX > 1.f)
    {
        if (!(_ReferenceContact.SeamBlendT >= 0.5f) || !(_CandidateContact.SeamBlendT <= 0.5f))
            return false;
    }

    const float endpointGap = GetTransitionLineEndpointGap(_ReferenceContact, _CandidateContact);
    return (endpointGap >= 0.f) && (endpointGap <= kSurfaceLineSeamEndpointMatchDistance);
}

bool CPlayerScript::IsConnectedTransitionLineContact(const SurfaceContact& _Contact) const
{
    if (!_Contact.TransitionSurface ||
        !_Contact.Attachable ||
        _Contact.WallLike ||
        _Contact.Circle ||
        _Contact.Surface == nullptr)
    {
        return false;
    }

    if (m_bHasCurrentSurfaceContact &&
        m_CurrentSurfaceContact.Surface != nullptr &&
        m_CurrentSurfaceContact.Surface == _Contact.Surface)
    {
        return true;
    }

    const SurfaceContact* pReferenceLineContact = GetReferenceAttachableLineContact();
    if (pReferenceLineContact == nullptr || pReferenceLineContact->Surface == nullptr)
        return false;

    return AreLineSurfaceEndpointsConnected(pReferenceLineContact->Surface, _Contact.Surface) &&
        AreTransitionLineContactsSeamCompatible(*pReferenceLineContact, _Contact);
}

bool CPlayerScript::IsRelaxedDownwardTransitionLineContact(const SurfaceContact& _Contact) const
{
    if (!_Contact.TransitionSurface ||
        !_Contact.Attachable ||
        _Contact.WallLike ||
        _Contact.Circle ||
        _Contact.Surface == nullptr)
    {
        return false;
    }

    const bool groundedLineContext = GetIsGround() || m_SurfaceGroundHoldTime > 0.f;
    if (!groundedLineContext)
        return false;

    const SurfaceContact* pReferenceLineContact = GetReferenceAttachableLineContact();
    if (pReferenceLineContact == nullptr ||
        pReferenceLineContact->Surface == nullptr ||
        pReferenceLineContact->Surface == _Contact.Surface)
    {
        return false;
    }

    if (!AreLineSurfaceEndpointsConnected(pReferenceLineContact->Surface, _Contact.Surface))
        return false;

    if (AreTransitionLineContactsSeamCompatible(*pReferenceLineContact, _Contact))
        return true;

    if (!_Contact.SeamBlendHasValue)
        return false;

    const float seamDirBias =
        ComputeSeamDirectionBias(_Contact.SeamBlendT, _Contact.SeamBlendHasValue, _Contact.TransitionSurface, vVelocity.x);
    if (seamDirBias < kSurfaceLineRelaxedDownwardSeamBiasMin)
        return false;

    const Vec2 previousNormal = NormalizeSafeVec2(pReferenceLineContact->Normal, vNormal);
    const Vec2 nextNormal = NormalizeSafeVec2(_Contact.Normal, previousNormal);
    if (!(previousNormal.y > nextNormal.y + kSurfaceLineSeamDownwardDelta))
        return false;

    const Vec2 nextTangent = Vec2(nextNormal.y, -nextNormal.x);
    const float nextTangentSpeed = DotVec2(vVelocity, nextTangent);
    if (!(nextTangentSpeed * nextTangent.y < -kSurfaceLineSeamRisingSlopeMoveMin))
        return false;

    return (_Contact.Score <= kSurfaceLineSeamTransitionDepth * 2.5f);
}

bool CPlayerScript::ShouldIgnoreGuidedCircleWallContact(GameObject* _Surface, const Vec2& _ContactPoint)
{
    if (_Surface == nullptr)
        return false;

    auto pSurface = _Surface->GetScript<CSurfaceScript>();
    auto pGuide = _Surface->GetScript<CSurfaceCircleGuideScript>();
    if (pSurface == nullptr ||
        pGuide == nullptr ||
        !pGuide->IsEnabled() ||
        !IsCircleGeometry(pSurface->GetGeometry()))
    {
        return false;
    }

    Vec2 center = {};
    float radius = 0.f;
    Vec2 boxMin = {};
    Vec2 boxMax = {};
    pSurface->GetArcWorldData(center, radius, boxMin, boxMax);
    if (radius <= 0.0001f)
        return false;

    return ShouldIgnoreGuidedInwardCirclePoint(center, radius, pGuide.Get(), _ContactPoint);
}

bool CPlayerScript::IsGuidedInwardCircleActiveArcPoint(const Vec2& _Center, float _Radius,
                                                       const CSurfaceCircleGuideScript* _Guide,
                                                       const Vec2& _Point)
{
    if (_Guide == nullptr || !_Guide->IsEnabled() || _Radius <= 0.0001f)
        return false;

    Vec2 leftPoint = {};
    Vec2 rightPoint = {};
    bool entryFromRight = false;
    bool passedHalf = false;
    bool lineOwnsRight = false;
    if (!TryGetGuidedInwardCircleRuntimeState(_Center, _Radius, _Guide,
                                              leftPoint, rightPoint,
                                              entryFromRight, passedHalf, lineOwnsRight))
    {
        return false;
    }

    float startAngle = 0.f;
    float endAngle = 0.f;
    bool counterClockwise = false;
    if (!TryResolveGuidedInwardCircleHalfArc(_Center, _Radius,
                                             leftPoint, rightPoint, lineOwnsRight,
                                             startAngle, endAngle, counterClockwise))
    {
        return false;
    }

    const float safeRadius = max(_Radius, 0.0001f);
    const float endpointTolerance = max(kSurfaceGuidedEndpointSwitchToleranceMin,
                                        _Radius * kSurfaceGuidedEndpointSwitchToleranceScale);
    const float angleTolerance = max(0.02f, min(0.14f, endpointTolerance / safeRadius));
    const float pointAngle = atan2f(_Point.y - _Center.y, _Point.x - _Center.x);
    return IsAngleOnGuidedInwardCircleArc(pointAngle, startAngle, endAngle, counterClockwise, angleTolerance);
}

bool CPlayerScript::ShouldIgnoreGuidedInwardCirclePoint(const Vec2& _Center, float _Radius,
                                                        const CSurfaceCircleGuideScript* _Guide,
                                                        const Vec2& _Point)
{
    Vec2 leftPoint = {};
    Vec2 rightPoint = {};
    bool entryFromRight = false;
    bool passedHalf = false;
    bool lineOwnsRight = false;
    if (!TryGetGuidedInwardCircleRuntimeState(_Center, _Radius, _Guide,
                                              leftPoint, rightPoint,
                                              entryFromRight, passedHalf, lineOwnsRight))
    {
        return false;
    }

    float startAngle = 0.f;
    float endAngle = 0.f;
    bool counterClockwise = false;
    if (!TryResolveGuidedInwardCircleHalfArc(_Center, _Radius,
                                             leftPoint, rightPoint, lineOwnsRight,
                                             startAngle, endAngle, counterClockwise))
    {
        return false;
    }

    const float safeRadius = max(_Radius, 0.0001f);
    const float endpointTolerance = max(kSurfaceGuidedEndpointSwitchToleranceMin,
                                        _Radius * kSurfaceGuidedEndpointSwitchToleranceScale);
    const float angleTolerance = max(0.02f, min(0.14f, endpointTolerance / safeRadius));
    const float pointAngle = atan2f(_Point.y - _Center.y, _Point.x - _Center.x);
    if (!IsAngleOnGuidedInwardCircleArc(pointAngle, startAngle, endAngle, counterClockwise, angleTolerance))
        return true;

    const Vec2 lineOwnedPoint = lineOwnsRight ? rightPoint : leftPoint;
    return (LengthVec2(_Point - lineOwnedPoint) <= endpointTolerance);
}

bool CPlayerScript::TryResolveGuidedInwardCircleEntryExit(const Vec2& _Center, float _Radius,
                                                          const CSurfaceCircleGuideScript* _Guide,
                                                          Vec2& _OutEntryPoint, Vec2& _OutExitPoint,
                                                          float& _OutEntryAngle, float& _OutExitAngle,
                                                          bool _CommitEntryState)
{
    _OutEntryPoint = _Center;
    _OutExitPoint = _Center;
    _OutEntryAngle = 0.f;
    _OutExitAngle = 0.f;

    if (_Guide == nullptr || !_Guide->IsEnabled() || _Radius <= 0.0001f)
        return false;

    const Vec2 corrStart = _Center + _Guide->GetCorrectionLineStartLocal();
    const Vec2 corrEnd = _Center + _Guide->GetCorrectionLineEndLocal();
    if (LengthVec2(corrEnd - corrStart) <= 1.f)
    {
        _OutEntryAngle = DegreesToRadians(_Guide->GetEntryAngleDeg());
        _OutExitAngle = DegreesToRadians(_Guide->GetExitAngleDeg());
        _OutEntryPoint = MakeCirclePointRad(_Center, _Radius, _OutEntryAngle);
        _OutExitPoint = MakeCirclePointRad(_Center, _Radius, _OutExitAngle);
        return true;
    }

    Vec2 leftPoint = corrStart;
    Vec2 rightPoint = corrEnd;
    if (rightPoint.x < leftPoint.x)
    {
        const Vec2 temp = leftPoint;
        leftPoint = rightPoint;
        rightPoint = temp;
    }

    bool entryFromRight = m_bGuidedInwardCircleEntryFromRight;
    bool lineOwnsRight = m_bGuidedInwardCircleLineOwnsRight;
    const bool sameTrackedCircle = IsSameGuidedInwardCircleEntryState(_Center, _Radius);

    if (_CommitEntryState && (!sameTrackedCircle || !m_bGuidedInwardCircleEntryTracked))
    {
        entryFromRight = InferGuidedInwardCircleEntryFromRight(_Center);

        m_bGuidedInwardCircleEntryTracked = true;
        m_GuidedInwardCircleEntryCenter = _Center;
        m_GuidedInwardCircleEntryRadius = _Radius;
        m_bGuidedInwardCircleEntryFromRight = entryFromRight;
        m_bGuidedInwardCircleLineOwnsRight = entryFromRight;
        m_bGuidedInwardCirclePassedHalf = false;
        m_bGuidedInwardCircleHalfZoneTracked = false;
        m_GuidedInwardCircleHalfZoneSign = 0;
        m_bGuidedInwardCircleHalfDeltaTracked = false;
        m_GuidedInwardCircleLastHalfDelta = 0.f;
        lineOwnsRight = m_bGuidedInwardCircleLineOwnsRight;
    }
    else if (!sameTrackedCircle)
    {
        // If the player is already inside and the entry state is gone, we must
        // not re-infer the entry side from the current position. The half-check
        // is the only thing allowed to swap ownership after entry.
        return false;
    }
    else
    {
        lineOwnsRight = m_bGuidedInwardCircleLineOwnsRight;
    }

    // Half traversal still spans the full upper cap from the current circle side to the line-owned side.
    _OutEntryPoint = lineOwnsRight ? leftPoint : rightPoint;
    _OutExitPoint = lineOwnsRight ? rightPoint : leftPoint;
    _OutEntryAngle = atan2f(_OutEntryPoint.y - _Center.y, _OutEntryPoint.x - _Center.x);
    _OutExitAngle = atan2f(_OutExitPoint.y - _Center.y, _OutExitPoint.x - _Center.x);
    return true;
}

bool CPlayerScript::ShouldIgnoreGuidedInwardCircleContact(const SurfaceContact& _Contact)
{
    Vec2 center = {};
    float radius = 0.f;
    const CSurfaceCircleGuideScript* pGuide = nullptr;
    if (!TryGetInwardCircleGuideData(_Contact, center, radius, pGuide) ||
        pGuide == nullptr ||
        radius <= 0.0001f)
    {
        return false;
    }

    if (ShouldIgnoreGuidedInwardCirclePoint(center, radius, pGuide, _Contact.ContactPoint))
        return true;

    Vec2 leftPoint = {};
    Vec2 rightPoint = {};
    bool passedHalf = false;
    bool openRight = false;
    if (!TryGetGuidedInwardCirclePassState(center, radius, pGuide, leftPoint, rightPoint, passedHalf, openRight))
        return false;

    if (!passedHalf || pGuide->GetLinkedCorrectionLineName().empty())
        return false;

    const auto isLineSurfaceObject = [](GameObject* _SurfaceObject) -> bool
    {
        if (_SurfaceObject == nullptr || _SurfaceObject->IsDead())
            return false;

        auto pSurface = _SurfaceObject->GetScript<CSurfaceScript>();
        return pSurface != nullptr &&
            pSurface->GetGeometry() == CSurfaceScript::SURFACE_GEOMETRY::LINE;
    };

    const auto tryMatchLineSurface = [&](GameObject* _SurfaceObject) -> GameObject*
    {
        if (_SurfaceObject == nullptr || _SurfaceObject->GetName() != pGuide->GetLinkedCorrectionLineName())
            return nullptr;

        return isLineSurfaceObject(_SurfaceObject) ? _SurfaceObject : nullptr;
    };

    GameObject* pLineSurfaceObject = nullptr;
    if (m_bHasCurrentSurfaceContact)
        pLineSurfaceObject = tryMatchLineSurface(m_CurrentSurfaceContact.Surface);

    if (pLineSurfaceObject == nullptr && m_bHasPendingSurfaceContact)
        pLineSurfaceObject = tryMatchLineSurface(m_PendingSurfaceContact.Surface);

    if (pLineSurfaceObject == nullptr)
    {
        for (GameObject* pSurfaceObject : m_vecActiveSurfaceObjects)
        {
            pLineSurfaceObject = tryMatchLineSurface(pSurfaceObject);
            if (pLineSurfaceObject != nullptr)
                break;
        }
    }

    if (pLineSurfaceObject == nullptr)
    {
        Ptr<ALevel> pCurLevel = LevelMgr::GetInst()->GetCurLevel();
        if (pCurLevel != nullptr)
        {
            const auto findLineSurfaceByName =
                [&](auto&& _Self, GameObject* _Object) -> GameObject*
            {
                if (_Object == nullptr || _Object->IsDead())
                    return nullptr;

                if (GameObject* pMatch = tryMatchLineSurface(_Object))
                    return pMatch;

                const vector<Ptr<GameObject>>& vecChild = _Object->GetChild();
                for (size_t childIdx = 0; childIdx < vecChild.size(); ++childIdx)
                {
                    if (GameObject* pMatch = _Self(_Self, vecChild[childIdx].Get()))
                        return pMatch;
                }

                return nullptr;
            };

            for (UINT layerIdx = 0; layerIdx < MAX_LAYER && pLineSurfaceObject == nullptr; ++layerIdx)
            {
                const vector<Ptr<GameObject>>& vecObjects = pCurLevel->GetLayer(layerIdx)->GetParentObjects();
                for (size_t objectIdx = 0; objectIdx < vecObjects.size(); ++objectIdx)
                {
                    pLineSurfaceObject = findLineSurfaceByName(findLineSurfaceByName, vecObjects[objectIdx].Get());
                    if (pLineSurfaceObject != nullptr)
                        break;
                }
            }
        }
    }

    if (pLineSurfaceObject == nullptr)
        return false;

    auto pLineSurface = pLineSurfaceObject->GetScript<CSurfaceScript>();
    if (pLineSurface == nullptr || pLineSurface->GetGeometry() != CSurfaceScript::SURFACE_GEOMETRY::LINE)
        return false;

    Vec2 lineStart = {};
    Vec2 lineEnd = {};
    pLineSurface->GetWorldEndpoints(lineStart, lineEnd);
    CanonicalizeLineEndpoints(lineStart, lineEnd);

    const Vec2 lineDelta = lineEnd - lineStart;
    const float lineLength = LengthVec2(lineDelta);
    if (lineLength <= 0.001f)
        return false;

    const Vec2 lineTangent = lineDelta / lineLength;
    const Vec2 lineNormal = NormalizeSafeVec2(
        pLineSurface->GetFillAbove() ? Vec2(lineTangent.y, -lineTangent.x) : Vec2(-lineTangent.y, lineTangent.x));

    Vec2 ownerCenter = {};
    Vec2 visualFootPoint = {};
    float visualFootDistance = 0.f;
    if (!GetPlayerVisualFootDataWorld(Collider2D(), lineNormal, ownerCenter, visualFootPoint, visualFootDistance))
        return false;

    const float centerProjection = DotVec2(ownerCenter - lineStart, lineTangent);
    const float segmentMargin = max(24.f, visualFootDistance + 8.f);
    if (centerProjection < -segmentMargin || centerProjection > lineLength + segmentMargin)
        return false;

    const Vec2 openPoint = openRight ? rightPoint : leftPoint;
    float endpointBlendRadius = max(kSurfaceGuidedEndpointBlendRadiusMin,
                                    radius * kSurfaceGuidedEndpointBlendRadiusScale);
    float endpointDistance = LengthVec2(_Contact.ContactPoint - openPoint);
    bool endpointReachedByArc = false;
    if (IsSameGuidedInwardCircleEntryState(center, radius))
    {
        if (openRight && !m_bGuidedInwardCircleEntryFromRight)
            endpointBlendRadius += min(6.f, max(2.f, radius * 0.04f));

        const Vec3 playerPos3 = Transform()->GetWorldPos();
        const Vec2 playerPos = Vec2(playerPos3.x, playerPos3.y);
        const Vec2 fallbackRadial = NormalizeSafeVec2(_Contact.ContactPoint - center, Vec2(0.f, -1.f));
        const Vec2 playerRadial = NormalizeSafeVec2(playerPos - center, fallbackRadial);
        const Vec2 projectedPlayerPoint = center + playerRadial * radius;
        endpointDistance = min(endpointDistance, LengthVec2(projectedPlayerPoint - openPoint));

        Vec2 entryPoint = {};
        Vec2 exitPoint = {};
        float entryAngle = 0.f;
        float exitAngle = 0.f;
        if (TryResolveGuidedInwardCircleEntryExit(center, radius, pGuide,
                                                  entryPoint, exitPoint,
                                                  entryAngle, exitAngle))
        {
            bool counterClockwise = false;
            const float halfAngle = DegreesToRadians(kSurfaceGuidedHalfCheckTopAngleDeg);
            if (TryResolveArcOrientationViaHalf(entryAngle, exitAngle, halfAngle, counterClockwise))
            {
                const float safeRadius = max(radius, 0.0001f);
                const float angleTolerance = max(0.02f, min(0.14f, endpointBlendRadius / safeRadius));
                const float arcToExit = DeltaAngleAlongOrientation(counterClockwise, entryAngle, exitAngle);
                float arcToBestPoint = DeltaAngleAlongOrientation(counterClockwise, entryAngle,
                                                                  atan2f(_Contact.ContactPoint.y - center.y,
                                                                         _Contact.ContactPoint.x - center.x));
                const float projectedArcToPoint = DeltaAngleAlongOrientation(counterClockwise, entryAngle,
                                                                             atan2f(projectedPlayerPoint.y - center.y,
                                                                                    projectedPlayerPoint.x - center.x));
                if (projectedArcToPoint > arcToBestPoint)
                    arcToBestPoint = projectedArcToPoint;

                endpointReachedByArc = (arcToBestPoint >= arcToExit - angleTolerance);
            }
        }
    }

    if (!endpointReachedByArc && endpointDistance > endpointBlendRadius)
        return false;

    const float footSignedDistance = DotVec2(visualFootPoint - lineStart, lineNormal);
    const float approachSpeedToLine = max(0.f, -DotVec2(vVelocity, lineNormal));
    const float preAttachDistance = max(3.f,
                                        min(10.f, approachSpeedToLine * DT * 0.50f + 1.5f));
    return (footSignedDistance <= preAttachDistance);
}

bool CPlayerScript::ShouldReleaseGuidedInwardCircle(const SurfaceContact& _Contact, const Vec2& _Center, float _Radius)
{
    const CSurfaceCircleGuideScript* pGuide = nullptr;
    Vec2 guideCenter = {};
    float guideRadius = 0.f;
    if (!TryGetInwardCircleGuideData(_Contact, guideCenter, guideRadius, pGuide) ||
        pGuide == nullptr ||
        _Radius <= 0.0001f)
    {
        return false;
    }

    Vec2 leftPoint = {};
    Vec2 rightPoint = {};
    bool passedHalf = false;
    bool openRight = false;
    if (!TryGetGuidedInwardCirclePassState(_Center, _Radius, pGuide, leftPoint, rightPoint, passedHalf, openRight))
    {
        return false;
    }

    const Vec2 openPoint = openRight ? rightPoint : leftPoint;
    float endpointTolerance = max(kSurfaceGuidedEndpointSwitchToleranceMin,
                                  _Radius * kSurfaceGuidedEndpointSwitchToleranceScale);
    float distToOpenPoint = LengthVec2(_Contact.ContactPoint - openPoint);
    bool endpointReachedByArc = false;
    if (IsSameGuidedInwardCircleEntryState(_Center, _Radius))
    {
        if (openRight && !m_bGuidedInwardCircleEntryFromRight)
            endpointTolerance += min(6.f, max(2.f, _Radius * 0.04f));

        const Vec3 playerPos3 = Transform()->GetWorldPos();
        const Vec2 playerPos = Vec2(playerPos3.x, playerPos3.y);
        const Vec2 fallbackRadial = NormalizeSafeVec2(_Contact.ContactPoint - _Center, Vec2(0.f, -1.f));
        const Vec2 playerRadial = NormalizeSafeVec2(playerPos - _Center, fallbackRadial);
        const Vec2 projectedPlayerPoint = _Center + playerRadial * _Radius;
        distToOpenPoint = min(distToOpenPoint, LengthVec2(projectedPlayerPoint - openPoint));

        Vec2 entryPoint = {};
        Vec2 exitPoint = {};
        float entryAngle = 0.f;
        float exitAngle = 0.f;
        if (TryResolveGuidedInwardCircleEntryExit(_Center, _Radius, pGuide,
                                                  entryPoint, exitPoint,
                                                  entryAngle, exitAngle))
        {
            bool counterClockwise = false;
            const float halfAngle = DegreesToRadians(kSurfaceGuidedHalfCheckTopAngleDeg);
            if (TryResolveArcOrientationViaHalf(entryAngle, exitAngle, halfAngle, counterClockwise))
            {
                const float safeRadius = max(_Radius, 0.0001f);
                const float angleTolerance = max(0.02f, min(0.14f, endpointTolerance / safeRadius));
                const float arcToExit = DeltaAngleAlongOrientation(counterClockwise, entryAngle, exitAngle);
                float arcToBestPoint = DeltaAngleAlongOrientation(counterClockwise, entryAngle,
                                                                  atan2f(_Contact.ContactPoint.y - _Center.y,
                                                                         _Contact.ContactPoint.x - _Center.x));
                const float projectedArcToPoint = DeltaAngleAlongOrientation(counterClockwise, entryAngle,
                                                                             atan2f(projectedPlayerPoint.y - _Center.y,
                                                                                    projectedPlayerPoint.x - _Center.x));
                if (projectedArcToPoint > arcToBestPoint)
                    arcToBestPoint = projectedArcToPoint;

                endpointReachedByArc = (arcToBestPoint >= arcToExit - angleTolerance);
            }
        }
    }

    return endpointReachedByArc || (distToOpenPoint <= endpointTolerance);
}

bool CPlayerScript::TryGetGuidedInwardCircleHalfDelta(const SurfaceContact& _Contact, bool& _OutOnActiveArc,
                                                      float& _OutHalfDelta, bool& _OutEntryOnRight)
{
    _OutOnActiveArc = false;
    _OutHalfDelta = 0.f;
    _OutEntryOnRight = false;

    Vec2 center = {};
    float radius = 0.f;
    const CSurfaceCircleGuideScript* pGuide = nullptr;
    if (!TryGetInwardCircleGuideData(_Contact, center, radius, pGuide) ||
        pGuide == nullptr ||
        radius <= 0.0001f)
    {
        return false;
    }

    Vec2 entryPoint = {};
    Vec2 exitPoint = {};
    float entryAngle = 0.f;
    float exitAngle = 0.f;
    if (!TryResolveGuidedInwardCircleEntryExit(center, radius, pGuide, entryPoint, exitPoint, entryAngle, exitAngle))
        return false;

    const float halfAngle = DegreesToRadians(kSurfaceGuidedHalfCheckTopAngleDeg);
    const Vec2 halfCheckPoint = _Contact.ContactPoint;
    const float contactAngle = atan2f(halfCheckPoint.y - center.y, halfCheckPoint.x - center.x);

    bool counterClockwise = false;
    if (!TryResolveArcOrientationViaHalf(entryAngle, exitAngle, halfAngle, counterClockwise))
        return false;

    const float arcToExit = DeltaAngleAlongOrientation(counterClockwise, entryAngle, exitAngle);
    const float arcToHalf = DeltaAngleAlongOrientation(counterClockwise, entryAngle, halfAngle);
    const float arcToContact = DeltaAngleAlongOrientation(counterClockwise, entryAngle, contactAngle);

    _OutOnActiveArc = (arcToContact <= arcToExit + 0.0001f);
    _OutHalfDelta = arcToContact - arcToHalf;
    _OutEntryOnRight = (entryPoint.x >= center.x);
    return true;
}

void CPlayerScript::UpdateGuidedInwardCircleHalfPhase(const SurfaceContact& _Contact, bool _ContinuingCurrentInwardCircle)
{
    Vec2 center = {};
    float radius = 0.f;
    const CSurfaceCircleGuideScript* pGuide = nullptr;
    if (!TryGetInwardCircleGuideData(_Contact, center, radius, pGuide) ||
        pGuide == nullptr ||
        radius <= 0.0001f)
    {
        return;
    }

    Vec2 entryPoint = {};
    Vec2 exitPoint = {};
    float entryAngle = 0.f;
    float exitAngle = 0.f;
    if (!TryResolveGuidedInwardCircleEntryExit(center, radius, pGuide, entryPoint, exitPoint, entryAngle, exitAngle))
        return;

    const float halfAngle = DegreesToRadians(kSurfaceGuidedHalfCheckTopAngleDeg);
    const Vec2 halfPoint = MakeCirclePointRad(center, radius, halfAngle);
    const Vec2 halfCheckPoint = _Contact.ContactPoint;
    const float contactAngle = atan2f(halfCheckPoint.y - center.y, halfCheckPoint.x - center.x);

    bool counterClockwise = false;
    if (!TryResolveArcOrientationViaHalf(entryAngle, exitAngle, halfAngle, counterClockwise))
        return;

    const float arcToHalf = DeltaAngleAlongOrientation(counterClockwise, entryAngle, halfAngle);
    const float arcToContact = DeltaAngleAlongOrientation(counterClockwise, entryAngle, contactAngle);
    const float halfDelta = arcToContact - arcToHalf;
    const float halfTouchRadius = max(kSurfaceGuidedHalfTouchRadiusMin, radius * kSurfaceGuidedHalfTouchRadiusScale);
    const bool touchingHalfPoint = (LengthVec2(halfCheckPoint - halfPoint) <= halfTouchRadius);
    if (!_ContinuingCurrentInwardCircle || !m_bGuidedInwardCircleHalfDeltaTracked)
    {
        m_bGuidedInwardCircleHalfDeltaTracked = true;
        m_GuidedInwardCircleLastHalfDelta = halfDelta;
        return;
    }

    const float previousHalfDelta = m_GuidedInwardCircleLastHalfDelta;
    const bool crossedHalfForward =
        (previousHalfDelta < -kSurfaceGuidedHalfCrossAngleEpsilon) &&
        (halfDelta >= -kSurfaceGuidedHalfCrossAngleEpsilon);
    if (crossedHalfForward && touchingHalfPoint)
    {
        m_bGuidedInwardCircleLineOwnsRight = !m_bGuidedInwardCircleLineOwnsRight;
        m_bGuidedInwardCirclePassedHalf = true;
        m_bGuidedInwardCircleHalfDeltaTracked = false;
        m_GuidedInwardCircleLastHalfDelta = 0.f;
        return;
    }

    m_bGuidedInwardCircleHalfDeltaTracked = true;
    m_GuidedInwardCircleLastHalfDelta = halfDelta;
}

bool CPlayerScript::TryEvaluateGuidedTopHalfInwardCircleContact(const SurfaceContact& _Contact,
                                                                bool& _OutOnActiveArc,
                                                                bool& _OutBeforeHalf,
                                                                bool& _OutBeforeHalfUsesTopRight)
{
    _OutOnActiveArc = false;
    _OutBeforeHalf = false;
    _OutBeforeHalfUsesTopRight = false;
    float halfDelta = 0.f;
    if (!TryGetGuidedInwardCircleHalfDelta(_Contact, _OutOnActiveArc, halfDelta, _OutBeforeHalfUsesTopRight))
        return false;

    _OutBeforeHalf = _OutOnActiveArc && (halfDelta <= 0.f);
    return true;
}

bool CPlayerScript::IsGuideLinkedCorrectionLine(const SurfaceContact& _LineContact, const SurfaceContact& _CircleContact) const
{
    if (_LineContact.Surface == nullptr || _CircleContact.Surface == nullptr)
        return false;

    if (_LineContact.Circle || !_CircleContact.Circle)
        return false;

    Vec2 center = {};
    float radius = 0.f;
    const CSurfaceCircleGuideScript* pGuide = nullptr;
    if (!TryGetInwardCircleGuideData(_CircleContact, center, radius, pGuide) ||
        pGuide == nullptr ||
        pGuide->GetLinkedCorrectionLineName().empty())
    {
        return false;
    }

    return (_LineContact.Surface->GetName() == pGuide->GetLinkedCorrectionLineName());
}

bool CPlayerScript::IsGuidedCorrectionLineContact(const SurfaceContact& _Contact) const
{
    if (_Contact.Surface == nullptr || _Contact.Circle || _Contact.WallLike || !_Contact.Attachable)
        return false;

    if (IsRecentGuidedCorrectionLineSurface(_Contact.Surface))
        return true;

    if (m_bHasCurrentSurfaceContact &&
        m_CurrentSurfaceContact.Surface != nullptr &&
        m_CurrentSurfaceContact.Circle &&
        IsGuideLinkedCorrectionLine(_Contact, m_CurrentSurfaceContact))
    {
        return true;
    }

    if (m_bHasPendingSurfaceContact &&
        m_PendingSurfaceContact.Surface != nullptr &&
        m_PendingSurfaceContact.Circle &&
        IsGuideLinkedCorrectionLine(_Contact, m_PendingSurfaceContact))
    {
        return true;
    }

    return false;
}

bool CPlayerScript::IsRecentGuidedCorrectionLineSurface(GameObject* _Surface) const
{
    if (_Surface == nullptr || m_RecentGuidedCorrectionLineTime <= 0.f || m_RecentGuidedCorrectionLineName.empty())
        return false;

    return (_Surface->GetName() == m_RecentGuidedCorrectionLineName);
}

void CPlayerScript::CacheRecentGuidedCorrectionLineFromSurface(GameObject* _Surface, float _Time)
{
    if (_Surface == nullptr || _Time <= 0.f)
        return;

    auto pGuide = _Surface->GetScript<CSurfaceCircleGuideScript>();
    if (pGuide == nullptr || !pGuide->IsEnabled() || pGuide->GetLinkedCorrectionLineName().empty())
        return;

    m_RecentGuidedCorrectionLineName = pGuide->GetLinkedCorrectionLineName();
    if (_Time > m_RecentGuidedCorrectionLineTime)
        m_RecentGuidedCorrectionLineTime = _Time;
}

void CPlayerScript::StartJump()
{
    IsJump = true;
    vVelocity += vNormal * m_JumpForce;
    IsGround = false;
    m_SurfaceGroundHoldTime = 0.f;

    if (IsAutoplayEnabled())
    {
        ++m_AutoplayJumpStartCount;
        m_bAutoplayPendingJumpDetach = true;
        LogAutoplayState(L"jump_start");
    }

    GameObject* pJumpSurface = m_bHasCurrentSurfaceContact ? m_CurrentSurfaceContact.Surface : nullptr;
    if (pJumpSurface != nullptr)
        BlockSurfaceAttach(pJumpSurface, 0.12f);

    if (pJumpSurface != nullptr)
    {
        auto pSurface = pJumpSurface->GetScript<CSurfaceScript>();
        if (pSurface != nullptr && IsCircleGeometry(pSurface->GetGeometry()))
        {
            Vec2 center = {};
            float radius = 0.f;
            Vec2 boxMin = {};
            Vec2 boxMax = {};
            pSurface->GetArcWorldData(center, radius, boxMin, boxMax);

            const Vec3 playerPos3 = Transform()->GetRelativePos();
            const Vec2 playerPos = Vec2(playerPos3.x, playerPos3.y);
            if (DotVec2(vNormal, playerPos - center) < 0.f)
            {
                // Jumping off a guided loop should not flip the arrow state.
                // We only need a short reattach block here.
                m_InwardCircleAttachBlockCenter = center;
                m_InwardCircleAttachBlockRadius = radius;
                m_InwardCircleAttachBlockTime = 0.12f;
            }
        }
    }

    m_bHasPendingSurfaceContact = false;
    m_bHasCurrentSurfaceContact = false;
    m_CurrentSurfaceContact = SurfaceContact{};
    ResetInwardCircleLoopState();
    m_bInwardCircleHalfCheckerTracked = false;
    m_InwardCircleHalfCheckerCenter = Vec2(0.f, 0.f);
    m_InwardCircleHalfCheckerRadius = 0.f;
    m_bInwardCircleLineOwnsTopRight = false;
    m_Action = ActionState::None;
    m_bBackReaction = false;
    m_BreakUngroundedTime = 0.f;
    m_IdleTime = 0.f;
}

void CPlayerScript::StartSkillDash()
{
    m_Action = ActionState::SkillDash;
    m_bBackReaction = false;
    m_ActionTime = 0.f;
    m_Pose = PoseState::None;

    vVelocity.x = m_SkillDashSpeed * (float)m_Facing;
}

void CPlayerScript::Rolling()
{
    m_Action = ActionState::Roll;
    m_bBackReaction = false;
    m_ActionTime = 0.f;
    m_Pose = PoseState::None;

    if (IsGround)
    {
        const Vec2 preferredGroundTangent = vTangent;

        float vt = vVelocity.Dot(preferredGroundTangent);
        float curSpeed = fabsf(vt);

        if (curSpeed < 150.f)
            curSpeed = 150.f;

        vt = (m_Facing == 1) ? curSpeed : -curSpeed;
        vVelocity = preferredGroundTangent * vt;
    }
    else
    {
        float curSpeed = fabsf(vVelocity.x);
        if (curSpeed < 150.f)
            curSpeed = 150.f;

        vVelocity.x = curSpeed * (float)m_Facing;
    }
}

bool CPlayerScript::CanMoveInput() const
{
    // ✅ 액션 중엔 이동 입력 잠금 (원하는 규칙에 따라 조절)
    if (m_Action == ActionState::SkillCharge) return false;
    if (m_Action == ActionState::SkillDash)   return false;
    if (m_Action == ActionState::Attack)      return false;
    if (m_Action == ActionState::KnockBack)   return false;
    if (m_Action == ActionState::Roll)        return false;
    if (m_Action == ActionState::Pushing)     return false;

    // ✅ 위/아래 보기 중에도 이동 금지
    if (m_Pose != PoseState::None) return false;

    return true;
}

void CPlayerScript::Simulate(const PlayerInput& in, float dt)
{
    Vec3 vPos = Transform()->GetRelativePos();
    Vec3 vScale = Transform()->GetRelativeScale();
    Vec3 vRotation = Transform()->GetRelativeRot();

    ApplyFacing(in, vScale);
    SimulateHorizontal(in, dt, vPos);
    SimulateVertical(dt, vPos);

    Transform()->SetRelativePos(vPos);
    Transform()->SetRelativeScale(vScale);
    Transform()->SetRelativeRot(vRotation);
}

void CPlayerScript::ApplyFacing(const PlayerInput& in, Vec3& vScale)
{
    bool lockFacing = (m_Action == ActionState::Break || m_Action == ActionState::Spring);
    GameObject* pFacingChild = nullptr;
    if (!GetOwner()->GetChild().empty() && GetOwner()->GetChild(0) != nullptr)
        pFacingChild = GetOwner()->GetChild(0).Get();

    if (!lockFacing)
    {
        float vt = vVelocity.Dot(vTangent);
    }

    // 실제 스케일 적용 (m_Facing 결정권은 유지)
    if (m_Facing == 1)
    {
        vScale.x = fabsf(vScale.x);
        if (pFacingChild != nullptr)
            pFacingChild->Transform()->SetRelativeRot(Vec3(0.f, 0.f, 0.f));
    }
    else
    {
        vScale.x = -fabsf(vScale.x);
        if (pFacingChild != nullptr)
            pFacingChild->Transform()->SetRelativeRot(Vec3(0.f, 0.f, XM_PI));
    }
}

void CPlayerScript::SimulateHorizontal(const PlayerInput& in, float dt, Vec3& vPos)
{
    Vec2 preferredGroundTangent = vTangent;

    if (m_Action == ActionState::Break)
    {
        float brakePower = 500.f;
        float breakSpeed = m_fBreakSpeed;
        if (m_bBreakWallLocked)
        {
            m_fBreakSpeed = 0.f;
            vVelocity.x = 0.f;
            vVelocity.y = 0.f;
            if (IsGround)
                vVelocity = preferredGroundTangent * 0.f;
            else
                vVelocity.x = 0.f;
        }
        else
        {
            breakSpeed -= brakePower * dt;
            if (breakSpeed < 0.f)
                breakSpeed = 0.f;
            m_fBreakSpeed = breakSpeed;
        }

        if (m_fBreakSpeed < 10.f && !m_bBreakWallLocked)
        {
            m_iBreakDirection = 1;
            m_Action = ActionState::None;
            m_bBreakWallLocked = false;
            if (m_TurnTargetFacing != 0)
                m_Facing = m_TurnTargetFacing;
            m_fBreakSpeed = 0.f;
            return;
        }

        if (m_bBreakWallLocked)
            return;

		// 브레이크 중 공중이면 접선 방향 이동을 끊고 수직 낙하만 하게 한다.
		if (!IsGround)
		{
			vVelocity.x = 0.f;
			return;
		}

		vVelocity = preferredGroundTangent * (m_fBreakSpeed * (float)m_iBreakDirection);

        vPos.x += vVelocity.x * dt;
        return;
    }
    // 1) 스킬 대시 이동
    else if (m_Action == ActionState::SkillDash)
    {
        const float skillFriction = 0.3f; // 네 기존 vFraction.x=0.5 효과
        float brakePower = 100.f;  // 반대키 감속 힘

        vVelocity.x -= vVelocity.x * skillFriction * dt;

        if (fabsf(vVelocity.x) < 100.f)
        {
            vVelocity.x = 0.f;
            m_Action = ActionState::None;
        }

        // 🔥 반대 방향 입력 시 추가 감속
        if ((vVelocity.x > 0.f && in.leftHeld) ||
            (vVelocity.x < 0.f && in.rightHeld))
        {
            if (vVelocity.x > 0.f)
                vVelocity.x -= brakePower * dt;
            else
                vVelocity.x += brakePower * dt;

        }

    }
    else if (m_Action == ActionState::Roll)
    {
        if (m_bBackReaction)
        {
            const float friction = IsGround ? kKnockBackGroundFriction : kKnockBackAirDrag;
            vVelocity.x -= vVelocity.x * friction * dt;

            if (IsGround && fabsf(vVelocity.x) < 25.f)
                vVelocity.x = 0.f;
        }
        else
        {
            float rollFriction = 0.3f; // 작게 줘야 자연스러움
            float brakePower = 200.f;  // 반대키 감속 힘
            vVelocity.x -= vVelocity.x * rollFriction * DT;

            if (IsGround)
            {
                float vt = vVelocity.Dot(preferredGroundTangent);
                if (fabsf(vt) < 200.f)
                {
                    vVelocity.x = 0.f;
                    m_Action = ActionState::None;
                }
            }
            
            // 🔥 반대 방향 입력 시 추가 감속
            if ((vVelocity.x > 0.f && in.leftHeld) ||
                (vVelocity.x < 0.f && in.rightHeld))
            {
                if (vVelocity.x > 0.f)
                    vVelocity.x -= brakePower * dt;
                else
                    vVelocity.x += brakePower * dt;
            }
        }
    }
    else if (m_Action == ActionState::Spring)
    {
        // 공중 저항이나 마찰력이 필요하다면 적용 (SkillDash 로직 참고)
        float springFriction = 0.1f;
        vVelocity.x -= vVelocity.x * springFriction * dt;   

        // 지면에 닿거나 속도가 매우 줄어들면 상태 해제
        if (IsGround || fabsf(vVelocity.x) < 50.f)
        {
            m_Action = ActionState::None;
            bIsSpringJump = false;
        }
    }
    else if (m_Action == ActionState::Pushing)
    {
        float targetSpeed = 0.f;

        if (m_PushingDir == 1 && in.rightHeld && !in.leftHeld)
            targetSpeed = m_PushSpeed;
        else if (m_PushingDir == -1 && in.leftHeld && !in.rightHeld)
            targetSpeed = -m_PushSpeed;

        vVelocity.x = Lerp(vVelocity.x, targetSpeed, 12.f * dt);

        if (fabsf(vVelocity.x) < 5.f && targetSpeed == 0.f)
            vVelocity.x = 0.f;
    }
    else if (m_Action == ActionState::KnockBack)
    {
        const float friction = IsGround ? kKnockBackGroundFriction : kKnockBackAirDrag;
        vVelocity.x -= vVelocity.x * friction * dt;

        if (IsGround && fabsf(vVelocity.x) < 25.f)
            vVelocity.x = 0.f;
    }
    else
    {
        // 2) 일반 이동/마찰
        if (CanMoveInput())
        {
            const bool wantsRight = in.rightHeld && !in.leftHeld;
            const bool wantsLeft = in.leftHeld && !in.rightHeld;

            vFraction.x = 1.f;
            if (IsGround)
            {
                float vt = vVelocity.Dot(preferredGroundTangent);

                if (wantsRight)
                {
                    m_Facing = 1;
                    vt += vAccel.x * dt;
                }
                else if (wantsLeft)
                {
                    m_Facing = -1;
                    vt -= vAccel.x * dt;              
                }
                else
                {
                    vt -= vt * vFraction.x * dt;      
                    if (fabsf(vt) < 30.f)
                        vt = 0.f;
                }

                if (vt > m_MaxMoveSpeed)  vt = m_MaxMoveSpeed;
                if (vt < -m_MaxMoveSpeed) vt = -m_MaxMoveSpeed;

                vVelocity = preferredGroundTangent * vt;
            }
            else
            {
                // 공중 제어 (약하게)
                float airAccel = vAccel.x * m_AirAccelScale;
                float airFraction = 1.f;

                if (wantsRight)
                    vVelocity.x += airAccel * dt;
                else if (wantsLeft)
                    vVelocity.x -= airAccel * dt;

                // 공중에서는 마찰 거의 없음
                else
                {
                    vVelocity.x -= vVelocity.x * airFraction * dt;
                    if (fabsf(vVelocity.x) < 10.f) vVelocity.x = 0.f;
                }
            }
        }
        else
        {
            // 이동 잠금 중이면 마찰로 멈추기
            vVelocity.x -= vVelocity.x * vFraction.x * dt;
            if (fabsf(vVelocity.x) < 10.f) vVelocity.x = 0.f;
        }
    }

    // 속도 제한
    if (vVelocity.x > m_MaxMoveSpeed)  vVelocity.x = m_MaxMoveSpeed;
    if (vVelocity.x < -m_MaxMoveSpeed) vVelocity.x = -m_MaxMoveSpeed;

    vPos.x += vVelocity.x * dt;
}

void CPlayerScript::SimulateVertical(float dt, Vec3& vPos)
{
    if (m_Action == ActionState::Break && m_bBreakWallLocked)
    {
        vVelocity.x = 0.f;
        vVelocity.y = 0.f;
        IsJump = false;
        bIsSpringJump = false;
        return;
    }

    if (IsGround)
    {
        // 접선 속도의 y 성분만 반영
        vPos.y += vVelocity.y * dt;
        IsJump = false;
        bIsSpringJump = false;
        return;
    }

    vVelocity.y -= vAccel.y * dt;
    vPos.y += vVelocity.y * dt;

    if (vVelocity.y > 600.f)
    {
        bIsSpringJump = true;
    }
    else if (vVelocity.y < -600.f)
    {
        bIsSpringJump = false;
    }
}

void CPlayerScript::UpdateTimers(float dt)
{
    // IdleLong 조건: 지상 + 액션 없음 + 포즈 없음 + 속도 매우 낮음
    const Vec2 preferredGroundTangent = vTangent;

    const float absSpeed = IsGround ? fabsf(vVelocity.Dot(preferredGroundTangent)) : fabsf(vVelocity.x);
    const bool idleCandidate =
        IsGround &&
        (m_Action == ActionState::None) &&
        (m_Pose == PoseState::None) &&
        (absSpeed < 15.f);

    if (idleCandidate) m_IdleTime += dt;
    else m_IdleTime = 0.f;

    if (m_Pose != PoseState::None) m_PoseTime += dt;
    else m_PoseTime = 0.f;

    if (m_Action != ActionState::None) m_ActionTime += dt;
    else m_ActionTime = 0.f;
}

void CPlayerScript::UpdateGroundRotation()
{
    float curRotZ = Transform()->GetRelativeRot().z;
    const bool bSnapGroundRotation =
        m_SurfaceRotationSnapTime > 0.f &&
        m_bHasCurrentSurfaceContact &&
        m_CurrentSurfaceContact.Surface != nullptr &&
        !m_CurrentSurfaceContact.Circle &&
        !m_CurrentSurfaceContact.WallLike &&
        m_CurrentSurfaceContact.Attachable;

    if (IsGround)
    {
        if (IsJump)
        {
            Transform()->SetRelativeRot(Vec3(0.f, 0.f, 0.f));
        }
        else
        {
            float fAngle = atan2f(vNormal.y, vNormal.x);
            float fTargetRot = fAngle + XM_PIDIV2 + XM_PI;
            if (bSnapGroundRotation)
                Transform()->SetRelativeRot(Vec3(0.f, 0.f, fTargetRot));
            else
            {
                float fSmoothedRot = LerpAngleRad(curRotZ, fTargetRot, kGroundRotationLerpSpeed, DT);
                Transform()->SetRelativeRot(Vec3(0.f, 0.f, fSmoothedRot));
            }
        }
    }
    else
    {
        if (IsJump)
        {
            Transform()->SetRelativeRot(Vec3(0.f, 0.f, 0.f));
        }
        else
        {
            float fAngle = atan2f(vNormal.y, vNormal.x);
            float fTargetRot = fAngle + XM_PIDIV2 + XM_PI;
            float fSmoothedRot = LerpAngleRad(curRotZ, fTargetRot, kGroundRotationLerpSpeed, DT);
            Transform()->SetRelativeRot(Vec3(0.f, 0.f, fSmoothedRot));
        }
    }
}

void CPlayerScript::UpdateAnimation(float dt)
{
    const float MaxSpeed = 300.f;
    const Vec2 preferredGroundTangent = vTangent;

    const float absSpeed = IsGround ? fabsf(vVelocity.Dot(preferredGroundTangent)) : fabsf(vVelocity.x);
    if (m_Action == ActionState::KnockBack)
    {
        Ptr<AFlipbook> pKnockBackFlipbook = FlipbookRender()->GetFlipbook(kKnockBackFlipbookIndex);
        if (nullptr != pKnockBackFlipbook && 0 < pKnockBackFlipbook->GetSpriteCount())
        {
            const float knockBackAnimFPS = (float)pKnockBackFlipbook->GetSpriteCount() / kKnockBackMaxDuration;
            if (FlipbookRender()->GetCurFlipbookIdx() != kKnockBackFlipbookIndex)
                FlipbookRender()->Play(kKnockBackFlipbookIndex, knockBackAnimFPS, 0);
        }
        return;
    }

    // 1) Action 우선
    if (m_Action == ActionState::SkillDash)
    {
        FlipbookRender()->Play(4, 20.f, -1);
        return;
    }
    if (m_Action == ActionState::SkillCharge)
    {
        FlipbookRender()->Play(7, 60.f, -1);
        return;
    }
    // TODO: Attack/Roll 애니
    // if (m_Action == ActionState::Attack) { ... return; }
    if (m_Action == ActionState::Roll)
    {
        FlipbookRender()->Play(4, 20.f, -1);
        return;
    }
    if (m_Action == ActionState::Break)
    {
        float breakSpeed = m_fBreakSpeed;
        if (breakSpeed <= 0.f)
            breakSpeed = IsGround
            ? fmaxf(fabsf(vVelocity.Dot(preferredGroundTangent)), fabsf(vVelocity.x))
            : fabsf(vVelocity.x);

        if (breakSpeed < 100.f)
            FlipbookRender()->Play(9, 10.f, -1);
        else
            FlipbookRender()->Play(8, 12.f, -1);
        return;
    }
    if (m_Action == ActionState::Pushing)
    {
        FlipbookRender()->Play(kPushingFlipbookIndex, 6.f, -1);
        return;
    }

    // 2) 공중(점프/낙하)
    if (!IsGround)
    {
        //Transform()->SetRelativeRot(Vec3(0.f, 0.f, 0.f));
        //float fAngle = atan2f(vNormal.y, vNormal.x);
        //float fTargetRot = fAngle + XM_PIDIV2 + XM_PI;
        //Transform()->SetRelativeRot(Vec3(0.f, 0.f, fTargetRot));

        if (bIsSpringJump)
        {
            FlipbookRender()->Play(10, 10.f, 1);
            return;
        }
        else
        {
            FlipbookRender()->Play(4, 15.f, -1);
            return;
        }
    }
    // 3) Pose(위/아래 보기)
    if (m_Pose == PoseState::LookUp)
    {
        // 네 기존 "시작 프레임 1회" 느낌 유지
        if (m_PoseTime < 0.1f)
            FlipbookRender()->Play(5, 0, 20.f, 0);
        //else
        //    FlipbookRender()->Play(5, 1, 10.f, -1);
        return;
    }
    if (m_Pose == PoseState::LookDown)
    {
        if (m_PoseTime < 0.1f)
            FlipbookRender()->Play(6, 0, 20.f, 0);
        //else
        //    FlipbookRender()->Play(6, 1, 10.f, -1); 
        return;
    }

    // 4) 이동/대기
    if (absSpeed < 15.f)
    {
        if (m_IdleTime > 5.f)
            FlipbookRender()->Play(2, 5.f, -1);   // IdleLong
        else
            FlipbookRender()->Play(1, 1.f, -1);   // Idle
        return;
    }

    if (absSpeed >= MaxSpeed - 1.f)
    {
        FlipbookRender()->Play(3, 10.f, -1);      // Run
    }
    else
        FlipbookRender()->Play(0, 8.f, -1);       // Walk

}

void CPlayerScript::ResetGroundContact()
{
    IsGround = false;
    vNormal = Vec2(0.f, 1.f);
    vTangent = Vec2(1.f, 0.f);
    m_bHasCurrentSurfaceContact = false;
    m_CurrentSurfaceContact = SurfaceContact{};
    m_bInwardCircleHalfCheckerTracked = false;
    m_InwardCircleHalfCheckerCenter = Vec2(0.f, 0.f);
    m_InwardCircleHalfCheckerRadius = 0.f;
    m_bInwardCircleLineOwnsTopRight = false;
}

void CPlayerScript::ForceFlatGroundContact(float _HoldTime)
{
    auto blockTrackedSurface = [this, _HoldTime](GameObject* _SurfaceObject)
    {
        if (_SurfaceObject == nullptr)
            return;

        BlockSurfaceAttach(_SurfaceObject, _HoldTime);
    };

    blockTrackedSurface(m_bHasCurrentSurfaceContact ? m_CurrentSurfaceContact.Surface : nullptr);
    blockTrackedSurface(m_bHasPendingSurfaceContact ? m_PendingSurfaceContact.Surface : nullptr);

    for (GameObject* pSurfaceObject : m_vecActiveSurfaceObjects)
        blockTrackedSurface(pSurfaceObject);

    m_bHasPendingSurfaceContact = false;
    m_PendingSurfaceContact = SurfaceContact{};
    m_bHasCurrentSurfaceContact = false;
    m_CurrentSurfaceContact = SurfaceContact{};
    m_SurfaceResolveFrame = E_Time;
    m_SurfaceResolveScore = 999999.f;
    m_pResolvedSurface = nullptr;

    IsGround = true;
    IsJump = false;
    bIsSpringJump = false;
    vNormal = Vec2(0.f, 1.f);
    vTangent = Vec2(1.f, 0.f);
    if (_HoldTime > m_ForcedFlatGroundLockTime)
        m_ForcedFlatGroundLockTime = _HoldTime;
    RefreshSurfaceGroundHold(_HoldTime);
    ResetInwardCircleLoopState();
    ResetInwardCircleHalfCheckerState();
}

bool CPlayerScript::HasPushContactDir(int _Dir) const
{
    if (_Dir == 0)
        return false;

    if (m_bPushContact && m_PushContactDir == _Dir)
        return true;

    for (const WallPushContact& contact : m_vecWallPushContacts)
    {
        if (contact.Surface != nullptr && contact.Dir == _Dir)
            return true;
    }

    return false;
}

void CPlayerScript::RegisterWallPushContact(GameObject* _Surface, int _Dir)
{
    if (_Surface == nullptr || (_Dir != 1 && _Dir != -1))
        return;

    for (WallPushContact& contact : m_vecWallPushContacts)
    {
        if (contact.Surface == _Surface)
        {
            contact.Dir = _Dir;
            return;
        }
    }

    WallPushContact contact = {};
    contact.Surface = _Surface;
    contact.Dir = _Dir;
    m_vecWallPushContacts.push_back(contact);
}

void CPlayerScript::UnregisterWallPushContact(GameObject* _Surface)
{
    if (_Surface == nullptr)
        return;

    auto iter = m_vecWallPushContacts.begin();
    while (iter != m_vecWallPushContacts.end())
    {
        if (iter->Surface == _Surface)
            iter = m_vecWallPushContacts.erase(iter);
        else
            ++iter;
    }
}

void CPlayerScript::BlockSurfaceAttach(GameObject* _Surface, float _Time)
{
    if (_Surface == nullptr || _Time <= 0.f)
        return;

    for (SurfaceAttachBlock& block : m_vecAttachBlockedSurfaces)
    {
        if (block.Surface != _Surface)
            continue;

        if (_Time > block.Time)
            block.Time = _Time;
        return;
    }

    SurfaceAttachBlock block = {};
    block.Surface = _Surface;
    block.Time = _Time;
    m_vecAttachBlockedSurfaces.push_back(block);
}

bool CPlayerScript::IsSurfaceAttachBlocked(GameObject* _Surface) const
{
    if (_Surface == nullptr)
        return false;

    for (const SurfaceAttachBlock& block : m_vecAttachBlockedSurfaces)
    {
        if (block.Surface == _Surface && block.Time > 0.f)
            return true;
    }

    return false;
}

void CPlayerScript::RegisterActiveSurface(GameObject* _SurfaceObject)
{
    if (_SurfaceObject == nullptr)
        return;

    for (GameObject* pSurface : m_vecActiveSurfaceObjects)
    {
        if (pSurface == _SurfaceObject)
            return;
    }

    m_vecActiveSurfaceObjects.push_back(_SurfaceObject);
}

void CPlayerScript::UnregisterActiveSurface(GameObject* _SurfaceObject)
{
    if (_SurfaceObject == nullptr)
        return;

    auto iter = m_vecActiveSurfaceObjects.begin();
    while (iter != m_vecActiveSurfaceObjects.end())
    {
        if (*iter == _SurfaceObject)
            iter = m_vecActiveSurfaceObjects.erase(iter);
        else
            ++iter;
    }
}

void CPlayerScript::RefreshNearbyAttachableSurfaces()
{
    m_bHasNearbyAttachableSurface = false;
    m_vecActiveSurfaceObjects.clear();

    Vec2 queryMin = {};
    Vec2 queryMax = {};
    if (!GetColliderQueryBounds(Collider2D(), queryMin, queryMax))
        return;

    std::vector<GameObject*> nearbySurfaceObjects;
    // Active line candidates must include nearby seams before the collider
    // fully overlaps them, otherwise steep slope entry misses the transition
    // window and the player drops through before correction can run.
    CSurfaceScript::QueryNearbySurfaceObjects(queryMin, queryMax, nearbySurfaceObjects, kSurfaceSceneProbeMargin, false);

    for (GameObject* pSurfaceObject : nearbySurfaceObjects)
    {
        if (pSurfaceObject == nullptr || pSurfaceObject->IsDead())
            continue;

        auto pSurface = pSurfaceObject->GetScript<CSurfaceScript>();
        if (pSurface == nullptr || !pSurface->IsAttachable())
            continue;

        m_vecActiveSurfaceObjects.push_back(pSurfaceObject);
    }

    m_bHasNearbyAttachableSurface = !m_vecActiveSurfaceObjects.empty();
}

bool CPlayerScript::IsActiveSurfaceObject(GameObject* _SurfaceObject) const
{
    if (_SurfaceObject == nullptr)
        return false;

    for (GameObject* pSurfaceObject : m_vecActiveSurfaceObjects)
    {
        if (pSurfaceObject == _SurfaceObject)
            return true;
    }

    return false;
}

float CPlayerScript::ComputeSurfaceCandidateScore(const SurfaceContact& _Contact, GameObject* _CurrentBestSurface) const
{
    const bool wasGround = GetIsGround();
    float candidateScore = _Contact.Score;
    const bool connectedTransitionLine =
        IsConnectedTransitionLineContact(_Contact) ||
        IsRelaxedDownwardTransitionLineContact(_Contact);
    const bool allowLineTransitionFavor =
        (_Contact.Circle || _Contact.WallLike || !_Contact.Attachable || connectedTransitionLine);
    const float seamDirBias = ComputeSeamDirectionBias(_Contact.SeamBlendT, _Contact.SeamBlendHasValue, _Contact.TransitionSurface, vVelocity.x);
    if (!_Contact.WallLike && !_Contact.Circle && seamDirBias > 0.f)
    {
        const float moveSpeed = fabsf(vVelocity.x);
        const float speedRate = Clamp01f(moveSpeed / kSurfaceLineSeamMoveSpeedGate);
        const float biasScale = 0.5f + 0.5f * speedRate;
        candidateScore -= kSurfaceLineSeamDirectionBias * seamDirBias * biasScale;
    }

    if (_Contact.TransitionSurface)
    {
        const bool canPreferTransition = _Contact.SeamBlendHasValue &&
            (_Contact.Score <= kSurfaceLineSeamTransitionDepth * 2.f);

        if (wasGround && _Contact.SeamBlendHasValue)
        {
            const Vec2 previousNormal = NormalizeSafeVec2(vNormal);
            const Vec2 nextNormal = NormalizeSafeVec2(_Contact.Normal, previousNormal);
            const float previousToNextDot = DotVec2(previousNormal, _Contact.Normal);
            const Vec2 nextTangent = Vec2(nextNormal.y, -nextNormal.x);
            const float nextTangentSpeed = DotVec2(vVelocity, nextTangent);
            const bool risingSlopeEntry =
                (previousNormal.y > nextNormal.y + 0.01f) &&
                (nextTangentSpeed * nextTangent.y > kSurfaceLineSeamRisingSlopeMoveMin);

            if (canPreferTransition && _CurrentBestSurface != _Contact.Surface && allowLineTransitionFavor)
                candidateScore -= kSurfaceLineSeamTransitionScoreFavor;

            if (previousToNextDot < kSurfaceLineSeamAngleShiftDot)
                candidateScore -= kSurfaceLineSeamAngleShiftScorePenalty;

            if (risingSlopeEntry && allowLineTransitionFavor)
                candidateScore -= kSurfaceLineSeamRisingSlopeScoreFavor;
        }
        else if (canPreferTransition && _CurrentBestSurface != _Contact.Surface && allowLineTransitionFavor)
        {
            candidateScore -= kSurfaceLineSeamTransitionScoreFavor;
        }
    }

    return candidateScore;
}

bool CPlayerScript::IsTopHalfInwardCircleContact(const SurfaceContact& _Contact)
{
    if (!_Contact.Circle || !_Contact.InwardCircle || _Contact.Surface == nullptr)
        return false;

    Vec2 center = {};
    float radius = 0.f;
    const CSurfaceCircleGuideScript* pGuide = nullptr;
    if (TryGetInwardCircleGuideData(_Contact, center, radius, pGuide) &&
        pGuide != nullptr &&
        radius > 0.0001f)
    {
        return !ShouldIgnoreGuidedInwardCircleContact(_Contact);
    }

    auto pSurface = _Contact.Surface->GetScript<CSurfaceScript>();
    if (pSurface == nullptr || !IsCircleGeometry(pSurface->GetGeometry()))
        return false;

    if (pSurface->GetGeometry() == CSurfaceScript::SURFACE_GEOMETRY::CIRCLE)
    {
        const CSurfaceScript::ARC_CORNER corner = pSurface->GetArcCorner();
        if (corner != CSurfaceScript::ARC_CORNER::TOP_LEFT &&
            corner != CSurfaceScript::ARC_CORNER::TOP_RIGHT)
        {
            return false;
        }
    }

    Vec2 boxMin = {};
    Vec2 boxMax = {};
    pSurface->GetArcWorldData(center, radius, boxMin, boxMax);
    return (_Contact.ContactPoint.y < center.y - 0.5f);
}

bool CPlayerScript::ShouldIgnoreTopHalfInwardCircleContact(const SurfaceContact& _Contact)
{
    if (_Contact.Surface == nullptr)
        return false;

    Vec2 center = {};
    float radius = 0.f;
    const CSurfaceCircleGuideScript* pGuide = nullptr;
    const bool hasGuide = TryGetInwardCircleGuideData(_Contact, center, radius, pGuide) &&
        pGuide != nullptr &&
        radius > 0.0001f;

    if (hasGuide)
    {
        const auto isLinkedCorrectionLineSurface = [&](GameObject* _SurfaceObject)
        {
            if (_SurfaceObject == nullptr || _SurfaceObject == _Contact.Surface)
                return false;

            auto pLineSurface = _SurfaceObject->GetScript<CSurfaceScript>();
            if (pLineSurface == nullptr ||
                pLineSurface->GetGeometry() != CSurfaceScript::SURFACE_GEOMETRY::LINE ||
                !pLineSurface->IsAttachable() ||
                pLineSurface->GetRole() == CSurfaceScript::SURFACE_ROLE::WALL)
            {
                return false;
            }

            return (_SurfaceObject->GetName() == pGuide->GetLinkedCorrectionLineName());
        };

        const bool hasLinkedCorrectionLineContact =
            isLinkedCorrectionLineSurface(m_bHasCurrentSurfaceContact ? m_CurrentSurfaceContact.Surface : nullptr) ||
            isLinkedCorrectionLineSurface(m_bHasPendingSurfaceContact ? m_PendingSurfaceContact.Surface : nullptr);

        if (!hasLinkedCorrectionLineContact)
            return false;

        if (ShouldIgnoreGuidedInwardCircleContact(_Contact))
            return true;

        if (!IsTopHalfInwardCircleContact(_Contact))
            return false;

        if (!HasInwardCircleLineContext())
            return false;

        if (IsSameGuidedInwardCircleEntryState(center, radius) && !m_bGuidedInwardCircleEntryFromRight)
        {
            const Vec3 playerPos3 = Transform()->GetWorldPos();
            const Vec2 playerPos = Vec2(playerPos3.x, playerPos3.y);
            const Vec2 fallbackRadial = NormalizeSafeVec2(_Contact.ContactPoint - center, Vec2(0.f, -1.f));
            const Vec2 playerRadial = NormalizeSafeVec2(playerPos - center, fallbackRadial);
            const Vec2 projectedPlayerPoint = center + playerRadial * radius;
            if (ShouldIgnoreGuidedInwardCirclePoint(center, radius, pGuide, projectedPlayerPoint))
                return true;
        }

        bool onActiveArc = false;
        bool beforeHalf = false;
        bool beforeHalfUsesTopRight = false;
        if (TryEvaluateGuidedTopHalfInwardCircleContact(_Contact, onActiveArc, beforeHalf, beforeHalfUsesTopRight))
            return !onActiveArc;

        return false;
    }

    if (ShouldIgnoreGuidedInwardCircleContact(_Contact))
        return true;

    if (!IsTopHalfInwardCircleContact(_Contact))
        return false;

    if (!HasInwardCircleLineContext())
        return false;

    auto pSurface = _Contact.Surface->GetScript<CSurfaceScript>();
    if (pSurface == nullptr)
        return false;

    Vec2 boxMin = {};
    Vec2 boxMax = {};
    pSurface->GetArcWorldData(center, radius, boxMin, boxMax);

    const auto isChordLikeLineSurface = [&](GameObject* _SurfaceObject)
    {
        if (_SurfaceObject == nullptr)
            return false;

        auto pLineSurface = _SurfaceObject->GetScript<CSurfaceScript>();
        if (pLineSurface == nullptr ||
            pLineSurface->GetGeometry() != CSurfaceScript::SURFACE_GEOMETRY::LINE ||
            !pLineSurface->IsAttachable())
        {
            return false;
        }

        Vec2 lineStart = {};
        Vec2 lineEnd = {};
        pLineSurface->GetWorldEndpoints(lineStart, lineEnd);

        const float safeRadius = max(radius, 0.0001f);
        const float endpointTolerance = max(8.f, safeRadius * 0.08f);
        const float insideMargin = max(4.f, safeRadius * 0.03f);
        const float startRadiusError = fabsf(LengthVec2(lineStart - center) - safeRadius);
        const float endRadiusError = fabsf(LengthVec2(lineEnd - center) - safeRadius);
        const Vec2 lineMid = (lineStart + lineEnd) * 0.5f;
        const float midRadius = LengthVec2(lineMid - center);

        return (startRadiusError <= endpointTolerance) &&
            (endRadiusError <= endpointTolerance) &&
            (midRadius < safeRadius - insideMargin);
    };

    if (m_bHasCurrentSurfaceContact && isChordLikeLineSurface(m_CurrentSurfaceContact.Surface))
        return false;

    if (m_bHasPendingSurfaceContact && isChordLikeLineSurface(m_PendingSurfaceContact.Surface))
        return false;

    for (GameObject* pSurfaceObject : m_vecActiveSurfaceObjects)
    {
        if (isChordLikeLineSurface(pSurfaceObject))
            return false;
    }

    UpdateInwardCircleHalfChecker(center, radius, _Contact.ContactPoint);
    const bool lineOwnsTopRight = ShouldLineOwnTopRightInwardCircleHalf(center, radius, _Contact.ContactPoint);
    if (pSurface->GetGeometry() == CSurfaceScript::SURFACE_GEOMETRY::FULL_CIRCLE)
    {
        if (lineOwnsTopRight)
            return (_Contact.ContactPoint.x >= center.x - kSurfaceInwardCircleTopHalfSwitchEpsilon);

        return (_Contact.ContactPoint.x <= center.x + kSurfaceInwardCircleTopHalfSwitchEpsilon);
    }

    const CSurfaceScript::ARC_CORNER corner = pSurface->GetArcCorner();
    if (lineOwnsTopRight)
        return (corner == CSurfaceScript::ARC_CORNER::TOP_RIGHT);

    return (corner == CSurfaceScript::ARC_CORNER::TOP_LEFT);
}

bool CPlayerScript::IsChordLineForInwardCircle(const SurfaceContact& _LineContact, const SurfaceContact& _CircleContact) const
{
    if (_LineContact.Surface == nullptr || _CircleContact.Surface == nullptr)
        return false;

    if (IsGuideLinkedCorrectionLine(_LineContact, _CircleContact))
        return true;

    auto pLineSurface = _LineContact.Surface->GetScript<CSurfaceScript>();
    auto pCircleSurface = _CircleContact.Surface->GetScript<CSurfaceScript>();
    if (pLineSurface == nullptr || pCircleSurface == nullptr)
        return false;

    if (pLineSurface->GetGeometry() != CSurfaceScript::SURFACE_GEOMETRY::LINE ||
        !IsCircleGeometry(pCircleSurface->GetGeometry()))
    {
        return false;
    }

    Vec2 lineStart = {};
    Vec2 lineEnd = {};
    pLineSurface->GetWorldEndpoints(lineStart, lineEnd);

    Vec2 center = {};
    float radius = 0.f;
    Vec2 boxMin = {};
    Vec2 boxMax = {};
    pCircleSurface->GetArcWorldData(center, radius, boxMin, boxMax);

    const float safeRadius = max(radius, 0.0001f);
    const float endpointTolerance = max(8.f, safeRadius * 0.08f);
    const float insideMargin = max(4.f, safeRadius * 0.03f);
    const float startRadiusError = fabsf(LengthVec2(lineStart - center) - safeRadius);
    const float endRadiusError = fabsf(LengthVec2(lineEnd - center) - safeRadius);
    const Vec2 lineMid = (lineStart + lineEnd) * 0.5f;
    const float midRadius = LengthVec2(lineMid - center);

    return (startRadiusError <= endpointTolerance) &&
        (endRadiusError <= endpointTolerance) &&
        (midRadius < safeRadius - insideMargin);
}

bool CPlayerScript::ShouldPreferLineOverTopHalfInwardCircle(const SurfaceContact& _LineContact, const SurfaceContact& _CircleContact)
{
    if (_LineContact.Circle || _LineContact.WallLike || !_LineContact.Attachable)
        return false;

    if (!IsTopHalfInwardCircleContact(_CircleContact))
        return false;

    const bool guideLinkedCorrectionLine = IsGuideLinkedCorrectionLine(_LineContact, _CircleContact);

    if (!ShouldIgnoreTopHalfInwardCircleContact(_CircleContact))
        return false;

    const Vec2 lineNormal = NormalizeSafeVec2(_LineContact.Normal);
    const Vec2 circleNormal = NormalizeSafeVec2(_CircleContact.Normal, lineNormal);
    if (!guideLinkedCorrectionLine && DotVec2(lineNormal, circleNormal) <= 0.1f)
        return false;

    if (guideLinkedCorrectionLine)
        return true;

    if (!IsChordLineForInwardCircle(_LineContact, _CircleContact))
        return true;

    return (fabsf(_LineContact.SignedDistance) <= kSurfaceLineAttachSnapMaxDistance);
}

bool CPlayerScript::ShouldPreferTopHalfInwardCircleOverLine(const SurfaceContact& _CircleContact, const SurfaceContact& _LineContact)
{
    if (!IsTopHalfInwardCircleContact(_CircleContact))
        return false;

    const bool guideLinkedCorrectionLine = IsGuideLinkedCorrectionLine(_LineContact, _CircleContact);

    if (ShouldIgnoreTopHalfInwardCircleContact(_CircleContact))
        return false;

    if (_LineContact.Circle || _LineContact.WallLike || !_LineContact.Attachable)
        return false;

    const Vec2 lineNormal = NormalizeSafeVec2(_LineContact.Normal);
    const Vec2 circleNormal = NormalizeSafeVec2(_CircleContact.Normal, lineNormal);
    if (!guideLinkedCorrectionLine && DotVec2(lineNormal, circleNormal) <= 0.1f)
        return false;

    if (!IsChordLineForInwardCircle(_LineContact, _CircleContact))
        return false;

    if (guideLinkedCorrectionLine)
        return true;

    return true;
}

void CPlayerScript::DrawGuidedInwardCircleDebug()
{
    if (LevelMgr::GetInst()->GetLevelState() != LEVEL_STATE::PLAY)
        return;

    const float debugLife = 0.05f;
    const float debugLineThickness = 4.f;

    const auto isGuidedCircleSurfaceObject =
        [](GameObject* _SurfaceObject, CSurfaceScript*& _OutSurface, CSurfaceCircleGuideScript*& _OutGuide)
    {
        _OutSurface = nullptr;
        _OutGuide = nullptr;

        if (_SurfaceObject == nullptr)
            return false;

        _OutSurface = _SurfaceObject->GetScript<CSurfaceScript>().Get();
        _OutGuide = _SurfaceObject->GetScript<CSurfaceCircleGuideScript>().Get();
        if (_OutSurface == nullptr ||
            _OutGuide == nullptr ||
            !_OutGuide->IsEnabled() ||
            !IsCircleGeometry(_OutSurface->GetGeometry()) ||
            !_OutSurface->GetFillInside())
        {
            _OutSurface = nullptr;
            _OutGuide = nullptr;
            return false;
        }

        return true;
    };

    const auto isLineSurfaceObject = [](GameObject* _SurfaceObject)
    {
        if (_SurfaceObject == nullptr)
            return false;

        auto pSurface = _SurfaceObject->GetScript<CSurfaceScript>();
        return pSurface != nullptr &&
            pSurface->GetGeometry() == CSurfaceScript::SURFACE_GEOMETRY::LINE;
    };

    const auto findLineSurfaceByName = [&](const wstring& _Name) -> GameObject*
    {
        if (_Name.empty())
            return nullptr;

        const auto tryMatch = [&](GameObject* _SurfaceObject) -> GameObject*
        {
            if (_SurfaceObject == nullptr || _SurfaceObject->GetName() != _Name)
                return nullptr;

            return isLineSurfaceObject(_SurfaceObject) ? _SurfaceObject : nullptr;
        };

        if (GameObject* pMatch = tryMatch(m_bHasCurrentSurfaceContact ? m_CurrentSurfaceContact.Surface : nullptr))
            return pMatch;

        if (GameObject* pMatch = tryMatch(m_bHasPendingSurfaceContact ? m_PendingSurfaceContact.Surface : nullptr))
            return pMatch;

        for (GameObject* pSurfaceObject : m_vecActiveSurfaceObjects)
        {
            if (GameObject* pMatch = tryMatch(pSurfaceObject))
                return pMatch;
        }

        return nullptr;
    };

    SurfaceContact circleContact = {};
    bool hasCircleContact = false;
    GameObject* pCircleSurfaceObject = nullptr;
    GameObject* pLineSurfaceObject = nullptr;

    const auto tryUseCircleContact = [&](const SurfaceContact& _Contact) -> bool
    {
        if (!_Contact.Circle || !_Contact.InwardCircle || _Contact.Surface == nullptr)
            return false;

        CSurfaceScript* pSurface = nullptr;
        CSurfaceCircleGuideScript* pGuide = nullptr;
        if (!isGuidedCircleSurfaceObject(_Contact.Surface, pSurface, pGuide))
            return false;

        pCircleSurfaceObject = _Contact.Surface;
        pLineSurfaceObject = findLineSurfaceByName(pGuide->GetLinkedCorrectionLineName());
        circleContact = _Contact;
        hasCircleContact = true;
        return true;
    };

    const auto findGuidedCircleByLinkedLine = [&](const wstring& _LineName) -> GameObject*
    {
        if (_LineName.empty())
            return nullptr;

        const auto tryMatch = [&](GameObject* _SurfaceObject) -> GameObject*
        {
            CSurfaceScript* pSurface = nullptr;
            CSurfaceCircleGuideScript* pGuide = nullptr;
            if (!isGuidedCircleSurfaceObject(_SurfaceObject, pSurface, pGuide))
                return nullptr;

            return (pGuide->GetLinkedCorrectionLineName() == _LineName) ? _SurfaceObject : nullptr;
        };

        if (GameObject* pMatch = tryMatch(m_bHasCurrentSurfaceContact ? m_CurrentSurfaceContact.Surface : nullptr))
            return pMatch;

        if (GameObject* pMatch = tryMatch(m_bHasPendingSurfaceContact ? m_PendingSurfaceContact.Surface : nullptr))
            return pMatch;

        for (GameObject* pSurfaceObject : m_vecActiveSurfaceObjects)
        {
            if (GameObject* pMatch = tryMatch(pSurfaceObject))
                return pMatch;
        }

        return nullptr;
    };

    const auto tryUseLineContact = [&](const SurfaceContact& _Contact) -> bool
    {
        if (_Contact.Surface == nullptr || _Contact.Circle || _Contact.WallLike || !_Contact.Attachable)
            return false;

        if (!isLineSurfaceObject(_Contact.Surface))
            return false;

        GameObject* pMatchedCircle = findGuidedCircleByLinkedLine(_Contact.Surface->GetName());
        if (pMatchedCircle == nullptr)
            return false;

        pCircleSurfaceObject = pMatchedCircle;
        pLineSurfaceObject = _Contact.Surface;
        return true;
    };

    if (!tryUseCircleContact(m_CurrentSurfaceContact) &&
        !tryUseCircleContact(m_PendingSurfaceContact) &&
        !tryUseLineContact(m_CurrentSurfaceContact) &&
        !tryUseLineContact(m_PendingSurfaceContact))
    {
        return;
    }

    if (pCircleSurfaceObject == nullptr)
        return;

    auto pCircleSurface = pCircleSurfaceObject->GetScript<CSurfaceScript>();
    auto pGuide = pCircleSurfaceObject->GetScript<CSurfaceCircleGuideScript>();
    if (pCircleSurface == nullptr || pGuide == nullptr)
        return;

    Vec2 center = {};
    float radius = 0.f;
    Vec2 boxMin = {};
    Vec2 boxMax = {};
    pCircleSurface->GetArcWorldData(center, radius, boxMin, boxMax);
    if (radius <= 0.0001f)
        return;

    const Vec2 lineStart = center + pGuide->GetCorrectionLineStartLocal();
    const Vec2 lineEnd = center + pGuide->GetCorrectionLineEndLocal();

    const float z = Transform()->GetRelativePos().z;
    DrawDebugLine2D(lineStart, lineEnd, debugLineThickness, Vec4(0.20f, 1.f, 0.35f, 1.f), debugLife, false, z);

    Vec2 pointA = lineStart;
    Vec2 pointB = lineEnd;
    if (pointB.x < pointA.x)
    {
        const Vec2 temp = pointA;
        pointA = pointB;
        pointB = temp;
    }

    DrawDebugCircle(Vec3(pointA.x, pointA.y, z), 6.f, Vec4(1.f, 1.f, 1.f, 1.f), debugLife, false);
    DrawDebugCircle(Vec3(pointB.x, pointB.y, z), 6.f, Vec4(1.f, 1.f, 1.f, 1.f), debugLife, false);

    const Vec2 halfPoint = MakeCirclePointRad(center, radius, DegreesToRadians(kSurfaceGuidedHalfCheckTopAngleDeg));
    Vec2 leftPoint = pointA;
    Vec2 rightPoint = pointB;
    bool passedHalf = false;
    bool lineOwnsRight = false;
    bool entryFromRight = false;
    Vec4 halfColor = Vec4(0.20f, 0.55f, 1.f, 1.f);
    if (TryGetGuidedInwardCircleRuntimeState(center, radius, pGuide.Get(),
                                             leftPoint, rightPoint,
                                             entryFromRight, passedHalf, lineOwnsRight))
    {
        halfColor = lineOwnsRight ? Vec4(1.f, 0.15f, 0.15f, 1.f) : Vec4(0.20f, 0.55f, 1.f, 1.f);
    }

    float activeArcStartAngle = 0.f;
    float activeArcEndAngle = 0.f;
    bool arcCounterClockwise = false;
    if (TryResolveGuidedInwardCircleHalfArc(center, radius,
                                            leftPoint, rightPoint, lineOwnsRight,
                                            activeArcStartAngle, activeArcEndAngle,
                                            arcCounterClockwise))
    {
        DrawDebugArc2D(center, radius, activeArcStartAngle, activeArcEndAngle, arcCounterClockwise,
                       debugLineThickness, Vec4(0.15f, 0.95f, 1.f, 1.f), debugLife, false, z);
    }

    const Vec2 lineOwnedPoint = lineOwnsRight ? rightPoint : leftPoint;
    const Vec2 circleOwnedPoint = lineOwnsRight ? leftPoint : rightPoint;
    DrawDebugCircle(Vec3(circleOwnedPoint.x, circleOwnedPoint.y, z), 5.f, Vec4(0.30f, 1.f, 0.40f, 1.f), debugLife, false);
    DrawDebugCircle(Vec3(halfPoint.x, halfPoint.y, z), 7.f, halfColor, debugLife, false);
    DrawDebugCircle(Vec3(lineOwnedPoint.x, lineOwnedPoint.y, z), 5.f, Vec4(1.f, 0.45f, 0.25f, 1.f), debugLife, false);
    const Vec2 arrowStart = halfPoint + Vec2(0.f, radius * 0.12f);
    const Vec2 arrowEnd = arrowStart + Vec2(lineOwnsRight ? radius * 0.28f : -radius * 0.28f, 0.f);
    DrawDebugArrow2D(arrowStart, arrowEnd, 3.f, halfColor, debugLife, false, z);

    Vec2 contactPoint = Vec2(0.f, 0.f);
    if (hasCircleContact)
    {
        contactPoint = circleContact.ContactPoint;
    }
    else
    {
        const Vec3 playerPos3 = Transform()->GetRelativePos();
        const Vec2 playerPos = Vec2(playerPos3.x, playerPos3.y);
        const Vec2 radial = NormalizeSafeVec2(playerPos - center, Vec2(0.f, -1.f));
        contactPoint = center + radial * radius;
    }

    DrawDebugCircle(Vec3(contactPoint.x, contactPoint.y, z), 4.f, Vec4(1.f, 0.25f, 0.25f, 1.f), debugLife, false);
}

bool CPlayerScript::TrySelectSurfaceContact(SurfaceContact _Contact, SurfaceContact& _BestContact,
                                            float& _BestScore, GameObject*& _BestSurface, bool& _HasBest)
{
    _Contact.Score = ComputeSurfaceCandidateScore(_Contact, _BestSurface);

    // =========================================================================
    // [🔥 핵심 방어 코드 🔥]
    // CBlockScript(일반 타일/블록)를 밟고 있을 때, 원형 지형이 억지로 끌어당기는 것을 막습니다.
    if (_Contact.Circle && HasPhysicalGroundOverlap())
    {
        // 현재 플레이어가 원형 지형을 이미 정상적으로 타고 있는 상태가 아니라면
        bool isAlreadyOnCircle = (m_bHasCurrentSurfaceContact && m_CurrentSurfaceContact.Circle);

        // 거리가 조금이라도 멀면(예: 2.0픽셀 이상) 원의 판정을 무시해버립니다.
        if (!isAlreadyOnCircle && fabsf(_Contact.SignedDistance) > 2.0f)
        {
            return false;
        }
    }
    // =========================================================================

    if (!_Contact.Circle && !_Contact.WallLike && _Contact.Attachable)
        PrimeInwardCircleHalfCheckerFromLineContext(_Contact);

    if (ShouldIgnoreTopHalfInwardCircleContact(_Contact))
        return false;

    const bool guidedCorrectionLineContact = IsGuidedCorrectionLineContact(_Contact);
    const bool bestGuidedCorrectionLineContact =
        _HasBest && IsGuidedCorrectionLineContact(_BestContact);

    bool accept = false;
    if (!_HasBest)
    {
        accept = true;
    }
    else if (guidedCorrectionLineContact != bestGuidedCorrectionLineContact)
    {
        accept = guidedCorrectionLineContact;
    }
    else if (ShouldIgnoreTopHalfInwardCircleContact(_BestContact))
    {
        accept = true;
    }
    else if (ShouldPreferLineOverTopHalfInwardCircle(_Contact, _BestContact))
    {
        accept = true;
    }
    else if (ShouldPreferLineOverTopHalfInwardCircle(_BestContact, _Contact))
    {
        accept = false;
    }
    else if (ShouldPreferTopHalfInwardCircleOverLine(_Contact, _BestContact))
    {
        accept = true;
    }
    else if (ShouldPreferTopHalfInwardCircleOverLine(_BestContact, _Contact))
    {
        accept = false;
    }
    else if (_BestSurface == _Contact.Surface)
    {
        accept = (_Contact.Score <= _BestScore + kSurfaceResolveScoreEpsilon);
    }
    else if (_Contact.Score + kSurfaceResolveScoreEpsilon < _BestScore)
    {
        accept = true;
    }

    if (!accept)
        return false;

    _BestContact = _Contact;
    _BestScore = _Contact.Score;
    _BestSurface = _Contact.Surface;
    _HasBest = true;
    return true;
}

bool CPlayerScript::ProbeSurfaceObject(GameObject* _SurfaceObject, bool _WasGround,
                                       SurfaceContact& _BestContact, float& _BestScore,
                                       GameObject*& _BestSurface, bool& _HasBest)
{
    if (_SurfaceObject == nullptr || _SurfaceObject->IsDead())
        return false;

    auto pSurface = _SurfaceObject->GetScript<CSurfaceScript>();
    if (pSurface == nullptr)
        return false;

    if (pSurface->GetRole() == CSurfaceScript::SURFACE_ROLE::WALL)
        return false;

    CSurfaceScript::CONTACT_PROBE probe = {};
    if (!pSurface->ProbePlayerContact(Collider2D(), _WasGround, probe))
        return false;

    SurfaceContact contact = {};
    contact.Surface = _SurfaceObject;
    contact.Normal = probe.Normal;
    contact.SignedDistance = probe.SignedDistance;
    contact.Score = probe.CandidateScore;
    contact.TransitionSurface = probe.TransitionSurface;
    contact.Attachable = probe.Attachable;
    contact.WallLike = probe.WallLike;
    contact.Circle = probe.Circle;
    contact.InwardCircle = probe.InwardCircle;
    contact.SeamBlendT = probe.SeamBlendT;
    contact.SeamBlendHasValue = probe.SeamBlendHasValue;
    contact.SeamStart = probe.SeamStart;
    contact.SeamEnd = probe.SeamEnd;
    contact.ContactPoint = probe.ContactPoint;

    const bool isCurrentSurface =
        m_bHasCurrentSurfaceContact &&
        (m_CurrentSurfaceContact.Surface == _SurfaceObject);
    const bool inactiveLineSurface =
        contact.Attachable &&
        !contact.WallLike &&
        !contact.Circle &&
        !IsActiveSurfaceObject(_SurfaceObject);
    const float surfaceNormalVelocity = DotVec2(vVelocity, contact.Normal);
    const bool staleDetachedLineContact =
        inactiveLineSurface &&
        ((isCurrentSurface && contact.SignedDistance > kSurfaceDetachedLinePositiveThreshold) ||
         (!GetIsGround() &&
          m_SurfaceGroundHoldTime <= 0.f &&
          surfaceNormalVelocity < -kSurfaceDetachedLineFallSpeedMin));
    const bool connectedTransitionLine = IsConnectedTransitionLineContact(contact);
    const bool relaxedDownwardTransitionLine =
        !connectedTransitionLine &&
        IsRelaxedDownwardTransitionLineContact(contact);
    const bool unrelatedGroundedPositiveLineContact =
        _WasGround &&
        contact.Attachable &&
        !contact.WallLike &&
        !contact.Circle &&
        !isCurrentSurface &&
        !connectedTransitionLine &&
        !relaxedDownwardTransitionLine &&
        (contact.SignedDistance > kSurfaceLineUnconnectedPositiveRejectDistance);

    if (staleDetachedLineContact || unrelatedGroundedPositiveLineContact)
        return false;

    return TrySelectSurfaceContact(contact, _BestContact, _BestScore, _BestSurface, _HasBest);
}

void CPlayerScript::ProbeSceneSurfaceContacts(bool _WasGround, SurfaceContact& _BestContact, float& _BestScore,
                                              GameObject*& _BestSurface, bool& _HasBest)
{
    Vec2 queryMin = {};
    Vec2 queryMax = {};
    const bool hasQueryBounds = GetColliderQueryBounds(Collider2D(), queryMin, queryMax);
    if (!hasQueryBounds)
        return;

    std::vector<GameObject*> nearbySurfaceObjects;
    CSurfaceScript::QueryNearbySurfaceObjects(queryMin, queryMax, nearbySurfaceObjects, kSurfaceSceneProbeMargin, false);

    for (GameObject* pSurfaceObject : nearbySurfaceObjects)
        ProbeSurfaceObject(pSurfaceObject, _WasGround, _BestContact, _BestScore, _BestSurface, _HasBest);
}

void CPlayerScript::ResolveBufferedSurfaceContacts()
{
    const bool wasGround = GetIsGround() || m_SurfaceGroundHoldTime > 0.f;
    RefreshNearbyAttachableSurfaces();
    if (IsSurfaceAutoplayScenario() && m_AutoplayTime < 0.5f)
        AppendSurfaceAutoplayDebug(L"resolve begin t=%.3f wasGround=%d active=%zu pending=%d current=%d",
                                   m_AutoplayTime, wasGround ? 1 : 0, m_vecActiveSurfaceObjects.size(),
                                   m_bHasPendingSurfaceContact ? 1 : 0, m_bHasCurrentSurfaceContact ? 1 : 0);
    SurfaceContact bestContact = {};
    float bestScore = 999999.f;
    GameObject* bestSurface = nullptr;
    bool hasBest = false;

    if (m_bHasPendingSurfaceContact)
        ProbeSurfaceObject(m_PendingSurfaceContact.Surface, wasGround, bestContact, bestScore, bestSurface, hasBest);

    if (m_bHasCurrentSurfaceContact)
        ProbeSurfaceObject(m_CurrentSurfaceContact.Surface, wasGround, bestContact, bestScore, bestSurface, hasBest);

    for (GameObject* pSurfaceObject : m_vecActiveSurfaceObjects)
        ProbeSurfaceObject(pSurfaceObject, wasGround, bestContact, bestScore, bestSurface, hasBest);

    if (!hasBest &&
        m_SurfaceGroundHoldTime <= 0.f)
    {
        ProbeSceneSurfaceContacts(wasGround, bestContact, bestScore, bestSurface, hasBest);
    }

    m_bHasPendingSurfaceContact = false;

    if (hasBest)
    {
        if (IsSurfaceAutoplayScenario() && m_AutoplayTime < 0.5f)
        {
            const wchar_t* surfaceName = (bestContact.Surface != nullptr) ? bestContact.Surface->GetName().c_str() : L"-";
            AppendSurfaceAutoplayDebug(L"resolve apply t=%.3f surface=%ls dist=%.3f normal=(%.3f,%.3f)",
                                       m_AutoplayTime, surfaceName, bestContact.SignedDistance, bestContact.Normal.x, bestContact.Normal.y);
        }
        if (ApplySurfaceContact(bestContact))
        {
            m_bHasCurrentSurfaceContact = true;
            m_CurrentSurfaceContact = bestContact;
        }
    }
    else if (m_SurfaceGroundHoldTime <= 0.f && !HasAnyGroundOverlap())
    {
        if (IsSurfaceAutoplayScenario() && m_AutoplayTime < 0.5f)
            AppendSurfaceAutoplayDebug(L"resolve reset t=%.3f", m_AutoplayTime);
        ResetGroundContact();
    }
}

void CPlayerScript::SubmitSurfaceContact(GameObject* _Surface, const Vec2& _Normal, float _SignedDistance,
                                         bool _TransitionSurface, bool _Attachable, bool _WallLike, bool _Circle, bool _InwardCircle, float _Score,
                                         float _SeamBlendT, bool _SeamBlendHasValue, const Vec2& _SeamStart,
                                         const Vec2& _SeamEnd, const Vec2& _ContactPoint)
{
    if (_Surface == nullptr)
        return;

    if (_Attachable && m_ForcedFlatGroundLockTime > 0.f && !IsJump)
        return;

    if (fabsf(m_SurfaceResolveFrame - E_Time) > kSurfaceResolveFrameEpsilon)
    {
        m_SurfaceResolveFrame = E_Time;
        m_SurfaceResolveScore = 999999.f;
        m_pResolvedSurface = nullptr;
        m_bHasPendingSurfaceContact = false;
    }

    SurfaceContact contact = {};
    contact.Surface = _Surface;
    contact.Normal = _Normal;
    contact.SignedDistance = _SignedDistance;
    contact.Score = _Score;
    contact.TransitionSurface = _TransitionSurface;
    contact.Attachable = _Attachable;
    contact.WallLike = _WallLike;
    contact.Circle = _Circle;
    contact.InwardCircle = _InwardCircle;
    contact.SeamBlendT = _SeamBlendT;
    contact.SeamBlendHasValue = _SeamBlendHasValue;
    contact.SeamStart = _SeamStart;
    contact.SeamEnd = _SeamEnd;
    contact.ContactPoint = _ContactPoint;

    TrySelectSurfaceContact(contact, m_PendingSurfaceContact, m_SurfaceResolveScore, m_pResolvedSurface, m_bHasPendingSurfaceContact);
}

bool CPlayerScript::ApplySurfaceContact(const SurfaceContact& _Contact)
{
    Vec2 normal = NormalizeSafeVec2(_Contact.Normal);

    // =========================================================
    // [추가된 순수 벽(Pure Wall / AABB) 처리 로직]
    // 벽(WallLike)이면서 파고들었을 경우(음수 거리), 즉시 밖으로 밀어냅니다.
    if (_Contact.WallLike && _Contact.SignedDistance < 0.f)
    {
        // 1. 파고든 깊이만큼 정확히 위치(Position)를 밀어냄 (AABB Resolution)
        Vec3 pos = Transform()->GetRelativePos();
        pos.x += normal.x * fabsf(_Contact.SignedDistance);
        pos.y += normal.y * fabsf(_Contact.SignedDistance);
        Transform()->SetRelativePos(pos);

        // 2. 벽 방향으로 향하는 속도를 0으로 만들어 뚫고 지나가는 것을 방지
        if (DotVec2(vVelocity, normal) < 0.f)
        {
            if (fabsf(normal.x) > 0.5f) vVelocity.x = 0.f; // 좌우 수직 벽에 부딪힘
            if (normal.y < -0.5f)       vVelocity.y = 0.f; // 천장에 머리를 박음 (점프 차단)
        }

        // 벽은 바닥처럼 타고 달리는 지형이 아니므로, 각도 회전이나 부착(Stick) 없이 즉시 종료
        return true;
    }

    const bool wasGround = GetIsGround();
    const PlayerInput surfaceInput = ReadInput();
    const bool jumpPressed = surfaceInput.spacePressed || surfaceInput.spaceHeld || IsJump;
    const bool isAttachableLineContact = _Contact.Attachable && !_Contact.WallLike && !_Contact.Circle;
    const bool isCurrentAttachableLine =
        m_bHasCurrentSurfaceContact &&
        m_CurrentSurfaceContact.Surface != nullptr &&
        m_CurrentSurfaceContact.Attachable &&
        !m_CurrentSurfaceContact.WallLike &&
        !m_CurrentSurfaceContact.Circle;
    const bool isSameCurrentLineSurface =
        isAttachableLineContact &&
        isCurrentAttachableLine &&
        (m_CurrentSurfaceContact.Surface == _Contact.Surface);
    const bool isConnectedTransitionLineContact =
        isAttachableLineContact &&
        _Contact.TransitionSurface &&
        (IsConnectedTransitionLineContact(_Contact) ||
         IsRelaxedDownwardTransitionLineContact(_Contact));
    float connectedTransitionLineEndpointGap = -1.f;
    if (isConnectedTransitionLineContact)
    {
        const SurfaceContact* pReferenceLineContact = GetReferenceAttachableLineContact();
        if (pReferenceLineContact != nullptr)
            connectedTransitionLineEndpointGap = GetTransitionLineEndpointGap(*pReferenceLineContact, _Contact);
    }
    const bool isRecentGuidedCorrectionLineContact =
        isAttachableLineContact &&
        IsRecentGuidedCorrectionLineSurface(_Contact.Surface);
    const bool isRecentChordLineContact =
        isAttachableLineContact &&
        m_bHasRecentReleasedInwardCircleContact &&
        m_RecentReleasedInwardCircleTime > 0.f &&
        IsChordLineForInwardCircle(_Contact, m_RecentReleasedInwardCircleContact);
    const bool isGuideLinkedCorrectionLineContact =
        isAttachableLineContact &&
        (isRecentGuidedCorrectionLineContact ||
         (m_bHasCurrentSurfaceContact &&
          m_CurrentSurfaceContact.Surface != nullptr &&
          m_CurrentSurfaceContact.Circle &&
          IsGuideLinkedCorrectionLine(_Contact, m_CurrentSurfaceContact)) ||
         (m_bHasPendingSurfaceContact &&
          m_PendingSurfaceContact.Surface != nullptr &&
          m_PendingSurfaceContact.Circle &&
          IsGuideLinkedCorrectionLine(_Contact, m_PendingSurfaceContact)));
    const bool isForcedChordCorrectionLineContact =
        isGuideLinkedCorrectionLineContact || isRecentChordLineContact;
    const bool bForceGuidedCorrectionLineLock =
        isForcedChordCorrectionLineContact &&
        !jumpPressed &&
        (isRecentGuidedCorrectionLineContact ||
         isRecentChordLineContact ||
         wasGround ||
         m_SurfaceGroundHoldTime > 0.f ||
         (m_bHasCurrentSurfaceContact && m_CurrentSurfaceContact.Circle) ||
         (m_bHasPendingSurfaceContact && m_PendingSurfaceContact.Circle));
    const bool allowLineCorrection =
        isAttachableLineContact &&
        (isSameCurrentLineSurface || isConnectedTransitionLineContact || isForcedChordCorrectionLineContact);

    bool inwardCircleContact = false;
    Vec2 inwardCircleCenter = Vec2(0.f, 0.f);
    float inwardCircleRadius = 0.f;
    float inwardCircleUpperHalfRatio = 0.f;
    float inwardCircleLowerHalfRatio = 0.f;
    bool inwardCircleSupportsImmediateLowSpeedFall = false;

    bool continuingCurrentInwardCircle =
        m_bHasCurrentSurfaceContact &&
        m_CurrentSurfaceContact.Surface != nullptr &&
        m_CurrentSurfaceContact.Circle &&
        m_CurrentSurfaceContact.InwardCircle &&
        (_Contact.Circle && _Contact.InwardCircle);

    if (continuingCurrentInwardCircle)
    {
        if (m_CurrentSurfaceContact.Surface == _Contact.Surface)
        {
            continuingCurrentInwardCircle = true;
        }
        else
        {
            Vec2 currentCircleCenter = {};
            float currentCircleRadius = 0.f;
            Vec2 nextCircleCenter = {};
            float nextCircleRadius = 0.f;
            continuingCurrentInwardCircle =
                TryGetInwardCircleSurfaceData(m_CurrentSurfaceContact, currentCircleCenter, currentCircleRadius) &&
                TryGetInwardCircleSurfaceData(_Contact, nextCircleCenter, nextCircleRadius) &&
                (fabsf(currentCircleCenter.x - nextCircleCenter.x) <= 0.5f) &&
                (fabsf(currentCircleCenter.y - nextCircleCenter.y) <= 0.5f) &&
                (fabsf(currentCircleRadius - nextCircleRadius) <= 0.5f);
        }
    }

    if (!_Contact.Circle || !_Contact.InwardCircle)
    {
        ResetInwardCircleLoopState();
    }
    else if (_Contact.Surface != nullptr)
    {
        auto pSurface = _Contact.Surface->GetScript<CSurfaceScript>();
        if (pSurface != nullptr)
        {
            Vec2 center = {};
            float radius = 0.f;
            Vec2 boxMin = {};
            Vec2 boxMax = {};
            pSurface->GetArcWorldData(center, radius, boxMin, boxMax);

            inwardCircleContact = true;
            inwardCircleCenter = center;
            inwardCircleRadius = radius;
            inwardCircleSupportsImmediateLowSpeedFall =
                (pSurface->GetGeometry() == CSurfaceScript::SURFACE_GEOMETRY::FULL_CIRCLE);
            const float safeRadius = max(radius, 0.0001f);
            inwardCircleUpperHalfRatio = (center.y - _Contact.ContactPoint.y) / safeRadius;
            inwardCircleLowerHalfRatio = (_Contact.ContactPoint.y - center.y) / safeRadius;

            if (IsInwardCircleAttachBlocked(center, radius))
                return false;

            const CSurfaceCircleGuideScript* pGuide = nullptr;
            if (TryGetInwardCircleGuideData(_Contact, center, radius, pGuide) &&
                pGuide != nullptr)
            {
                Vec2 guidedEntryPoint = {};
                Vec2 guidedExitPoint = {};
                float guidedEntryAngle = 0.f;
                float guidedExitAngle = 0.f;
                TryResolveGuidedInwardCircleEntryExit(center, radius, pGuide,
                    guidedEntryPoint, guidedExitPoint, guidedEntryAngle, guidedExitAngle,
                    !continuingCurrentInwardCircle);
            }

            if (pGuide == nullptr)
                UpdateInwardCircleHalfChecker(center, radius, _Contact.ContactPoint);
            NoteInwardCircleLoopProgress(center, radius, _Contact.ContactPoint);
            UpdateGuidedInwardCircleHalfPhase(_Contact, continuingCurrentInwardCircle);
            const bool shouldReleaseGuidedCircle =
                !jumpPressed &&
                continuingCurrentInwardCircle &&
                ShouldReleaseGuidedInwardCircle(_Contact, center, radius);
            const bool shouldReleaseLoopCircle =
                !jumpPressed &&
                continuingCurrentInwardCircle &&
                ShouldReleaseInwardCircle(center, radius, _Contact.ContactPoint);
            const bool shouldPreReleaseLoopCircle =
                !jumpPressed &&
                continuingCurrentInwardCircle &&
                m_bInwardCircleLoopTracked &&
                IsSameInwardCircleLoop(center, radius) &&
                m_bInwardCirclePassedLowerHalf &&
                (m_InwardCircleLoopAccumulatedAngle >= (kSurfaceInwardCircleLoopReleaseAngle - kSurfaceInwardCirclePreReleaseAngleMargin)) &&
                (inwardCircleUpperHalfRatio >= kSurfaceInwardCirclePreReleaseUpperHalfMin);
            if (shouldReleaseGuidedCircle || shouldReleaseLoopCircle || shouldPreReleaseLoopCircle)
            {
                BlockInwardCircleAttach(center, radius, kSurfaceInwardCircleAttachBlockTime);
                return false;
            }
        }
    }

    Vec2 oldNormal = vNormal;
    if (fabsf(oldNormal.x) < 0.0001f && fabsf(oldNormal.y) < 0.0001f)
        oldNormal = normal;
    else
        oldNormal = NormalizeSafeVec2(oldNormal, normal);

    float normalDot = DotVec2(oldNormal, normal);
    if (normalDot > 1.f) normalDot = 1.f;
    else if (normalDot < -1.f) normalDot = -1.f;

    const bool bStrongSteepLineRecovery =
        isAttachableLineContact &&
        !jumpPressed &&
        (_Contact.SignedDistance < 0.f) &&
        (wasGround || m_SurfaceGroundHoldTime > 0.f) &&
        (normalDot < 0.92f);

    float normalBlend = 1.f;
    if (_Contact.Circle && wasGround)
        normalBlend = kSurfaceCircleNormalBlend;

    if (wasGround && !_Contact.TransitionSurface)
        normalBlend = 1.f;

    if (_Contact.TransitionSurface)
        normalBlend = 1.f;

    const bool bLineSeamCandidate =
        _Contact.TransitionSurface &&
        !_Contact.WallLike &&
        !_Contact.Circle &&
        (!isAttachableLineContact || isSameCurrentLineSurface || isConnectedTransitionLineContact);

    Vec2 candidateTangent = Vec2(normal.y, -normal.x);
    const float candidateTangentSpeed = DotVec2(vVelocity, candidateTangent);
    const bool bLowSpeedInwardCircleDetachEligible =
        inwardCircleContact &&
        continuingCurrentInwardCircle &&
        m_bInwardCircleLoopTracked &&
        IsSameInwardCircleLoop(inwardCircleCenter, inwardCircleRadius) &&
        (m_InwardCircleLoopAccumulatedAngle >= kSurfaceInwardCircleLowSpeedDetachMinAngle);
    const bool bLowSpeedInwardCircleHighFallZone =
        inwardCircleSupportsImmediateLowSpeedFall &&
        (inwardCircleUpperHalfRatio >= kSurfaceInwardCircleLowSpeedFallUpperHalfMin) &&
        (normal.y <= kSurfaceInwardCircleLowSpeedFallMaxNormalY);
    const bool bLowSpeedInwardCircleDetach =
        bLowSpeedInwardCircleDetachEligible &&
        !jumpPressed &&
        bLowSpeedInwardCircleHighFallZone &&
        (fabsf(candidateTangentSpeed) < kSurfaceInwardCircleLowSpeedDetachSpeedMin);
    const float seamBlend = ComputeLineSeamBlend(
        _Contact.SeamBlendT,
        _Contact.SeamBlendHasValue,
        _Contact.TransitionSurface,
        _Contact.SeamStart,
        _Contact.SeamEnd,
        _Contact.ContactPoint,
        vVelocity.x);
    const bool bRisingSlopeTransition =
        bLineSeamCandidate &&
        wasGround &&
        (oldNormal.y > normal.y + 0.01f) &&
        (candidateTangentSpeed * candidateTangent.y > kSurfaceLineSeamRisingSlopeMoveMin) &&
        (fabsf(_Contact.SignedDistance) <= kSurfaceLineSeamTransitionDepth * 2.f);
    const bool bAggressiveRisingSlopeTransition = bRisingSlopeTransition;
    float lineTransitionBlend = seamBlend;
    lineTransitionBlend = Clamp01f(lineTransitionBlend * 2.0f);

    if (fabsf(vVelocity.x) > 20.f)
        lineTransitionBlend = powf(lineTransitionBlend, 0.85f);

    const float descendDeltaY = oldNormal.y - normal.y;
    const bool bFlatToSlopeTransition = bLineSeamCandidate &&
        wasGround &&
        (oldNormal.y > 0.88f) &&
        (normal.y < 0.93f) &&
        (fabsf(_Contact.SignedDistance) <= kSurfaceLineSeamTransitionDepth);

    const bool bSeamAngleShift = bLineSeamCandidate &&
        wasGround &&
        (normalDot < kSurfaceLineSeamAngleShiftDot) &&
        (fabsf(_Contact.SignedDistance) <= kSurfaceLineSeamTransitionDepth);

    const bool bDownwardSlopeTransition = bLineSeamCandidate &&
        wasGround &&
        (descendDeltaY > kSurfaceLineSeamDownwardDelta) &&
        fabsf(_Contact.SignedDistance) <= kSurfaceLineSeamTransitionDepth;

    const bool bSteepDownSlopeTransition = bDownwardSlopeTransition && fabsf(vVelocity.x) > 30.f;

    if (bFlatToSlopeTransition)
    {
        lineTransitionBlend = max(lineTransitionBlend, kSurfaceLineSeamFlatToSlopeBlendFloor);
    }
    else if (bSeamAngleShift)
    {
        lineTransitionBlend = max(lineTransitionBlend, kSurfaceLineSeamAngleShiftBlendFloor);
    }
    else if (bDownwardSlopeTransition)
    {
        lineTransitionBlend = max(lineTransitionBlend, kSurfaceLineSeamDownwardBlendFloor);
    }
    else if (fabsf(vVelocity.x) > 20.f && descendDeltaY > 0.04f)
    {
        lineTransitionBlend = max(lineTransitionBlend, kSurfaceLineSeamEnterDownGradeBonus * Clamp01f(descendDeltaY * 2.f));
        if (fabsf(vVelocity.x) > 400.f)
            lineTransitionBlend = max(lineTransitionBlend, 1.f);
    }
    else if (bLineSeamCandidate && _Contact.TransitionSurface && fabsf(vVelocity.x) > 20.f)
    {
        lineTransitionBlend = max(lineTransitionBlend, kSurfaceLineSeamEntryBlendFloor);
    }

    if (bSteepDownSlopeTransition)
    {
        lineTransitionBlend = max(lineTransitionBlend, kSurfaceLineSeamSteepDownBlendFloor);
    }

    if (bAggressiveRisingSlopeTransition)
        lineTransitionBlend = max(lineTransitionBlend, kSurfaceLineSeamRisingSlopeBlendFloor);

    const bool bLineSeamStick =
        !jumpPressed &&
        bLineSeamCandidate &&
        (wasGround || m_SurfaceGroundHoldTime > 0.f) &&
        (GetAction() == ActionState::None) &&
        fabsf(_Contact.SignedDistance) <= kSurfaceLineSeamTransitionDepth &&
        (normalDot >= kSurfaceLineSeamKeepDot);

    const bool bHoldLineSeam = bLineSeamStick &&
        !bDownwardSlopeTransition &&
        !bFlatToSlopeTransition &&
        !bSeamAngleShift;

    Vec2 curNormal = NormalizeSafeVec2(
        oldNormal * (1.f - normalBlend) + normal * normalBlend,
        normal);

    if (bLineSeamCandidate && _Contact.TransitionSurface)
    {
        const float blend = Clamp01f(lineTransitionBlend);
        curNormal = NormalizeSafeVec2(
            oldNormal * (1.f - blend) + normal * blend,
            (blend >= 0.5f) ? normal : oldNormal);
    }

    if (isGuideLinkedCorrectionLineContact || bForceGuidedCorrectionLineLock)
        curNormal = NormalizeSafeVec2(normal, curNormal);

    if (bLowSpeedInwardCircleDetach)
    {
        BlockInwardCircleAttach(inwardCircleCenter, inwardCircleRadius, kSurfaceInwardCircleLowSpeedDetachBlockTime);
        return false;
    }

    const float jumpSeparatingSpeed = DotVec2(vVelocity, curNormal);
    if (IsJump && !_Contact.WallLike && jumpSeparatingSpeed > kSurfaceJumpDetachSpeedMin)
    {
        SetIsGround(false);
        SetNormal(curNormal);
        SetGroundTangent(Vec2(curNormal.y, -curNormal.x));
        return true;
    }

    SetNormal(curNormal);

    Vec2 positionResolveNormal = curNormal;
    if (bLineSeamCandidate && _Contact.TransitionSurface)
        positionResolveNormal = NormalizeSafeVec2(normal, curNormal);

    if (isGuideLinkedCorrectionLineContact || bForceGuidedCorrectionLineLock)
        positionResolveNormal = NormalizeSafeVec2(normal, curNormal);

    float forcedGuidedLineFootCorrection = 0.f;
    if ((bForceGuidedCorrectionLineLock || bStrongSteepLineRecovery) &&
        _Contact.Surface != nullptr &&
        Collider2D() != nullptr)
    {
        auto pLineSurface = _Contact.Surface->GetScript<CSurfaceScript>();
        if (pLineSurface != nullptr)
        {
            Vec2 lineStart = {};
            Vec2 lineEnd = {};
            pLineSurface->GetWorldEndpoints(lineStart, lineEnd);
            CanonicalizeLineEndpoints(lineStart, lineEnd);

            const Vec2 lineDelta = lineEnd - lineStart;
            const float lineLength = LengthVec2(lineDelta);
            if (lineLength > 0.001f)
            {
                const Vec2 lineTangent = lineDelta / lineLength;
                const Vec2 lineNormal = NormalizeSafeVec2(positionResolveNormal, normal);

                Vec2 ownerCenter = {};
                Vec2 visualFootPoint = {};
                float visualFootDistance = 0.f;
                if (GetPlayerVisualFootDataWorld(Collider2D(), lineNormal, ownerCenter, visualFootPoint, visualFootDistance))
                {
                    const float footProjection = DotVec2(ownerCenter - lineStart, lineTangent);
                    const float projectionMargin = max(6.f, visualFootDistance + 4.f);
                    if (footProjection >= -projectionMargin && footProjection <= lineLength + projectionMargin)
                    {
                        const float footSignedDistance = DotVec2(visualFootPoint - lineStart, lineNormal);
                        if (footSignedDistance < 0.f)
                            forcedGuidedLineFootCorrection = -footSignedDistance + 0.001f;
                    }
                }
            }
        }
    }

    const float lineProjectionMaxDistance =
        isGuideLinkedCorrectionLineContact ? kSurfaceGuidedLineProjectionMaxDistance : kSurfaceLineStableProjectionMaxDistance;
    const bool hasGroundedSurfaceContext = wasGround || m_SurfaceGroundHoldTime > 0.f;
    const float circleProjectionMaxDistance =
        hasGroundedSurfaceContext ? kSurfaceCircleProjectionMaxDistance : kSurfaceCircleAirAcquireMaxDistance;
    const bool bAirborneCircleAcquire =
        !hasGroundedSurfaceContext &&
        _Contact.Circle &&
        _Contact.Attachable &&
        !_Contact.WallLike &&
        !_Contact.InwardCircle &&
        (fabsf(_Contact.SignedDistance) <= circleProjectionMaxDistance) &&
        (DotVec2(vVelocity, normal) <= 0.f) &&
        (normal.y >= 0.6f);
    const bool bDeepGuidedLineProjection =
        isGuideLinkedCorrectionLineContact &&
        (_Contact.SignedDistance < 0.f) &&
        (DotVec2(vVelocity, normal) <= 0.f) &&
        (hasGroundedSurfaceContext ||
         (m_bHasCurrentSurfaceContact && m_CurrentSurfaceContact.Circle));

    const bool bProjectToStableSurface =
        ((_Contact.Circle &&
          _Contact.Attachable &&
          !_Contact.WallLike &&
         (!_Contact.InwardCircle || !bLowSpeedInwardCircleDetach) &&
         (!_Contact.TransitionSurface || _Contact.Circle) &&
         (hasGroundedSurfaceContext || bAirborneCircleAcquire) &&
         (fabsf(_Contact.SignedDistance) <= circleProjectionMaxDistance)) ||
        (bForceGuidedCorrectionLineLock && (_Contact.SignedDistance <= 0.f || forcedGuidedLineFootCorrection > 0.f)) ||
        (bStrongSteepLineRecovery && forcedGuidedLineFootCorrection > 0.f) ||
        (allowLineCorrection &&
         (!_Contact.TransitionSurface || isGuideLinkedCorrectionLineContact || isSameCurrentLineSurface || isConnectedTransitionLineContact) &&
         (fabsf(_Contact.SignedDistance) <= lineProjectionMaxDistance || bDeepGuidedLineProjection)));

    bool bAppliedStableProjection = false;
    if (bProjectToStableSurface)
    {
        const float maxProjectionDistance =
            _Contact.Circle ? circleProjectionMaxDistance :
            ((bForceGuidedCorrectionLineLock || bStrongSteepLineRecovery) ? max(kSurfaceGuidedLineForceProjectionMaxDistance, forcedGuidedLineFootCorrection) :
             (bDeepGuidedLineProjection ? kSurfaceGuidedLineDeepProjectionMaxDistance : lineProjectionMaxDistance));
        float correction = max(-maxProjectionDistance, min(maxProjectionDistance, -_Contact.SignedDistance));

        if (bForceGuidedCorrectionLineLock || bStrongSteepLineRecovery)
            correction = max(correction, forcedGuidedLineFootCorrection);

        if (fabsf(correction) > 0.0001f)
        {
            Vec3 pos = Transform()->GetRelativePos();
            pos.x += positionResolveNormal.x * correction;
            pos.y += positionResolveNormal.y * correction;
            Transform()->SetRelativePos(pos);
            bAppliedStableProjection = true;
        }
    }

    const bool bAttachToLineSurface =
        allowLineCorrection &&
        _Contact.SeamBlendHasValue &&
        (wasGround || m_SurfaceGroundHoldTime > 0.f);

    if (!bAppliedStableProjection && bAttachToLineSurface && _Contact.SignedDistance > 0.f)
    {
        float snapDistance = _Contact.SignedDistance;
        const float maxSnapDistance =
            bAggressiveRisingSlopeTransition ? kSurfaceLineSeamRisingSlopeSnapMaxDistance : kSurfaceLineAttachSnapMaxDistance;
        float cappedSnapDistance = maxSnapDistance;
        if (isConnectedTransitionLineContact && connectedTransitionLineEndpointGap >= 0.f)
        {
            const float seamGapSnapLimit = connectedTransitionLineEndpointGap + 1.0f;
            cappedSnapDistance = min(cappedSnapDistance, seamGapSnapLimit);
        }
        if (snapDistance > cappedSnapDistance)
            snapDistance = cappedSnapDistance;

        Vec3 pos = Transform()->GetRelativePos();
        pos.x -= positionResolveNormal.x * snapDistance;
        pos.y -= positionResolveNormal.y * snapDistance;
        Transform()->SetRelativePos(pos);
    }
    else if (!bAppliedStableProjection &&
             _Contact.SignedDistance < 0.f &&
             (!_Contact.Circle ? allowLineCorrection && fabsf(_Contact.SignedDistance) <= kSurfaceLinePenetrationPushMaxDistance : true))
    {
        float worldPush = -_Contact.SignedDistance + kSurfaceContactPushBias;
        float maxPush = kSurfaceContactMaxPush;
        if (bAggressiveRisingSlopeTransition)
        {
            maxPush = min(maxPush, kSurfaceLineSeamRisingSlopePushLimit);
        }
        else
        {
            if (bFlatToSlopeTransition)
                maxPush = min(maxPush, kSurfaceLineSeamFlatToSlopePushLimit);
            if (bSeamAngleShift)
                maxPush = min(maxPush, kSurfaceLineSeamAngleShiftPushLimit);
            if (bSteepDownSlopeTransition)
                maxPush = min(maxPush, kSurfaceContactDownTransitionPushLimit);
        }
        if (worldPush > maxPush)
            worldPush = maxPush;

        Vec3 pos = Transform()->GetRelativePos();
        pos.x += positionResolveNormal.x * worldPush;
        pos.y += positionResolveNormal.y * worldPush;
        Transform()->SetRelativePos(pos);
    }

    if (isSameCurrentLineSurface &&
        fabsf(_Contact.SignedDistance) <= 0.25f &&
        normal.y >= 0.85f)
    {
        if (isRecentChordLineContact)
        {
            m_bHasRecentReleasedInwardCircleContact = false;
            m_RecentReleasedInwardCircleContact = SurfaceContact{};
            m_RecentReleasedInwardCircleTime = 0.f;
        }

        if (isRecentGuidedCorrectionLineContact ||
            (!m_RecentGuidedCorrectionLineName.empty() &&
             _Contact.Surface != nullptr &&
             _Contact.Surface->GetName() == m_RecentGuidedCorrectionLineName))
        {
            m_RecentGuidedCorrectionLineTime = 0.f;
            m_RecentGuidedCorrectionLineName.clear();
        }
    }

    if ((bForceGuidedCorrectionLineLock || isGuideLinkedCorrectionLineContact || isRecentChordLineContact) &&
        !_Contact.Circle &&
        !_Contact.WallLike &&
        normal.y >= 0.75f)
    {
        m_SurfaceRotationSnapTime = max(m_SurfaceRotationSnapTime, kSurfaceGuidedLineRotationSnapTime);
    }

    Vec2 tangent = Vec2(curNormal.y, -curNormal.x);
    float rawVn = DotVec2(vVelocity, curNormal);
    const float incomingVn = rawVn;
    float vt = DotVec2(vVelocity, tangent);

    const bool wallLikeSurface = _Contact.WallLike || !_Contact.Attachable;
    const bool blockedByWallLikeSurface =
        wallLikeSurface &&
        IsBreakOrRollAction() &&
        (fabsf(curNormal.y) < 0.35f) &&
        (fabsf(incomingVn) > 20.f);

    if (rawVn < 0.f)
    {
        vVelocity.x -= curNormal.x * rawVn;
        vVelocity.y -= curNormal.y * rawVn;
        rawVn = 0.f;
    }

    const float stickMinSpeed = 500.f;
    const float normalEnterY = 0.65f;
    const float normalKeepY = 0.45f;
    const float vnEnterTolerance = 5.f;
    const float vnKeepTolerance = 80.f;
    const float normalThreshold = wasGround ? normalKeepY : normalEnterY;
    const float vnTolerance = wasGround ? vnKeepTolerance : vnEnterTolerance;
    const float inwardCircleGravityTangent = inwardCircleContact ? DotVec2(Vec2(0.f, -1.f), tangent) : 0.f;
    const bool bLowSpeedOuterCircleFall =
        _Contact.Circle &&
        !_Contact.InwardCircle &&
        _Contact.Attachable &&
        !jumpPressed &&
        !blockedByWallLikeSurface &&
        (fabsf(_Contact.SignedDistance) <= circleProjectionMaxDistance) &&
        (curNormal.y <= kSurfaceCircleLowSpeedFallMinNormalY) &&
        (fabsf(vt) < kSurfaceCircleLowSpeedFallSpeedMin);
    const bool bLowSpeedInwardCircleBackSlip =
        bLowSpeedInwardCircleDetachEligible &&
        !bLowSpeedInwardCircleDetach &&
        !jumpPressed &&
        !blockedByWallLikeSurface &&
        !bLowSpeedInwardCircleHighFallZone &&
        (fabsf(inwardCircleGravityTangent) >= kSurfaceInwardCircleBackSlipMinGravityTangent) &&
        (curNormal.y <= kSurfaceInwardCircleBackSlipMaxNormalY) &&
        (fabsf(vt) < kSurfaceInwardCircleLowSpeedDetachSpeedMin);

    bool speedStick = (fabsf(vt) > stickMinSpeed) && (curNormal.y > 0.2f);
    bool wallAttach = _Contact.Attachable && (fabsf(curNormal.x) > 0.85f);
    bool canStick =
        (rawVn <= vnTolerance) &&
        (curNormal.y > normalThreshold || speedStick || wallAttach);

    if (bLineSeamCandidate && _Contact.TransitionSurface && curNormal.y < normalThreshold && !wallAttach)
    {
        const bool seamCanStick =
            (fabsf(curNormal.y) >= kSurfaceLineSeamMinY) &&
            (normalDot >= kSurfaceLineSeamKeepDot) &&
            (fabsf(vt) <= kSurfaceLineSeamMoveSpeedGate);

        if (bFlatToSlopeTransition || bDownwardSlopeTransition || bSeamAngleShift)
            canStick = true;
        else
            canStick = bHoldLineSeam && seamCanStick;
    }

    if (_Contact.Circle && _Contact.Attachable && !jumpPressed && !blockedByWallLikeSurface)
    {
        const bool bInwardCircleLowerHalfContact =
            inwardCircleContact &&
            (inwardCircleLowerHalfRatio >= 0.12f) &&
            (incomingVn <= vnKeepTolerance);
        const bool circleCanStick =
            !bLowSpeedOuterCircleFall &&
            (fabsf(_Contact.SignedDistance) <= circleProjectionMaxDistance) &&
            (bInwardCircleLowerHalfContact ||
             bLowSpeedInwardCircleBackSlip ||
             ((!_Contact.InwardCircle || inwardCircleUpperHalfRatio < kSurfaceInwardCircleLowSpeedDetachUpperHalfMin) &&
              ((curNormal.y >= kSurfaceCircleStickMinY) ||
               hasGroundedSurfaceContext ||
               bAirborneCircleAcquire)) ||
             fabsf(vt) >= kSurfaceCircleStickSpeedMin);

        if (circleCanStick)
            canStick = true;
    }

    if (blockedByWallLikeSurface)
    {
        if (GetAction() == ActionState::Break)
            RequestBreakWallStop();
        else
            StopBlockedAction();

        if (GetAction() == ActionState::Break)
        {
            Vec2 blockedVelocity = GetVelocity();
            blockedVelocity.x = 0.f;
            SetVelocity(blockedVelocity);
            return true;
        }
    }

    if (canStick)
    {
        RefreshSurfaceGroundHold(_Contact.Circle ? kSurfaceCircleGroundHoldTime : 0.08f);
        SetIsGround(true);
        SetNormal(curNormal);
        SetGroundTangent(tangent);

        if (bLowSpeedInwardCircleBackSlip)
        {
            const float downhillDir = (inwardCircleGravityTangent >= 0.f) ? 1.f : -1.f;
            vt += inwardCircleGravityTangent * vAccel.y * kSurfaceInwardCircleBackSlipAccelScale * DT;

            if (vt * downhillDir < 0.f || fabsf(vt) < kSurfaceInwardCircleBackSlipWalkSpeedMin)
                vt = downhillDir * kSurfaceInwardCircleBackSlipWalkSpeedMin;

            if (fabsf(vt) > kSurfaceInwardCircleBackSlipWalkSpeedMax)
                vt = downhillDir * kSurfaceInwardCircleBackSlipWalkSpeedMax;
        }
        else if (curNormal.y > 0.f)
        {
            float friction = 100.f;

            if (vt > 0.f)
            {
                vt -= friction * DT;
                if (vt < 0.f) vt = 0.f;
            }
            else if (vt < 0.f)
            {
                vt += friction * DT;
                if (vt > 0.f) vt = 0.f;
            }
        }

        vVelocity = tangent * vt;
    }
    else
    {
        const bool definiteAirborne =
            bLowSpeedOuterCircleFall ||
            (rawVn > vnKeepTolerance) ||
            (!wallAttach && curNormal.y < 0.2f && fabsf(vt) < stickMinSpeed);

        if (bLowSpeedOuterCircleFall)
            m_SurfaceGroundHoldTime = 0.f;

        if (definiteAirborne)
            SetIsGround(false);

        SetNormal(curNormal);
        SetGroundTangent(tangent);

        if (!bLowSpeedOuterCircleFall &&
            curNormal.y <= 0.65f &&
            fabsf(vt) < stickMinSpeed)
        {
            if (vVelocity.y > 0.f)
                vVelocity.y = 0.f;
        }
    }

    return true;
}

void CPlayerScript::Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    Ptr<CBlockPushingScript> pBlock = _OtherCollider->GetOwner()->GetScript<CBlockPushingScript>();
    if (pBlock == nullptr)
        return;

    const int pushDir = pBlock->GetIsPushing();
    if (pushDir == 1 || pushDir == -1)
    {
        m_bPushContact = true;
        m_PushContactDir = pushDir;
        return;
    }

    m_bPushContact = false;
    m_PushContactDir = 0;
}

void CPlayerScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (_OtherCollider == nullptr)
        return;

    auto pTile = _OtherCollider->GetOwner()->GetScript<CTileScript>();
    auto pSurface = _OtherCollider->GetOwner()->GetScript<CSurfaceScript>();
    auto pBlock = _OtherCollider->GetOwner()->GetScript<CBlockScript>();
    auto pBlockMove = _OtherCollider->GetOwner()->GetScript<CBlockMovingScript>();
    auto pBlockPush = _OtherCollider->GetOwner()->GetScript<CBlockPushingScript>();

    if (pSurface != nullptr && pSurface->GetRole() != CSurfaceScript::SURFACE_ROLE::WALL)
        RegisterActiveSurface(_OtherCollider->GetOwner());

    const bool bCountsAsPhysicalGroundOverlap =
        (pTile != nullptr) ||
        (pBlock != nullptr) ||
        (pBlockMove != nullptr) ||
        (pBlockPush != nullptr);
    const bool bCountsAsAttachableSurfaceOverlap =
        (pSurface != nullptr && pSurface->IsAttachable());

    if (bCountsAsPhysicalGroundOverlap)
        ++m_PhysicalGroundOverlapCount;

    if (bCountsAsAttachableSurfaceOverlap)
        ++m_AttachableSurfaceOverlapCount;
}

void CPlayerScript::EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (_OtherCollider == nullptr)
        return;

    auto pBlockPush = _OtherCollider->GetOwner()->GetScript<CBlockPushingScript>();
    if (pBlockPush != nullptr)
    {
        m_bPushContact = false;
        m_PushContactDir = 0;
        m_bPushing = false;
        m_PushingDir = 0;
        if (m_Action == ActionState::Pushing)
            m_Action = ActionState::None;
    }

    auto pTile = _OtherCollider->GetOwner()->GetScript<CTileScript>();
    auto pSurface = _OtherCollider->GetOwner()->GetScript<CSurfaceScript>();
    auto pBlock = _OtherCollider->GetOwner()->GetScript<CBlockScript>();
    auto pBlockMove = _OtherCollider->GetOwner()->GetScript<CBlockMovingScript>();

    if (pSurface != nullptr && pSurface->GetRole() != CSurfaceScript::SURFACE_ROLE::WALL)
        UnregisterActiveSurface(_OtherCollider->GetOwner());

    const bool bCountsAsPhysicalGroundOverlap =
        (pTile != nullptr) ||
        (pBlock != nullptr) ||
        (pBlockMove != nullptr) ||
        (pBlockPush != nullptr);
    const bool bCountsAsAttachableSurfaceOverlap =
        (pSurface != nullptr && pSurface->IsAttachable());

    if (bCountsAsPhysicalGroundOverlap)
    {
        --m_PhysicalGroundOverlapCount;
        if (m_PhysicalGroundOverlapCount < 0)
            m_PhysicalGroundOverlapCount = 0;
    }

    if (bCountsAsAttachableSurfaceOverlap)
    {
        --m_AttachableSurfaceOverlapCount;
        if (m_AttachableSurfaceOverlapCount < 0)
            m_AttachableSurfaceOverlapCount = 0;
    }
}

void CPlayerScript::SaveToLevelFile(FILE* _File)
{
    //fwrite(&m_Dir, sizeof(Vec3), 1, _File);
}

void CPlayerScript::LoadFromLevelFile(FILE* _File)
{
    //fread(&m_Dir, sizeof(Vec3), 1, _File);
}

