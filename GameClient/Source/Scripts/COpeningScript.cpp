#include "pch.h"
#include "COpeningScript.h"

#include "AssetMgr.h"
#include "ATexture.h"
#include "ASprite.h"
#include "Device.h"
#include "func.h"
#include "LevelMgr.h"
#include "GameObject.h"
#include "CFlipbookRender.h"
#include "CSpriteRender.h"
#include "CTransform.h"
#include "KeyMgr.h"
#include "PathMgr.h"
#include <TimeMgr.h>
#include <filesystem>

namespace
{
    struct PixelRect
    {
        float left;
        float top;
        float width;
        float height;
    };

    const Vec2 kOpeningMainSourceSize = Vec2(319.f, 223.f);
    constexpr PixelRect kOpeningEyeLocalRect = { 104.f, 48.f, 32.f, 48.f };
    constexpr PixelRect kOpeningHandLocalRect = { 176.f, 64.f, 48.f, 56.f };
    constexpr PixelRect kOpeningStartButtonLocalRect = { 112.f, 202.f, 96.f, 8.f };
    constexpr float kOpeningFrontOpeningDepth = 5.f;
    constexpr float kOpeningEyeHandWorldDepth = -1.f;
    constexpr float kOpeningSonicMainWorldDepth = 3.f;
    constexpr float kOpeningOverlayWorldDepth = 0.f;
    constexpr float kOpeningStartButtonLocalDepth = -30.f;
    constexpr float kOpeningOverlayFPS = 8.f;
    constexpr float kOpeningOverlayTargetY = -55.f;
    constexpr float kOpeningStartButtonRevealDelay = 0.25f;
    const Vec4 kOpeningChromaKey = Vec4(223.f / 255.f, 100.f / 255.f, 128.f / 255.f, 6.f / 255.f);
    const Vec4 kOpeningChromaKey2 = Vec4(188.f / 255.f, 32.f / 255.f, 55.f / 255.f, 6.f / 255.f);

    void FixTinyOpeningSpriteUV(Ptr<ASprite> _Sprite)
    {
        if (nullptr == _Sprite)
            return;

        const Vec2 slice = _Sprite->GetSliceUV();
        const Vec2 background = _Sprite->GetBackgroundUV();

        if ((slice.x <= 0.001f || slice.y <= 0.001f)
            && background.x > 0.f && background.y > 0.f)
        {
            _Sprite->SetSliceUV(background);
            _Sprite->SetOffsetUV(Vec2(0.f, 0.f));
        }
    }

    Vec2 GetSpritePixelSize(Ptr<ASprite> _Sprite)
    {
        if (nullptr == _Sprite || nullptr == _Sprite->GetAtlas())
            return Vec2(1.f, 1.f);

        Vec2 background = _Sprite->GetBackgroundUV();
        if (background.x <= 0.f || background.y <= 0.f)
            background = _Sprite->GetSliceUV();

        const float width = _Sprite->GetAtlas()->GetWidth() * background.x;
        const float height = _Sprite->GetAtlas()->GetHeight() * background.y;

        return Vec2((width > 1.f) ? width : 1.f, (height > 1.f) ? height : 1.f);
    }

    Ptr<ASprite> LoadFirstSprite(const initializer_list<const wchar_t*>& _SpritePaths)
    {
        for (const wchar_t* pPath : _SpritePaths)
        {
            if (nullptr == pPath)
                continue;

            Ptr<ASprite> pSprite = LOAD(ASprite, pPath);
            if (nullptr != pSprite)
                return pSprite;
        }

        return nullptr;
    }

