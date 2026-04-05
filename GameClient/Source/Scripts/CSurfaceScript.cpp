#include "pch.h"
#include "CSurfaceScript.h"

#include "CCollider2D.h"
#include "GameObject.h"
#include "CTransform.h"
#include "CPlayerScript.h"
#include "TimeMgr.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

namespace
{
    constexpr float kColliderMinThickness = 1.f;
    // Circle half-check transition should use a tiny world-space margin. A large
    // value caused early exit/edge snapping when the player was still safely on
    // the circular segment.
    constexpr float kCircleHalfCheckMargin = 0.12f;
    constexpr float kEndpointTransitionMargin = 0.12f;
    constexpr float kTransitionSnapThresholdAir = 2.0f;
    constexpr float kTransitionSnapThresholdGround = 8.f;
    constexpr float kCircleTransitionSnapThresholdAir = 0.6f;
    constexpr float kCircleTransitionSnapThresholdGround = 3.f;
    // 0.02f: more aggressive depth clamping (less sink-in),
    // 0.04f: conservative clamp (more permissive penetration).
    constexpr float kGroundDepthClamp = 0.02f;
    constexpr float kSurfaceEdgeEpsilon = 0.02f;
    constexpr float kSurfaceContactPositiveEpsilon = 0.015f;
    // Keep this seam depth aligned with CPlayerScript's
    // kSurfaceLineSeamTransitionDepth logic (0.03f).
    constexpr float kSurfaceLineSeamTransitionDepth = 0.03f;
    constexpr float kSurfaceTransitionScoreFavor = 0.30f;
    constexpr float kSurfaceWallScorePenalty = 0.25f;
    constexpr float kSurfaceLineSeamPixelMargin = 2.f;
    constexpr float kLineSupportDepthTolerance = 0.0015f;
    constexpr float kSupportPointInsideMargin = 0.08f;
    constexpr float kArcTransitionLocalMargin = 0.04f;
    constexpr float kArcSupportLocalMargin = 0.38f;
    constexpr float kLineSupportProjectionMarginMin = 4.f;
    constexpr float kLineSupportProjectionMarginMax = 10.f;
    constexpr float kSurfaceAirAttachMaxPenetration = 24.f;
    constexpr float kTopHalfInwardCircleLineContextAttachMaxDistance = 16.f;
    constexpr float kSurfaceSpatialCellSize = 256.f;
    constexpr float kVerticalEntryApproachMinSpeed = 35.f;
    constexpr float kVerticalEntryApproachDominance = 1.1f;

    std::unordered_map<long long, std::vector<CSurfaceScript*>> g_SurfaceSpatialBuckets;
    std::unordered_set<CSurfaceScript*> g_LiveSurfaceScripts;

    long long MakeSurfaceSpatialKey(int _CellX, int _CellY)
    {
        const unsigned long long x = static_cast<unsigned long long>(static_cast<unsigned int>(_CellX));
        const unsigned long long y = static_cast<unsigned long long>(static_cast<unsigned int>(_CellY));
        return static_cast<long long>((x << 32) | y);
    }

    int WorldToSurfaceCell(float _Value)
    {
        return static_cast<int>(floorf(_Value / kSurfaceSpatialCellSize));
    }

    float Dot(const Vec2& _A, const Vec2& _B)
    {
        return _A.x * _B.x + _A.y * _B.y;
    }

    float Length(const Vec2& _V)
    {
        return sqrtf(_V.x * _V.x + _V.y * _V.y);
    }

    Vec2 NormalizeSafe(const Vec2& _V, const Vec2& _Fallback = Vec2(0.f, 1.f))
    {
        float len = Length(_V);
        if (len <= 0.0001f)
            return _Fallback;

        return Vec2(_V.x / len, _V.y / len);
    }

    float Clamp01(float _Value)
    {
        if (_Value < 0.f)
            return 0.f;
        if (_Value > 1.f)
            return 1.f;
        return _Value;
    }

    bool IsInsideExpandedBox(const Vec2& _Point, const Vec2& _Min, const Vec2& _Max, float _Margin)
    {
        return (_Point.x >= _Min.x - _Margin && _Point.x <= _Max.x + _Margin
            && _Point.y >= _Min.y - _Margin && _Point.y <= _Max.y + _Margin);
    }

    bool GetColliderSupportData(CCollider2D* _Collider, const Vec2& _SurfaceNormal,
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

        Vec3 center3 = XMVector3TransformCoord(Vec3(0.f, 0.f, 0.f), matWorld);
        Vec3 halfRight3 = XMVector3TransformNormal(Vec3(0.5f, 0.f, 0.f), matWorld);
        Vec3 halfUp3 = XMVector3TransformNormal(Vec3(0.f, 0.5f, 0.f), matWorld);

        _OutCenter = Vec2(center3.x, center3.y);

        Vec2 n = NormalizeSafe(_SurfaceNormal);
        Vec2 halfRight = Vec2(halfRight3.x, halfRight3.y);
        Vec2 halfUp = Vec2(halfUp3.x, halfUp3.y);

        _OutSupportDistance = fabsf(Dot(n, halfRight)) + fabsf(Dot(n, halfUp));
        _OutSupportPoint = _OutCenter - n * _OutSupportDistance;
        return true;
    }

