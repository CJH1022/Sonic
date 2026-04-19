#include "pch.h"
#include "CMissileScript.h"

#include "CCamera.h"
#include "GameObject.h"
#include "CCollider2D.h"
#include "Engine.h"
#include "RenderMgr.h"
#include "TimeMgr.h"

CMissileScript::CMissileScript()
	: CScript(SCRIPT_TYPE::MISSILESCRIPT)
{
}

CMissileScript::~CMissileScript()
{
}

void CMissileScript::Begin()
{	
	ADD_DYNAMIC_BEGIN_OVERLAP(CMissileScript::BeginOverlap);
	ADD_DYNAMIC_OVERLAP(CMissileScript::Overlap);
	ADD_DYNAMIC_END_OVERLAP(CMissileScript::EndOverlap);
}

void CMissileScript::Tick()
{
	if (Transform() == nullptr || GetOwner() == nullptr)
		return;

	auto IsZeroDir = [](const Vec3& _Dir)
	{
		return fabsf(_Dir.x) <= 0.0001f &&
			fabsf(_Dir.y) <= 0.0001f &&
			fabsf(_Dir.z) <= 0.0001f;
	};

	auto ApplyMissileRotation = [this](const Vec3& _Dir)
	{
		Vec3 vBase = Vec3(0.f, 1.f, 0.f);
		float dot = vBase.Dot(_Dir);
		dot = max(-1.f, min(1.f, dot));
		float radian = acosf(dot);

		if (_Dir.x < 0.f)
			Transform()->SetRelativeRot(Vec3(0.f, 0.f, radian));
		else
			Transform()->SetRelativeRot(Vec3(0.f, 0.f, -radian));
	};

	Vec3 vPos = Transform()->GetRelativePos();

	if (IsValid(m_Target))
	{
		Vec3 vTargetPos = m_Target->Transform()->GetWorldPos();
		if (IsZeroDir(m_Dir))
		{
			m_Dir = vTargetPos - vPos;
			if (!IsZeroDir(m_Dir))
				m_Dir.Normalize();
		}
	}

	if (IsZeroDir(m_Dir))
		return;

	ApplyMissileRotation(m_Dir);
	vPos += m_Dir * 200.f * DT;
	Transform()->SetRelativePos(vPos);

	Ptr<CCamera> pCamera = RenderMgr::GetInst()->GetPOVCamera();
	if (pCamera == nullptr)
		pCamera = RenderMgr::GetInst()->GetEditorCamera();

	if (pCamera != nullptr && pCamera->Transform() != nullptr)
	{
		const Vec2 resol = Engine::GetInst()->GetResolution();
		const Vec3 cameraPos = pCamera->Transform()->GetWorldPos();
		const float bottomLimitY = cameraPos.y - (resol.y * 0.5f) - 220.f;
		const float sideLimit = (resol.x * 0.8f) + 220.f;

		if (vPos.y <= bottomLimitY || fabsf(vPos.x - cameraPos.x) >= sideLimit)
			GetOwner()->Destroy();
	}
}

void CMissileScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
}

void CMissileScript::Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
}

void CMissileScript::EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
}

void CMissileScript::SaveToLevelFile(FILE* _File)
{
	fwrite(&m_Dir, sizeof(Vec3), 1, _File);
}

void CMissileScript::LoadFromLevelFile(FILE* _File)
{
	fread(&m_Dir, sizeof(Vec3), 1, _File);
}
