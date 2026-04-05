#include "pch.h"
#include "ScriptMgr.h"

#include "Scripts/CBackgroundScript.h"
#include "Scripts/CBackScript.h"
#include "Scripts/CBlockMovingScript.h"
#include "Scripts/CBlockPushingScript.h"
#include "Scripts/CBlockScript.h"
#include "Scripts/CBossScript.h"
#include "Scripts/CCamMoveScript.h"
#include "Scripts/CDestroyBlockScript.h"
#include "Scripts/CFireScript.h"
#include "Scripts/CKnockbackScript.h"
#include "Scripts/CMissileScript.h"
#include "Scripts/CMonsterScript.h"
#include "Scripts/COpeningScript.h"
#include "Scripts/CPlayerScript.h"
#include "Scripts/CSpikeScript.h"
#include "Scripts/CSpringScript.h"
#include "Scripts/CSurfaceCircleGuideScript.h"
#include "Scripts/CSurfaceScript.h"
#include "Scripts/CTileScript.h"
#include "Scripts/CBackScript.h"
#include "Scripts/CBossScript.h"
#include "Scripts/CCylinderScript.h"
#include "Scripts/CFireScript.h"
#include "Scripts/COpeningScript.h"
#include "Scripts/CSpikeScript.h"
#include "Scripts/CSurfaceSetScript.h"

void ScriptMgr::GetScriptInfo(vector<wstring>& _vec)
{
	_vec.push_back(L"CBackgroundScript");
	_vec.push_back(L"CBackScript");
	_vec.push_back(L"CBlockMovingScript");
	_vec.push_back(L"CBlockPushingScript");
	_vec.push_back(L"CBlockScript");
	_vec.push_back(L"CBossScript");
	_vec.push_back(L"CCamMoveScript");
	_vec.push_back(L"CDestroyBlockScript");
	_vec.push_back(L"CFireScript");
	_vec.push_back(L"CKnockbackScript");
	_vec.push_back(L"CMissileScript");
	_vec.push_back(L"CMonsterScript");
	_vec.push_back(L"COpeningScript");
	_vec.push_back(L"CPlayerScript");
	_vec.push_back(L"CSpikeScript");
	_vec.push_back(L"CSpringScript");
	_vec.push_back(L"CSurfaceCircleGuideScript");
	_vec.push_back(L"CSurfaceScript");
	_vec.push_back(L"CTileScript");
	_vec.push_back(L"CBackScript");
	_vec.push_back(L"CBossScript");
	_vec.push_back(L"CCylinderScript");
	_vec.push_back(L"CFireScript");
	_vec.push_back(L"COpeningScript");
	_vec.push_back(L"CSpikeScript");
	_vec.push_back(L"CSurfaceSetScript");
}

CScript * ScriptMgr::GetScript(const wstring& _strScriptName)
{
	if (L"CBackgroundScript" == _strScriptName)
		return new CBackgroundScript;
	if (L"CBackScript" == _strScriptName)
		return new CBackScript;
	if (L"CBlockMovingScript" == _strScriptName)
		return new CBlockMovingScript;
	if (L"CBlockPushingScript" == _strScriptName)
		return new CBlockPushingScript;
	if (L"CBlockScript" == _strScriptName)
		return new CBlockScript;
	if (L"CBossScript" == _strScriptName)
		return new CBossScript;
	if (L"CCamMoveScript" == _strScriptName)
		return new CCamMoveScript;
	if (L"CDestroyBlockScript" == _strScriptName)
		return new CDestroyBlockScript;
	if (L"CFireScript" == _strScriptName)
		return new CFireScript;
	if (L"CKnockbackScript" == _strScriptName)
		return new CKnockbackScript;
	if (L"CMissileScript" == _strScriptName)
		return new CMissileScript;
	if (L"CMonsterScript" == _strScriptName)
		return new CMonsterScript;
	if (L"COpeningScript" == _strScriptName)
		return new COpeningScript;
	if (L"CPlayerScript" == _strScriptName)
		return new CPlayerScript;
	if (L"CSpikeScript" == _strScriptName)
		return new CSpikeScript;
	if (L"CSpringScript" == _strScriptName)
		return new CSpringScript;
	if (L"CSurfaceCircleGuideScript" == _strScriptName)
		return new CSurfaceCircleGuideScript;
	if (L"CSurfaceScript" == _strScriptName)
		return new CSurfaceScript;
	if (L"CTileScript" == _strScriptName)
		return new CTileScript;
	if (L"CBackScript" == _strScriptName)
		return new CBackScript;
	if (L"CBossScript" == _strScriptName)
		return new CBossScript;
	if (L"CCylinderScript" == _strScriptName)
		return new CCylinderScript;
	if (L"CFireScript" == _strScriptName)
		return new CFireScript;
	if (L"COpeningScript" == _strScriptName)
		return new COpeningScript;
	if (L"CSpikeScript" == _strScriptName)
		return new CSpikeScript;
	if (L"CSurfaceSetScript" == _strScriptName)
		return new CSurfaceSetScript;
	return nullptr;
}