    void CanonicalizeLine(Vec2& _A, Vec2& _B)
    {
        Vec2 delta = _B - _A;

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

    bool IsPastCircleExitEdge(CSurfaceScript::ARC_CORNER _Corner, const Vec2& _Point, const Vec2& _BoxMin, const Vec2& _BoxMax, float _Margin)
    {
        switch (_Corner)
        {
        case CSurfaceScript::ARC_CORNER::TOP_LEFT:
            return (_Point.x > _BoxMax.x + _Margin) || (_Point.y < _BoxMin.y - _Margin);
        case CSurfaceScript::ARC_CORNER::TOP_RIGHT:
            return (_Point.x < _BoxMin.x - _Margin) || (_Point.y < _BoxMin.y - _Margin);
        case CSurfaceScript::ARC_CORNER::BOTTOM_LEFT:
            return (_Point.x > _BoxMax.x + _Margin) || (_Point.y > _BoxMax.y + _Margin);
        case CSurfaceScript::ARC_CORNER::BOTTOM_RIGHT:
        default:
            return (_Point.x < _BoxMin.x - _Margin) || (_Point.y > _BoxMax.y + _Margin);
        }
    }

    bool GetPlayerFootPos(CCollider2D* _OtherCollider, bool _WasGround, Vec2& _OutFootPos)
    {
        if (_OtherCollider == nullptr || _OtherCollider->GetOwner() == nullptr)
            return false;

        GameObject* pOwner = _OtherCollider->GetOwner();
        auto pPlayer = pOwner->GetScript<CPlayerScript>();

        Vec2 sampleNormal = Vec2(0.f, 1.f);
        if (_WasGround && pPlayer != nullptr)
        {
            sampleNormal = NormalizeSafe(pPlayer->GetNormal(), Vec2(0.f, 1.f));

            // When descending from a circle, the carried-over surface normal can point
            // toward the body side instead of the true sole. For line probing we want
            // a stable "bottom" sample again, otherwise the player stays buried until
            // a jump resets the contact state.
            if (pPlayer->HasCurrentCircleSurfaceContact() && sampleNormal.y < 0.6f)
                sampleNormal = Vec2(0.f, 1.f);
        }

        Vec2 center = {};
        float supportDistance = 0.f;
        return GetColliderSupportData(_OtherCollider, sampleNormal, center, _OutFootPos, supportDistance);
    }

    bool IsCircleGeometry(CSurfaceScript::SURFACE_GEOMETRY _Geometry)
    {
        return _Geometry == CSurfaceScript::SURFACE_GEOMETRY::CIRCLE ||
               _Geometry == CSurfaceScript::SURFACE_GEOMETRY::FULL_CIRCLE;
    }

    bool IsVerticalEntryApproachAllowed(CPlayerScript* _Player, GameObject* _Surface)
    {
        if (_Player == nullptr || _Surface == nullptr)
            return false;

        if (_Player->GetReferenceAttachableLineSurface() == _Surface)
            return true;

        const Vec2 velocity = _Player->GetVelocity();
        const float absX = fabsf(velocity.x);
        const float absY = fabsf(velocity.y);
        return absY >= kVerticalEntryApproachMinSpeed &&
               absY >= absX * kVerticalEntryApproachDominance;
    }

}

CSurfaceScript::CSurfaceScript()
    : CScript(SCRIPT_TYPE::SURFACESCRIPT)
    , m_Geometry(SURFACE_GEOMETRY::LINE)
    , m_Role(SURFACE_ROLE::SURFACE)
    , m_ArcCorner(ARC_CORNER::BOTTOM_LEFT)
    , m_LocalStart(Vec2(-100.f, 0.f))
    , m_LocalEnd(Vec2(100.f, 0.f))
    , m_FillAbove(false)
    , m_FillInside(false)
    , m_Attachable(true)
    , m_LastSpatialWorldPos(Vec2(0.f, 0.f))
    , m_bSpatialRegistered(false)
{
    g_LiveSurfaceScripts.insert(this);
}

CSurfaceScript::CSurfaceScript(const CSurfaceScript& _Origin)
    : CScript(_Origin)
    , m_Geometry(_Origin.m_Geometry)
    , m_Role(_Origin.m_Role)
    , m_ArcCorner(_Origin.m_ArcCorner)
    , m_LocalStart(_Origin.m_LocalStart)
    , m_LocalEnd(_Origin.m_LocalEnd)
    , m_FillAbove(_Origin.m_FillAbove)
    , m_FillInside(_Origin.m_FillInside)
    , m_Attachable(_Origin.m_Attachable)
    , m_LastSpatialWorldPos(Vec2(0.f, 0.f))
    , m_bSpatialRegistered(false)
{
    g_LiveSurfaceScripts.insert(this);
}

CSurfaceScript::~CSurfaceScript()
{
    UnregisterSpatialRegistration();
    g_LiveSurfaceScripts.erase(this);
}

void CSurfaceScript::Begin()
{
    if (Collider2D())
    {
        Collider2D()->AddDynamicBeginOverlap(this, (COLLISION_EVENT)&CSurfaceScript::BeginOverlap);
        Collider2D()->AddDynamicOverlap(this, (COLLISION_EVENT)&CSurfaceScript::Overlap);
        Collider2D()->AddDynamicEndOverlap(this, (COLLISION_EVENT)&CSurfaceScript::EndOverlap);
    }

    UpdateBounds();
    RefreshSpatialRegistration();
}

void CSurfaceScript::Tick()
{
    GameObject* pOwner = GetOwner();
    if (pOwner == nullptr || pOwner->IsDead())
    {
        UnregisterSpatialRegistration();
        return;
    }

    CTransform* pTransform = pOwner->Transform().Get();
    if (pTransform == nullptr)
        return;

    const Vec3 worldPos = pTransform->GetWorldPos();
    if (!m_bSpatialRegistered ||
        fabsf(worldPos.x - m_LastSpatialWorldPos.x) > 0.001f ||
        fabsf(worldPos.y - m_LastSpatialWorldPos.y) > 0.001f)
    {
        RefreshSpatialRegistration();
    }
}

void CSurfaceScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    Overlap(_OwnCollider, _OtherCollider);
}

