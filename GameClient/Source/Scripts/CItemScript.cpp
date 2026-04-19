#include "pch.h"
#include "CItemScript.h"

#include "APrefab.h"
#include "ASprite.h"
#include "AssetMgr.h"
#include "Source/Scripts/CBlockScript.h"
#include "CCollider2D.h"
#include "CSpriteRender.h"
#include "CTransform.h"
#include "GameObject.h"
#include "LevelMgr.h"
#include "Source/Scripts/CBackScript.h"
#include "Source/Scripts/CCoinMgrScript.h"
#include "Source/Scripts/CPlayerScript.h"

namespace
{
    constexpr const wchar_t* kItemBoxBaseSpritePath = L"Sprite\\ItemBox_Base.sprite";
    constexpr const wchar_t* kItemBoxLidSpritePath = L"Sprite\\ItemBox_Lid.sprite";
    constexpr const wchar_t* kItemBoxDeadSpritePath = L"Sprite\\ItemBox_Dead.sprite";
    constexpr const wchar_t* kItemBoxLidChildName = L"ItemBox_Visual_Lid";
    constexpr const wchar_t* kItemBoxIconChildName = L"ItemBox_Visual_Icon";
    const Vec3 kItemBoxDefaultScale = Vec3(58.f, 76.f, 1.f);
    const Vec3 kItemBoxLidLocalPos = Vec3(0.f, 24.f, -0.1f);
    const Vec3 kItemBoxLidLocalScale = Vec3(58.f, 28.f, 1.f);
    const Vec3 kItemBoxIconLocalPos = Vec3(0.f, 8.f, -0.05f);
    const Vec3 kItemBoxIconLocalScale = Vec3(32.f, 32.f, 1.f);

    struct ItemBoxVisualInfo
    {
        const wchar_t* ObjectNamePrefix;
        const wchar_t* IconSpritePath;
        const wchar_t* PrefabPath;
        const wchar_t* FallbackPrefabPath;
    };

    bool TryGetItemBoxVisualInfo(CItemScript::ITEMBOX _Type, ItemBoxVisualInfo& _OutInfo)
    {
        switch (_Type)
        {
        case CItemScript::ITEMBOX::UPLIFEBOX:
            _OutInfo = { L"ItemBox_Life_", L"Sprite\\ItemBox_0.sprite", L"Prefab\\ITEMBOX_LIFE.pref", nullptr };
            return true;
        case CItemScript::ITEMBOX::ELECTIRCBOX:
            _OutInfo = { L"ItemBox_Electric_", L"Sprite\\ItemBox_1.sprite", L"Prefab\\ITEMBOX_ELECTRIC.pref", nullptr };
            return true;
        case CItemScript::ITEMBOX::FIREBOX:
            _OutInfo = { L"ItemBox_Fire_", L"Sprite\\ItemBox_2.sprite", L"Prefab\\ITEMBOX_FIRE.pref", L"Prefab\\ITEMBOX_FRIE.pref" };
            return true;
        case CItemScript::ITEMBOX::WATERBOX:
            _OutInfo = { L"ItemBox_Water_", L"Sprite\\ItemBox_3.sprite", L"Prefab\\ITEMBOX_WATER.pref", nullptr };
            return true;
        case CItemScript::ITEMBOX::STARBOX:
            _OutInfo = { L"ItemBox_Star_", L"Sprite\\ItemBox_4.sprite", L"Prefab\\ITEMBOX_STAR.pref", nullptr };
            return true;
        case CItemScript::ITEMBOX::COINBOX:
            _OutInfo = { L"ItemBox_Coin_", L"Sprite\\ItemBox_5.sprite", L"Prefab\\ITEMBOX_COIN.pref", nullptr };
            return true;
        default:
            break;
        }

        return false;
    }

    bool ShouldAutoRenameItemBox(const wstring& _Name)
    {
        if (_Name.empty())
            return true;

        return (0 == _Name.rfind(L"Empty_", 0))
            || (0 == _Name.rfind(L"ItemBox_", 0))
            || (0 == _Name.rfind(L"ITEMBOX_", 0));
    }

