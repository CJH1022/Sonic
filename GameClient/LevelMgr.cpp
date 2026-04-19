#include "pch.h"
#include "LevelMgr.h"

#include "CollisionMgr.h"
#include "CTransform.h"
#include "RenderMgr.h"
#include "Source/Scripts/CCylinderScript.h"
#include "Source/Scripts/CSurfaceScript.h"
#include "Source/Scripts/CUIMgrScript.h"

namespace
{
    void DrawStoppedCylinderDebug(Ptr<ALevel> _Level)
    {
        if (_Level == nullptr || !RenderMgr::GetInst()->IsDebugRender())
            return;

        for (UINT layerIdx = 0; layerIdx < MAX_LAYER; ++layerIdx)
        {
            Layer* pLayer = _Level->GetLayer(layerIdx);
            const vector<Ptr<GameObject>>& vecObjects = pLayer->GetAllObjects();

            for (size_t i = 0; i < vecObjects.size(); ++i)
            {
                if (vecObjects[i] == nullptr || vecObjects[i]->IsDead())
                    continue;

                Ptr<CCylinderScript> pCylinder = vecObjects[i]->GetScript<CCylinderScript>();
                if (pCylinder != nullptr)
                    pCylinder->DrawExitDoorDebug();
            }
        }
    }

    int ResolveUILayerIndex(Ptr<ALevel> _Level)
    {
        if (_Level == nullptr)
            return 31;

        for (int layerIdx = 0; layerIdx < MAX_LAYER; ++layerIdx)
        {
            Layer* pLayer = _Level->GetLayer(layerIdx);
            if (nullptr != pLayer && pLayer->GetName() == L"UI")
                return layerIdx;
        }

        return 31;
    }

    CUIMgrScript* FindOrCreateStageUIManager(Ptr<ALevel> _Level, bool& _OutHasPlayer)
    {
        _OutHasPlayer = false;
        if (_Level == nullptr)
            return nullptr;

        CUIMgrScript* pFoundStageUI = nullptr;

        for (UINT layerIdx = 0; layerIdx < MAX_LAYER; ++layerIdx)
        {
            Layer* pLayer = _Level->GetLayer(layerIdx);
            if (nullptr == pLayer)
                continue;

            const vector<Ptr<GameObject>>& vecObjects = pLayer->GetAllObjects();
            for (size_t i = 0; i < vecObjects.size(); ++i)
            {
                if (vecObjects[i] == nullptr || vecObjects[i]->IsDead())
                    continue;

                if (vecObjects[i]->GetName() == L"Player")
                    _OutHasPlayer = true;

                Ptr<CUIMgrScript> pUIMgr = vecObjects[i]->GetScript<CUIMgrScript>();
                if (nullptr != pUIMgr)
                {
                    pFoundStageUI = pUIMgr.Get();
                    return pFoundStageUI;
                }
            }
        }

        if (!_OutHasPlayer)
            return nullptr;

        Ptr<GameObject> pStageUI = new GameObject;
        pStageUI->SetName(L"StageUIAuto");
        pStageUI->AddComponent(new CTransform);
        CUIMgrScript* pUIMgr = new CUIMgrScript;
        pStageUI->AddComponent(pUIMgr);

        _Level->AddObject(ResolveUILayerIndex(_Level), pStageUI);
        _Level->SetChanged();
        return pUIMgr;
    }

    void EnsureDefaultStageUIManager(Ptr<ALevel> _Level)
    {
        bool hasPlayer = false;
        CUIMgrScript* pUIMgr = FindOrCreateStageUIManager(_Level, hasPlayer);
        if (!hasPlayer || nullptr == pUIMgr)
            return;

        pUIMgr->PrepareForLevelStart();
    }
}

LevelMgr::LevelMgr()
	: m_LevelState(LEVEL_STATE::STOP)
{
}

LevelMgr::~LevelMgr()
{
}

void LevelMgr::Init()
{
}

void LevelMgr::Progress()
{
	if (nullptr == m_CurLevel)
		return;

	m_CurLevel->Deregister();

	if (m_LevelState == LEVEL_STATE::PLAY)
	{
		m_CurLevel->Tick();
	}

	m_CurLevel->FinalTick();

    if (m_LevelState == LEVEL_STATE::STOP)
    {
        DrawStoppedCylinderDebug(m_CurLevel);
    }

	if (m_LevelState == LEVEL_STATE::PLAY)
		CollisionMgr::GetInst()->Progress(m_CurLevel);
}

Ptr<GameObject> LevelMgr::FindObjectByName(const wstring& _name)
{
	return m_CurLevel->FindObjectByName(_name);
}

void LevelMgr::ChangeLevel(Ptr<ALevel> _NextLevel)
{
	CSurfaceScript::ResetSpatialIndex();
	RenderMgr::GetInst()->ClearMainCamera();
	m_CurLevel = m_SharedLevel = _NextLevel;
    EnsureDefaultStageUIManager(m_SharedLevel);
	m_LevelState = LEVEL_STATE::STOP;
}

void LevelMgr::ChangeLevelState(LEVEL_STATE _NextState)
{
	if (m_LevelState == _NextState)
		return;

	if (m_LevelState == LEVEL_STATE::STOP
		&& _NextState == LEVEL_STATE::PLAY)
	{
		if (nullptr == m_SharedLevel)
			return;

		CSurfaceScript::ResetSpatialIndex();
		RenderMgr::GetInst()->ClearMainCamera();
		m_CurLevel = m_SharedLevel->Clone();
		m_CurLevel->SetChanged();
		m_LevelState = _NextState;
		m_CurLevel->Begin();
		return;
	}

	if ((m_LevelState == LEVEL_STATE::PLAY || m_LevelState == LEVEL_STATE::PAUSE)
		&& _NextState == LEVEL_STATE::STOP)
	{
		CSurfaceScript::ResetSpatialIndex();
		RenderMgr::GetInst()->ClearMainCamera();
		m_CurLevel = m_SharedLevel;
		if (nullptr != m_CurLevel)
		{
			m_CurLevel->SetChanged();
		}
	}

	m_LevelState = _NextState;
}
