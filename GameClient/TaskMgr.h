#pragma once

#include "GameObject.h"

class TaskMgr
	: public singleton<TaskMgr>
{
	SINGLE(TaskMgr);
private:
	vector<TaskInfo>		m_vecTask;
	vector<Ptr<GameObject>> m_Garbage;

public:
	void AddTask(const TaskInfo& _info) { m_vecTask.push_back(_info); }

public:
	void Progress();
};

