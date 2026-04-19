#include "pch.h"
#include "CUIMgrScript.h"

#include "ALevel.h"
#include "AssetMgr.h"
#include "ASprite.h"
#include "CCamera.h"
#include "CCollider2D.h"
#include "CSpriteRender.h"
#include "CTransform.h"
#include "Device.h"
#include "GameObject.h"
#include "Layer.h"
#include "LevelMgr.h"
#include "RenderMgr.h"
#include "Source/Scripts/CCoinMgrScript.h"
#include "Source/Scripts/CPlayerScript.h"
#include "TimeMgr.h"
#include "func.h"

namespace
{
    constexpr int kFallbackUIRenderLayer = 31;
    constexpr const wchar_t* kStageStartVisualName = L"UI_StageStartVisual";
    constexpr const wchar_t* kStageGeneralVisualName = L"UI_StageGeneralVisual";
    constexpr const wchar_t* kStageEndVisualName = L"UI_StageEndVisual";
    constexpr const wchar_t* kStageStartSpritePath = L"Sprite\\Stage_Start_0.sprite";
    constexpr const wchar_t* kStageGeneralSpritePath = L"Sprite\\Stage_General.sprite";
    constexpr const wchar_t* kStageEndSpritePath = L"Sprite\\Stage_End_0.sprite";
    constexpr const wchar_t* kNumberTexturePath = L"Texture\\Number.png";
    constexpr float kStageStartDuration = 5.f;
    constexpr float kStageStartEnterDuration = 1.25f;
    constexpr float kStageStartHoldDuration = 2.5f;
    constexpr float kStageUIRootDepth = 1.2f;
    constexpr float kStageStartVisualDepth = 0.f;
    constexpr float kStageGeneralVisualDepth = 0.f;
    constexpr float kStageEndVisualDepth = 0.f;
    constexpr float kStageGeneralDigitDepth = -40.f;
    constexpr float kStageTimerLimitSeconds = 600.f;
    constexpr int kStageGeneralScoreDigitCount = 7;
    constexpr int kStageGeneralTimeDigitCount = 3;
    constexpr int kStageGeneralCoinDigitCount = 3;
    constexpr int kStageGeneralLifeDigitCount = 1;
    constexpr int kCoinWrapThreshold = 20;
    const Vec2 kGeneralDigitPixelSize = Vec2(7.f, 11.f);
    constexpr float kStageGeneralWidth = 379.f;
    constexpr float kStageGeneralHeight = 252.f;
    const Vec2 kScoreDigitPixelCenters[kStageGeneralScoreDigitCount] =
    {
        Vec2(59.f, 16.f),
        Vec2(67.f, 16.f),
        Vec2(75.f, 16.f),
        Vec2(83.f, 16.f),
        Vec2(91.f, 16.f),
        Vec2(99.f, 16.f),
        Vec2(107.f, 16.f),
    };
    const Vec2 kTimeDigitPixelCenters[kStageGeneralTimeDigitCount] =
    {
        Vec2(59.f, 32.f),
        Vec2(75.f, 32.f),
        Vec2(83.f, 32.f),
    };
    const Vec2 kCoinDigitPixelCenters[kStageGeneralCoinDigitCount] =
    {
        Vec2(67.f, 48.f),
        Vec2(75.f, 48.f),
        Vec2(83.f, 48.f),
    };
    const Vec2 kLifeDigitPixelCenters[kStageGeneralLifeDigitCount] =
    {
        Vec2(31.f, 242.f),
    };

    struct NumberDigitSlice
    {
        int StartX;
        int Width;
    };

    constexpr NumberDigitSlice kNumberDigitSlices[10] =
    {
        { 0, 7 },
        { 10, 4 },
        { 18, 7 },
        { 27, 7 },
        { 36, 7 },
        { 45, 7 },
        { 54, 7 },
        { 63, 7 },
        { 72, 7 },
        { 81, 7 },
    };

    int ClampStageStateIndex(int _StateIndex)
    {
        if (_StateIndex < (int)CUIMgrScript::STAGESTATE::START)
            return (int)CUIMgrScript::STAGESTATE::START;

        if (_StateIndex > (int)CUIMgrScript::STAGESTATE::END)
            return (int)CUIMgrScript::STAGESTATE::END;

        return _StateIndex;
    }