    Ptr<ASprite> CreateFullTextureSprite(const wchar_t* _SpriteKey
        , const wchar_t* _TextureKey
        , const wchar_t* _TexturePath)
    {
        Ptr<ASprite> pSprite = FIND(ASprite, _SpriteKey);
        if (nullptr != pSprite)
            return pSprite;

        Ptr<ATexture> pTexture = AssetMgr::GetInst()->Load<ATexture>(_TextureKey, _TexturePath);
        if (nullptr == pTexture)
            return nullptr;

        pSprite = new ASprite;
        pSprite->SetName(_SpriteKey);
        pSprite->SetAtlas(pTexture);
        pSprite->SetLeftTopUV(Vec2(0.f, 0.f));
        pSprite->SetSliceUV(Vec2(1.f, 1.f));
        pSprite->SetBackgroundUV(Vec2(1.f, 1.f));
        pSprite->SetOffsetUV(Vec2(0.f, 0.f));

        AssetMgr::GetInst()->AddAsset(_SpriteKey, pSprite.Get());
        return pSprite;
    }

    Ptr<AFlipbook> CreateOverlayFlipbook(const initializer_list<Ptr<ASprite>>& _Sprites)
    {
        Ptr<AFlipbook> pFlipbook = new AFlipbook;

        for (const Ptr<ASprite>& pSprite : _Sprites)
        {
            if (nullptr == pSprite)
                continue;

            FixTinyOpeningSpriteUV(pSprite);
            pFlipbook->AddSprite(pSprite);
        }

        if (0 == pFlipbook->GetSpriteCount())
            return nullptr;

        return pFlipbook;
    }

    Vec3 GetOverlayLocalPos(const PixelRect& _Rect, const Vec2& _SourceSize, const Vec2& _DisplaySize, float _Z)
    {
        const float centerX = _Rect.left + (_Rect.width * 0.5f);
        const float centerY = _Rect.top + (_Rect.height * 0.5f);
        const float scaleX = (0.f < _SourceSize.x) ? (_DisplaySize.x / _SourceSize.x) : 1.f;
        const float scaleY = (0.f < _SourceSize.y) ? (_DisplaySize.y / _SourceSize.y) : 1.f;

        return Vec3((centerX - (_SourceSize.x * 0.5f)) * scaleX
            , ((_SourceSize.y * 0.5f) - centerY) * scaleY
            , _Z);
    }

    Vec3 GetOverlayLocalScale(const PixelRect& _Rect, const Vec2& _SourceSize, const Vec2& _DisplaySize)
    {
        const float scaleX = (0.f < _SourceSize.x) ? (_DisplaySize.x / _SourceSize.x) : 1.f;
        const float scaleY = (0.f < _SourceSize.y) ? (_DisplaySize.y / _SourceSize.y) : 1.f;
        return Vec3(_Rect.width * scaleX, _Rect.height * scaleY, 1.f);
    }

    template<typename T>
    void ApplyOpeningChromaKey(const Ptr<T>& _Render)
    {
        if (nullptr == _Render)
            return;

        _Render->CreateMaterial();
        Ptr<AMaterial> pSharedMaterial = _Render->GetSharedMaterial();
        if (nullptr == pSharedMaterial)
            return;

        Ptr<AMaterial> pCustomMaterial = pSharedMaterial->Clone();
        pCustomMaterial->SetScalar(INT_0, 1);
        pCustomMaterial->SetScalar(VEC4_1, kOpeningChromaKey);
        pCustomMaterial->SetScalar(VEC4_2, kOpeningChromaKey2);
        _Render->SetMaterial(pCustomMaterial);
    }

    GameObject* CreateOverlayFlipbookObject(const wchar_t* _Name
        , Ptr<AFlipbook> _Flipbook
        , const Vec3& _WorldPos
        , const Vec3& _WorldScale
        , float _FPS)
    {
        if (nullptr == _Flipbook)
            return nullptr;

        GameObject* pObject = new GameObject;
        pObject->SetName(_Name);
        pObject->AddComponent(new CTransform);
        pObject->AddComponent(new CFlipbookRender);

        pObject->Transform()->SetRelativePos(_WorldPos);
        pObject->Transform()->SetRelativeScale(_WorldScale);
        pObject->Transform()->SetIndependentScale(true);

        pObject->FlipbookRender()->AddFlipbook(_Flipbook);
        pObject->FlipbookRender()->Play(0, _FPS, -1);
        ApplyOpeningChromaKey(pObject->FlipbookRender());

        return pObject;
    }

