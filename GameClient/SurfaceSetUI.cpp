#include "pch.h"
#include "SurfaceSetUI.h"

#include "ASurfaceSet.h"
#include "AssetMgr.h"
#include "CTransform.h"
#include "EditorMgr.h"
#include "GameObject.h"
#include "Inspector.h"
#include "LevelMgr.h"
#include "Source/Scripts/CSurfaceCircleGuideScript.h"
#include "Source/Scripts/CSurfaceScript.h"
#include "Source/Scripts/CSurfaceSetScript.h"

#include <filesystem>

namespace
{
    constexpr const wchar_t* kSurfaceRootName = L"SurfaceEditorRoot";
    constexpr int kSurfaceLayerIndex = 2;

    SURFACE_SET_GEOMETRY ToSurfaceSetGeometry(CSurfaceScript::SURFACE_GEOMETRY _Geometry)
    {
        switch (_Geometry)
        {
        case CSurfaceScript::SURFACE_GEOMETRY::ARC:
            return SURFACE_SET_GEOMETRY::ARC;
        case CSurfaceScript::SURFACE_GEOMETRY::CIRCLE:
            return SURFACE_SET_GEOMETRY::CIRCLE;
        case CSurfaceScript::SURFACE_GEOMETRY::FULL_CIRCLE:
            return SURFACE_SET_GEOMETRY::FULL_CIRCLE;
        case CSurfaceScript::SURFACE_GEOMETRY::LINE:
        default:
            return SURFACE_SET_GEOMETRY::LINE;
        }
    }

    SURFACE_SET_ROLE ToSurfaceSetRole(CSurfaceScript::SURFACE_ROLE _Role)
    {
        switch (_Role)
        {
        case CSurfaceScript::SURFACE_ROLE::CORRECTION:
            return SURFACE_SET_ROLE::CORRECTION;
        case CSurfaceScript::SURFACE_ROLE::WALL:
            return SURFACE_SET_ROLE::WALL;
        case CSurfaceScript::SURFACE_ROLE::VERTICAL_ENTRY:
            return SURFACE_SET_ROLE::VERTICAL_ENTRY;
        case CSurfaceScript::SURFACE_ROLE::VERTICAL_STICKY_ENTRY:
            return SURFACE_SET_ROLE::VERTICAL_STICKY_ENTRY;
        case CSurfaceScript::SURFACE_ROLE::SURFACE:
        default:
            return SURFACE_SET_ROLE::SURFACE;
        }
    }

    SURFACE_SET_ARC_CORNER ToSurfaceSetArcCorner(CSurfaceScript::ARC_CORNER _Corner)
    {
        switch (_Corner)
        {
        case CSurfaceScript::ARC_CORNER::TOP_LEFT:
            return SURFACE_SET_ARC_CORNER::TOP_LEFT;
        case CSurfaceScript::ARC_CORNER::TOP_RIGHT:
            return SURFACE_SET_ARC_CORNER::TOP_RIGHT;
        case CSurfaceScript::ARC_CORNER::BOTTOM_RIGHT:
            return SURFACE_SET_ARC_CORNER::BOTTOM_RIGHT;
        case CSurfaceScript::ARC_CORNER::BOTTOM_LEFT:
        default:
            return SURFACE_SET_ARC_CORNER::BOTTOM_LEFT;
        }
    }

    bool IsCircleSurfaceGeometry(SURFACE_SET_GEOMETRY _Geometry)
    {
        return _Geometry == SURFACE_SET_GEOMETRY::CIRCLE ||
            _Geometry == SURFACE_SET_GEOMETRY::FULL_CIRCLE;
    }

    std::wstring BuildSurfaceSetActorName(ASurfaceSet* _SurfaceSet)
    {
        if (_SurfaceSet == nullptr)
            return L"SurfaceSetActor";

        std::wstring assetLabel = _SurfaceSet->GetKey();
        if (assetLabel.empty())
            assetLabel = L"SurfaceSet";

        std::filesystem::path assetPath(assetLabel);
        const std::wstring stem = assetPath.stem().wstring();
        return stem.empty() ? L"SurfaceSetActor" : (stem + L"_Actor");
    }