    CUIMgrScript::STAGESTATE ToStageState(int _StateIndex)
    {
        return (CUIMgrScript::STAGESTATE)ClampStageStateIndex(_StateIndex);
    }

    float Clamp01(float _Value)
    {
        if (_Value < 0.f)
            return 0.f;
        if (_Value > 1.f)
            return 1.f;
        return _Value;
    }

    float EaseOutCubic(float _T)
    {
        const float clampedT = Clamp01(_T);
        const float invT = 1.f - clampedT;
        return 1.f - (invT * invT * invT);
    }

    float EaseInCubic(float _T)
    {
        const float clampedT = Clamp01(_T);
        return clampedT * clampedT * clampedT;
    }

    Vec3 ConvertUIPixelCenterToLocal(const Vec2& _PixelCenter)
    {
        const Vec2 resolution = Device::GetInst()->GetRenderResolution();
        const float localX = ((_PixelCenter.x / kStageGeneralWidth) - 0.5f) * resolution.x;
        const float localY = (0.5f - (_PixelCenter.y / kStageGeneralHeight)) * resolution.y;
        return Vec3(localX, localY, 0.f);
    }

    Vec3 ConvertUIPixelSizeToLocalScale(const Vec2& _PixelSize)
    {
        const Vec2 resolution = Device::GetInst()->GetRenderResolution();
        return Vec3((_PixelSize.x / kStageGeneralWidth) * resolution.x,
                    (_PixelSize.y / kStageGeneralHeight) * resolution.y,
                    1.f);
    }

    Ptr<ASprite> FindOrCreateNumberDigitSprite(int _Digit)
    {
        const int clampedDigit = max(0, min(9, _Digit));
        const wstring spriteKey = L"UI_NumberDigit_" + std::to_wstring(clampedDigit);

        Ptr<ASprite> pFoundSprite = AssetMgr::GetInst()->Find<ASprite>(spriteKey);
        if (nullptr != pFoundSprite)
            return pFoundSprite;

        Ptr<ATexture> pNumberAtlas = LOAD(ATexture, kNumberTexturePath);
        if (nullptr == pNumberAtlas)
            return nullptr;

        const NumberDigitSlice& slice = kNumberDigitSlices[clampedDigit];
        const float atlasWidth = max(1.f, pNumberAtlas->GetWidth());
        const float atlasHeight = max(1.f, pNumberAtlas->GetHeight());

        Ptr<ASprite> pSprite = new ASprite;
        pSprite->SetName(spriteKey);
        pSprite->SetAtlas(pNumberAtlas);
        pSprite->SetLeftTopUV(Vec2((float)slice.StartX / atlasWidth, 0.f));
        pSprite->SetSliceUV(Vec2((float)slice.Width / atlasWidth, 1.f));
        AssetMgr::GetInst()->AddAsset(spriteKey, pSprite.Get());
        return pSprite;
    }
}

CUIMgrScript::CUIMgrScript()
    : CScript(SCRIPT_TYPE::UIMGRSCRIPT)
{
    RegisterScriptParams();
}

CUIMgrScript::CUIMgrScript(const CUIMgrScript& _Origin)
    : CScript(SCRIPT_TYPE::UIMGRSCRIPT)
    , m_curState(_Origin.m_curState)
    , m_StageStateIndex(_Origin.m_StageStateIndex)
    , m_UsePlayerTrigger(_Origin.m_UsePlayerTrigger)
    , m_AutoSyncCoin(_Origin.m_AutoSyncCoin)
    , m_AutoSyncLife(_Origin.m_AutoSyncLife)
    , m_AutoAdvanceToPlay(_Origin.m_AutoAdvanceToPlay)
    , m_AutoChangeLevelOnEnd(_Origin.m_AutoChangeLevelOnEnd)
    , m_Time(_Origin.m_Time)
    , m_curScore(_Origin.m_curScore)
    , m_curCoin(_Origin.m_curCoin)
    , m_curLife(_Origin.m_curLife)
    , m_LastSyncedCoinForScore(_Origin.m_LastSyncedCoinForScore)
    , m_bCoinScoreInitialized(_Origin.m_bCoinScoreInitialized)
    , m_NextLevelName(_Origin.m_NextLevelName)
{
    RegisterScriptParams();
}

