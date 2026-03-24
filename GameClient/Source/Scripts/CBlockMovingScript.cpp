#include "pch.h"
#include "CBlockScript.h"
#include "TimeMgr.h"
#include "CCollider2D.h"
#include "GameObject.h"
#include "CTransform.h"
#include "LevelMgr.h"
#include "CPlayerScript.h"
#include "CBlockMovingScript.h"
#include <cmath>

namespace
{
    struct ContactInfo
    {
        float myCenterY = 0.f;
        float otherCenterY = 0.f;
        float myHalfX = 0.f;
        float myHalfY = 0.f;
        float otherHalfY = 0.f;
        float otherOffsetY = 0.f;
        float overlapX = 0.f;
        float overlapY = 0.f;
    };

    bool BuildContactInfo(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider, ContactInfo& _Out)
    {
        if (_OwnCollider == nullptr || _OtherCollider == nullptr)
            return false;

        Vec3 myTransformPos = _OwnCollider->GetOwner()->Transform()->GetRelativePos();
        Vec3 myTransformScale = _OwnCollider->GetOwner()->Transform()->GetRelativeScale();
        Vec3 otherTransformPos = _OtherCollider->GetOwner()->Transform()->GetRelativePos();
        Vec3 otherTransformScale = _OtherCollider->GetOwner()->Transform()->GetRelativeScale();

        Vec2 myColOffset = _OwnCollider->GetOffset();
        Vec2 myColScale = _OwnCollider->GetScale();
        Vec2 otherColOffset = _OtherCollider->GetOffset();
        Vec2 otherColScale = _OtherCollider->GetScale();

        Vec2 myCenter = Vec2(myTransformPos.x + myColOffset.x, myTransformPos.y + myColOffset.y);
        Vec2 otherCenter = Vec2(otherTransformPos.x + otherColOffset.x, otherTransformPos.y + otherColOffset.y);

        float myHalfX = fabsf(myTransformScale.x * myColScale.x) * 0.5f;
        float myHalfY = fabsf(myTransformScale.y * myColScale.y) * 0.5f;
        float otherHalfX = fabsf(otherTransformScale.x * otherColScale.x) * 0.5f;
        float otherHalfY = fabsf(otherTransformScale.y * otherColScale.y) * 0.5f;

        float dx = otherCenter.x - myCenter.x;
        float dy = otherCenter.y - myCenter.y;

        float overlapX = (myHalfX + otherHalfX) - fabsf(dx);
        float overlapY = (myHalfY + otherHalfY) - fabsf(dy);

        if (overlapX <= 0.f || overlapY <= 0.f)
            return false;

        _Out.myCenterY = myCenter.y;
        _Out.otherCenterY = otherCenter.y;
        _Out.myHalfX = myHalfX;
        _Out.myHalfY = myHalfY;
        _Out.otherHalfY = otherHalfY;
        _Out.otherOffsetY = otherColOffset.y;
        _Out.overlapX = overlapX;
        _Out.overlapY = overlapY;
        return true;
    }

