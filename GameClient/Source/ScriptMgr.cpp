#include "pch.h"
#include "ScriptMgr.h"

#include "Scripts/CBlockMovingScript.h"
#include "Scripts/CBlockPushingScript.h"
#include "Scripts/CBlockScript.h"
#include "Scripts/CCamMoveScript.h"
#include "Scripts/CMissileScript.h"
#include "Scripts/CMonsterScript.h"
#include "Scripts/CPlayerScript.h"
#include "Scripts/CSpringScript.h"
#include "Scripts/CTileScript.h"

void ScriptMgr::GetScriptInfo(vector<wstring>& _vec)
{
	_vec.push_back(L"CBlockMovingScript");
	_vec.push_back(L"CBlockPushingScript");
	_vec.push_back(L"CBlockScript");
	_vec.push_back(L"CCamMoveScript");
	_vec.push_back(L"CMissileScript");
	_vec.push_back(L"CMonsterScript");
	_vec.push_back(L"CPlayerScript");
	_vec.push_back(L"CSpringScript");
	_vec.push_back(L"CTileScript");
}

CScript * ScriptMgr::GetScript(const wstring& _strScriptName)
{
	if (L"CBlockMovingScript" == _strScriptName)
		return new CBlockMovingScript;
	if (L"CBlockPushingScript" == _strScriptName)
		return new CBlockPushingScript;
	if (L"CBlockScript" == _strScriptName)
		return new CBlockScript;
	if (L"CCamMoveScript" == _strScriptName)
		return new CCamMoveScript;
	if (L"CMissileScript" == _strScriptName)
		return new CMissileScript;
	if (L"CMonsterScript" == _strScriptName)
		return new CMonsterScript;
	if (L"CPlayerScript" == _strScriptName)
		return new CPlayerScript;
	if (L"CSpringScript" == _strScriptName)
		return new CSpringScript;
	if (L"CTileScript" == _strScriptName)
		return new CTileScript;
	return nullptr;
}

CScript * ScriptMgr::GetScript(UINT _iScriptType)
{
	switch (_iScriptType)
	{
	case (UINT)SCRIPT_TYPE::BLOCKMOVINGSCRIPT:
		return new CBlockMovingScript;
		break;
	case (UINT)SCRIPT_TYPE::BLOCKPUSHINGSCRIPT:
		return new CBlockPushingScript;
		break;
	case (UINT)SCRIPT_TYPE::BLOCKSCRIPT:
		return new CBlockScript;
		break;
	case (UINT)SCRIPT_TYPE::CAMMOVESCRIPT:
		return new CCamMoveScript;
		break;
	case (UINT)SCRIPT_TYPE::MISSILESCRIPT:
		return new CMissileScript;
		break;
	case (UINT)SCRIPT_TYPE::MONSTERSCRIPT:
		return new CMonsterScript;
		break;
	case (UINT)SCRIPT_TYPE::PLAYERSCRIPT:
		return new CPlayerScript;
		break;
	case (UINT)SCRIPT_TYPE::SPRINGSCRIPT:
		return new CSpringScript;
		break;
	case (UINT)SCRIPT_TYPE::TILESCRIPT:
		return new CTileScript;
		break;
	}
	return nullptr;
}

const wchar_t * ScriptMgr::GetScriptName(CScript * _pScript)
{
	switch ((SCRIPT_TYPE)_pScript->GetScriptType())
	{
	case SCRIPT_TYPE::BLOCKMOVINGSCRIPT:
		return L"CBlockMovingScript";
		break;

	case SCRIPT_TYPE::BLOCKPUSHINGSCRIPT:
		return L"CBlockPushingScript";
		break;

	case SCRIPT_TYPE::BLOCKSCRIPT:
		return L"CBlockScript";
		break;

	case SCRIPT_TYPE::CAMMOVESCRIPT:
		return L"CCamMoveScript";
		break;

	case SCRIPT_TYPE::MISSILESCRIPT:
		return L"CMissileScript";
		break;

	case SCRIPT_TYPE::MONSTERSCRIPT:
		return L"CMonsterScript";
		break;

	case SCRIPT_TYPE::PLAYERSCRIPT:
		return L"CPlayerScript";
		break;

	case SCRIPT_TYPE::SPRINGSCRIPT:
		return L"CSpringScript";
		break;

	case SCRIPT_TYPE::TILESCRIPT:
		return L"CTileScript";
		break;

	}
	return nullptr;
}