void CSurfaceScript::Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (_OtherCollider == nullptr || _OtherCollider->GetOwner() == nullptr)
        return;

    if (_OtherCollider->GetOwner()->GetName() != L"Player")
        return;

    auto pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer == nullptr)
        return;

    if (m_Role == SURFACE_ROLE::WALL)
    {
        Vec2 normal = {};
        float signedDistance = 0.f;
        if (!EvaluateWallProbe(_OtherCollider, normal, signedDistance))
        {
            pPlayer->UnregisterWallPushContact(GetOwner());
            return;
        }

        Vec3 playerPos = pPlayer->Transform()->GetRelativePos();
        Vec2 playerVelocity = pPlayer->GetVelocity();
        // Keep pure-wall side contacts on the boundary instead of nudging the
        // player fully outside. A positive bias caused overlap to disappear for
        // one frame, which made wall-push contact and the pushing animation
        // flicker while the player was continuously pressing the wall.
        const float resolveDistance = max(-signedDistance, 0.f);

        if (fabsf(normal.x) > 0.5f)
        {
            const int pushDir = (normal.x < 0.f) ? 1 : -1;
            pPlayer->RegisterWallPushContact(GetOwner(), pushDir);
            playerPos.x += normal.x * resolveDistance;

            if (pPlayer->GetAction() == ActionState::Break)
            {
                playerVelocity = Vec2(0.f, 0.f);
                pPlayer->RequestBreakWallStop();
            }
            else
            {
                if (playerVelocity.x * normal.x < 0.f)
                    playerVelocity.x = 0.f;

                pPlayer->StopBlockedAction();
            }
        }
        else
        {
            pPlayer->UnregisterWallPushContact(GetOwner());
            playerPos.y += normal.y * resolveDistance;

            if (normal.y > 0.5f)
            {
                pPlayer->ForceFlatGroundContact(0.12f);
                if (playerVelocity.y < 0.f)
                    playerVelocity.y = 0.f;
            }
            else if (normal.y < -0.5f)
            {
                if (playerVelocity.y > 0.f)
                    playerVelocity.y = 0.f;
            }
        }

        pPlayer->Transform()->SetRelativePos(playerPos);
        pPlayer->SetVelocity(playerVelocity);
        return;
    }

    CONTACT_PROBE probe = {};
    if (!ProbePlayerContact(_OtherCollider, pPlayer->GetIsGround(), probe))
        return;

    pPlayer->SubmitSurfaceContact(GetOwner(), probe.Normal, probe.SignedDistance, probe.TransitionSurface,
                                  probe.Attachable, probe.WallLike, probe.Circle, probe.InwardCircle,
                                  probe.CandidateScore,
                                  probe.SeamBlendT,
                                  probe.SeamBlendHasValue,
                                  probe.SeamStart,
                                  probe.SeamEnd,
                                  probe.ContactPoint);
}

bool CSurfaceScript::ProbePlayerContact(CCollider2D* _OtherCollider, bool _WasGround, CONTACT_PROBE& _OutProbe)
{
    _OutProbe = CONTACT_PROBE{};

    if (_OtherCollider == nullptr || _OtherCollider->GetOwner() == nullptr)
        return false;

    if (_OtherCollider->GetOwner()->GetName() != L"Player")
        return false;

    auto pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer == nullptr)
        return false;

    Vec2 normal = {};
    float signedDistance = 0.f;
    bool transitionSurface = false;
    float seamBlendT = 0.f;
    bool seamBlendHasValue = false;
    Vec2 seamStart = {};
    Vec2 seamEnd = {};
    Vec2 contactPoint = {};
    bool hit = false;

    if (m_Role == SURFACE_ROLE::WALL)
        hit = EvaluateWallProbe(_OtherCollider, normal, signedDistance);
    else
    {
        if (pPlayer->IsSurfaceAttachBlocked(GetOwner()))
            return false;

        if (m_Role == SURFACE_ROLE::VERTICAL_ENTRY &&
            m_Geometry == SURFACE_GEOMETRY::LINE &&
            !IsVerticalEntryApproachAllowed(pPlayer.Get(), GetOwner()))
        {
            return false;
        }

        Vec2 footPos = {};
        if (!GetPlayerFootPos(_OtherCollider, _WasGround, footPos))
            return false;

        if (m_Geometry == SURFACE_GEOMETRY::LINE)
            hit = EvaluateLineProbe(_OtherCollider, footPos, normal, signedDistance, transitionSurface, seamBlendT, seamBlendHasValue, seamStart, seamEnd, contactPoint, _WasGround);
        else if (IsCircleGeometry(m_Geometry))
            hit = EvaluateCircleProbe(_OtherCollider, footPos, normal, signedDistance, transitionSurface, contactPoint, _WasGround);
        else if (m_Geometry == SURFACE_GEOMETRY::ARC)
            hit = EvaluateArcProbe(_OtherCollider, footPos, normal, signedDistance, transitionSurface, contactPoint, _WasGround);
        else
            hit = EvaluateWallProbe(_OtherCollider, normal, signedDistance);
    }

    if (!hit)
        return false;

    bool inwardCircleContact = false;
    bool topHalfInwardCircleWithLineContext = false;
    if (IsCircleGeometry(m_Geometry))
    {
        Vec2 circleCenter = {};
        float circleRadius = 0.f;
        Vec2 boxMin = {};
        Vec2 boxMax = {};
        GetArcWorldData(circleCenter, circleRadius, boxMin, boxMax);

        const Vec2 radialToContact = contactPoint - circleCenter;
        inwardCircleContact = (Dot(normal, radialToContact) < 0.f);
        if (inwardCircleContact && pPlayer->IsInwardCircleAttachBlocked(circleCenter, circleRadius))
            return false;

        // Guided loops visually disable one arc side at a time, so probe hits on
        // the hidden side should never become physical contacts either.
        if (pPlayer->ShouldIgnoreGuidedCircleWallContact(GetOwner(), contactPoint))
            return false;

        if (inwardCircleContact &&
            m_FillInside &&
            pPlayer->HasInwardCircleLineContext() &&
            (m_Geometry == SURFACE_GEOMETRY::FULL_CIRCLE ||
                m_ArcCorner == ARC_CORNER::TOP_LEFT ||
                m_ArcCorner == ARC_CORNER::TOP_RIGHT) &&
            contactPoint.y < circleCenter.y - 0.5f)
        {
            topHalfInwardCircleWithLineContext = true;
        }
    }

    float transitionSnapThreshold = _WasGround ? kTransitionSnapThresholdGround : kTransitionSnapThresholdAir;
    if (IsCircleGeometry(m_Geometry))
        transitionSnapThreshold = _WasGround ? kCircleTransitionSnapThresholdGround : kCircleTransitionSnapThresholdAir;
    if (topHalfInwardCircleWithLineContext)
        transitionSnapThreshold = max(transitionSnapThreshold, kTopHalfInwardCircleLineContextAttachMaxDistance);
    if (signedDistance > transitionSnapThreshold)
        return false;

    const bool keepAttachedSurfaceContact =
        (m_Geometry == SURFACE_GEOMETRY::LINE ||
         m_Geometry == SURFACE_GEOMETRY::ARC ||
         IsCircleGeometry(m_Geometry)) &&
        IsAttachable();
    const float positiveContactLimit =
        keepAttachedSurfaceContact ? transitionSnapThreshold : kSurfaceContactPositiveEpsilon;

    if (signedDistance > positiveContactLimit)
        return false;

    if (!_WasGround && signedDistance < -kSurfaceAirAttachMaxPenetration)
        return false;

    const float absSignedDistance = fabsf(signedDistance);
    float transitionScoreFavor = 0.f;
    if (transitionSurface)
    {
        const float depthRatio = Clamp01(absSignedDistance / (kSurfaceLineSeamTransitionDepth * 2.f));
        transitionScoreFavor = kSurfaceTransitionScoreFavor * (1.f - depthRatio);
    }

    float candidateScore = absSignedDistance - transitionScoreFavor;
    if (m_Role == SURFACE_ROLE::WALL)
        candidateScore += kSurfaceWallScorePenalty;

    _OutProbe.Normal = normal;
    _OutProbe.SignedDistance = signedDistance;
    _OutProbe.CandidateScore = candidateScore;
    _OutProbe.TransitionSurface = transitionSurface;
    _OutProbe.Attachable = IsAttachable();
    _OutProbe.WallLike = (m_Role == SURFACE_ROLE::WALL);
    _OutProbe.Circle = IsCircleGeometry(m_Geometry);
    _OutProbe.InwardCircle = inwardCircleContact;
    _OutProbe.SeamBlendT = seamBlendT;
    _OutProbe.SeamBlendHasValue = seamBlendHasValue;
    _OutProbe.SeamStart = seamStart;
    _OutProbe.SeamEnd = seamEnd;
    _OutProbe.ContactPoint = contactPoint;
    return true;
}

