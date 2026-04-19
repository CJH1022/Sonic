#include "pch.h"
#include "CScript.h"

#include "APrefab.h"
#include "TaskMgr.h"
#include "func.h"

CScript::CScript(int _ScriptType)
	: Component(COMPONENT_TYPE::SCRIPT)
	, m_ScriptType(_ScriptType)
	, m_bTickEnabled(true)
{
}

CScript::CScript(const CScript& _Origin)
	: Component(_Origin)
	, m_ScriptType(_Origin.m_ScriptType)
	, m_bTickEnabled(_Origin.m_bTickEnabled)
{
}

CScript::~CScript()
{
}

void CScript::Instantiate(APrefab* _Prefab, int _LayerIdx, Vec3 _WorldPos)
{
	if (nullptr == _Prefab)
		return;

	GameObject* pObject = _Prefab->Instantiate();
	if (nullptr == pObject)
		return;

	pObject->Transform()->SetRelativePos(_WorldPos);
	CreateObject(pObject, _LayerIdx);
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
