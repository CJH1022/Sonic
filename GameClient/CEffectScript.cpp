#include "pch.h"
#include "CEffectScript.h"
#include "CFlipbookRender.h"

CEffectScript::CEffectScript()
    : CScript(SCRIPT_TYPE::EFFECTSCRIPT)
    , m_bDestroyAtFinish(true)
{
}

CEffectScript::~CEffectScript()
{
}

void CEffectScript::Begin()
{
    // 필요 시 초기화 로직
}

void CEffectScript::Tick()
{
    // 1. 애니메이션 재생이 끝났는지 확인
    // FlipbookRender가 있고, 현재 재생 중인 플립북이 Finish 상태라면
    if (m_bDestroyAtFinish && FlipbookRender() != nullptr)
    {
        if (FlipbookRender()->IsFinish())
        {
            // 2. 자기 자신 삭제 예약
            Destroy();
        }
    }
}