    bool IsTopContact(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
    {
        ContactInfo info = {};
        if (!BuildContactInfo(_OwnCollider, _OtherCollider, info))
            return false;

        float dy = info.otherCenterY - info.myCenterY;
        float blockTop = info.myCenterY + info.myHalfY;
        float playerBottom = info.otherCenterY - info.otherHalfY;
        const float topTolerance = 8.f;

        return (dy > 0.f)
            && (info.overlapY <= info.overlapX + 2.f)
            && (playerBottom >= blockTop - topTolerance);
    }

    bool IsAscendingFromBelow(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider, const Vec2& _Velocity)
    {
        ContactInfo info = {};
        if (!BuildContactInfo(_OwnCollider, _OtherCollider, info))
            return false;

        const float upVelThreshold = 5.f;
        bool bBelow = (info.otherCenterY < info.myCenterY);
        bool bMovingUp = (_Velocity.y > upVelThreshold);
        return bBelow && bMovingUp;
    }

    void SnapPlayerToTop(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider, CPlayerScript* _Player)
    {
        if (_OwnCollider == nullptr || _OtherCollider == nullptr || _Player == nullptr)
            return;

        ContactInfo info = {};
        if (!BuildContactInfo(_OwnCollider, _OtherCollider, info))
            return;

        float blockTop = info.myCenterY + info.myHalfY;
        const float skin = 0.05f;

        Vec3 playerPos = _OtherCollider->GetOwner()->Transform()->GetRelativePos();
        float targetCenterY = blockTop + info.otherHalfY + skin;
        playerPos.y = targetCenterY - info.otherOffsetY;
        _OtherCollider->GetOwner()->Transform()->SetRelativePos(playerPos);

        Vec2 vel = _Player->GetVelocity();
        if (vel.y < 0.f)
            vel.y = 0.f;
        _Player->SetVelocity(vel);
        _Player->SetIsGround(true);
        _Player->SetNormal(Vec2(0.f, 1.f));
        _Player->SetGroundTangent(Vec2(1.f, 0.f));
    }

    bool IsWithinHorizontalRange(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider, float _Margin)
    {
        if (_OwnCollider == nullptr || _OtherCollider == nullptr)
            return false;

        Vec3 myTransformPos = _OwnCollider->GetOwner()->Transform()->GetRelativePos();
        Vec3 myTransformScale = _OwnCollider->GetOwner()->Transform()->GetRelativeScale();
        Vec3 otherTransformPos = _OtherCollider->GetOwner()->Transform()->GetRelativePos();
        Vec3 otherTransformScale = _OtherCollider->GetOwner()->Transform()->GetRelativeScale();

        Vec2 myColOffset = _OwnCollider->GetOffset();
        Vec2 myColScale = _OwnCollider->GetScale();
        Vec2 otherColOffset = _OtherCollider->GetOffset();
        Vec2 otherColScale = _OtherCollider->GetScale();

        float myCenterX = myTransformPos.x + myColOffset.x;
        float otherCenterX = otherTransformPos.x + otherColOffset.x;
        float myHalfX = fabsf(myTransformScale.x * myColScale.x) * 0.5f;
        float otherHalfX = fabsf(otherTransformScale.x * otherColScale.x) * 0.5f;

        float dx = fabsf(otherCenterX - myCenterX);
        return dx <= (myHalfX + otherHalfX + _Margin);
    }

    bool IsFootInsideTopSurface(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider, float _Margin)
    {
        if (_OwnCollider == nullptr || _OtherCollider == nullptr)
            return false;

        Vec3 myTransformPos = _OwnCollider->GetOwner()->Transform()->GetRelativePos();
        Vec3 myTransformScale = _OwnCollider->GetOwner()->Transform()->GetRelativeScale();
        Vec3 otherTransformPos = _OtherCollider->GetOwner()->Transform()->GetRelativePos();

        Vec2 myColOffset = _OwnCollider->GetOffset();
        Vec2 myColScale = _OwnCollider->GetScale();
        Vec2 otherColOffset = _OtherCollider->GetOffset();

        float myCenterX = myTransformPos.x + myColOffset.x;
        float footX = otherTransformPos.x + otherColOffset.x;
        float myHalfX = fabsf(myTransformScale.x * myColScale.x) * 0.5f;

        float left = myCenterX - myHalfX - _Margin;
        float right = myCenterX + myHalfX + _Margin;
        return (left <= footX && footX <= right);
    }
}

CBlockMovingScript::CBlockMovingScript()
    : CScript(SCRIPT_TYPE::BLOCKMOVINGSCRIPT)
    , vstartPos(Vec3(0.f, 0.f, 0.f))
    , vendPos(Vec3(0.f, 0.f, 0.f))
    , vVelocity(Vec2(100.f, 100.f))
    , m_speed(100.f)
    , Dist(0.f)
    , curDist(0.f)
    , curState(MOVESTATE::START)
    , m_vPrevPos(Vec3(0.f, 0.f, 0.f))
    , m_pOnPlayer(nullptr)
{
}

CBlockMovingScript::~CBlockMovingScript()
{
}

void CBlockMovingScript::Begin()
{
    Collider2D()->AddDynamicBeginOverlap(this, (COLLISION_EVENT)&CBlockMovingScript::BeginOverlap);
    Collider2D()->AddDynamicOverlap(this, (COLLISION_EVENT)&CBlockMovingScript::Overlap);
    Collider2D()->AddDynamicEndOverlap(this, (COLLISION_EVENT)&CBlockMovingScript::EndOverlap);

    Vec3 curPos = GetOwner()->Transform()->GetRelativePos();
    m_vPrevPos = curPos;

    if (vstartPos == vendPos)
    {
        vstartPos = curPos;
        vendPos = curPos;
        vDir = Vec2(0.f, 0.f);
        Dist = 0.f;
    }
    else
    {
        vDir = Vec2(vendPos.x - vstartPos.x, vendPos.y - vstartPos.y);
        if (vDir.LengthSquared() > 0.000001f)
            vDir.Normalize();
        Dist = (vendPos - vstartPos).Length();
    }

    float sx = fabsf(vVelocity.x);
    float sy = fabsf(vVelocity.y);
    m_speed = (sx > sy) ? sx : sy;
    if (m_speed <= 0.0001f)
        m_speed = 100.f;
}

void CBlockMovingScript::Tick()
{
    Vec3 vMyPos = GetOwner()->Transform()->GetRelativePos();

    if (Dist > 0.0001f && vDir.LengthSquared() > 0.000001f)
    {
        (curState == MOVESTATE::START) ? curDist = (vMyPos - vstartPos).Length() : curDist = (vMyPos - vendPos).Length();

        if (Dist < curDist)
        {
            curState = (curState == MOVESTATE::START) ? MOVESTATE::BACK : MOVESTATE::START;
            vMyPos = (curState == MOVESTATE::START) ? vstartPos : vendPos;
        }

        curvDir = (curState == MOVESTATE::START) ? vDir : -vDir;
        vMyPos.x += curvDir.x * m_speed * DT;
        vMyPos.y += curvDir.y * m_speed * DT;
        vMyPos.z = 10.f;
        GetOwner()->Transform()->SetRelativePos(vMyPos);
    }

    if (m_pOnPlayer != nullptr)
    {
        Ptr<CCollider2D> pOwnCol = Collider2D();
        Ptr<CCollider2D> pPlayerCol = m_pOnPlayer->GetOwner()->Collider2D();

        const float footMargin = 0.f;
        const float jumpDetachVel = 80.f;
        bool bOutOfTopSurface = (pOwnCol == nullptr || pPlayerCol == nullptr)
            ? true
            : !IsFootInsideTopSurface(pOwnCol.Get(), pPlayerCol.Get(), footMargin);
        bool bJumpDetach = (m_pOnPlayer->GetVelocity().y > jumpDetachVel);

        if (bOutOfTopSurface || bJumpDetach)
        {
            m_pOnPlayer->SetIsGround(false);
            m_pOnPlayer = nullptr;
            m_vPrevPos = vMyPos;
            return;
        }

        Vec3 vDeltaPos = vMyPos - m_vPrevPos;
        Vec3 vPlayerPos = m_pOnPlayer->GetOwner()->Transform()->GetRelativePos();
        vPlayerPos.x += vDeltaPos.x;
        vPlayerPos.y += vDeltaPos.y;
        m_pOnPlayer->GetOwner()->Transform()->SetRelativePos(vPlayerPos);

        m_pOnPlayer->SetIsGround(true);
        m_pOnPlayer->SetNormal(Vec2(0.f, 1.f));
        m_pOnPlayer->SetGroundTangent(Vec2(1.f, 0.f));
    }

    m_vPrevPos = vMyPos;
}

void CBlockMovingScript::Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    Ptr<CPlayerScript> pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer == nullptr)
        return;

    if (IsAscendingFromBelow(_OwnCollider, _OtherCollider, pPlayer->GetVelocity()))
    {
        if (m_pOnPlayer == pPlayer)
        {
            m_pOnPlayer = nullptr;
            pPlayer->SetIsGround(false);
        }
        return;
    }

    if (IsTopContact(_OwnCollider, _OtherCollider))
    {
        if (pPlayer->GetVelocity().y <= 50.f)
        {
            SnapPlayerToTop(_OwnCollider, _OtherCollider, pPlayer.Get());
            m_pOnPlayer = pPlayer;
        }
    }
    else if (m_pOnPlayer == pPlayer)
    {
        // Overlap에서 즉시 해제하면 하강 발판에서 떨어지기 쉬워 EndOverlap에서 최종 판단
        if (pPlayer->GetVelocity().y > 80.f)
        {
            m_pOnPlayer = nullptr;
            pPlayer->SetIsGround(false);
        }
    }
}

void CBlockMovingScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    Ptr<CPlayerScript> pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer == nullptr)
        return;

    if (IsAscendingFromBelow(_OwnCollider, _OtherCollider, pPlayer->GetVelocity()))
        return;

    if (IsTopContact(_OwnCollider, _OtherCollider))
    {
        if (pPlayer->GetVelocity().y <= 50.f)
        {
            SnapPlayerToTop(_OwnCollider, _OtherCollider, pPlayer.Get());
            m_pOnPlayer = pPlayer;
        }
    }
}

void CBlockMovingScript::EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    Ptr<CPlayerScript> pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer == nullptr)
        return;

    if (m_pOnPlayer == pPlayer)
    {
        const float jumpDetachVel = 80.f;
        const float horizontalMargin = 8.f;

        bool bJumpDetach = (pPlayer->GetVelocity().y > jumpDetachVel);
        bool bInsideX = IsWithinHorizontalRange(_OwnCollider, _OtherCollider, horizontalMargin);

        if (bJumpDetach || !bInsideX)
        {
            pPlayer->SetIsGround(false);
            m_pOnPlayer = nullptr;
        }
    }
}
