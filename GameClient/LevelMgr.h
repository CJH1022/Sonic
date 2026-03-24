#pragma once

#include "ALevel.h"

class LevelMgr
	: public singleton<LevelMgr>
{
	SINGLE(LevelMgr)
private:
	Ptr<ALevel>		m_CurLevel;
	Ptr<ALevel>		m_SharedLevel;	// 에셋매니저를 통해서 관리되는 레벨 에셋
	LEVEL_STATE		m_LevelState;

public:
	Ptr<ALevel> GetCurLevel() { return m_CurLevel; }
	Ptr<GameObject> FindObjectByName(const wstring& _name);	
	LEVEL_STATE GetLevelState() { return m_LevelState; }
	
private:
	void ChangeLevel(Ptr<ALevel> _NextLevel);
	void ChangeLevelState(LEVEL_STATE _NextState);

public:
	void Init();
	void Progress();

	friend class TaskMgr;
};

