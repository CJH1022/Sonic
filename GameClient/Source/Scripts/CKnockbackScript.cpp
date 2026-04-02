#include "pch.h"
#include "CKnockbackScript.h"
#include "CCollider2D.h"
#include "CPlayerScript.h"


CKnockbackScript::CKnockbackScript()
	: CScript(SCRIPT_TYPE::KNOCKBACKSCRIPT)
	, m_vKnockBackPower(Vec2(350.f, 220.f))
{
	AddScriptParam(SCRIPT_PARAM::VEC2, &m_vKnockBackPower, L"KnockBackPower", false, 10.f);
}

CKnockbackScript::~CKnockbackScript()
{
}

void CKnockbackScript::Begin()
{
	Collider2D()->AddDynamicBeginOverlap(this, (COLLISION_EVENT)&CKnockbackScript::BeginOverlap);
}

void CKnockbackScript::Tick()
{
}

void CKnockbackScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
	if (_OwnCollider == nullptr || _OtherCollider == nullptr)
		return;

	Ptr<CPlayerScript> pScript = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
	if (pScript == nullptr)
		return;

	if (pScript->IsKnockBackInvincible())
		return;

	if (pScript->GetAction() == ActionState::KnockBack)
		return;

	const Vec3 myPos = GetOwner()->Transform()->GetWorldPos();
	const Vec3 playerPos = pScript->Transform()->GetWorldPos();
	const Vec2 playerVel = pScript->GetVelocity();

	const float knockBackDirX = (playerPos.x >= myPos.x) ? 1.f : -1.f;
	Vec2 knockBackVel = Vec2(knockBackDirX * m_vKnockBackPower.x, m_vKnockBackPower.y);

	if (fabsf(playerVel.x) > fabsf(knockBackVel.x) && playerVel.x * knockBackDirX > 0.f)
		knockBackVel.x = playerVel.x;

	if (playerVel.y > knockBackVel.y)
		knockBackVel.y = playerVel.y;

	pScript->SetVelocity(knockBackVel);
	pScript->SetFacing((knockBackDirX >= 0.f) ? 1 : -1);
	pScript->SetKnockBackState(ActionState::KnockBack);
}

