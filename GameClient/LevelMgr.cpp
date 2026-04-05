#include "pch.h"
#include "LevelMgr.h"

#include "CollisionMgr.h"
#include "RenderMgr.h"
#include "Source/Scripts/CSurfaceScript.h"

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
