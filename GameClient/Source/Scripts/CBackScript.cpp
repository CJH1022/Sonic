#include "pch.h"
#include "CBackScript.h"
#include "CCollider2D.h"
#include "CPlayerScript.h"
#include "CBossScript.h"


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

		const Vec3 myPos = GetOwner()->Transform()->GetWorldPos();
		const Vec3 playerPos = pScript->Transform()->GetWorldPos();
		const Vec2 playerVel = pScript->GetVelocity();

		pBossScript->ApplyDamage(1, playerPos);

		const float BackDirX = (playerPos.x >= myPos.x) ? 1.f : -1.f;
		Vec2 BackVel = Vec2(BackDirX * m_vBackPower.x, m_vBackPower.y);

		if (fabsf(playerVel.x) > fabsf(BackVel.x) && playerVel.x * BackDirX > 0.f)
			BackVel.x = playerVel.x;

		if (playerVel.y > BackVel.y)
			BackVel.y = playerVel.y;

		pScript->SetVelocity(BackVel);
		pScript->SetFacing((BackDirX >= 0.f) ? 1 : -1);
		pScript->SetBackState();
	}
}