    void ApplyOpeningWorldZ(const Ptr<GameObject>& _Object, float _WorldZ)
    {
        if (nullptr == _Object || nullptr == _Object->Transform())
            return;

        Vec3 pos = _Object->Transform()->GetRelativePos();
        pos.z = _WorldZ;
        _Object->Transform()->SetRelativePos(pos);
    }

    Vec2 GetOpeningMainSourceSize(GameObject* _SonicMain)
    {
        if (nullptr == _SonicMain || nullptr == _SonicMain->SpriteRender())
            return kOpeningMainSourceSize;

        Vec2 mainSourceSize = GetSpritePixelSize(_SonicMain->SpriteRender()->GetSprite());
        if (mainSourceSize.x <= 1.f || mainSourceSize.y <= 1.f)
            return kOpeningMainSourceSize;

        return mainSourceSize;
    }

    void AttachOpeningEyeHandChildren(GameObject* _SonicMain
        , const Vec2& _MainSourceSize
        , const Vec2& _DisplaySize
        , float _LocalZ
        , int _LayerIdx)
    {
        if (nullptr == _SonicMain || _SonicMain->Transform() == nullptr)
            return;

        const Vec3 sonicMainWorldPos = _SonicMain->Transform()->GetWorldPos();

        Ptr<ASprite> pEye0 = LoadFirstSprite({
            L"Sprite\\Sonic_Eye.sprite",
        });
        Ptr<ASprite> pEye1 = LoadFirstSprite({
            L"Sprite\\Sonic_Eye_1.sprite",
        });
        Ptr<ASprite> pEye2 = LoadFirstSprite({
            L"Sprite\\Sonic_Eye_2.sprite",
        });

        Ptr<AFlipbook> pEyeFlipbook = CreateOverlayFlipbook({ pEye0, pEye1, pEye2, pEye2, pEye1, pEye0 });
        const Vec3 eyeLocalPos = GetOverlayLocalPos(kOpeningEyeLocalRect, _MainSourceSize, _DisplaySize, _LocalZ);
        const Vec3 eyeWorldPos = Vec3(sonicMainWorldPos.x + eyeLocalPos.x
            , sonicMainWorldPos.y + eyeLocalPos.y
            , _LocalZ);
        const Vec3 eyeWorldScale = GetOverlayLocalScale(kOpeningEyeLocalRect, _MainSourceSize, _DisplaySize);

        Ptr<GameObject> pExistingEye = LevelMgr::GetInst()->FindObjectByName(L"Opening_Sonic_Eye");
        if (nullptr != pExistingEye && pExistingEye->Transform() != nullptr)
        {
            pExistingEye->Transform()->SetRelativePos(eyeWorldPos);
            pExistingEye->Transform()->SetRelativeScale(eyeWorldScale);
        }
        else
        {
            GameObject* pEyeObject = CreateOverlayFlipbookObject(L"Opening_Sonic_Eye"
                , pEyeFlipbook
                , eyeWorldPos
                , eyeWorldScale
                , kOpeningOverlayFPS);
            if (nullptr != pEyeObject)
                CreateObject(pEyeObject, _LayerIdx);
        }

        Ptr<ASprite> pHand0 = LoadFirstSprite({
            L"Sprite\\Sonic_Hand.sprite",
        });
        Ptr<ASprite> pHand1 = LoadFirstSprite({
            L"Sprite\\Sonic_Hand_1.sprite",
        });
        Ptr<ASprite> pHand2 = LoadFirstSprite({
            L"Sprite\\Sonic_Hand_2.sprite",
        });

        Ptr<AFlipbook> pHandFlipbook = CreateOverlayFlipbook({ pHand0, pHand1, pHand2, pHand2, pHand1, pHand0 });
        const Vec3 handLocalPos = GetOverlayLocalPos(kOpeningHandLocalRect, _MainSourceSize, _DisplaySize, _LocalZ);
        const Vec3 handWorldPos = Vec3(sonicMainWorldPos.x + handLocalPos.x
            , sonicMainWorldPos.y + handLocalPos.y
            , _LocalZ);
        const Vec3 handWorldScale = GetOverlayLocalScale(kOpeningHandLocalRect, _MainSourceSize, _DisplaySize);

        Ptr<GameObject> pExistingHand = LevelMgr::GetInst()->FindObjectByName(L"Opening_Sonic_Hand");
        if (nullptr != pExistingHand && pExistingHand->Transform() != nullptr)
        {
            pExistingHand->Transform()->SetRelativePos(handWorldPos);
            pExistingHand->Transform()->SetRelativeScale(handWorldScale);
        }
        else
        {
            GameObject* pHandObject = CreateOverlayFlipbookObject(L"Opening_Sonic_Hand"
                , pHandFlipbook
                , handWorldPos
                , handWorldScale
                , kOpeningOverlayFPS);
            if (nullptr != pHandObject)
                CreateObject(pHandObject, _LayerIdx);
        }
    }