CScript * ScriptMgr::GetScript(UINT _iScriptType)
{
	switch (_iScriptType)
	{
	case (UINT)SCRIPT_TYPE::BACKGROUNDSCRIPT:
		return new CBackgroundScript;
		break;
	case (UINT)SCRIPT_TYPE::BACKSCRIPT:
		return new CBackScript;
		break;
	case (UINT)SCRIPT_TYPE::BLOCKMOVINGSCRIPT:
		return new CBlockMovingScript;
		break;
	case (UINT)SCRIPT_TYPE::BLOCKPUSHINGSCRIPT:
		return new CBlockPushingScript;
		break;
	case (UINT)SCRIPT_TYPE::BLOCKSCRIPT:
		return new CBlockScript;
		break;
	case (UINT)SCRIPT_TYPE::BOSSSCRIPT:
		return new CBossScript;
		break;
	case (UINT)SCRIPT_TYPE::CAMMOVESCRIPT:
		return new CCamMoveScript;
		break;
	case (UINT)SCRIPT_TYPE::DESTROYBLOCKSCRIPT:
		return new CDestroyBlockScript;
		break;
	case (UINT)SCRIPT_TYPE::FIRESCRIPT:
		return new CFireScript;
		break;
	case (UINT)SCRIPT_TYPE::KNOCKBACKSCRIPT:
		return new CKnockbackScript;
		break;
	case (UINT)SCRIPT_TYPE::MISSILESCRIPT:
		return new CMissileScript;
		break;
	case (UINT)SCRIPT_TYPE::MONSTERSCRIPT:
		return new CMonsterScript;
		break;
	case (UINT)SCRIPT_TYPE::OPENINGSCRIPT:
		return new COpeningScript;
		break;
	case (UINT)SCRIPT_TYPE::PLAYERSCRIPT:
		return new CPlayerScript;
		break;
	case (UINT)SCRIPT_TYPE::SPIKESCRIPT:
		return new CSpikeScript;
		break;
	case (UINT)SCRIPT_TYPE::SPRINGSCRIPT:
		return new CSpringScript;
		break;
	case (UINT)SCRIPT_TYPE::SURFACECIRCLEGUIDESCRIPT:
		return new CSurfaceCircleGuideScript;
		break;
	case (UINT)SCRIPT_TYPE::SURFACESCRIPT:
		return new CSurfaceScript;
		break;
	case (UINT)SCRIPT_TYPE::TILESCRIPT:
		return new CTileScript;
		break;
	case (UINT)SCRIPT_TYPE::BACKSCRIPT:
		return new CBackScript;
		break;
	case (UINT)SCRIPT_TYPE::BOSSSCRIPT:
		return new CBossScript;
		break;
	case (UINT)SCRIPT_TYPE::CYLINDERSCRIPT:
		return new CCylinderScript;
		break;
	case (UINT)SCRIPT_TYPE::FIRESCRIPT:
		return new CFireScript;
		break;
	case (UINT)SCRIPT_TYPE::OPENINGSCRIPT:
		return new COpeningScript;
		break;
	case (UINT)SCRIPT_TYPE::SPIKESCRIPT:
		return new CSpikeScript;
		break;
	case (UINT)SCRIPT_TYPE::SURFACESETSCRIPT:
		return new CSurfaceSetScript;
		break;
	}
	return nullptr;
}

const wchar_t * ScriptMgr::GetScriptName(CScript * _pScript)
{
	switch ((SCRIPT_TYPE)_pScript->GetScriptType())
	{
	case SCRIPT_TYPE::BACKGROUNDSCRIPT:
		return L"CBackgroundScript";
		break;

	case SCRIPT_TYPE::BACKSCRIPT:
		return L"CBackScript";
		break;

	case SCRIPT_TYPE::BLOCKMOVINGSCRIPT:
		return L"CBlockMovingScript";
		break;

	case SCRIPT_TYPE::BLOCKPUSHINGSCRIPT:
		return L"CBlockPushingScript";
		break;

	case SCRIPT_TYPE::BLOCKSCRIPT:
		return L"CBlockScript";
		break;

	case SCRIPT_TYPE::BOSSSCRIPT:
		return L"CBossScript";
		break;

	case SCRIPT_TYPE::CAMMOVESCRIPT:
		return L"CCamMoveScript";
		break;

	case SCRIPT_TYPE::DESTROYBLOCKSCRIPT:
		return L"CDestroyBlockScript";
		break;

	case SCRIPT_TYPE::FIRESCRIPT:
		return L"CFireScript";
		break;

	case SCRIPT_TYPE::KNOCKBACKSCRIPT:
		return L"CKnockbackScript";
		break;

	case SCRIPT_TYPE::MISSILESCRIPT:
		return L"CMissileScript";
		break;

	case SCRIPT_TYPE::MONSTERSCRIPT:
		return L"CMonsterScript";
		break;

	case SCRIPT_TYPE::OPENINGSCRIPT:
		return L"COpeningScript";
		break;

	case SCRIPT_TYPE::PLAYERSCRIPT:
		return L"CPlayerScript";
		break;

	case SCRIPT_TYPE::SPIKESCRIPT:
		return L"CSpikeScript";
		break;

	case SCRIPT_TYPE::SPRINGSCRIPT:
		return L"CSpringScript";
		break;

	case SCRIPT_TYPE::SURFACECIRCLEGUIDESCRIPT:
		return L"CSurfaceCircleGuideScript";
		break;

	case SCRIPT_TYPE::SURFACESCRIPT:
		return L"CSurfaceScript";
		break;

	case SCRIPT_TYPE::TILESCRIPT:
		return L"CTileScript";
		break;

	case SCRIPT_TYPE::BACKSCRIPT:
		return L"CBackScript";
		break;

	case SCRIPT_TYPE::BOSSSCRIPT:
		return L"CBossScript";
		break;

	case SCRIPT_TYPE::CYLINDERSCRIPT:
		return L"CCylinderScript";
		break;

	case SCRIPT_TYPE::FIRESCRIPT:
		return L"CFireScript";
		break;

	case SCRIPT_TYPE::OPENINGSCRIPT:
		return L"COpeningScript";
		break;

	case SCRIPT_TYPE::SPIKESCRIPT:
		return L"CSpikeScript";
		break;

	case SCRIPT_TYPE::SURFACESETSCRIPT:
		return L"CSurfaceSetScript";
		break;

	}
	return nullptr;
}