void CSurfaceScript::EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (m_Role != SURFACE_ROLE::WALL || _OtherCollider == nullptr || _OtherCollider->GetOwner() == nullptr)
        return;

    auto pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer != nullptr)
        pPlayer->UnregisterWallPushContact(GetOwner());
}

void CSurfaceScript::ConfigureLine(const Vec2& _LocalStart, const Vec2& _LocalEnd, SURFACE_ROLE _Role, bool _FillAbove, bool _Attachable)
{
    m_Geometry = SURFACE_GEOMETRY::LINE;
    m_Role = _Role;
    m_LocalStart = _LocalStart;
    m_LocalEnd = _LocalEnd;
    m_FillAbove = _FillAbove;
    m_Attachable = _Attachable;
    UpdateBounds();
}

void CSurfaceScript::ConfigureArc(const Vec2& _LocalStart, const Vec2& _LocalEnd, SURFACE_ROLE _Role, ARC_CORNER _Corner, bool _FillInside, bool _Attachable)
{
    m_Geometry = SURFACE_GEOMETRY::ARC;
    m_Role = _Role;
    m_LocalStart = _LocalStart;
    m_LocalEnd = _LocalEnd;
    m_ArcCorner = _Corner;
    m_FillInside = _FillInside;
    m_Attachable = _Attachable;
    UpdateBounds();
}

void CSurfaceScript::ConfigureCircle(const Vec2& _LocalStart, const Vec2& _LocalEnd, SURFACE_ROLE _Role, ARC_CORNER _Corner, bool _FillInside, bool _Attachable)
{
    m_Geometry = SURFACE_GEOMETRY::CIRCLE;
    m_Role = _Role;
    m_LocalStart = _LocalStart;
    m_LocalEnd = _LocalEnd;
    m_ArcCorner = _Corner;
    m_FillInside = _FillInside;
    m_Attachable = _Attachable;
    UpdateBounds();
}

void CSurfaceScript::ConfigureFullCircle(float _Radius, SURFACE_ROLE _Role, bool _FillInside, bool _Attachable)
{
    const float safeRadius = max(_Radius, 1.f);
    m_Geometry = SURFACE_GEOMETRY::FULL_CIRCLE;
    m_Role = _Role;
    m_LocalStart = Vec2(-safeRadius, -safeRadius);
    m_LocalEnd = Vec2(safeRadius, safeRadius);
    m_ArcCorner = ARC_CORNER::TOP_LEFT;
    m_FillInside = _FillInside;
    m_Attachable = _Attachable;
    UpdateBounds();
}

void CSurfaceScript::GetWorldEndpoints(Vec2& _OutStart, Vec2& _OutEnd)
{
    GameObject* pOwner = GetOwner();
    if (pOwner == nullptr || pOwner->Transform() == nullptr)
    {
        _OutStart = m_LocalStart;
        _OutEnd = m_LocalEnd;
        return;
    }

    Vec3 worldPos = pOwner->Transform()->GetWorldPos();
    _OutStart = Vec2(worldPos.x + m_LocalStart.x, worldPos.y + m_LocalStart.y);
    _OutEnd = Vec2(worldPos.x + m_LocalEnd.x, worldPos.y + m_LocalEnd.y);
}

void CSurfaceScript::GetArcWorldData(Vec2& _OutCenter, float& _OutRadius, Vec2& _OutBoxMin, Vec2& _OutBoxMax)
{
    GameObject* pOwner = GetOwner();
    if (pOwner == nullptr || pOwner->Transform() == nullptr)
    {
        _OutCenter = Vec2(0.f, 0.f);
        _OutRadius = 0.f;
        _OutBoxMin = Vec2(0.f, 0.f);
        _OutBoxMax = Vec2(0.f, 0.f);
        return;
    }

    if (m_Geometry == SURFACE_GEOMETRY::FULL_CIRCLE)
    {
        Vec3 worldPos = pOwner->Transform()->GetWorldPos();
        _OutCenter = Vec2(worldPos.x, worldPos.y);
        _OutRadius = max(fabsf(m_LocalStart.x), fabsf(m_LocalEnd.x));
        _OutRadius = max(_OutRadius, max(fabsf(m_LocalStart.y), fabsf(m_LocalEnd.y)));
        _OutBoxMin = _OutCenter - Vec2(_OutRadius, _OutRadius);
        _OutBoxMax = _OutCenter + Vec2(_OutRadius, _OutRadius);
        return;
    }

    Vec2 startWorld = {};
    Vec2 endWorld = {};
    GetWorldEndpoints(startWorld, endWorld);

    float left = min(startWorld.x, endWorld.x);
    float right = max(startWorld.x, endWorld.x);
    float bottom = min(startWorld.y, endWorld.y);
    float top = max(startWorld.y, endWorld.y);

    float width = right - left;
    float height = top - bottom;
    float side = min(width, height);

    if (side <= 0.001f)
        side = max(width, height);

    switch (m_ArcCorner)
    {
    case ARC_CORNER::TOP_LEFT:
        _OutBoxMin = Vec2(left, top - side);
        _OutBoxMax = Vec2(left + side, top);
        _OutCenter = Vec2(_OutBoxMin.x, _OutBoxMax.y);
        break;
    case ARC_CORNER::TOP_RIGHT:
        _OutBoxMin = Vec2(right - side, top - side);
        _OutBoxMax = Vec2(right, top);
        _OutCenter = Vec2(_OutBoxMax.x, _OutBoxMax.y);
        break;
    case ARC_CORNER::BOTTOM_LEFT:
        _OutBoxMin = Vec2(left, bottom);
        _OutBoxMax = Vec2(left + side, bottom + side);
        _OutCenter = Vec2(_OutBoxMin.x, _OutBoxMin.y);
        break;
    case ARC_CORNER::BOTTOM_RIGHT:
    default:
        _OutBoxMin = Vec2(right - side, bottom);
        _OutBoxMax = Vec2(right, bottom + side);
        _OutCenter = Vec2(_OutBoxMax.x, _OutBoxMin.y);
        break;
    }

    _OutRadius = side;
}