CUIMgrScript::~CUIMgrScript()
{
    // Runtime UI visuals are owned by the active play-level clone.
    // Do not queue Destroy tasks while the level clone is already tearing down.
    m_pStageStartVisual = nullptr;
    m_pStageGeneralVisual = nullptr;
    m_pStageEndVisual = nullptr;
    m_vecStageGeneralScoreDigits.clear();
    m_vecStageGeneralTimeDigits.clear();
    m_vecStageGeneralCoinDigits.clear();
    m_vecStageGeneralLifeDigits.clear();
}

void CUIMgrScript::RegisterScriptParams()
{
    AddScriptParam(SCRIPT_PARAM::INT, &m_StageStateIndex, L"Stage State", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::INT, &m_UsePlayerTrigger, L"Use Player Trigger", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::INT, &m_AutoSyncCoin, L"Auto Sync Coin", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::INT, &m_AutoSyncLife, L"Auto Sync Life", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::INT, &m_AutoAdvanceToPlay, L"Auto Advance", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::INT, &m_AutoChangeLevelOnEnd, L"Auto Change Level", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::FLOAT, &m_Time, L"Stage Time", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::INT, &m_curScore, L"Score", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::INT, &m_curCoin, L"Coin", false, 1.f);
    AddScriptParam(SCRIPT_PARAM::INT, &m_curLife, L"Life", false, 1.f);
}

void CUIMgrScript::Begin()
{
    if (Collider2D() != nullptr)
        Collider2D()->AddDynamicBeginOverlap(this, (COLLISION_EVENT)&CUIMgrScript::BeginOverlap);

    SyncStageStateFromInspector();
    ApplyStageUI();
    SyncUIRootToCamera();
}

void CUIMgrScript::Tick()
{
    SyncStageStateFromInspector();

    if (m_curState == STAGESTATE::GENERAL)
        m_Time += DT;

    RefreshRuntimeStats();
    ApplyStageUI();
    SyncUIRootToCamera();

    if (m_curState == STAGESTATE::START)
        UpdateStageStartUI();
    else if (m_curState == STAGESTATE::GENERAL)
        UpdateStageGeneralUI();

    if (m_curState == STAGESTATE::END
        && 0 != m_AutoChangeLevelOnEnd
        && !m_bEndLevelChangeQueued
        && !m_NextLevelName.empty())
    {
        m_bEndLevelChangeQueued = true;
        ChangeLevel(m_NextLevelName);
        ChangeLevelState(LEVEL_STATE::PLAY);
    }
}

void CUIMgrScript::SaveToLevelFile(FILE* _File)
{
    fwrite(&m_StageStateIndex, sizeof(int), 1, _File);
    fwrite(&m_UsePlayerTrigger, sizeof(int), 1, _File);
    fwrite(&m_AutoSyncCoin, sizeof(int), 1, _File);
    fwrite(&m_AutoSyncLife, sizeof(int), 1, _File);
    fwrite(&m_AutoAdvanceToPlay, sizeof(int), 1, _File);
    fwrite(&m_AutoChangeLevelOnEnd, sizeof(int), 1, _File);
    fwrite(&m_Time, sizeof(float), 1, _File);
    fwrite(&m_curScore, sizeof(int), 1, _File);
    fwrite(&m_curCoin, sizeof(int), 1, _File);
    fwrite(&m_curLife, sizeof(int), 1, _File);
    SaveWString(_File, m_NextLevelName);
}

void CUIMgrScript::LoadFromLevelFile(FILE* _File)
{
    fread(&m_StageStateIndex, sizeof(int), 1, _File);
    fread(&m_UsePlayerTrigger, sizeof(int), 1, _File);
    fread(&m_AutoSyncCoin, sizeof(int), 1, _File);
    fread(&m_AutoSyncLife, sizeof(int), 1, _File);
    fread(&m_AutoAdvanceToPlay, sizeof(int), 1, _File);
    fread(&m_AutoChangeLevelOnEnd, sizeof(int), 1, _File);
    fread(&m_Time, sizeof(float), 1, _File);
    fread(&m_curScore, sizeof(int), 1, _File);
    fread(&m_curCoin, sizeof(int), 1, _File);
    fread(&m_curLife, sizeof(int), 1, _File);
    m_NextLevelName = LoadWString(_File);

    m_curState = ToStageState(m_StageStateIndex);
    m_bInitializedStageUI = false;
    m_bEndLevelChangeQueued = false;
    m_StageStartAnimTime = 0.f;
    m_pStageStartVisual = nullptr;
    m_pStageGeneralVisual = nullptr;
    m_pStageEndVisual = nullptr;
    m_vecStageGeneralScoreDigits.clear();
    m_vecStageGeneralTimeDigits.clear();
    m_vecStageGeneralCoinDigits.clear();
    m_vecStageGeneralLifeDigits.clear();
    m_LastSyncedCoinForScore = m_curCoin;
    m_bCoinScoreInitialized = false;
}

void CUIMgrScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (0 == m_UsePlayerTrigger || nullptr == _OtherCollider || nullptr == _OtherCollider->GetOwner())
        return;

    if (_OtherCollider->GetOwner()->GetName() == L"Player")
        AdvanceToNextStage();
}

void CUIMgrScript::SetStageState(STAGESTATE _State)
{
    m_curState = _State;
    m_StageStateIndex = (int)_State;
    m_bInitializedStageUI = false;
    m_bEndLevelChangeQueued = false;
}

void CUIMgrScript::SetStageStateByIndex(int _StateIndex)
{
    SetStageState(ToStageState(_StateIndex));
}

void CUIMgrScript::PrepareForLevelStart()
{
    m_Time = 0.f;
    m_curScore = 0;
    if (0 != m_AutoSyncLife)
        m_curLife = max(0, CPlayerScript::GetPlayerLife());
    m_bCoinScoreInitialized = false;
    m_LastSyncedCoinForScore = 0;
    m_StageStartAnimTime = 0.f;
    SetStageState(STAGESTATE::START);
}

void CUIMgrScript::SyncStageStateFromInspector()
{
    const int clampedIndex = ClampStageStateIndex(m_StageStateIndex);
    if (clampedIndex != m_StageStateIndex)
        m_StageStateIndex = clampedIndex;

    if (m_curState != ToStageState(m_StageStateIndex))
        SetStageStateByIndex(m_StageStateIndex);
}

Vec3 CUIMgrScript::GetStageUICameraAnchorPos() const
{
    Vec3 vPos = Vec3(0.f, 0.f, kStageUIRootDepth);

    Ptr<CCamera> pCamera = nullptr;
    if (LevelMgr::GetInst()->GetLevelState() == LEVEL_STATE::STOP)
        pCamera = RenderMgr::GetInst()->GetEditorCamera();

    if (nullptr == pCamera)
        pCamera = RenderMgr::GetInst()->GetPOVCamera();

    if (nullptr == pCamera)
        pCamera = RenderMgr::GetInst()->GetEditorCamera();

    if (nullptr != pCamera && nullptr != pCamera->GetOwner() && nullptr != pCamera->GetOwner()->Transform())
        vPos = pCamera->GetOwner()->Transform()->GetRelativePos();

    vPos.z = kStageUIRootDepth;
    return vPos;
}

int CUIMgrScript::ResolveUIRenderLayer()
{
    Ptr<ALevel> pCurLevel = LevelMgr::GetInst()->GetCurLevel();
    if (nullptr != pCurLevel)
    {
        for (int i = 0; i < MAX_LAYER; ++i)
        {
            Layer* pLayer = pCurLevel->GetLayer(i);
            if (nullptr != pLayer && pLayer->GetName() == L"UI")
                return i;
        }
    }

    if (GetOwner() != nullptr)
        return GetOwner()->GetLayerIdx();

    return kFallbackUIRenderLayer;
}

void CUIMgrScript::SyncUIRootToCamera()
{
    const Vec2 resolution = Device::GetInst()->GetRenderResolution();
    const Vec3 cameraAnchorPos = GetStageUICameraAnchorPos();

    if (nullptr != m_pStageGeneralVisual && nullptr != m_pStageGeneralVisual->Transform())
    {
        m_pStageGeneralVisual->Transform()->SetRelativePos(Vec3(cameraAnchorPos.x, cameraAnchorPos.y, cameraAnchorPos.z + kStageGeneralVisualDepth));
        m_pStageGeneralVisual->Transform()->SetRelativeScale(Vec3(resolution.x, resolution.y, 1.f));
    }

    if (nullptr != m_pStageEndVisual && nullptr != m_pStageEndVisual->Transform())
    {
        m_pStageEndVisual->Transform()->SetRelativePos(Vec3(cameraAnchorPos.x, cameraAnchorPos.y, cameraAnchorPos.z + kStageEndVisualDepth));
        m_pStageEndVisual->Transform()->SetRelativeScale(Vec3(resolution.x, resolution.y, 1.f));
    }
}

