#include "pch.h"
#include "CEffectScript.h"
#include "CFlipbookRender.h"
#include <TimeMgr.h>
#include "CTransform.h"

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
    m_ElapsedTime = 0.f;
}

void CEffectScript::Tick()
{
    if (m_LifeTime > 0.f)
    {
        m_ElapsedTime += DT;
        if (m_ElapsedTime >= m_LifeTime)
        {
            Destroy();
            return;
        }
    }

    // 1. 중력 적용
    m_vVelocity.y -= m_fGravity * DT;
    m_vVelocity += m_vAccel * DT;
    // 2. 위치 업데이트
    Vec3 vPos = Transform()->GetRelativePos();
    vPos.x += m_vVelocity.x * DT;
    vPos.y += m_vVelocity.y * DT;
    Transform()->SetRelativePos(vPos);

    // 1. 애니메이션 재생이 끝났는지 확인
    // FlipbookRender가 있고, 현재 재생 중인 플립북이 Finish 상태라면
    if (m_bDestroyAtFinish && FlipbookRender() != nullptr)
    {
        if (FlipbookRender()->IsFinish())
        {
            Destroy();
        }
    }
}
