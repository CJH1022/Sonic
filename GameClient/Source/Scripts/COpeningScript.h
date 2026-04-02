#pragma once
#include "CScript.h"

class COpeningScript
    : public CScript
{
private:
    bool m_bSpawnedSonicMain;

private:
    float           m_TransitionAccTime; // 애니메이션 경과 시간
    bool            m_bTransitionEnd;    // 애니메이션 종료 여부
    Ptr<GameObject> m_pOverlay20;        // 20번째 스프라이트 (포인터 보관)

public:
    virtual void Begin() override;
    virtual void Tick() override;

public:
    CLONE(COpeningScript);
    COpeningScript();
    virtual ~COpeningScript();
};
