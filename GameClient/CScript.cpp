#include "pch.h"
#include "CScript.h"

#include "TaskMgr.h"

CScript::CScript(int _ScriptType)
	: Component(COMPONENT_TYPE::SCRIPT)
	, m_ScriptType(_ScriptType)
{
}

CScript::~CScript()
{
}

void CScript::Destroy()
{
	if (GetOwner()->IsDead())
		return;

	TaskInfo info = {};

	info.Type = TASK_TYPE::DESTROY_OBJECT;
	info.Param_0 = (DWORD_PTR)GetOwner();

	TaskMgr::GetInst()->AddTask(info);
}
