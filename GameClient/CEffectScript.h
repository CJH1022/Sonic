#pragma once
#include "CScript.h"

class CEffectScript :
    public CScript
{
private:
    bool    m_bDestroyAtFinish; // 애니메이션 종료 시 파괴 여부 (기본값 true)

public:
    virtual void Begin() override;
    virtual void Tick() override;

public:
    // 옵션: 애니메이션이 끝나도 안 지워지게 하고 싶을 때를 위해
    void SetDestroyAtFinish(bool _b) { m_bDestroyAtFinish = _b; }

public:
    CLONE(CEffectScript);
    CEffectScript();
    ~CEffectScript();
};