    Ptr<GameObject> FindDirectChildByName(GameObject* _Owner, const wchar_t* _Name)
    {
        if (_Owner == nullptr || _Name == nullptr)
            return nullptr;

        const vector<Ptr<GameObject>>& vecChild = _Owner->GetChild();
        for (size_t i = 0; i < vecChild.size(); ++i)
        {
            if (vecChild[i] != nullptr && vecChild[i]->GetName() == _Name)
                return vecChild[i];
        }

        return nullptr;
    }

    void DestroyDirectChildByName(GameObject* _Owner, const wchar_t* _Name)
    {
        Ptr<GameObject> pChild = FindDirectChildByName(_Owner, _Name);
        if (nullptr != pChild)
            pChild->Destroy();
    }

    Ptr<GameObject> EnsureSpriteChild(GameObject* _Owner, const wchar_t* _Name)
    {
        Ptr<GameObject> pChild = FindDirectChildByName(_Owner, _Name);
        if (nullptr == pChild)
        {
            pChild = new GameObject;
            pChild->SetName(_Name);
            pChild->AddComponent(new CTransform);
            pChild->AddComponent(new CSpriteRender);
            _Owner->AddChild(pChild);
        }
        else
        {
            if (nullptr == pChild->Transform())
                pChild->AddComponent(new CTransform);

            if (nullptr == pChild->SpriteRender())
                pChild->AddComponent(new CSpriteRender);
        }

        return pChild;
    }

    Ptr<ASprite> LoadSpriteWithFallback(const wchar_t* _PrimaryPath, const wchar_t* _FallbackPath = nullptr)
    {
        Ptr<ASprite> pSprite = (_PrimaryPath != nullptr) ? LOAD(ASprite, _PrimaryPath) : nullptr;
        if (nullptr == pSprite && nullptr != _FallbackPath)
            pSprite = LOAD(ASprite, _FallbackPath);
        return pSprite;
    }

    bool ShouldResetItemBoxScaleToDefault(const Vec3& _Scale)
    {
        const float absX = fabsf(_Scale.x);
        const float absY = fabsf(_Scale.y);

        const bool bUnconfiguredScale = (absX <= 5.f || absY <= 5.f);
        const bool bLegacyOversizedScale =
            fabsf(absX - 100.f) <= 5.f &&
            fabsf(absY - 120.f) <= 5.f;

        return bUnconfiguredScale || bLegacyOversizedScale;
    }

}

CItemScript::CItemScript()
    : CScript(SCRIPT_TYPE::ITEMSCRIPT)
    , m_Curstate(ITEMBOX::NONE)
{
}

CItemScript::~CItemScript()
{
}

void CItemScript::Begin()
{
    if (nullptr != Collider2D())
        ADD_DYNAMIC_BEGIN_OVERLAP(CItemScript::BeginOverlap);

    if (m_Curstate != ITEMBOX::NONE)
        ApplyEditorBoxSetup();
}

void CItemScript::Tick()
{
}

void CItemScript::SaveToLevelFile(FILE* _File)
{
    int boxType = (int)m_Curstate;
    fwrite(&boxType, sizeof(int), 1, _File);
}

void CItemScript::LoadFromLevelFile(FILE* _File)
{
    int boxType = 0;
    fread(&boxType, sizeof(int), 1, _File);

    if (boxType < (int)ITEMBOX::NONE || boxType > (int)ITEMBOX::COINBOX)
        boxType = (int)ITEMBOX::COINBOX;

    m_Curstate = (ITEMBOX)boxType;
    ApplyEditorBoxSetup();
}

void CItemScript::SetBoxType(ITEMBOX _Type)
{
    if (_Type < ITEMBOX::NONE || _Type > ITEMBOX::COINBOX)
        _Type = ITEMBOX::COINBOX;

    m_Curstate = _Type;

    if (nullptr != GetOwner())
        ApplyEditorBoxSetup();
}