void CUIMgrScript::RefreshRuntimeStats()
{
    if (m_curState != STAGESTATE::GENERAL)
        return;

    if (0 != m_AutoSyncCoin)
    {
        const int coinValue = CCoinMgrScript::GetCoinCount();
        SyncScoreFromCoinDelta(coinValue);
        m_curCoin = coinValue;
    }

    if (0 != m_AutoSyncLife)
        m_curLife = max(0, CPlayerScript::GetPlayerLife());

    HandleTimeOver();
}

void CUIMgrScript::ApplyStageUI()
{
    if (m_bInitializedStageUI)
        return;

    if (m_curState == STAGESTATE::START)
        CreateStageStartUI();
    else if (m_curState == STAGESTATE::GENERAL)
        CreateStageGeneralUI();
    else
        CreateStageEndUI();

    m_bInitializedStageUI = true;
}

void CUIMgrScript::AdvanceToNextStage()
{
    if (m_curState == STAGESTATE::START)
        SetStageState(STAGESTATE::GENERAL);
    else if (m_curState == STAGESTATE::GENERAL)
        SetStageState(STAGESTATE::END);
}

void CUIMgrScript::UpdateStageStartUI()
{
    if (nullptr == m_pStageStartVisual || nullptr == m_pStageStartVisual->Transform())
        return;

    const Vec2 resolution = Device::GetInst()->GetRenderResolution();
    const Vec3 cameraAnchorPos = GetStageUICameraAnchorPos();
    m_StageStartAnimTime += DT;

    float yOffset = 0.f;
    if (m_StageStartAnimTime < kStageStartEnterDuration)
    {
        const float t = EaseOutCubic(m_StageStartAnimTime / kStageStartEnterDuration);
        yOffset = (1.f - t) * resolution.y;
    }
    else if (m_StageStartAnimTime >= kStageStartEnterDuration + kStageStartHoldDuration)
    {
        const float exitDuration = max(0.01f, kStageStartDuration - (kStageStartEnterDuration + kStageStartHoldDuration));
        const float t = EaseInCubic((m_StageStartAnimTime - (kStageStartEnterDuration + kStageStartHoldDuration)) / exitDuration);
        yOffset = t * resolution.y;
    }

    m_pStageStartVisual->Transform()->SetRelativePos(Vec3(cameraAnchorPos.x, cameraAnchorPos.y + yOffset, cameraAnchorPos.z + kStageStartVisualDepth));
    m_pStageStartVisual->Transform()->SetRelativeScale(Vec3(resolution.x, resolution.y, 1.f));

    if (0 != m_AutoAdvanceToPlay && m_StageStartAnimTime >= kStageStartDuration)
        SetStageState(STAGESTATE::GENERAL);
}

void CUIMgrScript::UpdateStageGeneralUI()
{
    RefreshGeneralUIDigits();
}

GameObject* CUIMgrScript::CreateFullscreenSpriteChild(const wchar_t* _Name, const wchar_t* _SpritePath, float _LocalZ)
{
    Ptr<ALevel> pCurLevel = LevelMgr::GetInst()->GetCurLevel();
    if (nullptr == pCurLevel)
        return nullptr;

    Ptr<ASprite> pSprite = LOAD(ASprite, _SpritePath);
    if (nullptr == pSprite)
        return nullptr;

    Ptr<GameObject> pChild = new GameObject;
    pChild->SetName(_Name);
    pChild->AddComponent(new CTransform);
    pChild->AddComponent(new CSpriteRender);

    pChild->Transform()->SetIndependentScale(true);
    pChild->Transform()->SetRelativePos(Vec3(0.f, 0.f, _LocalZ));

    const Vec2 resolution = Device::GetInst()->GetRenderResolution();
    pChild->Transform()->SetRelativeScale(Vec3(resolution.x, resolution.y, 1.f));

    pChild->SpriteRender()->SetSprite(pSprite);
    pChild->SpriteRender()->CreateMaterial();

    GameObject* pCreatedChild = pChild.Get();
    pCurLevel->AddObject(ResolveUIRenderLayer(), pChild);
    return pCreatedChild;
}