    void EnsureOpeningEyeHandChildren(GameObject* _SonicMain, int _LayerIdx)
    {
        if (nullptr == _SonicMain)
            return;

        AttachOpeningEyeHandChildren(_SonicMain
            , GetOpeningMainSourceSize(_SonicMain)
            , Device::GetInst()->GetRenderResolution()
            , kOpeningEyeHandWorldDepth
            , _LayerIdx);
    }

    Ptr<GameObject> AttachOverlaySpriteChild(GameObject* _Parent
        , const wchar_t* _Name
        , Ptr<ASprite> _Sprite
        , const Vec3& _LocalPos
        , const Vec3& _LocalScale)
    {
        if (nullptr == _Parent || nullptr == _Sprite)
            return nullptr;

        Ptr<GameObject> pChild = new GameObject;
        pChild->SetName(_Name);
        pChild->AddComponent(new CTransform);
        pChild->AddComponent(new CSpriteRender);

        pChild->Transform()->SetRelativePos(_LocalPos);
        pChild->Transform()->SetRelativeScale(_LocalScale);
        pChild->Transform()->SetIndependentScale(true);
        pChild->SpriteRender()->SetSprite(_Sprite);

        _Parent->AddChild(pChild);
        return pChild;
    }

    Ptr<GameObject> CreateOpeningStartButtonChild(GameObject* _Overlay
        , const Vec2& _OverlaySourceSize
        , const Vec2& _OverlayDisplaySize
        , Ptr<ASprite> _OffSprite
        , Ptr<ASprite> _OnSprite
        , float _LocalZ)
    {
        if (nullptr == _Overlay)
            return nullptr;

        Ptr<ASprite> pDefaultButtonSprite = _OffSprite;
        if (nullptr == pDefaultButtonSprite)
            pDefaultButtonSprite = _OnSprite;

        if (nullptr == pDefaultButtonSprite)
            return nullptr;

        Vec3 buttonLocalPos = GetOverlayLocalPos(kOpeningStartButtonLocalRect, _OverlaySourceSize, _OverlayDisplaySize, _LocalZ);

        return AttachOverlaySpriteChild(_Overlay
            , L"Opening_Start_Button"
            , pDefaultButtonSprite
            , buttonLocalPos
            , GetOverlayLocalScale(kOpeningStartButtonLocalRect, _OverlaySourceSize, _OverlayDisplaySize));
    }

    Vec2 GetMouseLevelPos()
    {
        const Vec2 displaySize = Device::GetInst()->GetRenderResolution();
        const Vec2 mousePos = KeyMgr::GetInst()->GetMousePos();

        return Vec2(mousePos.x - displaySize.x * 0.5f
            , displaySize.y * 0.5f - mousePos.y);
    }

