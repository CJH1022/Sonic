#include "pch.h"
#include "LevelMgr.h"

#include "CollisionMgr.h"

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
	// 실행할 레벨이 지정된게 없으면 리턴
	if (nullptr == m_CurLevel)
		return;

	// 이전에 등록된 모든 오브젝트들 제거
	m_CurLevel->Deregister();

	// 레벨안에 있는 오브젝트들이 이번 DT 동안 할 일 수행
	// 레벨의 상태가 Play 일 때만 Level 의 Tick 을 수행
	if (m_LevelState == LEVEL_STATE::PLAY)
	{
		m_CurLevel->Tick();
	}

	m_CurLevel->FinalTick();

	// 충돌 검사 진행
	if (m_LevelState == LEVEL_STATE::PLAY)	
		CollisionMgr::GetInst()->Progress(m_CurLevel);
}

Ptr<GameObject> LevelMgr::FindObjectByName(const wstring& _name)
{
	return m_CurLevel->FindObjectByName(_name);
}

void LevelMgr::ChangeLevel(Ptr<ALevel> _NextLevel)
{
	m_CurLevel = m_SharedLevel = _NextLevel;

	m_LevelState = LEVEL_STATE::STOP;
}

void LevelMgr::ChangeLevelState(LEVEL_STATE _NextState)
{
	if (m_LevelState == _NextState)
		return;

	// Stop -> Play
	if (m_LevelState == LEVEL_STATE::STOP
		&& _NextState == LEVEL_STATE::PLAY)
	{
		// 원본 에셋 레벨의 복제본 레벨을 만들어서 현재 레벨로 가리킨다.
		m_CurLevel = m_SharedLevel->Clone();
		m_CurLevel->SetChanged();
		m_CurLevel->Begin();
	}
	
	else if (  (m_LevelState == LEVEL_STATE::PLAY || m_LevelState == LEVEL_STATE::PAUSE)
			  && _NextState == LEVEL_STATE::STOP)
	{
		m_CurLevel = m_SharedLevel;
		m_CurLevel->SetChanged();
	}

	m_LevelState = _NextState;
}