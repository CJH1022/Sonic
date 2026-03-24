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

CBlockPushingScript::CBlockPushingScript()
    : CScript(SCRIPT_TYPE::BLOCKPUSHINGSCRIPT)
    , vNormal(Vec2(0.f, 0.f))
    , bIsMovePossible(true)
    , IsPushing(0)
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

    if (nullptr == _OtherCollider)
        return;
}

void CBlockPushingScript::Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    // 1. 타일과 충돌한 경우의 처리
    Ptr<CTileScript> pTile = _OtherCollider->GetOwner()->GetScript<CTileScript>();
    if (pTile != nullptr)
    {
        // 바닥이 밀 수 있는 블럭인지 판단하여 멤버 변수 갱신
        if (pTile->GetTileType() == TILETYPE::LINE_BLOCK_3 || pTile->GetTileType() == TILETYPE::LINE_BLOCK_6)
        {
            bIsMovePossible = true;
        }
        else
        {
            bIsMovePossible = false;
        }
        return; // 타일과의 충돌 처리는 여기서 끝
    }

    Ptr<CPlayerScript> pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer == nullptr)
        return;

    const float epsilon = 0.01f;

    // =========================
    // 박스 정보
    // =========================
    Vec3 blockPos = Transform()->GetRelativePos();
    Vec3 blockScale = Transform()->GetRelativeScale();

    Vec2 blockColOffset = _OwnCollider->GetOffset();
    Vec2 blockColScale = _OwnCollider->GetScale();

    Vec2 blockCenter = Vec2(blockPos.x + blockColOffset.x, blockPos.y + blockColOffset.y);
    float blockHalfX = fabsf(blockScale.x * blockColScale.x) * 0.5f;
    float blockHalfY = fabsf(blockScale.y * blockColScale.y) * 0.5f;

    // =========================
    // 플레이어 정보
    // =========================
    Vec3 playerPos = pPlayer->Transform()->GetRelativePos();
    Vec3 playerScale = pPlayer->Transform()->GetRelativeScale();

    Vec2 playerColOffset = _OtherCollider->GetOffset();
    Vec2 playerColScale = _OtherCollider->GetScale();

    Vec2 playerCenter = Vec2(playerPos.x + playerColOffset.x, playerPos.y + playerColOffset.y);
    float playerHalfX = fabsf(playerScale.x * playerColScale.x) * 0.5f;
    float playerHalfY = fabsf(playerScale.y * playerColScale.y) * 0.5f;

    // =========================
    // AABB 충돌량 계산
    // =========================
    float dx = playerCenter.x - blockCenter.x;
    float dy = playerCenter.y - blockCenter.y;

    float overlapX = (blockHalfX + playerHalfX) - fabsf(dx);
    float overlapY = (blockHalfY + playerHalfY) - fabsf(dy);

    if (overlapX <= 0.f || overlapY <= 0.f)
        return;

    Vec2 vVelocity = pPlayer->GetVelocity();
    const bool bBreakOrRoll = pPlayer->IsBreakOrRollAction();

    // ==================================
    // 가로 충돌
    // ==================================
    if (overlapX < overlapY)
    {
    if (bBreakOrRoll)
    {
        IsPushing = 0;

        if (pPlayer->GetAction() == ActionState::Break)
        {
            vVelocity.x = 0.f;
            vVelocity.y = 0.f;
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

        // 플레이어가 박스의 왼쪽에 있고, 오른쪽으로 움직이는 중
        if (pPlayer->GetIsGround() && bIsMovePossible && dx < 0.f && pPlayer->GetFacing() == 1)
        {
            bPushRight = true;
            IsPushing = 1;
        }
        // 플레이어가 박스의 오른쪽에 있고, 왼쪽으로 움직이는 중
        else if (pPlayer->GetIsGround() && bIsMovePossible && dx > 0.f && pPlayer->GetFacing() == -1)
        {
            bPushLeft = true;
            IsPushing = -1;
        }

        if (bPushRight)
        {
            // 박스를 오른쪽으로 민다
            IsPushing = 1;
            blockPos.x += overlapX + epsilon;
            Transform()->SetRelativePos(blockPos);
            vVelocity.x = 30.f;
        }
        else if (bPushLeft)
        {
            // 박스를 왼쪽으로 민다
            IsPushing = -1;
            blockPos.x -= overlapX + epsilon;
            Transform()->SetRelativePos(blockPos);
            vVelocity.x = -30.f;
        }
        else
        {
            // 못 미는 상황이면 플레이어만 밀어낸다
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

    // ==================================
    // 세로 충돌
    // ==================================
    if (dy >= 0.f)
    {
        // 플레이어가 박스 위에 있음
        playerPos.y += overlapY + epsilon;

        pPlayer->SetIsGround(true);
        pPlayer->SetNormal(Vec2(0.f, 1.f));
        pPlayer->SetGroundTangent(Vec2(1.f, 0.f));

        if (vVelocity.y < 0.f)
            vVelocity.y = 0.f;
    }
    else
    {
        // 플레이어가 박스 아래에서 머리 부딪힘
        playerPos.y -= overlapY + epsilon;

        if (vVelocity.y > 0.f)
            vVelocity.y = 0.f;
    }

    pPlayer->Transform()->SetRelativePos(playerPos);
    pPlayer->SetVelocity(vVelocity);
}

void CBlockPushingScript::EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    Ptr<CPlayerScript> pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer != nullptr)
    {
        IsPushing = 0;
        return;
    }

    if (nullptr == _OtherCollider)
        return;

    // 여기서 플레이어 SetIsGround(true) 하면 안 됨
}