Vec4 CSurfaceScript::GetEditorColor() const
{
    switch (m_Role)
    {
    case SURFACE_ROLE::SURFACE:
        return Vec4(0.2f, 0.75f, 1.f, 1.f);
    case SURFACE_ROLE::CORRECTION:
        return Vec4(0.2f, 1.f, 0.45f, 1.f);
    case SURFACE_ROLE::WALL:
        return Vec4(1.f, 0.45f, 0.2f, 1.f);
    case SURFACE_ROLE::VERTICAL_ENTRY:
        return Vec4(1.f, 0.9f, 0.2f, 1.f);
    default:
        return Vec4(1.f, 1.f, 1.f, 1.f);
    }
}

void CSurfaceScript::SaveToLevelFile(FILE* _File)
{
    int geometry = (int)m_Geometry;
    int role = (int)m_Role;
    int corner = (int)m_ArcCorner;

    fwrite(&geometry, sizeof(int), 1, _File);
    fwrite(&role, sizeof(int), 1, _File);
    fwrite(&corner, sizeof(int), 1, _File);
    fwrite(&m_LocalStart, sizeof(Vec2), 1, _File);
    fwrite(&m_LocalEnd, sizeof(Vec2), 1, _File);
    fwrite(&m_FillAbove, sizeof(bool), 1, _File);
    fwrite(&m_FillInside, sizeof(bool), 1, _File);
    fwrite(&m_Attachable, sizeof(bool), 1, _File);
}

void CSurfaceScript::LoadFromLevelFile(FILE* _File)
{
    int geometry = 0;
    int role = 0;
    int corner = 0;

    fread(&geometry, sizeof(int), 1, _File);
    fread(&role, sizeof(int), 1, _File);
    fread(&corner, sizeof(int), 1, _File);
    fread(&m_LocalStart, sizeof(Vec2), 1, _File);
    fread(&m_LocalEnd, sizeof(Vec2), 1, _File);
    fread(&m_FillAbove, sizeof(bool), 1, _File);
    fread(&m_FillInside, sizeof(bool), 1, _File);
    fread(&m_Attachable, sizeof(bool), 1, _File);

    m_Geometry = (SURFACE_GEOMETRY)geometry;
    m_Role = (SURFACE_ROLE)role;
    m_ArcCorner = (ARC_CORNER)corner;

    UpdateBounds();
}

void CSurfaceScript::UpdateBounds()
{
    GameObject* pOwner = GetOwner();
    if (pOwner == nullptr)
        return;

    CTransform* pTransform = pOwner->Transform().Get();
    if (pTransform == nullptr)
        return;

    float minX = min(m_LocalStart.x, m_LocalEnd.x);
    float maxX = max(m_LocalStart.x, m_LocalEnd.x);
    float minY = min(m_LocalStart.y, m_LocalEnd.y);
    float maxY = max(m_LocalStart.y, m_LocalEnd.y);

    if (m_Geometry == SURFACE_GEOMETRY::FULL_CIRCLE)
    {
        float radius = max(fabsf(m_LocalStart.x), fabsf(m_LocalEnd.x));
        radius = max(radius, max(fabsf(m_LocalStart.y), fabsf(m_LocalEnd.y)));
        minX = -radius;
        maxX = radius;
        minY = -radius;
        maxY = radius;
    }
    else if (m_Geometry == SURFACE_GEOMETRY::ARC || m_Geometry == SURFACE_GEOMETRY::CIRCLE)
    {
        float left = min(m_LocalStart.x, m_LocalEnd.x);
        float right = max(m_LocalStart.x, m_LocalEnd.x);
        float bottom = min(m_LocalStart.y, m_LocalEnd.y);
        float top = max(m_LocalStart.y, m_LocalEnd.y);
        float side = min(right - left, top - bottom);
        if (side <= 0.001f)
            side = max(right - left, top - bottom);

        switch (m_ArcCorner)
        {
        case ARC_CORNER::TOP_LEFT:
            minX = left;
            maxX = left + side;
            minY = top - side;
            maxY = top;
            break;
        case ARC_CORNER::TOP_RIGHT:
            minX = right - side;
            maxX = right;
            minY = top - side;
            maxY = top;
            break;
        case ARC_CORNER::BOTTOM_LEFT:
            minX = left;
            maxX = left + side;
            minY = bottom;
            maxY = bottom + side;
            break;
        case ARC_CORNER::BOTTOM_RIGHT:
        default:
            minX = right - side;
            maxX = right;
            minY = bottom;
            maxY = bottom + side;
            break;
        }
    }

    if (Collider2D() != nullptr)
    {
        float width = maxX - minX;
        float height = maxY - minY;
        Vec2 colliderCenter = Vec2((minX + maxX) * 0.5f, (minY + maxY) * 0.5f);

        // A line can be perfectly horizontal/vertical, so one axis may collapse to
        // zero. Keep only a tiny thickness on the collapsed axis so the collider
        // still exists without introducing the large broadphase margin that caused
        // inaccurate overlaps around adjacent surfaces.
        width = max(kColliderMinThickness, width);
        height = max(kColliderMinThickness, height);

        // Collider2D offset is expressed in normalized local space before the
        // owner's transform scale is applied. Arc/circle objects live at the
        // curve center, while their broadphase AABB lives on the quarter box
        // center, so we must move the broadphase box away from (0,0) to match the
        // actual curve region. Leaving this at zero places the trigger box at the
        // wrong location and can prevent overlap callbacks from firing at all.
        Vec2 normalizedOffset = Vec2(0.f, 0.f);
        if (width > 0.0001f)
            normalizedOffset.x = colliderCenter.x / width;
        if (height > 0.0001f)
            normalizedOffset.y = colliderCenter.y / height;

        pTransform->SetIndependentScale(true);
        pTransform->SetRelativeScale(Vec3(width, height, 1.f));
        Collider2D()->SetOffset(normalizedOffset);
        Collider2D()->SetScale(Vec2(1.f, 1.f));
    }
}

