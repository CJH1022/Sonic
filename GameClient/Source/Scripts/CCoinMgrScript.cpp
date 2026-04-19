#include "pch.h"
#include "CCoinMgrScript.h"

// static 멤버 변수 초기화 (파일 상단)
int CCoinMgrScript::m_CoinCount = 0;

#include "CCoinScript.h"
#include "CPlayerScript.h"

#include <random>
#include <cmath>
#include "ALevel.h"
#include "LevelMgr.h"
#include "APrefab.h"
#include "AssetMgr.h"
// CCoinMgrScript.cpp
#include "func.h"      // CreateObject

namespace
{
    constexpr int kKnockbackCoinMaxSpawnCount = 48;
    constexpr int kKnockbackCoinProbeStagger = 3;
}

CCoinMgrScript* CCoinMgrScript::g_CoinMgr = nullptr;

CCoinMgrScript::CCoinMgrScript()
    : CScript(SCRIPT_TYPE::COINMGRSCRIPT)
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

void CCoinMgrScript::Tick()
{
}

void CCoinMgrScript::SaveToLevelFile(FILE* _File)
{
    fwrite(&m_bKnockbackCoinGravity, sizeof(bool), 1, _File);
}

void CCoinMgrScript::LoadFromLevelFile(FILE* _File)
{
    fread(&m_bKnockbackCoinGravity, sizeof(bool), 1, _File);
}

void CCoinMgrScript::CreateCoin(int _CoinCount, Vec2 _Position)
{
    if (_CoinCount <= 0)
        return;

    Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();
    if (nullptr == pLevel)
        return;

    if (LevelMgr::GetInst()->GetLevelState() != LEVEL_STATE::PLAY)
        return;

    Ptr<APrefab> pCoinPrefab = LOAD(APrefab, L"Prefab\\Coin.pref");
    if (nullptr == pCoinPrefab)
        return;

    const bool useKnockbackCoinGravity = (g_CoinMgr != nullptr)
        ? g_CoinMgr->GetKnockbackCoinGravity()
        : true;

    const int spawnCount = max(1, min(_CoinCount, kKnockbackCoinMaxSpawnCount));
    const int baseCoinValue = (_CoinCount > 0) ? (_CoinCount / spawnCount) : 0;
    const int extraValueCoinCount = (_CoinCount > 0) ? (_CoinCount % spawnCount) : 0;

    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> distDegree(20.f, 160.f);
    std::uniform_real_distribution<float> distR(50.f, 500.f);

    for (int i = 0; i < spawnCount; ++i)
    {
        const float degree = distDegree(gen);
        const float power = distR(gen);
        const float rad = XMConvertToRadians(degree);

        const float vx = power * cosf(rad);
        const float vy = power * sinf(rad);

        GameObject* pCoin = pCoinPrefab->Instantiate();
        if (nullptr == pCoin)
            continue;

        pCoin->SetName(L"Coin_" + std::to_wstring(pCoin->GetID()));

        if (nullptr != pCoin->Transform())
            pCoin->Transform()->SetRelativePos(Vec3(_Position.x, _Position.y, 9.f));

        Ptr<CCoinScript> pCoinScript = pCoin->GetScript<CCoinScript>();
        if (nullptr != pCoinScript)
        {
            const int coinValue = max(1, baseCoinValue + ((i < extraValueCoinCount) ? 1 : 0));
            pCoinScript->SetVelocity(Vec2(vx, vy));
            pCoinScript->Setbgravity(useKnockbackCoinGravity);
            pCoinScript->SetCoinValue(coinValue);
            pCoinScript->SetInitialLineGroundProbeFrame(i % kKnockbackCoinProbeStagger);
        }

        CreateObject(pCoin, 5);
    }
}
