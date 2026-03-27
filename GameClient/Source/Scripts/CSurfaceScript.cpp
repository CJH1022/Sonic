#include "pch.h"
#include "CSurfaceScript.h"

#include "CCollider2D.h"
#include "GameObject.h"
#include "CTransform.h"
#include "CPlayerScript.h"
#include "TimeMgr.h"

#include <cmath>

namespace
{
    constexpr float kColliderMinThickness = 1.f;
    // Circle half-check transition should use a tiny world-space margin. A large
    // value caused early exit/edge snapping when the player was still safely on
    // the circular segment.
    constexpr float kCircleHalfCheckMargin = 0.12f;
    constexpr float kProjectionResolveEpsilon = 0.0001f;
    constexpr float kCircleHalfCheckBlockTime = 0.12f;
    constexpr float kEndpointTransitionMargin = 0.12f;
    constexpr float kDetachThresholdAir = 0.5f;
    constexpr float kDetachThresholdGround = 2.f;
    constexpr float kTransitionSnapThresholdAir = 0.5f;
    constexpr float kTransitionSnapThresholdGround = 8.f;
    constexpr float kSurfaceEdgeEpsilon = 0.02f;
    constexpr float kSurfaceContactPositiveEpsilon = 0.03f;
    constexpr float kSupportPointInsideMargin = 0.08f;
    constexpr float kArcTransitionLocalMargin = 0.04f;
    constexpr float kLineSupportProjectionMargin = 0.08f;

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
        Vec3 playerPos = pOwner->Transform()->GetWorldPos();
        Vec3 playerScale = pOwner->Transform()->GetRelativeScale();

        float halfW = fabsf(playerScale.x) * 0.5f;
        float halfH = fabsf(playerScale.y) * 0.5f;

        _OutFootPos = Vec2(playerPos.x, playerPos.y);
        if (_WasGround)
        {
            float rotZ = pOwner->Transform()->GetRelativeRot().z;
            Vec2 localDown = Vec2(sinf(rotZ), -cosf(rotZ));
            _OutFootPos += localDown * halfH;
        }
        else
        {
            float radius = (halfW < halfH) ? halfW : halfH;
            _OutFootPos.y -= radius;
        }