GameObject* CUIMgrScript::CreateGeneralDigitChild(GameObject* _Parent, const wchar_t* _Name, const Vec2& _PixelCenter, const Vec2& _PixelSize, float _LocalZ)
{
    if (nullptr == _Parent)
        return nullptr;

    Ptr<GameObject> pChild = new GameObject;
    pChild->SetName(_Name);
    pChild->AddComponent(new CTransform);
    pChild->AddComponent(new CSpriteRender);

    pChild->Transform()->SetIndependentScale(true);
    pChild->Transform()->SetRelativePos(ConvertUIPixelCenterToLocal(_PixelCenter) + Vec3(0.f, 0.f, _LocalZ));
    pChild->Transform()->SetRelativeScale(ConvertUIPixelSizeToLocalScale(_PixelSize));

    Ptr<ASprite> pDigitSprite = FindOrCreateNumberDigitSprite(0);
    if (nullptr != pDigitSprite)
        pChild->SpriteRender()->SetSprite(pDigitSprite);
    pChild->SpriteRender()->CreateMaterial();

    GameObject* pCreatedChild = pChild.Get();
    _Parent->AddChild(pChild);
    return pCreatedChild;
}

void CUIMgrScript::DestroyUIChild(GameObject*& _Child)
{
    if (nullptr != _Child)
    {
        _Child->Destroy();
        _Child = nullptr;
    }
}

void CUIMgrScript::DestroyUIChildren(std::vector<GameObject*>& _Children)
{
    for (size_t i = 0; i < _Children.size(); ++i)
    {
        if (_Children[i] != nullptr)
            _Children[i]->Destroy();
    }

    _Children.clear();
}

void CUIMgrScript::RefreshGeneralUIDigits()
{
    if (m_curState != STAGESTATE::GENERAL || nullptr == m_pStageGeneralVisual)
        return;

    if ((int)m_vecStageGeneralScoreDigits.size() != kStageGeneralScoreDigitCount
        || (int)m_vecStageGeneralTimeDigits.size() != kStageGeneralTimeDigitCount
        || (int)m_vecStageGeneralCoinDigits.size() != kStageGeneralCoinDigitCount
        || (int)m_vecStageGeneralLifeDigits.size() != kStageGeneralLifeDigitCount)
    {
        return;
    }

    RefreshGeneralDigitTransforms(m_vecStageGeneralScoreDigits, kScoreDigitPixelCenters, kStageGeneralScoreDigitCount, kGeneralDigitPixelSize, kStageGeneralDigitDepth);
    RefreshGeneralDigitTransforms(m_vecStageGeneralTimeDigits, kTimeDigitPixelCenters, kStageGeneralTimeDigitCount, kGeneralDigitPixelSize, kStageGeneralDigitDepth);
    RefreshGeneralDigitTransforms(m_vecStageGeneralCoinDigits, kCoinDigitPixelCenters, kStageGeneralCoinDigitCount, kGeneralDigitPixelSize, kStageGeneralDigitDepth);
    RefreshGeneralDigitTransforms(m_vecStageGeneralLifeDigits, kLifeDigitPixelCenters, kStageGeneralLifeDigitCount, kGeneralDigitPixelSize, kStageGeneralDigitDepth);

    int scoreValue = max(0, m_curScore) % 10000000;
    for (int i = kStageGeneralScoreDigitCount - 1; i >= 0; --i)
    {
        SetGeneralDigitSprite(m_vecStageGeneralScoreDigits[i], scoreValue % 10);
        scoreValue /= 10;
    }

    int coinValue = max(0, m_curCoin) % 1000;
    for (int i = kStageGeneralCoinDigitCount - 1; i >= 0; --i)
    {
        SetGeneralDigitSprite(m_vecStageGeneralCoinDigits[i], coinValue % 10);
        coinValue /= 10;
    }

    const int displaySeconds = (int)fmodf(max(0.f, m_Time), kStageTimerLimitSeconds);
    const int minutes = (displaySeconds / 60) % 10;
    const int seconds = displaySeconds % 60;
    SetGeneralDigitSprite(m_vecStageGeneralTimeDigits[0], minutes);
    SetGeneralDigitSprite(m_vecStageGeneralTimeDigits[1], seconds / 10);
    SetGeneralDigitSprite(m_vecStageGeneralTimeDigits[2], seconds % 10);

    SetGeneralDigitSprite(m_vecStageGeneralLifeDigits[0], max(0, m_curLife) % 10);
}