    void SetSurfaceSetStatusText(std::string* _OutStatusText, const std::string& _StatusText)
    {
        if (_OutStatusText != nullptr)
            *_OutStatusText = _StatusText;
    }

    std::wstring SanitizeSurfaceSetStem(const std::wstring& _RawStem)
    {
        std::wstring stem = std::filesystem::path(_RawStem).stem().wstring();
        if (stem.empty())
            stem = _RawStem;

        if (stem.empty())
            stem = L"CurrentLevel";

        for (wchar_t& ch : stem)
        {
            if (nullptr != wcschr(L"<>:\"/\\|?*", ch))
                ch = L'_';
            else if (ch == L'\r' || ch == L'\n' || ch == L'\t')
                ch = L' ';
        }

        while (!stem.empty() && (stem.back() == L' ' || stem.back() == L'.'))
            stem.pop_back();

        if (stem.empty())
            stem = L"CurrentLevel";

        return stem;
    }

    std::wstring BuildSurfaceSetStemFromLevelAsset(ALevel* _Level)
    {
        if (_Level == nullptr)
            return L"CurrentLevel";

        std::wstring source = _Level->GetRelativePath();
        if (source.empty())
            source = _Level->GetKey();
        if (source.empty())
            source = _Level->GetName();

        return SanitizeSurfaceSetStem(source);
    }

    std::wstring BuildUniqueSurfaceSetKey(const std::wstring& _Stem)
    {
        const std::wstring stem = _Stem.empty() ? L"CurrentLevel" : _Stem;
        const std::wstring prefix = L"SurfaceSet\\";
        const std::wstring baseName = stem + L"_SurfaceSet";
        const std::wstring ext = L".sset";

        std::wstring key = prefix + baseName + ext;
        if (AssetMgr::GetInst()->FindAsset(ASSET_TYPE::SURFACESET, key) == nullptr)
            return key;

        for (int i = 1; ; ++i)
        {
            key = prefix + baseName + L"_" + std::to_wstring(i) + ext;
            if (AssetMgr::GetInst()->FindAsset(ASSET_TYPE::SURFACESET, key) == nullptr)
                return key;
        }
    }

    Ptr<ASurfaceSet> EnsureCurrentLevelSurfaceSetAssetImpl(std::string* _OutStatusText)
    {
        Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();
        if (pLevel == nullptr)
        {
            SetSurfaceSetStatusText(_OutStatusText, "No level is loaded.");
            return nullptr;
        }

        const std::wstring stem = BuildSurfaceSetStemFromLevelAsset(pLevel.Get());
        const std::wstring baseKey = L"SurfaceSet\\" + stem + L"_SurfaceSet.sset";

        Ptr<ASurfaceSet> pExistingSurfaceSet = AssetMgr::GetInst()->Find<ASurfaceSet>(baseKey);
        if (pExistingSurfaceSet != nullptr)
        {
            SetSurfaceSetStatusText(_OutStatusText, "Reusing the current level SurfaceSet asset.");
            return pExistingSurfaceSet;
        }

        CreateDirectoryW((std::wstring(CONTENT_PATH) + L"SurfaceSet").c_str(), nullptr);

        const std::wstring key = BuildUniqueSurfaceSetKey(stem);
        Ptr<ASurfaceSet> pSurfaceSet = new ASurfaceSet;
        AssetMgr::GetInst()->AddAsset(key, pSurfaceSet.Get());

        if (FAILED(pSurfaceSet->Save(std::wstring(CONTENT_PATH) + key)))
        {
            SetSurfaceSetStatusText(_OutStatusText, "Failed to create a SurfaceSet asset for the current level.");
            return nullptr;
        }

        pSurfaceSet = AssetMgr::GetInst()->Load<ASurfaceSet>(key, key);
        if (pSurfaceSet == nullptr)
        {
            SetSurfaceSetStatusText(_OutStatusText, "The SurfaceSet asset was saved, but could not be reloaded.");
            return nullptr;
        }

        SetSurfaceSetStatusText(_OutStatusText, "Created a SurfaceSet asset for the current level.");
        return pSurfaceSet;
    }

