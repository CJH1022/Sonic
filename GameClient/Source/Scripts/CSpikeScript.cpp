#include "pch.h"
#include "CSpikeSCript.h"
#include "Source/Scripts/CBlockScript.h"
#include "Source/Scripts/CKnockbackScript.h"

#include "KeyMgr.h"
#include "TimeMgr.h"
#include "RenderMgr.h"
#include "CTransform.h"
#include "LevelMgr.h"
#include "GameObject.h"
#include "CCollider2D.h"
#include "TaskMgr.h"

#include <cmath> // fabsf
#include "AssetMgr.h"

CSpikeScript::CSpikeScript()
	:CScript(SCRIPT_TYPE::SPIKESCRIPT)
{
}

CSpikeScript::~CSpikeScript()
{
}

void CSpikeScript::Begin()
{
    if (GetOwner() == nullptr)
        return;

    if (!GetOwner()->GetChild().empty())
        return;

    GetOwner()->Transform()->SetRelativeScale(Vec3(150.f, 150.f, 1.f));
    GetOwner()->SpriteRender()->SetSprite(LOAD(ASprite, L"Sprite\\Spike.sprite"));

    Ptr<GameObject> pBlock = new GameObject;
    pBlock->SetName(L"Spike_Block");
    pBlock->AddComponent(new CTransform);
    pBlock->AddComponent(new CCollider2D);
    pBlock->AddComponent(new CBlockScript);
    pBlock->Transform()->SetIndependentScale(true);
    pBlock->Transform()->SetRelativePos(Vec3(0.f, -5.f, 0.f));
    pBlock->Transform()->SetRelativeScale(Vec3(150.f, 140.f, 1.f));

    Ptr<GameObject> pHit = new GameObject;
    pHit->SetName(L"Spike_Hit");
    pHit->AddComponent(new CTransform);
    pHit->AddComponent(new CCollider2D);
    pHit->Transform()->SetIndependentScale(true);

    auto* pKnock = new CKnockbackScript;
    pKnock->SetKnockBackPower(Vec2(350.f, 220.f));
    pHit->AddComponent(pKnock);

    pHit->Transform()->SetRelativePos(Vec3(0.f, 70.f, 0.f));
    pHit->Transform()->SetRelativeScale(Vec3(140.f, 10.f, 1.f));

    GetOwner()->AddChild(pBlock);
    GetOwner()->AddChild(pHit);

    if (GetOwner()->Collider2D() != nullptr)
    {
        // Keep the prefab's root collider out of play; the child colliders own the real spike logic.
        GetOwner()->Collider2D()->SetOffset(Vec2(0.f, -100000.f));
        GetOwner()->Collider2D()->SetScale(Vec2(0.01f, 0.01f));
    }
}


void CSpikeScript::Tick()
{
}

void CSpikeScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
}