void CSurfaceScript::GetWorldBounds(Vec2& _OutMin, Vec2& _OutMax)
{
    GameObject* pOwner = GetOwner();
    if (pOwner == nullptr || pOwner->Transform() == nullptr)
    {
        _OutMin = Vec2(0.f, 0.f);
        _OutMax = Vec2(0.f, 0.f);
        return;
    }

    float minX = min(m_LocalStart.x, m_LocalEnd.x);
    float maxX = max(m_LocalStart.x, m_LocalEnd.x);
    float minY = min(m_LocalStart.y, m_LocalEnd.y);
    float maxY = max(m_LocalStart.y, m_LocalEnd.y);

    if (m_Geometry == SURFACE_GEOMETRY::ARC || IsCircleGeometry(m_Geometry))
    {
        Vec2 center = {};
        float radius = 0.f;
        GetArcWorldData(center, radius, _OutMin, _OutMax);
    }
    else
    {
        Vec3 worldPos = pOwner->Transform()->GetWorldPos();

        float width = maxX - minX;
        float height = maxY - minY;

        // Pure wall probes use an axis-aligned box. Horizontal/vertical walls can
        // collapse on one axis, so keep only the same tiny minimum thickness that
        // the broadphase collider uses.
        if (width < kColliderMinThickness)
        {
            const float halfExpand = (kColliderMinThickness - width) * 0.5f;
            minX -= halfExpand;
            maxX += halfExpand;
        }

        if (height < kColliderMinThickness)
        {
            const float halfExpand = (kColliderMinThickness - height) * 0.5f;
            minY -= halfExpand;
            maxY += halfExpand;
        }

        _OutMin = Vec2(worldPos.x + minX, worldPos.y + minY);
        _OutMax = Vec2(worldPos.x + maxX, worldPos.y + maxY);
    }
}

bool CSurfaceScript::IntersectsWorldBounds(const Vec2& _Min, const Vec2& _Max, float _Margin)
{
    Vec2 worldMin = {};
    Vec2 worldMax = {};
    GetWorldBounds(worldMin, worldMax);

    return !(worldMax.x < _Min.x - _Margin ||
             worldMin.x > _Max.x + _Margin ||
             worldMax.y < _Min.y - _Margin ||
             worldMin.y > _Max.y + _Margin);
}

void CSurfaceScript::ResetSpatialIndex()
{
    g_SurfaceSpatialBuckets.clear();
}

void CSurfaceScript::QueryNearbySurfaceObjects(const Vec2& _Min, const Vec2& _Max,
                                               std::vector<GameObject*>& _OutObjects,
                                               float _Margin, bool _IncludeWalls)
{
    _OutObjects.clear();

    const int minCellX = WorldToSurfaceCell(_Min.x - _Margin);
    const int maxCellX = WorldToSurfaceCell(_Max.x + _Margin);
    const int minCellY = WorldToSurfaceCell(_Min.y - _Margin);
    const int maxCellY = WorldToSurfaceCell(_Max.y + _Margin);

    std::unordered_set<CSurfaceScript*> uniqueSurfaces;

    for (int cellY = minCellY; cellY <= maxCellY; ++cellY)
    {
        for (int cellX = minCellX; cellX <= maxCellX; ++cellX)
        {
            const auto iter = g_SurfaceSpatialBuckets.find(MakeSurfaceSpatialKey(cellX, cellY));
            if (iter == g_SurfaceSpatialBuckets.end())
                continue;

            for (CSurfaceScript* pSurface : iter->second)
            {
                if (pSurface == nullptr)
                    continue;

                if (g_LiveSurfaceScripts.find(pSurface) == g_LiveSurfaceScripts.end())
                    continue;

                GameObject* pOwner = pSurface->GetOwner();
                if (pOwner == nullptr || pOwner->IsDead())
                    continue;

                if (!_IncludeWalls && pSurface->GetRole() == SURFACE_ROLE::WALL)
                    continue;

                if (!pSurface->IntersectsWorldBounds(_Min, _Max, _Margin))
                    continue;

                if (uniqueSurfaces.insert(pSurface).second)
                    _OutObjects.push_back(pOwner);
            }
        }
    }
}

void CSurfaceScript::RefreshSpatialRegistration()
{
    GameObject* pOwner = GetOwner();
    if (pOwner == nullptr || pOwner->IsDead())
    {
        UnregisterSpatialRegistration();
        return;
    }

    CTransform* pTransform = pOwner->Transform().Get();
    if (pTransform == nullptr)
    {
        UnregisterSpatialRegistration();
        return;
    }

    Vec2 worldMin = {};
    Vec2 worldMax = {};
    GetWorldBounds(worldMin, worldMax);

    const int minCellX = WorldToSurfaceCell(worldMin.x);
    const int maxCellX = WorldToSurfaceCell(worldMax.x);
    const int minCellY = WorldToSurfaceCell(worldMin.y);
    const int maxCellY = WorldToSurfaceCell(worldMax.y);

    std::vector<long long> newKeys;
    newKeys.reserve(static_cast<size_t>(maxCellX - minCellX + 1) * static_cast<size_t>(maxCellY - minCellY + 1));
    for (int cellY = minCellY; cellY <= maxCellY; ++cellY)
    {
        for (int cellX = minCellX; cellX <= maxCellX; ++cellX)
            newKeys.push_back(MakeSurfaceSpatialKey(cellX, cellY));
    }

    const Vec3 worldPos = pTransform->GetWorldPos();
    if (m_bSpatialRegistered &&
        m_SpatialCellKeys == newKeys &&
        fabsf(worldPos.x - m_LastSpatialWorldPos.x) <= 0.001f &&
        fabsf(worldPos.y - m_LastSpatialWorldPos.y) <= 0.001f)
    {
        return;
    }

    UnregisterSpatialRegistration();

    for (long long key : newKeys)
        g_SurfaceSpatialBuckets[key].push_back(this);

    m_SpatialCellKeys = std::move(newKeys);
    m_LastSpatialWorldPos = Vec2(worldPos.x, worldPos.y);
    m_bSpatialRegistered = true;
}