        return true;
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
{
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
{
}

CSurfaceScript::~CSurfaceScript()
{
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
}

void CSurfaceScript::Tick()
{
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

    if (pPlayer->IsSurfaceAttachBlocked(GetOwner()))
        return;

    const bool wasGround = pPlayer->GetIsGround();
    Vec2 footPos = {};
    if (!GetPlayerFootPos(_OtherCollider, wasGround, footPos))
        return;

    Vec2 normal = {};
    float signedDistance = 0.f;
    bool transitionSurface = false;
    float seamBlendT = 0.f;
    bool seamBlendHasValue = false;
    Vec2 seamStart = {};
    Vec2 seamEnd = {};
    Vec2 contactPoint = {};
    bool hit = false;

    // Keep the original geometric solver for all surface roles (surface/correction/
    // wall). The role only changes how player control logic treats the hit, not
    // the contact normal/signature itself.
    if (m_Geometry == SURFACE_GEOMETRY::LINE)
        hit = EvaluateLineProbe(_OtherCollider, footPos, normal, signedDistance, transitionSurface, seamBlendT, seamBlendHasValue, seamStart, seamEnd, contactPoint);
    else if (m_Geometry == SURFACE_GEOMETRY::CIRCLE)
        hit = EvaluateCircleProbe(_OtherCollider, footPos, normal, signedDistance, transitionSurface);
    else if (m_Geometry == SURFACE_GEOMETRY::ARC)
        hit = EvaluateArcProbe(_OtherCollider, footPos, normal, signedDistance, transitionSurface);
    else
        hit = EvaluateWallProbe(_OtherCollider, normal, signedDistance);

    if (!hit)
        return;

    const float detachThreshold = wasGround ? kDetachThresholdGround : kDetachThresholdAir;
    const float transitionSnapThreshold = wasGround ? kTransitionSnapThresholdGround : kTransitionSnapThresholdAir;
    if (signedDistance > transitionSnapThreshold)
        return;

    if (signedDistance > kSurfaceContactPositiveEpsilon)
        return;

    transitionSurface = transitionSurface || (signedDistance > detachThreshold);

    float candidateScore = fabsf(signedDistance);
    if (transitionSurface)
        candidateScore -= 0.15f;
    if (m_Role == SURFACE_ROLE::WALL)
        candidateScore += 0.25f;

    pPlayer->SubmitSurfaceContact(GetOwner(), normal, signedDistance, transitionSurface,
                                  m_Attachable, (m_Role == SURFACE_ROLE::WALL), m_Geometry == SURFACE_GEOMETRY::CIRCLE,
                                  candidateScore,
                                  seamBlendT,
                                  seamBlendHasValue,
                                  seamStart,
                                  seamEnd,
                                  contactPoint);
}

void CSurfaceScript::EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
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

void CSurfaceScript::GetWorldEndpoints(Vec2& _OutStart, Vec2& _OutEnd)
{
    Vec3 worldPos = Transform()->GetWorldPos();
    _OutStart = Vec2(worldPos.x + m_LocalStart.x, worldPos.y + m_LocalStart.y);
    _OutEnd = Vec2(worldPos.x + m_LocalEnd.x, worldPos.y + m_LocalEnd.y);
}

void CSurfaceScript::GetArcWorldData(Vec2& _OutCenter, float& _OutRadius, Vec2& _OutBoxMin, Vec2& _OutBoxMax)
{
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
    if (nullptr == Transform() || nullptr == Collider2D())
        return;

    float minX = min(m_LocalStart.x, m_LocalEnd.x);
    float maxX = max(m_LocalStart.x, m_LocalEnd.x);
    float minY = min(m_LocalStart.y, m_LocalEnd.y);
    float maxY = max(m_LocalStart.y, m_LocalEnd.y);

    if (m_Geometry == SURFACE_GEOMETRY::ARC || m_Geometry == SURFACE_GEOMETRY::CIRCLE)
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

    Transform()->SetIndependentScale(true);
    Transform()->SetRelativeScale(Vec3(width, height, 1.f));
    Collider2D()->SetOffset(normalizedOffset);
    Collider2D()->SetScale(Vec2(1.f, 1.f));
}

void CSurfaceScript::GetWorldBounds(Vec2& _OutMin, Vec2& _OutMax)
{
    float minX = min(m_LocalStart.x, m_LocalEnd.x);
    float maxX = max(m_LocalStart.x, m_LocalEnd.x);
    float minY = min(m_LocalStart.y, m_LocalEnd.y);
    float maxY = max(m_LocalStart.y, m_LocalEnd.y);

    if (m_Geometry == SURFACE_GEOMETRY::ARC || m_Geometry == SURFACE_GEOMETRY::CIRCLE)
    {
        Vec2 center = {};
        float radius = 0.f;
        GetArcWorldData(center, radius, _OutMin, _OutMax);
    }
    else
    {
        Vec3 worldPos = Transform()->GetWorldPos();

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

    Vec3 playerPos = _OtherCollider->GetOwner()->Transform()->GetWorldPos();
    Vec3 playerScale = _OtherCollider->GetOwner()->Transform()->GetRelativeScale();
    Vec2 colOffset = _OtherCollider->GetOffset();
    Vec2 colScale = _OtherCollider->GetScale();

    Vec2 center = Vec2(playerPos.x + colOffset.x, playerPos.y + colOffset.y);
    float halfW = fabsf(playerScale.x * colScale.x) * 0.5f;
    float halfH = fabsf(playerScale.y * colScale.y) * 0.5f;

    Vec2 n = NormalizeSafe(_SurfaceNormal);
    float support = fabsf(n.x) * halfW + fabsf(n.y) * halfH;
    _OutSupportPoint = center - n * support;
    return true;
}

bool CSurfaceScript::EvaluateLineProbe(CCollider2D* _OtherCollider, const Vec2& _FootPos, Vec2& _OutNormal, float& _OutSignedDistance, bool& _OutTransitionSurface, float& _OutSeamBlendT, bool& _OutSeamBlendHasValue, Vec2& _OutSeamStart, Vec2& _OutSeamEnd, Vec2& _OutContactPoint)
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

    const float rawWidth = fabsf(worldB.x - worldA.x);
    const float rawHeight = fabsf(worldB.y - worldA.y);
    if (rawWidth <= 0.001f && rawHeight <= 0.001f)
        return false;

    // Very steep "line" segments are effectively x(y) instead of y(x).
    // Keep a dedicated segment fallback for that case only, and use the
    // formula-based path for the normal slope/flat cases.
    if (rawWidth <= kColliderMinThickness)
    {
        Vec2 delta = worldB - worldA;
        float length = Length(delta);
        if (length <= 0.001f)
            return false;

        Vec2 tangent = delta / length;
        Vec2 upNormal = Vec2(-tangent.y, tangent.x);
        Vec2 outward = m_FillAbove ? -upNormal : upNormal;

        float projection = Dot(_FootPos - worldA, tangent);
        float projectionNorm = projection / length;
        if (projectionNorm < -kEndpointTransitionMargin || projectionNorm > 1.f + kEndpointTransitionMargin)
            return false;

        float clampedProjectionNorm = Clamp01(projectionNorm);
        Vec2 closest = worldA + tangent * (clampedProjectionNorm * length);
        Vec2 supportPoint = _FootPos;
        if (!GetPlayerSupportPoint(_OtherCollider, outward, supportPoint))
            return false;
        Vec2 lineMin = Vec2(min(worldA.x, worldB.x), min(worldA.y, worldB.y));
        Vec2 lineMax = Vec2(max(worldA.x, worldB.x), max(worldA.y, worldB.y));
        if (!IsInsideExpandedBox(supportPoint, lineMin, lineMax, kSupportPointInsideMargin))
            return false;

        _OutSignedDistance = Dot(supportPoint - closest, outward);
        _OutNormal = outward;
        _OutSeamBlendT = Clamp01(projectionNorm);
        _OutSeamBlendHasValue = true;
        _OutSeamStart = worldA;
        _OutSeamEnd = worldB;
        _OutContactPoint = _FootPos;
        _OutTransitionSurface =
            projectionNorm <= kEndpointTransitionMargin ||
            projectionNorm >= 1.f - kEndpointTransitionMargin ||
            fabsf(projectionNorm - clampedProjectionNorm) > kProjectionResolveEpsilon;
        return true;
    }

    Vec2 ab = worldB - worldA;
    float len2 = Dot(ab, ab);
    if (len2 <= 0.0001f)
        return false;

    float projection = Dot(_FootPos - worldA, ab) / len2;
    if (projection < -kEndpointTransitionMargin || projection > 1.f + kEndpointTransitionMargin)
        return false;

    float projectionNorm = Clamp01(projection);
    float clampedFootX = worldA.x + ab.x * projectionNorm;
    float clampedFootY = worldA.y + ab.y * projectionNorm;
    Vec2 clampedFootPoint = Vec2(clampedFootX, clampedFootY);

    const float left = min(worldA.x, worldB.x);
    const float right = max(worldA.x, worldB.x);
    const float top = max(worldA.y, worldB.y);
    const float bottom = min(worldA.y, worldB.y);
    const float width = max(kColliderMinThickness, right - left);
    const float height = max(kColliderMinThickness, top - bottom);

    float localX = (clampedFootPoint.x - left) / width;
    float localY = (top - clampedFootPoint.y) / height;

    if (localX < -kSurfaceEdgeEpsilon || localX > 1.f + kSurfaceEdgeEpsilon ||
        localY < -kSurfaceEdgeEpsilon || localY > 1.f + kSurfaceEdgeEpsilon)
        return false;

    localX = Clamp01(localX);
    localY = Clamp01(localY);

    const float localA = Clamp01((top - worldA.y) / height);
    const float localB = Clamp01((top - worldB.y) / height);
    const float c = m_FillAbove ? 1.f : -1.f;

    const float dfdx = c * (localA - localB);
    const float dfdy = c;

    const float gradWorldX = dfdx / width;
    const float gradWorldY = -dfdy / height;
    const float gradWorldLen = sqrtf(gradWorldX * gradWorldX + gradWorldY * gradWorldY);
    if (gradWorldLen <= 0.0001f)
        return false;

    Vec2 candidateNormal = Vec2(gradWorldX / gradWorldLen, gradWorldY / gradWorldLen);
    Vec2 supportPoint = _FootPos;
    if (!GetPlayerSupportPoint(_OtherCollider, candidateNormal, supportPoint))
        return false;

    float supportProjection = Dot(supportPoint - worldA, ab) / len2;
    if (supportProjection < -kLineSupportProjectionMargin || supportProjection > 1.f + kLineSupportProjectionMargin)
        return false;

    float supportLocalX = (supportPoint.x - left) / width;
    float supportLocalY = (top - supportPoint.y) / height;
    const float f = c * (supportLocalY - ((localB - localA) * supportLocalX + localA));

    _OutNormal = candidateNormal;
    _OutSignedDistance = f / gradWorldLen;
    _OutSeamBlendT = Clamp01(projectionNorm);
    _OutSeamBlendHasValue = true;
    _OutSeamStart = worldA;
    _OutSeamEnd = worldB;
    _OutContactPoint = _FootPos;
    _OutTransitionSurface =
        projection <= kEndpointTransitionMargin ||
        projection >= 1.f - kEndpointTransitionMargin ||
        localY <= kArcTransitionLocalMargin ||
        localY >= 1.f - kArcTransitionLocalMargin;
    return true;
}

bool CSurfaceScript::EvaluateArcProbe(CCollider2D* _OtherCollider, const Vec2& _FootPos, Vec2& _OutNormal, float& _OutSignedDistance, bool& _OutTransitionSurface)
{
    _OutTransitionSurface = false;

    Vec2 arcCenter = {};
    float radius = 0.f;
    Vec2 boxMin = {};
    Vec2 boxMax = {};
    GetArcWorldData(arcCenter, radius, boxMin, boxMax);

    const float width = max(kColliderMinThickness, boxMax.x - boxMin.x);
    const float height = max(kColliderMinThickness, boxMax.y - boxMin.y);
    const float top = boxMax.y;

    float localX = (_FootPos.x - boxMin.x) / width;
    float localY = (top - _FootPos.y) / height;

    if (localX < -kSurfaceEdgeEpsilon || localX > 1.f + kSurfaceEdgeEpsilon ||
        localY < -kSurfaceEdgeEpsilon || localY > 1.f + kSurfaceEdgeEpsilon)
        return false;

    localX = Clamp01(localX);
    localY = Clamp01(localY);

    float centerX = 0.f;
    float centerY = 0.f;
    switch (m_ArcCorner)
    {
    case ARC_CORNER::TOP_LEFT:
        centerX = 0.f;
        centerY = 0.f;
        break;
    case ARC_CORNER::TOP_RIGHT:
        centerX = 1.f;
        centerY = 0.f;
        break;
    case ARC_CORNER::BOTTOM_LEFT:
        centerX = 0.f;
        centerY = 1.f;
        break;
    case ARC_CORNER::BOTTOM_RIGHT:
    default:
        centerX = 1.f;
        centerY = 1.f;
        break;
    }

    const float c = m_FillInside ? 1.f : -1.f;
    const float dx = localX - centerX;
    const float dy = localY - centerY;
    const float dfdx = c * 2.f * dx;
    const float dfdy = c * 2.f * dy;

    const float gradWorldX = dfdx / width;
    const float gradWorldY = -dfdy / height;
    const float gradWorldLen = sqrtf(gradWorldX * gradWorldX + gradWorldY * gradWorldY);
    if (gradWorldLen <= 0.0001f)
        return false;

    Vec2 candidateNormal = Vec2(gradWorldX / gradWorldLen, gradWorldY / gradWorldLen);
    Vec2 supportPoint = _FootPos;
    if (!GetPlayerSupportPoint(_OtherCollider, candidateNormal, supportPoint))
        return false;
    if (!IsInsideExpandedBox(supportPoint, boxMin, boxMax, kSupportPointInsideMargin))
        return false;

    float supportLocalX = (supportPoint.x - boxMin.x) / width;
    float supportLocalY = (top - supportPoint.y) / height;
    const float supportDx = supportLocalX - centerX;
    const float supportDy = supportLocalY - centerY;
    const float supportF = c * (supportDx * supportDx + supportDy * supportDy - 1.f);

    _OutNormal = candidateNormal;
    _OutSignedDistance = supportF / gradWorldLen;
    _OutTransitionSurface =
        localX <= kEndpointTransitionMargin ||
        localX >= 1.f - kEndpointTransitionMargin ||
        localX <= kArcTransitionLocalMargin ||
        localX >= 1.f - kArcTransitionLocalMargin ||
        localY <= kArcTransitionLocalMargin ||
        localY >= 1.f - kArcTransitionLocalMargin;
    return true;
}

bool CSurfaceScript::EvaluateCircleProbe(CCollider2D* _OtherCollider, const Vec2& _FootPos, Vec2& _OutNormal, float& _OutSignedDistance, bool& _OutTransitionSurface)
{
    _OutTransitionSurface = false;

    if (_OtherCollider == nullptr || _OtherCollider->GetOwner() == nullptr)
        return false;

    Vec2 arcCenter = {};
    float radius = 0.f;
    Vec2 boxMin = {};
    Vec2 boxMax = {};
    GetArcWorldData(arcCenter, radius, boxMin, boxMax);

    if (IsPastCircleExitEdge(m_ArcCorner, _FootPos, boxMin, boxMax, kCircleHalfCheckMargin))
    {
        auto pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
        if (pPlayer != nullptr)
            pPlayer->BlockSurfaceAttach(GetOwner(), kCircleHalfCheckBlockTime);
        return false;
    }

    return EvaluateArcProbe(_OtherCollider, _FootPos, _OutNormal, _OutSignedDistance, _OutTransitionSurface);
}
