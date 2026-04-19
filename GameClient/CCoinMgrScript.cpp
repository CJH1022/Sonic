#include "pch.h"
#include "CCoinMgrScript.h"

CCoinMgrScript* CCoinMgrScript::g_CoinMgr = nullptr;

CCoinMgrScript::CCoinMgrScript()
    : CScript(SCRIPT_TYPE::COINMGRSCRIPT)
    , m_CoinCount(0)
{
    g_CoinMgr = this;
}

CCoinMgrScript::~CCoinMgrScript()
{
    if (g_CoinMgr == this)
        g_CoinMgr = nullptr;
}

void CCoinMgrScript::Begin()
{
    m_CoinCount = 0; // 스테이지 시작 시 초기화
}