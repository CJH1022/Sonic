#include "pch.h"
#include "CCamMoveScript.h"

#include "KeyMgr.h"
#include "TimeMgr.h"
#include "CTransform.h"
#include "CCamera.h"
#include "LevelMgr.h"
#include <cmath>

CCamMoveScript::CCamMoveScript()
	: CScript(SCRIPT_TYPE::CAMMOVESCRIPT)
	, m_TargetName(L"Player")
{
}

CCamMoveScript::~CCamMoveScript()
{
}

void CCamMoveScript::Begin()
{
	if (!IsValid(m_Target))
		m_Target = FindTarget();
}

void CCamMoveScript::Tick()
{

	//if (!IsValid(m_Target))
	//	m_Target = FindTarget();

	if (PROJ_TYPE::PERSPECTIVE == Camera()->GetProjType())
		MovePerspective();
	else
		MoveOrthographic();
}

void CCamMoveScript::MovePerspective()
{
	Vec3 vPos = Transform()->GetRelativePos();
	Vec3 vRot = Transform()->GetRelativeRot();

	Vec3 vFront = Transform()->GetDir(DIR::FRONT);
	Vec3 vRight = Transform()->GetDir(DIR::RIGHT);


	if (1 == KeyMgr::GetInst()->GetMouseWheel())
		vPos += vFront * 10.f;
	if (-1 == KeyMgr::GetInst()->GetMouseWheel())
		vPos -= vFront * 10.f;

	if (KEY_PRESSED(KEY::W))
		vPos += vFront * 500.f * DT;
	if (KEY_PRESSED(KEY::S))
		vPos -= vFront * 500.f * DT;
	if (KEY_PRESSED(KEY::A))
		vPos -= vRight * 500.f * DT;
	if (KEY_PRESSED(KEY::D))
		vPos += vRight * 500.f * DT;

	if (KEY_PRESSED(KEY::RBTN))
	{
		Vec2 vMouseDir = KeyMgr::GetInst()->GetMouseDir();
		vRot.y += vMouseDir.x * DT * XM_2PI * 3.f;
		vRot.x += vMouseDir.y * DT * XM_2PI * 3.f;
	}

	Transform()->SetRelativePos(vPos);
	Transform()->SetRelativeRot(vRot);
}

void CCamMoveScript::MoveOrthographic()
{
	Vec3 vPos = Transform()->GetRelativePos();

	if (IsValid(m_Target))
	{
		Vec3 vTargetPos = m_Target->Transform()->GetRelativePos();

		vPos.x = vTargetPos.x;
		vPos.y = vTargetPos.y;

		Transform()->SetRelativePos(vPos);
		Transform()->SetRelativeRot(Vec3(0.f, 0.f, 0.f));
		return;
	}

	if (KEY_PRESSED(KEY::W)) vPos.y += DT * 500.f;
	if (KEY_PRESSED(KEY::S)) vPos.y -= DT * 500.f;
	if (KEY_PRESSED(KEY::A)) vPos.x -= DT * 500.f;
	if (KEY_PRESSED(KEY::D)) vPos.x += DT * 500.f;

	Transform()->SetRelativePos(vPos);
}


Ptr<GameObject> CCamMoveScript::FindTarget()
{

	Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();

	if (nullptr == pLevel)
		return nullptr;

	for (UINT i = 0; i < MAX_LAYER; ++i)
	{
		Layer* pLayer = pLevel->GetLayer(i);
		const vector<Ptr<GameObject>>& vecObjects = pLayer->GetParentObjects();
		// const vector<Ptr<GameObject>>& vecObjects = pLayer->GetAllObjects();

		for (size_t j = 0; j < vecObjects.size(); ++j)
		{
			if (nullptr == vecObjects[j])
				continue;

			if (vecObjects[j]->GetName() == m_TargetName)
			{
				return vecObjects[j];
			}
		}
	}
	return nullptr;
}