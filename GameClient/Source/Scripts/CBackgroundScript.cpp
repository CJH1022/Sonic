#include "pch.h"
#include "CBackgroundScript.h"

#include "RenderMgr.h"

CBackgroundScript::CBackgroundScript()
    : CScript(SCRIPT_TYPE::BACKGROUNDSCRIPT)
    , m_vInitialPos(0.f, 0.f, 0.f)
    , m_fParallaxRatio(0.1f)
{
}

CBackgroundScript::~CBackgroundScript()
{
}

void CBackgroundScript::Begin()
{
    m_vInitialPos = Transform()->GetRelativePos();
}

void CBackgroundScript::Tick()
{
    Ptr<CCamera> pMainCam = RenderMgr::GetInst()->GetPOVCamera();
    if (nullptr == pMainCam)
        return;

    Vec3 vCameraPos = pMainCam->Transform()->GetRelativePos();

    Vec3 vPos = Transform()->GetRelativePos();
    vPos.x = m_vInitialPos.x + (vCameraPos.x * m_fParallaxRatio);
    vPos.y = m_vInitialPos.y + (vCameraPos.y * m_fParallaxRatio);
    Transform()->SetRelativePos(vPos);
    
}