    bool IsMouseInsideButton(const Ptr<GameObject>& _Button)
    {
        if (nullptr == _Button)
            return false;

        const Vec2 mousePos = GetMouseLevelPos();
        const Vec3 worldPos = _Button->Transform()->GetWorldPos();
        const Vec3 worldScale = _Button->Transform()->GetWorldScale();
        const float halfWidth = worldScale.x * 0.5f;
        const float halfHeight = worldScale.y * 0.5f;

        return mousePos.x >= worldPos.x - halfWidth
            && mousePos.x <= worldPos.x + halfWidth
            && mousePos.y >= worldPos.y - halfHeight
            && mousePos.y <= worldPos.y + halfHeight;
    }

    void StartSavedLevelFromOpening()
    {
        namespace fs = std::filesystem;

        const fs::path levelDir = fs::path(CONTENT_PATH) / L"Level";
        const fs::path preferredTestLevelPath = levelDir / L"TestLevel.lv";

        std::error_code ec;
        if (fs::exists(preferredTestLevelPath, ec) && !ec)
        {
            Ptr<ALevel> pLoadedLevel = AssetMgr::GetInst()->Load<ALevel>(L"OpeningStartLevel", L"Level\\TestLevel.lv");
            if (nullptr != pLoadedLevel)
            {
                SetGameBGMPlaylist(GAME_BGM_PLAYLIST::STAGE);
                ChangeLevel(L"OpeningStartLevel");
                ChangeLevelState(LEVEL_STATE::PLAY);
                return;
            }
        }

        fs::path selectedLevelPath;
        fs::file_time_type selectedWriteTime{};
        bool foundSavedLevel = false;

        if (fs::exists(levelDir, ec))
        {
            for (const fs::directory_entry& entry : fs::directory_iterator(levelDir, ec))
            {
                if (ec)
                    break;

                if (!entry.is_regular_file(ec) || ec)
                    continue;

                if (entry.path().extension() != L".lv")
                    continue;

                if (entry.path().filename() == L"OpenLevel.lv")
                    continue;
                if (entry.path().filename() == L"OpeningStartLevel.lv")
                    continue;
                if (entry.path().filename().wstring().find(L"backup") != wstring::npos
                    || entry.path().filename().wstring().find(L"Backup") != wstring::npos)
                    continue;

                const fs::file_time_type writeTime = entry.last_write_time(ec);
                if (ec)
                    continue;

                if (!foundSavedLevel || selectedWriteTime < writeTime)
                {
                    selectedWriteTime = writeTime;
                    selectedLevelPath = entry.path();
                    foundSavedLevel = true;
                }
            }
        }

        if (foundSavedLevel)
        {
            const wstring levelFileName = selectedLevelPath.filename().wstring();
            const wstring levelKey = L"OpeningStartLevel";
            const wstring relativePath = L"Level\\" + levelFileName;

            Ptr<ALevel> pLoadedLevel = AssetMgr::GetInst()->Load<ALevel>(levelKey, relativePath);
            if (nullptr != pLoadedLevel)
            {
                SetGameBGMPlaylist(GAME_BGM_PLAYLIST::STAGE);
                ChangeLevel(levelKey);
                ChangeLevelState(LEVEL_STATE::PLAY);
                return;
            }
        }

        if (nullptr == AssetMgr::GetInst()->Find<ALevel>(L"TestLevel"))
            CreateTestLevel();
        else
        {
            SetGameBGMPlaylist(GAME_BGM_PLAYLIST::STAGE);
            ChangeLevel(L"TestLevel");
        }

        ChangeLevelState(LEVEL_STATE::PLAY);
    }
}

void COpeningScript::RegisterScriptParams()
{
    // Opening draw order is intentionally fixed in code.
}