void CSurfaceScript::UnregisterSpatialRegistration()
{
    if (!m_bSpatialRegistered)
        return;

    for (long long key : m_SpatialCellKeys)
    {
        auto iter = g_SurfaceSpatialBuckets.find(key);
        if (iter == g_SurfaceSpatialBuckets.end())
            continue;

        auto& bucket = iter->second;
        bucket.erase(std::remove(bucket.begin(), bucket.end(), this), bucket.end());
        if (bucket.empty())
            g_SurfaceSpatialBuckets.erase(iter);
    }

    m_SpatialCellKeys.clear();
    m_bSpatialRegistered = false;
}

bool CSurfaceScript::EvaluateWallProbe(CCollider2D* _OtherCollider, Vec2& _OutNormal, float& _OutSignedDistance)
{
    if (_OtherCollider == nullptr || _OtherCollider->GetOwner() == nullptr)
        return false;

    Vec2 boxMin = {};
    Vec2 boxMax = {};
    GetWorldBounds(boxMin, boxMax);

    Vec3 playerPos = _OtherCollider->GetOwner()->Transform()->GetWorldPos();
    Vec3 playerScale = _OtherCollider->GetOwner()->Transform()->GetRelativeScale();
    Vec2 colOffset = _OtherCollider->GetOffset();
    Vec2 colScale = _OtherCollider->GetScale();

    Vec2 playerCenter = Vec2(playerPos.x + colOffset.x, playerPos.y + colOffset.y);
    float halfW = fabsf(playerScale.x * colScale.x) * 0.5f;
    float halfH = fabsf(playerScale.y * colScale.y) * 0.5f;

    float playerMinX = playerCenter.x - halfW;
    float playerMaxX = playerCenter.x + halfW;
    float playerMinY = playerCenter.y - halfH;
    float playerMaxY = playerCenter.y + halfH;

    if (playerMaxX < boxMin.x || playerMinX > boxMax.x ||
        playerMaxY < boxMin.y || playerMinY > boxMax.y)
    {
        return false;
    }

    const float penLeft = playerMaxX - boxMin.x;
    const float penRight = boxMax.x - playerMinX;
    const float penBottom = playerMaxY - boxMin.y;
    const float penTop = boxMax.y - playerMinY;

    float bestPen = penLeft;
    Vec2 bestNormal = Vec2(-1.f, 0.f);

    if (penRight < bestPen)
    {
        bestPen = penRight;
        bestNormal = Vec2(1.f, 0.f);
    }

    if (penBottom < bestPen)
    {
        bestPen = penBottom;
        bestNormal = Vec2(0.f, -1.f);
    }

    if (penTop < bestPen)
    {
        bestPen = penTop;
        bestNormal = Vec2(0.f, 1.f);
    }

    _OutNormal = bestNormal;
    _OutSignedDistance = -bestPen;
    return true;
}

bool CSurfaceScript::GetPlayerSupportPoint(CCollider2D* _OtherCollider, const Vec2& _SurfaceNormal, Vec2& _OutSupportPoint)
{
    if (_OtherCollider == nullptr || _OtherCollider->GetOwner() == nullptr)
        return false;

    Vec2 center = {};
    float supportDistance = 0.f;
    return GetColliderSupportData(_OtherCollider, _SurfaceNormal, center, _OutSupportPoint, supportDistance);
}
bool CSurfaceScript::EvaluateLineProbe(CCollider2D* _OtherCollider, const Vec2& _FootPos, Vec2& _OutNormal, float& _OutSignedDistance, bool& _OutTransitionSurface, float& _OutSeamBlendT, bool& _OutSeamBlendHasValue, Vec2& _OutSeamStart, Vec2& _OutSeamEnd, Vec2& _OutContactPoint, bool _WasGround)
{
    _OutTransitionSurface = false;
    _OutSeamBlendT = 0.f;
    _OutSeamBlendHasValue = false;
    _OutSeamStart = Vec2(0.f, 0.f);
    _OutSeamEnd = Vec2(0.f, 0.f);
    _OutContactPoint = _FootPos;

    Vec2 worldA = {};
    Vec2 worldB = {};
    GetWorldEndpoints(worldA, worldB);
    CanonicalizeLine(worldA, worldB);

    Vec2 ab = worldB - worldA;
    float length = Length(ab);
    if (length <= 0.001f)
        return false;

    Vec2 tangent = ab / length;
    // 위쪽 채우기 여부에 따라 법선 방향 결정
    Vec2 outward = m_FillAbove ? Vec2(tangent.y, -tangent.x) : Vec2(-tangent.y, tangent.x);

    // [핵심 1] 선분 위에 있는지 투영(Projection) 검사. 여유값은 float 오차를 막을 아주 작은 값만 줍니다.
    float projection = Dot(_FootPos - worldA, tangent);
    const float strictMargin = kSurfaceLineSeamPixelMargin;
    if (projection < -strictMargin || projection > length + strictMargin)
        return false;

    const float clampedProjection = Clamp01(projection / length);
    _OutContactPoint = worldA + tangent * (clampedProjection * length);
    // [핵심 2] FootPos를 기본으로 깊이/법선을 계산하되, 지원점에서 추가 안정화
    const float footSignedDistance = Dot(_FootPos - worldA, outward);
    float supportSignedDistance = footSignedDistance;
    _OutNormal = outward;

    Vec2 supportPoint = {};
    if (!GetPlayerSupportPoint(_OtherCollider, outward, supportPoint))
        return false;

    supportSignedDistance = Dot(supportPoint - worldA, outward);
    if (_WasGround)
    {
        supportSignedDistance = max(supportSignedDistance, footSignedDistance - kGroundDepthClamp);
    }

    // A line should never become the active ground when the player is already
    // deeply buried below it. This was most visible right after descending out
    // of a circle, where the line contact could stay latched with a huge
    // negative depth until a jump reset the state.
    if (supportSignedDistance < -kSurfaceAirAttachMaxPenetration)
        return false;

    const float supportProjection = Dot(supportPoint - worldA, tangent);
    float supportProjectionMargin = kLineSupportProjectionMarginMin + fabsf(Dot(supportPoint - _FootPos, tangent));
    if (supportProjectionMargin > kLineSupportProjectionMarginMax)
        supportProjectionMargin = kLineSupportProjectionMarginMax;
    if (supportProjection < -supportProjectionMargin || supportProjection > length + supportProjectionMargin)
        return false;

    if (supportSignedDistance < -kSurfaceContactPositiveEpsilon && Dot(supportPoint - worldA, outward) > kLineSupportDepthTolerance)
        return false;

    _OutSignedDistance = supportSignedDistance;

    // 플레이어 support point가 접선 방향으로 미리 앞서가기 때문에, seam을 선 끝 2px로만
    // 보면 다음 surface를 밟고도 기존 slope가 계속 잡고 있게 된다.
    float seamMargin = kSurfaceLineSeamPixelMargin + fabsf(Dot(supportPoint - _FootPos, tangent));
    seamMargin = min(seamMargin, length * 0.35f);
    _OutTransitionSurface = (projection <= seamMargin || projection >= length - seamMargin);

    _OutSeamBlendT = clampedProjection;
    _OutSeamBlendHasValue = true;
    _OutSeamStart = worldA;
    _OutSeamEnd = worldB;

    return true;
}