    void CollectSceneSurfaceObjects(vector<Ptr<GameObject>>& _OutObjects)
    {
        _OutObjects.clear();

        Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();
        if (pLevel == nullptr)
            return;

        vector<Ptr<GameObject>> stack;
        for (int layerIdx = 0; layerIdx < MAX_LAYER; ++layerIdx)
        {
            const vector<Ptr<GameObject>>& vecParents = pLevel->GetLayer(layerIdx)->GetParentObjects();
            stack.insert(stack.end(), vecParents.begin(), vecParents.end());
        }

        while (!stack.empty())
        {
            Ptr<GameObject> pObject = stack.back();
            stack.pop_back();

            if (pObject == nullptr || pObject->IsDead())
                continue;

            if (pObject->GetScript<CSurfaceScript>() != nullptr)
                _OutObjects.push_back(pObject);

            const vector<Ptr<GameObject>>& vecChild = pObject->GetChild();
            stack.insert(stack.end(), vecChild.begin(), vecChild.end());
        }
    }

    bool BakeSceneSurfacesImpl(ASurfaceSet* _SurfaceSet, UINT& _OutCount, std::string* _OutStatusText)
    {
        _OutCount = 0;

        if (_SurfaceSet == nullptr)
        {
            SetSurfaceSetStatusText(_OutStatusText, "No SurfaceSet asset is selected.");
            return false;
        }

        Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();
        if (pLevel == nullptr)
        {
            SetSurfaceSetStatusText(_OutStatusText, "No level is loaded.");
            return false;
        }

        vector<Ptr<GameObject>> vecChild;
        CollectSceneSurfaceObjects(vecChild);
        _SurfaceSet->Clear();
        _SurfaceSet->Reserve((UINT)vecChild.size());

        for (const Ptr<GameObject>& pChild : vecChild)
        {
            if (pChild == nullptr || pChild->IsDead())
                continue;

            Ptr<CSurfaceScript> pSurface = pChild->GetScript<CSurfaceScript>();
            if (pSurface == nullptr)
                continue;

            ASurfaceSet::SURFACE_DESC desc = {};
            desc.Name = pChild->GetName();
            desc.Geometry = ToSurfaceSetGeometry(pSurface->GetGeometry());
            desc.Role = ToSurfaceSetRole(pSurface->GetRole());
            desc.ArcCorner = ToSurfaceSetArcCorner(pSurface->GetArcCorner());
            desc.FillAbove = pSurface->GetFillAbove();
            desc.FillInside = pSurface->GetFillInside();
            desc.Attachable = (desc.Role != SURFACE_SET_ROLE::WALL) && pSurface->IsAttachable();

            Vec3 worldPos = pChild->Transform()->GetWorldPos();
            desc.LocalPosition = worldPos;

            Vec2 worldStart = {};
            Vec2 worldEnd = {};
            pSurface->GetWorldEndpoints(worldStart, worldEnd);
            desc.LocalStart = worldStart - Vec2(worldPos.x, worldPos.y);
            desc.LocalEnd = worldEnd - Vec2(worldPos.x, worldPos.y);

            Ptr<CSurfaceCircleGuideScript> pGuide = pChild->GetScript<CSurfaceCircleGuideScript>();
            if (pGuide != nullptr && IsCircleSurfaceGeometry(desc.Geometry))
            {
                desc.HasCircleGuide = true;
                desc.CircleGuideEnabled = pGuide->IsEnabled();
                desc.GuideEntryAngleDeg = pGuide->GetEntryAngleDeg();
                desc.GuideHalfCheckAngleDeg = pGuide->GetHalfCheckAngleDeg();
                desc.GuideExitAngleDeg = pGuide->GetExitAngleDeg();
                desc.GuideCorrectionLineStartLocal = pGuide->GetCorrectionLineStartLocal();
                desc.GuideCorrectionLineEndLocal = pGuide->GetCorrectionLineEndLocal();
                desc.LinkedCorrectionLineName = pGuide->GetLinkedCorrectionLineName();
            }

            _SurfaceSet->AddSurface(desc);
            ++_OutCount;
        }

        if (_OutCount == 0)
        {
            SetSurfaceSetStatusText(_OutStatusText, "There are no scene surface objects to bake in the current level.");
            return false;
        }

        return true;
    }

