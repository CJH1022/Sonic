#include "pch.h"
#include "CBlockScript.h"
#include "TimeMgr.h"
#include "CCollider2D.h"
#include "GameObject.h"
#include "CTransform.h"
#include "LevelMgr.h"
#include "CPlayerScript.h"

CBlockScript::CBlockScript()
	: CScript(SCRIPT_TYPE::BLOCKSCRIPT)
{
}

CBlockScript::~CBlockScript()
{ 
}

void CBlockScript::Begin()
{
    Collider2D()->AddDynamicBeginOverlap(this, (COLLISION_EVENT)&CBlockScript::BeginOverlap);
	Collider2D()->AddDynamicOverlap(this, (COLLISION_EVENT)&CBlockScript::Overlap);
    Collider2D()->AddDynamicEndOverlap(this, (COLLISION_EVENT)&CBlockScript::EndOverlap);
}

void CBlockScript::Tick()
{
}

void CBlockScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    Overlap(_OwnCollider, _OtherCollider);
}

void CBlockScript::Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
	Ptr<CPlayerScript> pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
	if (pPlayer == nullptr)
        return;

	Vec3 myTransformPos = Transform()->GetRelativePos();
	Vec3 myTransformScale = Transform()->GetRelativeScale();

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
        return;

    Vec3 vPos = pPlayer->Transform()->GetRelativePos();
    Vec2 vVelocity = pPlayer->GetVelocity();
    const float epsilon = 0.01f;

    bool bBreakLock = false;

    if (overlapX < overlapY)
    {
        if (dx >= 0.f)
            vPos.x += overlapX + epsilon;
        else
            vPos.x -= overlapX + epsilon;

        vVelocity.x = 0.f;
        if (pPlayer->GetAction() == ActionState::Break)
        {
            bBreakLock = true;
            vVelocity = Vec2(0.f, 0.f);
            pPlayer->RequestBreakWallStop();
        }
        else
        {
            pPlayer->SetVelocity(vVelocity);
            pPlayer->StopBlockedAction();
        }
    }
    else
    {
        if (dy >= 0.f)
        {
            vPos.y += overlapY + epsilon;
            pPlayer->ForceFlatGroundContact(0.12f);
            if (vVelocity.y < 0.f)
                vVelocity.y = 0.f;
        }
        else
        {
            vPos.y -= overlapY + epsilon;
            if (vVelocity.y > 0.f)
                vVelocity.y = 0.f;
        }
    }

    if (!bBreakLock)
        pPlayer->Transform()->SetRelativePos(vPos);
    pPlayer->SetVelocity(vVelocity);
}

void CBlockScript::EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    return;
}