COpeningScript::COpeningScript()
    : CScript(SCRIPT_TYPE::OPENINGSCRIPT)
    , m_bSpawnedSonicMain(false)
    , m_bSpawnedEyeHand(false)
    , m_FrontOpeningZ(kOpeningFrontOpeningDepth)
    , m_EyeHandLocalZ(kOpeningEyeHandWorldDepth)
    , m_SonicMainZ(kOpeningSonicMainWorldDepth)
    , m_Overlay20Z(kOpeningOverlayWorldDepth)
    , m_StartButtonLocalZ(kOpeningStartButtonLocalDepth)
    , m_TransitionAccTime(0.f)
    , m_PostTransitionAccTime(0.f)
    , m_bTransitionEnd(false)
    , m_pSonicMain(nullptr)
    , m_pOverlay20(nullptr)
    , m_pStartButton(nullptr)
    , m_pStartButtonOnSprite(nullptr)
    , m_pStartButtonOffSprite(nullptr)
    , m_bStartLevelRequested(false)
{
    RegisterScriptParams();
}

COpeningScript::COpeningScript(const COpeningScript& _Origin)
    : CScript(_Origin)
    , m_bSpawnedSonicMain(false)
    , m_bSpawnedEyeHand(false)
    , m_FrontOpeningZ(_Origin.m_FrontOpeningZ)
    , m_EyeHandLocalZ(_Origin.m_EyeHandLocalZ)
    , m_SonicMainZ(_Origin.m_SonicMainZ)
    , m_Overlay20Z(_Origin.m_Overlay20Z)
    , m_StartButtonLocalZ(_Origin.m_StartButtonLocalZ)
    , m_TransitionAccTime(0.f)
    , m_PostTransitionAccTime(0.f)
    , m_bTransitionEnd(false)
    , m_pSonicMain(nullptr)
    , m_pOverlay20(nullptr)
    , m_pStartButton(nullptr)
    , m_pStartButtonOnSprite(nullptr)
    , m_pStartButtonOffSprite(nullptr)
    , m_bStartLevelRequested(false)
{
    RegisterScriptParams();
}

COpeningScript::~COpeningScript()
{
}

void COpeningScript::Begin()
{
    const int openingLayerIdx = (GetOwner() != nullptr) ? GetOwner()->GetLayerIdx() : 3;

    m_bSpawnedSonicMain = false;
    m_bSpawnedEyeHand = false;
    m_TransitionAccTime = 0.f;
    m_PostTransitionAccTime = 0.f;
    m_bTransitionEnd = false;
    m_pSonicMain = nullptr;
    m_pOverlay20 = nullptr;
    m_pStartButton = nullptr;
    m_pStartButtonOnSprite = nullptr;
    m_pStartButtonOffSprite = nullptr;
    m_bStartLevelRequested = false;

    if (GetOwner() != nullptr && GetOwner()->Transform() != nullptr)
    {
        Vec3 ownerPos = GetOwner()->Transform()->GetRelativePos();
        ownerPos.z = kOpeningFrontOpeningDepth;
        GetOwner()->Transform()->SetRelativePos(ownerPos);
    }

    m_pSonicMain = LevelMgr::GetInst()->FindObjectByName(L"Opening_Sonic_main");
    if (nullptr != m_pSonicMain)
    {
        ApplyOpeningWorldZ(m_pSonicMain, kOpeningSonicMainWorldDepth);
        m_bSpawnedSonicMain = true;
    }

    m_pOverlay20 = LevelMgr::GetInst()->FindObjectByName(L"Opening_Overlay_20");
    if (nullptr != m_pOverlay20)
    {
        ApplyOpeningWorldZ(m_pOverlay20, kOpeningOverlayWorldDepth);
    }

    if (nullptr != m_pSonicMain)
    {
        EnsureOpeningEyeHandChildren(m_pSonicMain.Get(), openingLayerIdx);
        m_bSpawnedEyeHand = true;
    }

    SetGameBGMPlaylist(GAME_BGM_PLAYLIST::OPENING);
}

