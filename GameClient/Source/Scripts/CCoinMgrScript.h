#pragma once
#include "CScript.h"
#include "LevelMgr.h"
#include "CPlayerScript.h"

class CCoinMgrScript : public CScript
{
private:
    static CCoinMgrScript* g_CoinMgr; // 어디서든 접근 가능하게 함
    static int m_CoinCount;

    bool m_bKnockbackCoinGravity = true;
public:
    static CCoinMgrScript* GetInst() { return g_CoinMgr; }
    static int GetCoinCount() { return m_CoinCount; }
    static void SetCoinCount(int _CoinCount)
    {
        m_CoinCount = (_CoinCount < 0) ? 0 : _CoinCount;
    }

    bool GetKnockbackCoinGravity() const { return m_bKnockbackCoinGravity; }
    void SetKnockbackCoinGravity(bool _UseGravity) { m_bKnockbackCoinGravity = _UseGravity; }

    // 코인을 먹었을 때 호출될 함수

    static void AddCoin(int _amount)
    {
        m_CoinCount += _amount;
        if (m_CoinCount < 0)
            m_CoinCount = 0;
        // 여기서 UI 업데이트 함수를 호출하면 편리합니다!

        // 2. 10개가 되었는지 확인
        while (m_CoinCount >= 20)
        {
            // 3. 플레이어 객체를 찾음 (보통 레벨에서 태그나 이름으로 찾음)
            Ptr<GameObject> pPlayer = LevelMgr::GetInst()->FindObjectByName(L"Player");
            if (pPlayer != nullptr)
            {
                // 4. 플레이어 스크립트의 AddLife 호출
                pPlayer->GetScript<CPlayerScript>()->AddLife();
            }

            // 5. 사용한 코인 개수 차감 (0으로 만들거나 10개를 뺌)
            m_CoinCount -= 20;
        }
    }
    static void ResetCoin()
    {
        m_CoinCount = 0;
        // 여기서 UI 업데이트 함수를 호출하면 편리합니다!
        // 예시: UI 매니저를 통해 코인 UI 텍스트를 "0"으로 변경
    // CUIMgr::GetInst()->UpdateCoinUI(m_CoinCount);
    }

    static void CreateCoin(int _CoinCount, Vec2 _Position);
    virtual void Begin() override;
    virtual void Tick() override;
    virtual void SaveToLevelFile(FILE* _File) override;
    virtual void LoadFromLevelFile(FILE* _File) override;

    CLONE(CCoinMgrScript);
    CCoinMgrScript();
    ~CCoinMgrScript();
};