void CUIMgrScript::RefreshGeneralDigitTransforms(std::vector<GameObject*>& _Digits, const Vec2* _PixelCenters, int _DigitCount, const Vec2& _PixelSize, float _LocalZ)
{
    for (int i = 0; i < _DigitCount; ++i)
    {
        if (_Digits[i] == nullptr || _Digits[i]->Transform() == nullptr)
            continue;

        _Digits[i]->Transform()->SetRelativePos(ConvertUIPixelCenterToLocal(_PixelCenters[i]) + Vec3(0.f, 0.f, _LocalZ));
        _Digits[i]->Transform()->SetRelativeScale(ConvertUIPixelSizeToLocalScale(_PixelSize));
    }
}

void CUIMgrScript::SetGeneralDigitSprite(GameObject* _DigitObj, int _Digit)
{
    if (nullptr == _DigitObj || nullptr == _DigitObj->SpriteRender() || nullptr == _DigitObj->Transform())
        return;

    const int clampedDigit = max(0, min(9, _Digit));
    Ptr<ASprite> pSprite = FindOrCreateNumberDigitSprite(clampedDigit);
    if (nullptr == pSprite)
        return;

    _DigitObj->SpriteRender()->SetSprite(pSprite);

    const NumberDigitSlice& slice = kNumberDigitSlices[clampedDigit];
    _DigitObj->Transform()->SetRelativeScale(ConvertUIPixelSizeToLocalScale(Vec2((float)slice.Width, kGeneralDigitPixelSize.y)));
}

void CUIMgrScript::SyncScoreFromCoinDelta(int _CoinValue)
{
    const int clampedCoin = max(0, _CoinValue);
    if (!m_bCoinScoreInitialized)
    {
        m_LastSyncedCoinForScore = clampedCoin;
        m_bCoinScoreInitialized = true;
        return;
    }

    int delta = clampedCoin - m_LastSyncedCoinForScore;
    while (delta < 0)
        delta += kCoinWrapThreshold;

    if (delta > 0)
        m_curScore += delta * 100;

    m_LastSyncedCoinForScore = clampedCoin;
}

void CUIMgrScript::HandleTimeOver()
{
    if (m_Time < kStageTimerLimitSeconds)
        return;

    Ptr<GameObject> pPlayer = LevelMgr::GetInst()->FindObjectByName(L"Player");
    if (nullptr != pPlayer)
    {
        Ptr<CPlayerScript> pPlayerScript = pPlayer->GetScript<CPlayerScript>();
        if (nullptr != pPlayerScript)
            pPlayerScript->Dead();
    }

    m_Time = fmodf(m_Time, kStageTimerLimitSeconds);
}

void CUIMgrScript::CreateStageStartUI()
{
    DestroyUIChild(m_pStageGeneralVisual);
    DestroyUIChild(m_pStageEndVisual);
    DestroyUIChildren(m_vecStageGeneralScoreDigits);
    DestroyUIChildren(m_vecStageGeneralTimeDigits);
    DestroyUIChildren(m_vecStageGeneralCoinDigits);
    DestroyUIChildren(m_vecStageGeneralLifeDigits);

    if (nullptr == m_pStageStartVisual)
        m_pStageStartVisual = CreateFullscreenSpriteChild(kStageStartVisualName, kStageStartSpritePath, kStageStartVisualDepth);

    m_StageStartAnimTime = 0.f;
    if (nullptr != m_pStageStartVisual && nullptr != m_pStageStartVisual->Transform())
    {
        const Vec2 resolution = Device::GetInst()->GetRenderResolution();
        const Vec3 cameraAnchorPos = GetStageUICameraAnchorPos();
        m_pStageStartVisual->Transform()->SetRelativePos(Vec3(cameraAnchorPos.x, cameraAnchorPos.y + resolution.y, cameraAnchorPos.z + kStageStartVisualDepth));
        m_pStageStartVisual->Transform()->SetRelativeScale(Vec3(resolution.x, resolution.y, 1.f));
    }
}

