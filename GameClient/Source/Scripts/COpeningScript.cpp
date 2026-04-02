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
#include <TimeMgr.h>

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
    constexpr float kOpeningOverlayDepth = -0.05f;
    constexpr float kOpeningOverlayFPS = 8.f;
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

    Vec3 GetOverlayLocalPos(const PixelRect& _Rect, const Vec2& _SourceSize, const Vec2& _DisplaySize)
    {
        const float centerX = _Rect.left + (_Rect.width * 0.5f);
        const float centerY = _Rect.top + (_Rect.height * 0.5f);
        const float scaleX = (0.f < _SourceSize.x) ? (_DisplaySize.x / _SourceSize.x) : 1.f;
        const float scaleY = (0.f < _SourceSize.y) ? (_DisplaySize.y / _SourceSize.y) : 1.f;

        return Vec3((centerX - (_SourceSize.x * 0.5f)) * scaleX
            , ((_SourceSize.y * 0.5f) - centerY) * scaleY
            , kOpeningOverlayDepth);
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

    void AttachOverlayFlipbookChild(GameObject* _Parent
        , const wchar_t* _Name
        , Ptr<AFlipbook> _Flipbook
        , const Vec3& _LocalPos
        , const Vec3& _LocalScale
        , float _FPS)
    {
        if (nullptr == _Parent || nullptr == _Flipbook)
            return;

        Ptr<GameObject> pChild = new GameObject;
        pChild->SetName(_Name);
        pChild->AddComponent(new CTransform);
        pChild->AddComponent(new CFlipbookRender);

        pChild->Transform()->SetRelativePos(_LocalPos);
        pChild->Transform()->SetRelativeScale(_LocalScale);
        pChild->Transform()->SetIndependentScale(true);

        pChild->FlipbookRender()->AddFlipbook(_Flipbook);
        pChild->FlipbookRender()->Play(0, _FPS, -1);
        ApplyOpeningChromaKey(pChild->FlipbookRender());

        _Parent->AddChild(pChild);
    }
}

COpeningScript::COpeningScript()
    : CScript(SCRIPT_TYPE::OPENINGSCRIPT)
    , m_bSpawnedSonicMain(false)
    , m_TransitionAccTime(0.f)
    , m_bTransitionEnd(false)
    , m_pOverlay20(nullptr)
{
}

COpeningScript::~COpeningScript()
{
}

void COpeningScript::Begin()
{
    m_bSpawnedSonicMain = false;
    m_TransitionAccTime = 0.f;
    m_bTransitionEnd = false;
    m_pOverlay20 = nullptr;
}

void COpeningScript::Tick()
{
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
        const float targetY = -55.f;
        const float jumpHeight = 300.f;
        const float smoothRatio = 1.f - powf(1.f - ratio, 3.f);
        const float jumpOffset = sinf(ratio * 3.141592f) * jumpHeight;
        const float currentY = startY + (targetY - startY) * smoothRatio + jumpOffset;

        m_pOverlay20->Transform()->SetRelativePos(Vec3(0.f, currentY, -0.2f));
    }

    if (m_bSpawnedSonicMain)
        return;

    if (nullptr == GetOwner() || nullptr == GetOwner()->FlipbookRender())
        return;

    if (!GetOwner()->FlipbookRender()->IsFinish())
        return;

    if (nullptr != LevelMgr::GetInst()->FindObjectByName(L"Opening_Sonic_main"))
    {
        m_bSpawnedSonicMain = true;
        return;
    }

    Ptr<ASprite> pSprite = LOAD(ASprite, L"Sprite\\Opening_Sonic_main.sprite");
    if (nullptr == pSprite)
        return;

    FixTinyOpeningSpriteUV(pSprite);
    const Vec2 spriteSize = GetSpritePixelSize(pSprite);
    const Vec2 mainSourceSize = Vec2(
        (spriteSize.x > 1.f) ? spriteSize.x : kOpeningMainSourceSize.x,
        (spriteSize.y > 1.f) ? spriteSize.y : kOpeningMainSourceSize.y);
    const Vec2 displaySize = Device::GetInst()->GetRenderResolution();

    GameObject* pSonicMain = new GameObject;
    pSonicMain->SetName(L"Opening_Sonic_main");
    pSonicMain->AddComponent(new CTransform);
    pSonicMain->AddComponent(new CSpriteRender);

    pSonicMain->Transform()->SetRelativePos(Vec3(0.f, 0.f, 1.1f));
    pSonicMain->Transform()->SetRelativeScale(Vec3(displaySize.x, displaySize.y, 1.f));
    pSonicMain->SpriteRender()->SetSprite(pSprite);
    ApplyOpeningChromaKey(pSonicMain->SpriteRender());

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
    AttachOverlayFlipbookChild(pSonicMain
        , L"Opening_Sonic_Eye"
        , pEyeFlipbook
        , GetOverlayLocalPos(kOpeningEyeLocalRect, mainSourceSize, displaySize)
        , GetOverlayLocalScale(kOpeningEyeLocalRect, mainSourceSize, displaySize)
        , kOpeningOverlayFPS);

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
    AttachOverlayFlipbookChild(pSonicMain
        , L"Opening_Sonic_Hand"
        , pHandFlipbook
        , GetOverlayLocalPos(kOpeningHandLocalRect, mainSourceSize, displaySize)
        , GetOverlayLocalScale(kOpeningHandLocalRect, mainSourceSize, displaySize)
        , kOpeningOverlayFPS);

    // --- [추가] 20번째 스프라이트 최상단에 띄우기 ---
    Ptr<ASprite> pSprite20 = LOAD(ASprite, L"Sprite\\Opening_20.sprite"); // 실제 파일명으로 변경하세요
    if (nullptr != pSprite20)
    {
        FixTinyOpeningSpriteUV(pSprite20);

        Ptr<GameObject> pOverlay20 = new GameObject;
        m_pOverlay20 = pOverlay20;
        pOverlay20->SetName(L"Opening_Overlay_20");
        pOverlay20->AddComponent(new CTransform);
        pOverlay20->AddComponent(new CSpriteRender);

        // 1. 위치 설정: 눈/손이 -0.05f 이므로, -0.2f 정도로 설정하여 제일 윗단(카메라 쪽)으로 뺍니다.
        // X, Y 좌표를 조절하여 원하는 위치에 배치하세요.
        pOverlay20->Transform()->SetRelativePos(Vec3(0.f, -displaySize.y, -0.2f));
        pOverlay20->Transform()->SetRelativeScale(Vec3(displaySize.x * 0.8f, displaySize.y * 0.8f, 1.f));
        pOverlay20->Transform()->SetIndependentScale(true);

        // 3. 스프라이트 적용
        pOverlay20->SpriteRender()->SetSprite(pSprite20);

        // 4. [핵심] 만들어두신 분홍색 투명화(크로마키) 함수 적용
        ApplyOpeningChromaKey(pOverlay20->SpriteRender());

        // 5. pSonicMain의 자식으로 붙여서 함께 관리 (Layer 3에 같이 올라감)
        pSonicMain->AddChild(pOverlay20.Get());
    }
    CreateObject(pSonicMain, 3);
    m_bSpawnedSonicMain = true;
}
