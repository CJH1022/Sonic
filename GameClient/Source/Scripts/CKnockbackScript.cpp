#include "pch.h"
#include "CKnockbackScript.h"
#include "CCollider2D.h"
#include "CPlayerScript.h"
#include "CCoinMgrScript.h"
#include "func.h"

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
	if (Collider2D() == nullptr)
		return;

	Collider2D()->AddDynamicBeginOverlap(this, (COLLISION_EVENT)&CKnockbackScript::BeginOverlap);
	Collider2D()->AddDynamicOverlap(this, (COLLISION_EVENT)&CKnockbackScript::Overlap);
}

void CKnockbackScript::Tick()
{
}

void CKnockbackScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
	TryApplyKnockback(_OwnCollider, _OtherCollider);
}

void CKnockbackScript::Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
	TryApplyKnockback(_OwnCollider, _OtherCollider);
}

void CKnockbackScript::TryApplyKnockback(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
	if (_OwnCollider == nullptr || _OtherCollider == nullptr || GetOwner() == nullptr || GetOwner()->Transform() == nullptr)
		return;

	GameObject* pOtherOwner = _OtherCollider->GetOwner();
	if (pOtherOwner == nullptr || pOtherOwner->GetName() != L"Player")
		return;

	Ptr<CPlayerScript> pScript = pOtherOwner->GetScript<CPlayerScript>();
	if (pScript == nullptr || pScript->Transform() == nullptr)
		return;

	if (pScript->IsKnockBackInvincible())
		return;
	if (pScript->GetAction() == ActionState::KnockBack)
		return;

	PlayGameSFX(L"Sound\\맞아서 코인 뱉을떄.wav", 0.85f, true);

	const Vec3 myPos = GetOwner()->Transform()->GetWorldPos();
	const Vec3 playerPos = pScript->Transform()->GetWorldPos();
	const Vec2 playerVel = pScript->GetVelocity();

	// 1. 매 호출 시점의 최신 코인 개수를 가져옵니다 (static 제거)
	int curCount = CCoinMgrScript::GetCoinCount();

	if (curCount > 0)
	{
		// 코인이 있을 때: 절반(최소 1개)을 튕겨낼 숫자로 정함
		int scatterCount = max(1, curCount / 2);

		// 코인 생성 로직 호출 (이전에 만드신 CreateCoin 함수 활용)
		CCoinMgrScript::CreateCoin(scatterCount, Vec2(playerPos.x, playerPos.y));

		// 매니저의 코인 개수 리셋
		CCoinMgrScript::ResetCoin();
	}
	else
	{
		// 코인이 0개일 때: 사망 또는 파괴 처리
		// pOtherOwner가 플레이어라면 플레이어의 Die() 함수 등을 호출하는 것이 좋습니다.
		CPlayerScript::SubLife();
	}

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