    bool SaveSurfaceSetAssetImpl(ASurfaceSet* _SurfaceSet, std::string* _OutStatusText)
    {
        if (_SurfaceSet == nullptr)
            return false;

        std::wstring relativePath = _SurfaceSet->GetRelativePath();
        if (relativePath.empty())
            relativePath = _SurfaceSet->GetKey();

        if (relativePath.empty())
        {
            SetSurfaceSetStatusText(_OutStatusText, "SurfaceSet asset path is empty, so it could not be saved.");
            return false;
        }

        CreateDirectoryW((std::wstring(CONTENT_PATH) + L"SurfaceSet").c_str(), nullptr);
        if (FAILED(_SurfaceSet->Save(std::wstring(CONTENT_PATH) + relativePath)))
        {
            SetSurfaceSetStatusText(_OutStatusText, "Failed to save the SurfaceSet asset to disk.");
            return false;
        }

        if (!_SurfaceSet->GetKey().empty())
            AssetMgr::GetInst()->Load<ASurfaceSet>(_SurfaceSet->GetKey(), relativePath);

        return true;
    }

    Ptr<GameObject> CreateSurfaceSetActorImpl(ASurfaceSet* _SurfaceSet)
    {
        if (_SurfaceSet == nullptr)
            return nullptr;

        Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();
        if (pLevel == nullptr)
            return nullptr;

        const std::wstring actorName = BuildSurfaceSetActorName(_SurfaceSet);
        Ptr<GameObject> pActor = pLevel->FindObjectByName(actorName);
        CSurfaceSetScript* pSurfaceSetScript = (pActor != nullptr) ? pActor->GetScript<CSurfaceSetScript>().Get() : nullptr;

        if (pActor == nullptr || pSurfaceSetScript == nullptr)
        {
            pActor = new GameObject;
            pActor->SetName(actorName);
            pActor->AddComponent(new CTransform);
            pActor->Transform()->SetRelativePos(Vec3(0.f, 0.f, 0.f));

            pSurfaceSetScript = new CSurfaceSetScript;
            pActor->AddComponent(pSurfaceSetScript);
            pLevel->AddObject(kSurfaceLayerIndex, pActor);
        }

        pSurfaceSetScript->SetSurfaceSet(_SurfaceSet);
        pLevel->SetChanged();
        return pActor;
    }

    bool ReplaceSceneSurfacesWithActorImpl(ASurfaceSet* _SurfaceSet, UINT& _OutCount, std::string* _OutStatusText)
    {
        _OutCount = 0;

        if (!BakeSceneSurfacesImpl(_SurfaceSet, _OutCount, _OutStatusText))
            return false;

        if (!SaveSurfaceSetAssetImpl(_SurfaceSet, _OutStatusText))
            return false;

        Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();
        if (pLevel == nullptr)
        {
            SetSurfaceSetStatusText(_OutStatusText, "No level is loaded.");
            return false;
        }

        Ptr<GameObject> pRoot = pLevel->FindObjectByName(kSurfaceRootName);
        if (pRoot == nullptr)
        {
            SetSurfaceSetStatusText(_OutStatusText, "SurfaceEditorRoot was not found in the scene.");
            return false;
        }

        Ptr<GameObject> pActor = CreateSurfaceSetActorImpl(_SurfaceSet);
        if (pActor == nullptr)
        {
            SetSurfaceSetStatusText(_OutStatusText, "Failed to create a SurfaceSet actor in the scene.");
            return false;
        }

        const vector<Ptr<GameObject>>& vecChild = pRoot->GetChild();
        for (const Ptr<GameObject>& pChild : vecChild)
        {
            if (pChild == nullptr || pChild->IsDead())
                continue;

            if (pChild->GetScript<CSurfaceScript>() != nullptr)
                pChild->Destroy();
        }

        Ptr<Inspector> pInspector = (Inspector*)EditorMgr::GetInst()->FindUI("Inspector").Get();
        if (pInspector != nullptr)
            pInspector->SetTargetObject(pActor.Get());

        SetSurfaceSetStatusText(_OutStatusText,
                                "Baked " + std::to_string(_OutCount) + " scene surfaces and replaced them with a SurfaceSet actor.");
        return true;
    }
}

