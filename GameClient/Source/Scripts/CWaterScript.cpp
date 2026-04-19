#include "pch.h"
#include "CWaterScript.h"

#include "AssetMgr.h"
#include "AGraphicShader.h"
#include "AMaterial.h"
#include "ATexture.h"
#include "CCollider2D.h"
#include "CMeshRender.h"
#include "CPlayerScript.h"
#include "CTransform.h"
#include "GameObject.h"

namespace
{
    constexpr UINT kWaterScriptSaveMagic = 0x57415452; // WATR

    Ptr<AGraphicShader> FindOrCreateWaterShader()
    {
        Ptr<AGraphicShader> pShader = AssetMgr::GetInst()->Find<AGraphicShader>(L"WaterPostProcessShader");
        if (nullptr != pShader)
            return pShader;

        pShader = new AGraphicShader;
        pShader->SetName(L"WaterPostProcessShader");
        pShader->CreateVertexShader(L"Shader\\water_postprocess.fx", "VS_WaterPostProcess");
        pShader->CreatePixelShader(L"Shader\\water_postprocess.fx", "PS_WaterPostProcess");
        pShader->SetBSType(BS_TYPE::ALPHABLEND);
        pShader->SetRSType(RS_TYPE::CULL_NONE);
        pShader->SetDSType(DS_TYPE::NO_TEST_NO_WRITE);
        AssetMgr::GetInst()->AddAsset(pShader->GetName(), pShader.Get());
        return pShader;
    }

    Ptr<AMaterial> FindOrCreateWaterMaterial()
    {
        Ptr<AMaterial> pMaterial = AssetMgr::GetInst()->Find<AMaterial>(L"WaterPostProcessMtrl");
        if (nullptr != pMaterial)
            return pMaterial;

        pMaterial = new AMaterial;
        pMaterial->SetName(L"WaterPostProcessMtrl");
        pMaterial->SetShader(FindOrCreateWaterShader());
        pMaterial->SetDomain(RENDER_DOMAIN::DOMAIN_TRANSPARENT);
        pMaterial->SetTexture(TEX_0, LOAD(ATexture, L"Texture\\noise\\noise_03.jpg"));
        AssetMgr::GetInst()->AddAsset(pMaterial->GetName(), pMaterial.Get());
        return pMaterial;
    }

    GameObject* FindChildByName(GameObject* _Owner, const wchar_t* _Name)
    {
        if (_Owner == nullptr || _Name == nullptr)
            return nullptr;

        const vector<Ptr<GameObject>>& children = _Owner->GetChild();
        for (const Ptr<GameObject>& pChild : children)
        {
            if (pChild != nullptr && pChild->GetName() == _Name)
                return pChild.Get();
        }

        return nullptr;
    }
}

CWaterScript::CWaterScript()
    : CScript(SCRIPT_TYPE::WATERSCRIPT)
    , m_WaterAccelScale(0.7f)
    , m_WaveSpeed(0.2f)
    , m_WaveTiling(2.2f)
    , m_ShimmerStrength(1.0f)
    , m_OverlayAlpha(0.45f)
    , m_TintColor(Vec4(0.2f, 0.45f, 1.f, 1.f))
{
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_WaterAccelScale, L"Water Accel Scale", false, 0.05f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_WaveSpeed, L"Wave Speed", false, 0.05f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_WaveTiling, L"Wave Tiling", false, 0.05f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_ShimmerStrength, L"Shimmer Strength", false, 0.05f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_OverlayAlpha, L"Overlay Alpha", false, 0.05f);
    AddScriptParam(SCRIPT_PARAM::VEC4, &m_TintColor, L"Tint Color", false, 0.05f);
}

CWaterScript::CWaterScript(const CWaterScript& _Origin)
    : CScript(_Origin)
    , m_WaterAccelScale(_Origin.m_WaterAccelScale)
    , m_WaveSpeed(_Origin.m_WaveSpeed)
    , m_WaveTiling(_Origin.m_WaveTiling)
    , m_ShimmerStrength(_Origin.m_ShimmerStrength)
    , m_OverlayAlpha(_Origin.m_OverlayAlpha)
    , m_TintColor(_Origin.m_TintColor)
{
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_WaterAccelScale, L"Water Accel Scale", false, 0.05f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_WaveSpeed, L"Wave Speed", false, 0.05f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_WaveTiling, L"Wave Tiling", false, 0.05f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_ShimmerStrength, L"Shimmer Strength", false, 0.05f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_OverlayAlpha, L"Overlay Alpha", false, 0.05f);
    AddScriptParam(SCRIPT_PARAM::VEC4, &m_TintColor, L"Tint Color", false, 0.05f);
}

CWaterScript::~CWaterScript()
{
}

