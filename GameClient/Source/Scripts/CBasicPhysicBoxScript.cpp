#include "pch.h"
#include "CBasicPhysicBoxScript.h"

#include "TimeMgr.h"
#include "CCollider2D.h"
#include "GameObject.h"
#include "CTransform.h"
#include "LevelMgr.h"
#include "CPlayerScript.h"

CBasicPhysicBoxScript::CBasicPhysicBoxScript()
	:CScript(SCRIPT_TYPE::BASICPHYSICBOXSCRIPT)
{
}

CBasicPhysicBoxScript::~CBasicPhysicBoxScript()
{
}

void CBasicPhysicBoxScript::Begin()
{
	ADD_DYNAMIC_BEGIN_OVERLAP(CBasicPhysicBoxScript::BeginOverlap);
}

void CBasicPhysicBoxScript::Tick()
{
}

void CBasicPhysicBoxScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
	GameObject* pOther = _OtherCollider->GetOwner();

	if (pOther->GetName() != L"Player")
		return;

	auto pPlayerScript = pOther->GetScript<CPlayerScript>();
	Vec2 PlayerVel = pPlayerScript->GetVelocity();

	if (PlayerVel.Length() > 600.f)
	{
		PlayerVel *= 1.5f;
	}
	
	pPlayerScript->SetVelocity(PlayerVel);
	// 플레이어 점프대 에니메이션 추가
}