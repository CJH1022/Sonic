#include "pch.h"
#include "CBackScript.h"
#include "CCollider2D.h"
#include "CPlayerScript.h"
#include "CBossScript.h"

namespace
{
	Vec2 ComputeBackVelocity(const Vec2& _BackPower, GameObject* _Owner, CPlayerScript* _PlayerScript)
	{
		if (_Owner == nullptr || _Owner->Transform() == nullptr || _PlayerScript == nullptr || _PlayerScript->Transform() == nullptr)
			return Vec2(0.f, 0.f);

		const Vec3 myPos = _Owner->Transform()->GetWorldPos();
		const Vec3 playerPos = _PlayerScript->Transform()->GetWorldPos();
		const Vec2 playerVel = _PlayerScript->GetVelocity();

		const float backDirX = (playerPos.x >= myPos.x) ? 1.f : -1.f;
		Vec2 backVel = Vec2(backDirX * _BackPower.x, _BackPower.y);

		if (fabsf(playerVel.x) > fabsf(backVel.x) && playerVel.x * backDirX > 0.f)
			backVel.x = playerVel.x;

		if (playerVel.y > backVel.y)
			backVel.y = playerVel.y;

		return backVel;
	}
}


CBackScript::CBackScript()
	: CScript(SCRIPT_TYPE::BACKSCRIPT)
	, m_vBackPower(Vec2(400.f, 220.f))
{
	AddScriptParam(SCRIPT_PARAM::VEC2, &m_vBackPower, L"BackPower", false, 10.f);
}

CBackScript::~CBackScript()
{
}

void CBackScript::Begin()
{
	if (Collider2D() == nullptr)
		return;

	Collider2D()->AddDynamicBeginOverlap(this, (COLLISION_EVENT)&CBackScript::BeginOverlap);
}

void CBackScript::Tick()
{
}

void CBackScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{

	if (_OwnCollider == nullptr || _OtherCollider == nullptr || GetOwner() == nullptr || GetOwner()->Transform() == nullptr)
		return;

	GameObject* pOwnOwner = _OwnCollider->GetOwner();
	GameObject* pOtherOwner = _OtherCollider->GetOwner();
	if (pOwnOwner == nullptr || pOtherOwner == nullptr)
		return;

	Ptr<CPlayerScript> pScript = pOtherOwner->GetScript<CPlayerScript>();
	if (pScript == nullptr || pScript->Transform() == nullptr)
		return;

	if (pScript->GetAction() == ActionState::KnockBack)
		return;

	if (pScript->IsBackReaction())
		return;

	if (pScript->GetIsGround() == false && pScript->GetIsJump() == true)
	{
		GameObject* pBossObject = _OwnCollider->GetOwner();
		if (pBossObject == nullptr)
			return;

		Ptr<CBossScript> pBossScript = nullptr;

		while (pBossObject != nullptr && pBossScript == nullptr)
		{
			pBossScript = pBossObject->GetScript<CBossScript>();

			Ptr<GameObject> pParent = pBossObject->GetParent();
			pBossObject = (pBossScript == nullptr && pParent != nullptr) ? pParent.Get() : nullptr;
		}

		if (pBossScript == nullptr || pBossScript->GetOwner() == nullptr || pBossScript->GetOwner()->IsDead())
			return;

		const Vec3 playerPos = pScript->Transform()->GetWorldPos();

		pBossScript->ApplyDamage(1, playerPos);

		const Vec2 BackVel = ComputeBackVelocity(m_vBackPower, GetOwner(), pScript.Get());
		const float BackDirX = (BackVel.x >= 0.f) ? 1.f : -1.f;

		pScript->SetVelocity(BackVel);
		pScript->SetFacing((BackDirX >= 0.f) ? 1 : -1);
		pScript->SetBackState();
	}
}

void CBackScript::ExecuteItemBounce(GameObject* _Target)
{
	if (_Target == nullptr || GetOwner() == nullptr || GetOwner()->Transform() == nullptr)
		return;

	Ptr<CPlayerScript> pScript = _Target->GetScript<CPlayerScript>();
	if (pScript == nullptr || pScript->Transform() == nullptr)
		return;

	const Vec2 backVel = ComputeBackVelocity(m_vBackPower, GetOwner(), pScript.Get());
	const float backDirX = (backVel.x >= 0.f) ? 1.f : -1.f;

	pScript->SetVelocity(backVel);
	pScript->SetFacing((backDirX >= 0.f) ? 1 : -1);
	pScript->SetItemBoxBounceState();
}