SurfaceSetUI::SurfaceSetUI()
    : AssetUI(ASSET_TYPE::SURFACESET)
{
}

SurfaceSetUI::~SurfaceSetUI()
{
}

Ptr<ASurfaceSet> SurfaceSetUI::EnsureCurrentLevelSurfaceSetAsset(std::string* _OutStatusText)
{
    return EnsureCurrentLevelSurfaceSetAssetImpl(_OutStatusText);
}

void SurfaceSetUI::Tick_UI()
{
    OutputTitle();

    ASurfaceSet* pSurfaceSet = (ASurfaceSet*)GetTargetAsset().Get();
    if (nullptr == pSurfaceSet)
        return;

    UINT lineCount = 0;
    UINT curveCount = 0;
    UINT wallCount = 0;
    UINT attachableCount = 0;
    UINT guideCount = 0;

    for (const ASurfaceSet::SURFACE_DESC& desc : pSurfaceSet->GetSurfaces())
    {
        if (desc.Geometry == SURFACE_SET_GEOMETRY::LINE)
            ++lineCount;
        else
            ++curveCount;

        if (desc.Role == SURFACE_SET_ROLE::WALL)
            ++wallCount;

        if (desc.Attachable)
            ++attachableCount;

        if (desc.HasCircleGuide)
            ++guideCount;
    }

    ImGui::Text("Surface Count : %u", pSurfaceSet->GetSurfaceCount());
    ImGui::Text("Line Count : %u", lineCount);
    ImGui::Text("Curve Count : %u", curveCount);
    ImGui::Text("Wall Count : %u", wallCount);
    ImGui::Text("Attachable : %u", attachableCount);
    ImGui::Text("Guide Count : %u", guideCount);

    const bool canEditScene = (LevelMgr::GetInst()->GetLevelState() == LEVEL_STATE::STOP);
    if (!canEditScene)
        ImGui::TextWrapped("Bake / replace is disabled while the level is playing.");

    if (ImGui::Button("Bake Scene Surfaces") && canEditScene)
    {
        UINT bakedCount = 0;
        if (BakeSceneSurfaces(pSurfaceSet, bakedCount) && SaveSurfaceSetAsset(pSurfaceSet))
            m_StatusText = "Baked " + std::to_string(bakedCount) + " scene surfaces into the selected SurfaceSet asset.";
    }

    if (ImGui::Button("Bake && Replace With Actor") && canEditScene)
    {
        UINT bakedCount = 0;
        if (ReplaceSceneSurfacesWithActor(pSurfaceSet, bakedCount))
            m_StatusText = "Baked " + std::to_string(bakedCount) + " scene surfaces and replaced them with a SurfaceSet actor.";
    }

    if (!m_StatusText.empty())
    {
        ImGui::Separator();
        ImGui::TextWrapped("%s", m_StatusText.c_str());
    }
}

bool SurfaceSetUI::ReplaceSceneSurfacesWithActorInLevel(ASurfaceSet* _SurfaceSet, UINT& _OutCount, std::string* _OutStatusText)
{
    return ReplaceSceneSurfacesWithActorImpl(_SurfaceSet, _OutCount, _OutStatusText);
}

bool SurfaceSetUI::BakeSceneSurfaces(ASurfaceSet* _SurfaceSet, UINT& _OutCount)
{
    return BakeSceneSurfacesImpl(_SurfaceSet, _OutCount, &m_StatusText);
}

bool SurfaceSetUI::SaveSurfaceSetAsset(ASurfaceSet* _SurfaceSet)
{
    return SaveSurfaceSetAssetImpl(_SurfaceSet, &m_StatusText);
}

Ptr<GameObject> SurfaceSetUI::CreateSurfaceSetActor(ASurfaceSet* _SurfaceSet)
{
    return CreateSurfaceSetActorImpl(_SurfaceSet);
}

bool SurfaceSetUI::ReplaceSceneSurfacesWithActor(ASurfaceSet* _SurfaceSet, UINT& _OutCount)
{
    return ReplaceSceneSurfacesWithActorImpl(_SurfaceSet, _OutCount, &m_StatusText);
}