void CWaterScript::Begin()
{
    if (Collider2D() != nullptr)
    {
        ADD_DYNAMIC_BEGIN_OVERLAP(CWaterScript::BeginOverlap);
        ADD_DYNAMIC_END_OVERLAP(CWaterScript::EndOverlap);
    }

    EnsureOverlay();
    UpdateOverlay();
}

void CWaterScript::Tick()
{
    EnsureOverlay();
    UpdateOverlay();
}

void CWaterScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (_OtherCollider == nullptr || _OtherCollider->GetOwner() == nullptr)
        return;

    Ptr<CPlayerScript> pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer == nullptr)
        return;

    pPlayer->EnterWaterVolume(m_WaterAccelScale);
}

void CWaterScript::EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (_OtherCollider == nullptr || _OtherCollider->GetOwner() == nullptr)
        return;

    Ptr<CPlayerScript> pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer == nullptr)
        return;

    pPlayer->ExitWaterVolume();
}

void CWaterScript::EnsureOverlay()
{
    if (GetOwner() == nullptr)
        return;

    if (m_OverlayObject == nullptr || m_OverlayObject->IsDead())
    {
        GameObject* pExisting = FindChildByName(GetOwner(), L"WaterOverlay");
        if (pExisting != nullptr)
            m_OverlayObject = pExisting;
    }

    if (m_OverlayObject != nullptr)
    {
        if (m_OverlayObject->MeshRender() == nullptr)
            m_OverlayObject->AddComponent(new CMeshRender);
        return;
    }

    GameObject* pOverlay = new GameObject;
    pOverlay->SetName(L"WaterOverlay");
    pOverlay->AddComponent(new CTransform);
    pOverlay->AddComponent(new CMeshRender);
    pOverlay->MeshRender()->SetMesh(AssetMgr::GetInst()->Find<AMesh>(L"RectMesh"));
    pOverlay->MeshRender()->SetMaterial(FindOrCreateWaterMaterial()->Clone());
    pOverlay->Transform()->SetRelativePos(Vec3(0.f, 0.f, 0.f));
    pOverlay->Transform()->SetRelativeScale(Vec3(1.f, 1.f, 1.f));

    GetOwner()->AddChild(pOverlay);
    m_OverlayObject = pOverlay;
}

void CWaterScript::UpdateOverlay()
{
    if (m_OverlayObject == nullptr || m_OverlayObject->MeshRender() == nullptr)
        return;

    Vec2 overlayScale = Vec2(1.f, 1.f);
    Vec2 overlayOffset = Vec2(0.f, 0.f);

    if (Collider2D() != nullptr)
    {
        overlayScale = Collider2D()->GetScale();
        overlayOffset = Collider2D()->GetOffset();
    }

    m_OverlayObject->Transform()->SetRelativePos(Vec3(overlayOffset.x, overlayOffset.y, 0.f));
    m_OverlayObject->Transform()->SetRelativeScale(Vec3(overlayScale.x, overlayScale.y, 1.f));

    Ptr<AMaterial> pMaterial = m_OverlayObject->MeshRender()->GetMaterial();
    if (nullptr == pMaterial)
    {
        pMaterial = FindOrCreateWaterMaterial()->Clone();
        m_OverlayObject->MeshRender()->SetMaterial(pMaterial);
    }

    pMaterial->SetTexture(TEX_0, LOAD(ATexture, L"Texture\\noise\\noise_03.jpg"));
    pMaterial->SetScalar(VEC4_0, m_TintColor);
    pMaterial->SetScalar(VEC4_1, Vec4(m_WaveSpeed, m_WaveTiling, m_ShimmerStrength, m_OverlayAlpha));
}

void CWaterScript::SaveToLevelFile(FILE* _File)
{
    fwrite(&kWaterScriptSaveMagic, sizeof(UINT), 1, _File);
    fwrite(&m_WaterAccelScale, sizeof(float), 1, _File);
    fwrite(&m_WaveSpeed, sizeof(float), 1, _File);
    fwrite(&m_WaveTiling, sizeof(float), 1, _File);
    fwrite(&m_ShimmerStrength, sizeof(float), 1, _File);
    fwrite(&m_OverlayAlpha, sizeof(float), 1, _File);
    fwrite(&m_TintColor, sizeof(Vec4), 1, _File);
}

void CWaterScript::LoadFromLevelFile(FILE* _File)
{
    UINT magic = 0;
    fread(&magic, sizeof(UINT), 1, _File);
    if (magic != kWaterScriptSaveMagic)
        return;

    fread(&m_WaterAccelScale, sizeof(float), 1, _File);
    fread(&m_WaveSpeed, sizeof(float), 1, _File);
    fread(&m_WaveTiling, sizeof(float), 1, _File);
    fread(&m_ShimmerStrength, sizeof(float), 1, _File);
    fread(&m_OverlayAlpha, sizeof(float), 1, _File);
    fread(&m_TintColor, sizeof(Vec4), 1, _File);
}