bool CItemScript::ApplyEditorBoxSetup()
{
    GameObject* pOwner = GetOwner();
    if (nullptr == pOwner)
        return false;

    if (nullptr == pOwner->Transform())
        pOwner->AddComponent(new CTransform);

    if (nullptr != pOwner->GetRenderCom() && nullptr == pOwner->SpriteRender())
        return false;

    if (nullptr == pOwner->SpriteRender())
        pOwner->AddComponent(new CSpriteRender);

    if (nullptr == pOwner->Collider2D())
        pOwner->AddComponent(new CCollider2D);

    if (nullptr == pOwner->GetScript<CBlockScript>())
        pOwner->AddComponent(new CBlockScript);

    if (nullptr == pOwner->GetScript<CBackScript>())
        pOwner->AddComponent(new CBackScript);

    if (m_Curstate == ITEMBOX::NONE)
    {
        if (nullptr != pOwner->SpriteRender())
            pOwner->SpriteRender()->SetSprite(nullptr);

        DestroyDirectChildByName(pOwner, kItemBoxLidChildName);
        DestroyDirectChildByName(pOwner, kItemBoxIconChildName);
        pOwner->RegisterAsParent();
        return false;
    }

    ItemBoxVisualInfo visualInfo = {};
    if (!TryGetItemBoxVisualInfo(m_Curstate, visualInfo))
        return false;

    Vec3 ownerScale = pOwner->Transform()->GetRelativeScale();
    if (ShouldResetItemBoxScaleToDefault(ownerScale))
        pOwner->Transform()->SetRelativeScale(kItemBoxDefaultScale);

    Ptr<ASprite> pBaseSprite = LoadSpriteWithFallback(kItemBoxBaseSpritePath, kItemBoxDeadSpritePath);
    if (nullptr != pBaseSprite)
        pOwner->SpriteRender()->SetSprite(pBaseSprite);

    if (nullptr != pOwner->Collider2D())
    {
        pOwner->Collider2D()->SetOffset(Vec2(0.f, -0.05f));
        pOwner->Collider2D()->SetScale(Vec2(0.8f, 1.0f));
    }

    Ptr<GameObject> pLid = EnsureSpriteChild(pOwner, kItemBoxLidChildName);
    if (nullptr != pLid && nullptr != pLid->Transform())
    {
        pLid->Transform()->SetRelativePos(kItemBoxLidLocalPos);
        pLid->Transform()->SetRelativeScale(kItemBoxLidLocalScale);
        if (nullptr != pLid->SpriteRender())
            pLid->SpriteRender()->SetSprite(LOAD(ASprite, kItemBoxLidSpritePath));
    }

    Ptr<GameObject> pIcon = EnsureSpriteChild(pOwner, kItemBoxIconChildName);
    if (nullptr != pIcon && nullptr != pIcon->Transform())
    {
        pIcon->Transform()->SetRelativePos(kItemBoxIconLocalPos);
        pIcon->Transform()->SetRelativeScale(kItemBoxIconLocalScale);
        if (nullptr != pIcon->SpriteRender())
            pIcon->SpriteRender()->SetSprite(LoadSpriteWithFallback(visualInfo.IconSpritePath, L"Sprite\\ItemBox_5.sprite"));
    }

    if (ShouldAutoRenameItemBox(pOwner->GetName()))
        pOwner->SetName(wstring(visualInfo.ObjectNamePrefix) + std::to_wstring(pOwner->GetID()));

    pOwner->RegisterAsParent();
    return true;
}

bool CItemScript::CanPlayerBreakItemBox(const CPlayerScript* _PlayerScript) const
{
    if (_PlayerScript == nullptr)
        return false;

    if (_PlayerScript->GetAction() == ActionState::SkillDash)
        return true;

    return _PlayerScript->GetIsJump();
}

void CItemScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (_OwnCollider == nullptr || _OtherCollider == nullptr || GetOwner() == nullptr || GetOwner()->IsDead())
        return;

    GameObject* pPlayerObj = _OtherCollider->GetOwner();
    if (nullptr == pPlayerObj || pPlayerObj->GetName() != L"Player")
        return;

    Ptr<CPlayerScript> pPlayerScript = pPlayerObj->GetScript<CPlayerScript>();
    if (nullptr == pPlayerScript || !CanPlayerBreakItemBox(pPlayerScript.Get()))
        return;

    Ptr<CBackScript> pBackScript = GetOwner()->GetScript<CBackScript>();
    if (nullptr != pBackScript)
        pBackScript->ExecuteItemBounce(pPlayerObj);
    else
        pPlayerScript->SetItemBoxBounceState();

    ApplyItemEffect(pPlayerScript.Get());
    Dead();
}

void CItemScript::ApplyItemEffect(CPlayerScript* _PlayerScript)
{
    if (nullptr == _PlayerScript)
        return;

    switch (m_Curstate)
    {
    case ITEMBOX::COINBOX:
        CCoinMgrScript::AddCoin(10);
        break;
    case ITEMBOX::FIREBOX:
        _PlayerScript->SetItemState(CPlayerScript::ITEM_STATE::FIRE);
        break;
    case ITEMBOX::WATERBOX:
        _PlayerScript->SetItemState(CPlayerScript::ITEM_STATE::WATER);
        break;
    case ITEMBOX::ELECTIRCBOX:
        _PlayerScript->SetItemState(CPlayerScript::ITEM_STATE::ELECTRIC);
        break;
    case ITEMBOX::STARBOX:
        _PlayerScript->SetItemState(CPlayerScript::ITEM_STATE::STAR);
        break;
    case ITEMBOX::UPLIFEBOX:
        CPlayerScript::AddLife();
        break;
    default:
        break;
    }
}

void CItemScript::CreateItemBox(ITEMBOX _Type)
{
    SetBoxType(_Type);

    ItemBoxVisualInfo visualInfo = {};
    if (!TryGetItemBoxVisualInfo(_Type, visualInfo))
        return;

    Ptr<APrefab> pPrefab = LOAD(APrefab, visualInfo.PrefabPath);
    if (nullptr == pPrefab && nullptr != visualInfo.FallbackPrefabPath)
        pPrefab = LOAD(APrefab, visualInfo.FallbackPrefabPath);

    if (nullptr == pPrefab || nullptr == Transform())
        return;

    Instantiate(pPrefab.Get(), 5, Transform()->GetWorldPos());
}

void CItemScript::Dead()
{
    if (GetOwner() == nullptr || GetOwner()->IsDead())
        return;

    GameObject* pDeadBox = new GameObject;
    pDeadBox->SetName(wstring(L"ItemBox_Dead_") + std::to_wstring(GetOwner()->GetID()));
    pDeadBox->AddComponent(new CTransform);
    pDeadBox->AddComponent(new CSpriteRender);

    if (GetOwner()->Transform() != nullptr)
    {
        pDeadBox->Transform()->SetRelativePos(GetOwner()->Transform()->GetWorldPos());
        pDeadBox->Transform()->SetRelativeScale(GetOwner()->Transform()->GetWorldScale());
        pDeadBox->Transform()->SetRelativeRot(GetOwner()->Transform()->GetRelativeRot());
    }
    else
    {
        pDeadBox->Transform()->SetRelativeScale(kItemBoxDefaultScale);
    }

    Ptr<ASprite> pDeadSprite = LoadSpriteWithFallback(kItemBoxDeadSpritePath, kItemBoxBaseSpritePath);
    if (nullptr != pDeadSprite)
        pDeadBox->SpriteRender()->SetSprite(pDeadSprite);

    Ptr<ALevel> pCurLevel = LevelMgr::GetInst()->GetCurLevel();
    if (nullptr != pCurLevel)
    {
        int layerIdx = GetOwner()->GetLayerIdx();
        if (layerIdx < 0)
            layerIdx = 5;

        pCurLevel->AddObject(layerIdx, pDeadBox);
    }

    GetOwner()->Destroy();
}
