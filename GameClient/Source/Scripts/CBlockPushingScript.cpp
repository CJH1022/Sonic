#include "pch.h"
#include "CBlockPushingScript.h"

#include "TimeMgr.h"
#include "CCollider2D.h"
#include "GameObject.h"
#include "CTransform.h"
#include "LevelMgr.h"
#include "CPlayerScript.h"
#include "CTileScript.h"

#include <cmath>

namespace
{
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
}

CBlockPushingScript::CBlockPushingScript()
    : CScript(SCRIPT_TYPE::BLOCKPUSHINGSCRIPT)
    , vNormal(Vec2(0.f, 0.f))
    , bIsMovePossible(true)
    , IsPushing(0)
    , m_pOnPlayer(nullptr)
{
}

CBlockPushingScript::~CBlockPushingScript()
{
}

void CBlockPushingScript::Begin()
{
    if (nullptr == Collider2D())
        return;

    Collider2D()->AddDynamicBeginOverlap(this, (COLLISION_EVENT)&CBlockPushingScript::BeginOverlap);
    Collider2D()->AddDynamicOverlap(this, (COLLISION_EVENT)&CBlockPushingScript::Overlap);
    Collider2D()->AddDynamicEndOverlap(this, (COLLISION_EVENT)&CBlockPushingScript::EndOverlap);
}

void CBlockPushingScript::Tick()
{
}

void CBlockPushingScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (_OtherCollider == nullptr)
        return;

    Overlap(_OwnCollider, _OtherCollider);
}

void CBlockPushingScript::Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (_OwnCollider == nullptr || _OtherCollider == nullptr)
        return;

    Ptr<CTileScript> pTile = _OtherCollider->GetOwner()->GetScript<CTileScript>();
    if (pTile != nullptr)
    {
        if (pTile->GetTileType() == TILETYPE::LINE_BLOCK_3 || pTile->GetTileType() == TILETYPE::LINE_BLOCK_6)
            bIsMovePossible = true;
        else
            bIsMovePossible = false;
        return;
    }

    Ptr<CPlayerScript> pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer == nullptr)
        return;

    const float epsilon = 0.01f;

    Vec3 blockPos = Transform()->GetRelativePos();
    Vec3 blockScale = Transform()->GetRelativeScale();
    Vec2 blockColOffset = _OwnCollider->GetOffset();
    Vec2 blockColScale = _OwnCollider->GetScale();

    Vec2 blockCenter = Vec2(blockPos.x + blockColOffset.x, blockPos.y + blockColOffset.y);
    float blockHalfX = fabsf(blockScale.x * blockColScale.x) * 0.5f;
    float blockHalfY = fabsf(blockScale.y * blockColScale.y) * 0.5f;

    Vec3 playerPos = pPlayer->Transform()->GetRelativePos();
    Vec3 playerScale = pPlayer->Transform()->GetRelativeScale();
    Vec2 playerColOffset = _OtherCollider->GetOffset();
    Vec2 playerColScale = _OtherCollider->GetScale();

    Vec2 playerCenter = Vec2(playerPos.x + playerColOffset.x, playerPos.y + playerColOffset.y);
    float playerHalfX = fabsf(playerScale.x * playerColScale.x) * 0.5f;
    float playerHalfY = fabsf(playerScale.y * playerColScale.y) * 0.5f;

    float dx = playerCenter.x - blockCenter.x;
    float dy = playerCenter.y - blockCenter.y;

    float overlapX = (blockHalfX + playerHalfX) - fabsf(dx);
    float overlapY = (blockHalfY + playerHalfY) - fabsf(dy);

    if (overlapX <= 0.f || overlapY <= 0.f)
        return;

    Vec2 vVelocity = pPlayer->GetVelocity();
    const bool bBreakOrRoll = pPlayer->IsBreakOrRollAction();

    if (overlapX < overlapY)
    {
        if (m_pOnPlayer == pPlayer.Get())
            m_pOnPlayer = nullptr;

        if (bBreakOrRoll)
        {
            IsPushing = 0;

            if (pPlayer->GetAction() == ActionState::Break)
            {
                vVelocity = Vec2(0.f, 0.f);
                pPlayer->SetVelocity(vVelocity);
                pPlayer->RequestBreakWallStop();
                return;
            }

            if (dx >= 0.f)
                playerPos.x += overlapX + epsilon;
            else
                playerPos.x -= overlapX + epsilon;

            vVelocity.x = 0.f;
            pPlayer->Transform()->SetRelativePos(playerPos);
            pPlayer->SetVelocity(vVelocity);
            pPlayer->StopBlockedAction();
            return;
        }

        bool bPushRight = false;
        bool bPushLeft = false;

        if (pPlayer->GetIsGround() && bIsMovePossible && dx < 0.f && pPlayer->GetFacing() == 1)
        {
            bPushRight = true;
            IsPushing = 1;
        }
        else if (pPlayer->GetIsGround() && bIsMovePossible && dx > 0.f && pPlayer->GetFacing() == -1)
        {
            bPushLeft = true;
            IsPushing = -1;
        }

        if (bPushRight)
        {
            IsPushing = 1;
            blockPos.x += overlapX + epsilon;
            Transform()->SetRelativePos(blockPos);
            vVelocity.x = 30.f;
        }
        else if (bPushLeft)
        {
            IsPushing = -1;
            blockPos.x -= overlapX + epsilon;
            Transform()->SetRelativePos(blockPos);
            vVelocity.x = -30.f;
        }
        else
        {
            const bool bBreakContact = (pPlayer->GetAction() == ActionState::Break);
            if (dx >= 0.f)
            {
                IsPushing = 1;
                playerPos.x += overlapX + epsilon;
                if (vVelocity.x < 0.f)
                    vVelocity.x = 0.f;
            }
            else
            {
                IsPushing = -1;
                playerPos.x -= overlapX + epsilon;
                if (vVelocity.x > 0.f)
                    vVelocity.x = 0.f;
            }

            if (bBreakContact)
            {
                pPlayer->SetVelocity(Vec2(0.f, 0.f));
                pPlayer->RequestBreakWallStop();
            }
            else
            {
                pPlayer->Transform()->SetRelativePos(playerPos);
                pPlayer->SetVelocity(vVelocity);
                pPlayer->StopBlockedAction();
            }
        }

        return;
    }

    if (dy >= 0.f)
    {
        playerPos.y += overlapY + epsilon;
        pPlayer->ForceFlatGroundContact(0.12f);
        m_pOnPlayer = pPlayer.Get();

        if (vVelocity.y < 0.f)
            vVelocity.y = 0.f;
    }
    else
    {
        if (m_pOnPlayer == pPlayer.Get())
            m_pOnPlayer = nullptr;

        playerPos.y -= overlapY + epsilon;

        if (vVelocity.y > 0.f)
            vVelocity.y = 0.f;
    }

    pPlayer->Transform()->SetRelativePos(playerPos);
    pPlayer->SetVelocity(vVelocity);
}

void CBlockPushingScript::EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (_OtherCollider == nullptr)
        return;

    Ptr<CPlayerScript> pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer == nullptr)
        return;

    IsPushing = 0;

    if (m_pOnPlayer == pPlayer.Get())
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