bool CSurfaceScript::EvaluateArcProbe(CCollider2D* _OtherCollider, const Vec2& _FootPos, Vec2& _OutNormal, float& _OutSignedDistance, bool& _OutTransitionSurface, Vec2& _OutContactPoint, bool _WasGround)
{
    _OutTransitionSurface = false;
    _OutContactPoint = _FootPos;

    Vec2 arcCenter = {};
    float radius = 0.f;
    Vec2 boxMin = {};
    Vec2 boxMax = {};
    GetArcWorldData(arcCenter, radius, boxMin, boxMax);

    const float width = max(kColliderMinThickness, boxMax.x - boxMin.x);
    const float height = max(kColliderMinThickness, boxMax.y - boxMin.y);
    const float top = boxMax.y;

    Vec2 colliderCenter = {};
    Vec2 unusedSupportPoint = {};
    float unusedSupportDistance = 0.f;
    if (!GetColliderSupportData(_OtherCollider, Vec2(0.f, 1.f), colliderCenter, unusedSupportPoint, unusedSupportDistance))
        return false;

    Vec2 centerRadial = colliderCenter - arcCenter;
    float centerRadialLen = Length(centerRadial);
    if (centerRadialLen <= 0.0001f)
    {
        centerRadial = _FootPos - arcCenter;
        centerRadialLen = Length(centerRadial);
        if (centerRadialLen <= 0.0001f)
            return false;
    }

    Vec2 centerOutward = Vec2(centerRadial.x / centerRadialLen, centerRadial.y / centerRadialLen);
    Vec2 candidateNormal = m_FillInside ? Vec2(-centerOutward.x, -centerOutward.y) : centerOutward;

    Vec2 supportPoint = {};
    float supportDistance = 0.f;
    if (!GetColliderSupportData(_OtherCollider, candidateNormal, colliderCenter, supportPoint, supportDistance))
        return false;

    const Vec2 supportRadial = supportPoint - arcCenter;
    const float supportRadialLen = Length(supportRadial);
    if (supportRadialLen <= 0.0001f)
        return false;

    Vec2 supportOutward = Vec2(supportRadial.x / supportRadialLen, supportRadial.y / supportRadialLen);
    Vec2 surfaceNormal = m_FillInside ? Vec2(-supportOutward.x, -supportOutward.y) : supportOutward;
    const Vec2 supportContactPoint = arcCenter + supportOutward * radius;
    _OutContactPoint = supportContactPoint;

    float localX = (_OutContactPoint.x - boxMin.x) / width;
    float localY = (top - _OutContactPoint.y) / height;
    const float localEdgeMargin = _WasGround ? kSupportPointInsideMargin : kSurfaceEdgeEpsilon;
    if (localX < -localEdgeMargin || localX > 1.f + localEdgeMargin ||
        localY < -localEdgeMargin || localY > 1.f + localEdgeMargin)
        return false;

    localX = Clamp01(localX);
    localY = Clamp01(localY);

    float supportSignedDistance = Dot(supportPoint - supportContactPoint, surfaceNormal);
    const float centerSignedDistance = Dot(colliderCenter - supportContactPoint, surfaceNormal) - supportDistance;

    if (_WasGround)
    {
        supportSignedDistance = max(supportSignedDistance, centerSignedDistance - kGroundDepthClamp);
    }

    _OutNormal = surfaceNormal;
    _OutSignedDistance = supportSignedDistance;
    _OutTransitionSurface =
        localX <= kEndpointTransitionMargin ||
        localX >= 1.f - kEndpointTransitionMargin ||
        localX <= kArcTransitionLocalMargin ||
        localX >= 1.f - kArcTransitionLocalMargin ||
        localY <= kArcTransitionLocalMargin ||
        localY >= 1.f - kArcTransitionLocalMargin;
    return true;
}

bool CSurfaceScript::EvaluateCircleProbe(CCollider2D* _OtherCollider, const Vec2& _FootPos, Vec2& _OutNormal, float& _OutSignedDistance, bool& _OutTransitionSurface, Vec2& _OutContactPoint, bool _WasGround)
{
    _OutTransitionSurface = false;
    _OutContactPoint = _FootPos;

    if (_OtherCollider == nullptr || _OtherCollider->GetOwner() == nullptr)
        return false;

    return EvaluateArcProbe(_OtherCollider, _FootPos, _OutNormal, _OutSignedDistance, _OutTransitionSurface, _OutContactPoint, _WasGround);
}
