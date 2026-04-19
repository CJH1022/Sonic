#include "pch.h"
#include "CCamMoveScript.h"

#include "KeyMgr.h"
#include "TimeMgr.h"
#include "CTransform.h"
#include "CCamera.h"
#include "LevelMgr.h"
#include "ALevel.h"
#include "Layer.h"
#include "Source/Scripts/CUIMgrScript.h"
#include <cmath>

namespace
{
    CUIMgrScript* FindStageUIManager()
    {
        Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();
        if (nullptr == pLevel)
            return nullptr;

        for (UINT layerIdx = 0; layerIdx < MAX_LAYER; ++layerIdx)
        {
            Layer* pLayer = pLevel->GetLayer(layerIdx);
            if (nullptr == pLayer)
                continue;

            const vector<Ptr<GameObject>>& vecObjects = pLayer->GetAllObjects();
            for (size_t i = 0; i < vecObjects.size(); ++i)
            {
                if (vecObjects[i] == nullptr || vecObjects[i]->IsDead())
                    continue;

                Ptr<CUIMgrScript> pUI = vecObjects[i]->GetScript<CUIMgrScript>();
                if (nullptr != pUI)
                    return pUI.Get();
            }
        }

        return nullptr;
    }
}

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
	Vec3 vCamPos = Transform()->GetRelativePos();
    CUIMgrScript* pStageUI = FindStageUIManager();
    const bool bStageUILocked = (pStageUI != nullptr)
        && (pStageUI->GetStageState() == CUIMgrScript::STAGESTATE::START
            || pStageUI->GetStageState() == CUIMgrScript::STAGESTATE::END);
    const int stageLockState = (pStageUI != nullptr) ? (int)pStageUI->GetStageState() : -1;
    const bool bWasStageUILockActive = m_bStageUILockActive;

	if (m_bLockPosition)
	{
		vCamPos = m_LockedPosition;
		ClampToCameraBounds(vCamPos);
		Transform()->SetRelativePos(vCamPos);
		Transform()->SetRelativeRot(Vec3(0.f, 0.f, 0.f));
		return;
	}

    if (bStageUILocked)
    {
        if (!m_bStageUILockActive || m_StageUILockState != stageLockState)
        {
            m_bStageUILockActive = true;
            m_StageUILockState = stageLockState;
            m_StageUILockedPosition = Transform()->GetRelativePos();
        }

        vCamPos = m_StageUILockedPosition;
        ClampToCameraBounds(vCamPos);
        Transform()->SetRelativePos(vCamPos);
        Transform()->SetRelativeRot(Vec3(0.f, 0.f, 0.f));
        return;
    }

    m_bStageUILockActive = false;
    m_StageUILockState = -1;

	if (IsValid(m_Target))
	{
		Vec3 vTargetPos = m_Target->Transform()->GetRelativePos();
        vTargetPos.y += m_fFollowOffsetY;
		Vec3 vCamPos = Transform()->GetRelativePos();
        if (bWasStageUILockActive)
        {
            vCamPos = vTargetPos;
        }
        else
        {
		    float LerpTime = DT * 5.f;
		    vCamPos = DirectX::SimpleMath::Vector3::Lerp(vCamPos, vTargetPos, LerpTime);
        }

		// 3. [핵심] 카메라 벽(Camera Wall) 제한 적용
		// m_fMinX, m_fMaxX 등은 레벨 디자인에 맞춰 미리 설정해둡니다.
		ClampToCameraBounds(vCamPos);

		Transform()->SetRelativePos(vCamPos);
		Transform()->SetRelativeRot(Vec3(0.f, 0.f, 0.f));
		return;
	}
	if (KEY_PRESSED(KEY::W)) vCamPos.y += DT * 1500.f;
	if (KEY_PRESSED(KEY::S)) vCamPos.y -= DT * 1500.f;
	if (KEY_PRESSED(KEY::A)) vCamPos.x -= DT * 1500.f;
	if (KEY_PRESSED(KEY::D)) vCamPos.x += DT * 1500.f;

	Transform()->SetRelativePos(vCamPos);
}

void CCamMoveScript::ClampToCameraBounds(Vec3& _InOutCamPos) const
{
	if (_InOutCamPos.x < m_fMinLimitX) _InOutCamPos.x = m_fMinLimitX;
	if (_InOutCamPos.x > m_fMaxLimitX) _InOutCamPos.x = m_fMaxLimitX;

	if (_InOutCamPos.y < m_fMinLimitY) _InOutCamPos.y = m_fMinLimitY;
	if (_InOutCamPos.y > m_fMaxLimitY) _InOutCamPos.y = m_fMaxLimitY;

	if (_InOutCamPos.x > m_fMaxLimitX && _InOutCamPos.y > m_fMaxLimitY)
	{
		_InOutCamPos.x = m_fMaxLimitX;
		_InOutCamPos.y = m_fMaxLimitY;
	}
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