void CUIMgrScript::CreateStageGeneralUI()
{
    DestroyUIChild(m_pStageStartVisual);
    DestroyUIChild(m_pStageEndVisual);
    DestroyUIChildren(m_vecStageGeneralScoreDigits);
    DestroyUIChildren(m_vecStageGeneralTimeDigits);
    DestroyUIChildren(m_vecStageGeneralCoinDigits);
    DestroyUIChildren(m_vecStageGeneralLifeDigits);

    if (nullptr == m_pStageGeneralVisual)
        m_pStageGeneralVisual = CreateFullscreenSpriteChild(kStageGeneralVisualName, kStageGeneralSpritePath, kStageGeneralVisualDepth);

    for (int i = 0; i < kStageGeneralScoreDigitCount; ++i)
    {
        wchar_t nameBuffer[64] = {};
        swprintf_s(nameBuffer, L"UI_General_ScoreDigit_%d", i);
        m_vecStageGeneralScoreDigits.push_back(CreateGeneralDigitChild(m_pStageGeneralVisual, nameBuffer, kScoreDigitPixelCenters[i], kGeneralDigitPixelSize, kStageGeneralDigitDepth));
    }

    for (int i = 0; i < kStageGeneralTimeDigitCount; ++i)
    {
        wchar_t nameBuffer[64] = {};
        swprintf_s(nameBuffer, L"UI_General_TimeDigit_%d", i);
        m_vecStageGeneralTimeDigits.push_back(CreateGeneralDigitChild(m_pStageGeneralVisual, nameBuffer, kTimeDigitPixelCenters[i], kGeneralDigitPixelSize, kStageGeneralDigitDepth));
    }

    for (int i = 0; i < kStageGeneralCoinDigitCount; ++i)
    {
        wchar_t nameBuffer[64] = {};
        swprintf_s(nameBuffer, L"UI_General_CoinDigit_%d", i);
        m_vecStageGeneralCoinDigits.push_back(CreateGeneralDigitChild(m_pStageGeneralVisual, nameBuffer, kCoinDigitPixelCenters[i], kGeneralDigitPixelSize, kStageGeneralDigitDepth));
    }

    for (int i = 0; i < kStageGeneralLifeDigitCount; ++i)
    {
        wchar_t nameBuffer[64] = {};
        swprintf_s(nameBuffer, L"UI_General_LifeDigit_%d", i);
        m_vecStageGeneralLifeDigits.push_back(CreateGeneralDigitChild(m_pStageGeneralVisual, nameBuffer, kLifeDigitPixelCenters[i], kGeneralDigitPixelSize, kStageGeneralDigitDepth));
    }

    m_LastSyncedCoinForScore = m_curCoin;
    m_bCoinScoreInitialized = false;
    UpdateStageGeneralUI();
}

void CUIMgrScript::CreateStageEndUI()
{
    DestroyUIChild(m_pStageStartVisual);
    DestroyUIChild(m_pStageGeneralVisual);
    DestroyUIChildren(m_vecStageGeneralScoreDigits);
    DestroyUIChildren(m_vecStageGeneralTimeDigits);
    DestroyUIChildren(m_vecStageGeneralCoinDigits);
    DestroyUIChildren(m_vecStageGeneralLifeDigits);

    if (nullptr == m_pStageEndVisual)
        m_pStageEndVisual = CreateFullscreenSpriteChild(kStageEndVisualName, kStageEndSpritePath, kStageEndVisualDepth);

    if (nullptr != m_pStageEndVisual && nullptr != m_pStageEndVisual->Transform())
    {
        const Vec2 resolution = Device::GetInst()->GetRenderResolution();
        const Vec3 cameraAnchorPos = GetStageUICameraAnchorPos();
        m_pStageEndVisual->Transform()->SetRelativePos(Vec3(cameraAnchorPos.x, cameraAnchorPos.y, cameraAnchorPos.z + kStageEndVisualDepth));
        m_pStageEndVisual->Transform()->SetRelativeScale(Vec3(resolution.x, resolution.y, 1.f));
    }
}