void COpeningScript::Tick()
{
    const float frontOpeningZ = kOpeningFrontOpeningDepth;
    const float sonicMainZ = kOpeningSonicMainWorldDepth;
    const float overlay20Z = kOpeningOverlayWorldDepth;

    if (GetOwner() != nullptr && GetOwner()->Transform() != nullptr)
    {
        Vec3 ownerPos = GetOwner()->Transform()->GetRelativePos();
        if (fabsf(ownerPos.z - frontOpeningZ) > 0.001f)
        {
            ownerPos.z = frontOpeningZ;
            GetOwner()->Transform()->SetRelativePos(ownerPos);
        }
    }

    if (m_pOverlay20 != nullptr && !m_bTransitionEnd)
    {
        m_TransitionAccTime += DT;

        float duration = 1.5f;
        float ratio = m_TransitionAccTime / duration;

        if (ratio >= 1.f)
        {
            ratio = 1.f;
            m_bTransitionEnd = true;
        }

        const Vec2 displaySize = Device::GetInst()->GetRenderResolution();
        const float startY = -displaySize.y;
        const float targetY = kOpeningOverlayTargetY;
        const float jumpHeight = 300.f;
        const float smoothRatio = 1.f - powf(1.f - ratio, 3.f);
        const float jumpOffset = sinf(ratio * 3.141592f) * jumpHeight;
        const float currentY = startY + (targetY - startY) * smoothRatio + jumpOffset;

        m_pOverlay20->Transform()->SetRelativePos(Vec3(0.f, currentY, overlay20Z));
    }

    if (m_bTransitionEnd)
        m_PostTransitionAccTime += DT;
    else
        m_PostTransitionAccTime = 0.f;

    if (nullptr != m_pSonicMain && !m_bSpawnedEyeHand)
    {
        EnsureOpeningEyeHandChildren(m_pSonicMain.Get(), (GetOwner() != nullptr) ? GetOwner()->GetLayerIdx() : 3);
        m_bSpawnedEyeHand = true;
    }

    if (nullptr == m_pStartButton
        && nullptr != m_pOverlay20
        && m_bTransitionEnd
        && m_PostTransitionAccTime >= kOpeningStartButtonRevealDelay)
    {
        Ptr<ASprite> pOverlaySprite = m_pOverlay20->SpriteRender()->GetSprite();
        Vec2 overlaySourceSize = GetSpritePixelSize(pOverlaySprite);
        const Vec3 overlayScale = m_pOverlay20->Transform()->GetRelativeScale();
        Vec2 overlayDisplaySize = Vec2(overlayScale.x, overlayScale.y);

        m_pStartButtonOffSprite = CreateFullTextureSprite(L"Opening_StartButton_Off"
            , L"OpeningStartButtonOffTexture"
            , L"Texture\\Mouseoff.png");
        m_pStartButtonOnSprite = CreateFullTextureSprite(L"Opening_StartButton_On"
            , L"OpeningStartButtonOnTexture"
            , L"Texture\\Mouseon.png");

        m_pStartButton = CreateOpeningStartButtonChild(m_pOverlay20.Get()
            , overlaySourceSize
            , overlayDisplaySize
            , m_pStartButtonOffSprite
            , m_pStartButtonOnSprite
            , kOpeningStartButtonLocalDepth);
    }

    if (nullptr != m_pStartButton)
    {
        const bool bCanInteract = m_bTransitionEnd && !m_bStartLevelRequested;
        const bool bHovered = bCanInteract && IsMouseInsideButton(m_pStartButton);
        Ptr<ASprite> pTargetSprite = bHovered ? m_pStartButtonOnSprite : m_pStartButtonOffSprite;

        if (nullptr == pTargetSprite)
            pTargetSprite = m_pStartButtonOnSprite;

        if (nullptr != pTargetSprite)
            m_pStartButton->SpriteRender()->SetSprite(pTargetSprite);

        if (bHovered && KEY_TAP(KEY::LBTN))
        {
            m_bStartLevelRequested = true;
            StartSavedLevelFromOpening();
            return;
        }
    }

    if (m_bSpawnedSonicMain)
        return;

    if (nullptr == GetOwner() || nullptr == GetOwner()->FlipbookRender())
        return;

    if (!GetOwner()->FlipbookRender()->IsFinish())
        return;

    Ptr<GameObject> pExistingSonicMain = LevelMgr::GetInst()->FindObjectByName(L"Opening_Sonic_main");
    if (nullptr != pExistingSonicMain)
    {
        ApplyOpeningWorldZ(pExistingSonicMain, sonicMainZ);

        Ptr<GameObject> pExistingOverlay20 = LevelMgr::GetInst()->FindObjectByName(L"Opening_Overlay_20");
        if (nullptr != pExistingOverlay20)
        {
            ApplyOpeningWorldZ(pExistingOverlay20, overlay20Z);
            m_pOverlay20 = pExistingOverlay20;
        }

        m_pSonicMain = pExistingSonicMain;
        EnsureOpeningEyeHandChildren(m_pSonicMain.Get(), (GetOwner() != nullptr) ? GetOwner()->GetLayerIdx() : 3);
        m_bSpawnedEyeHand = true;
        m_bSpawnedSonicMain = true;
        return;
    }

    Ptr<ASprite> pSprite = LOAD(ASprite, L"Sprite\\Opening_Sonic_main.sprite");
    if (nullptr == pSprite)
        return;

    FixTinyOpeningSpriteUV(pSprite);
    const Vec2 displaySize = Device::GetInst()->GetRenderResolution();

    GameObject* pSonicMain = new GameObject;
    pSonicMain->SetName(L"Opening_Sonic_main");
    pSonicMain->AddComponent(new CTransform);
    pSonicMain->AddComponent(new CSpriteRender);

    pSonicMain->Transform()->SetRelativePos(Vec3(0.f, 0.f, sonicMainZ));
    pSonicMain->Transform()->SetRelativeScale(Vec3(displaySize.x, displaySize.y, 1.f));
    pSonicMain->SpriteRender()->SetSprite(pSprite);
    ApplyOpeningChromaKey(pSonicMain->SpriteRender());

    Ptr<ASprite> pSprite20 = LOAD(ASprite, L"Sprite\\Opening_20.sprite");
    const int openingLayerIdx = (GetOwner() != nullptr) ? GetOwner()->GetLayerIdx() : 3;
    if (nullptr != pSprite20)
    {
        FixTinyOpeningSpriteUV(pSprite20);
        const Vec2 overlayDisplaySize = Vec2(displaySize.x * 0.8f, displaySize.y * 0.8f);

        GameObject* pOverlay20 = new GameObject;
        m_pOverlay20 = pOverlay20;
        pOverlay20->SetName(L"Opening_Overlay_20");
        pOverlay20->AddComponent(new CTransform);
        pOverlay20->AddComponent(new CSpriteRender);

        pOverlay20->Transform()->SetRelativePos(Vec3(0.f, -displaySize.y, overlay20Z));
        pOverlay20->Transform()->SetRelativeScale(Vec3(overlayDisplaySize.x, overlayDisplaySize.y, 1.f));
        pOverlay20->Transform()->SetIndependentScale(true);
        pOverlay20->SpriteRender()->SetSprite(pSprite20);
        ApplyOpeningChromaKey(pOverlay20->SpriteRender());
        CreateObject(pOverlay20, openingLayerIdx);
    }

    // Opening pieces share the owner's layer so z values decide their stack cleanly.
    CreateObject(pSonicMain, openingLayerIdx);
    m_pSonicMain = pSonicMain;
    EnsureOpeningEyeHandChildren(m_pSonicMain.Get(), openingLayerIdx);
    m_bSpawnedEyeHand = true;
    m_bSpawnedSonicMain = true;
}




