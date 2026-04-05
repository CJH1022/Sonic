#include "pch.h"
#include "CSurfaceSetScript.h"

#include "ASurfaceSet.h"
#include "AssetMgr.h"
#include "CCollider2D.h"
#include "CTransform.h"
#include "GameObject.h"
#include "LevelMgr.h"
#include "Source/Scripts/CSurfaceCircleGuideScript.h"
#include "Source/Scripts/CSurfaceScript.h"
#include "func.h"

#include <cmath>

namespace
{
    CSurfaceScript::SURFACE_GEOMETRY ToRuntimeGeometry(SURFACE_SET_GEOMETRY _Geometry)
    {
        switch (_Geometry)
        {
        case SURFACE_SET_GEOMETRY::ARC:
            return CSurfaceScript::SURFACE_GEOMETRY::ARC;
        case SURFACE_SET_GEOMETRY::CIRCLE:
            return CSurfaceScript::SURFACE_GEOMETRY::CIRCLE;
        case SURFACE_SET_GEOMETRY::FULL_CIRCLE:
            return CSurfaceScript::SURFACE_GEOMETRY::FULL_CIRCLE;
        case SURFACE_SET_GEOMETRY::LINE:
        default:
            return CSurfaceScript::SURFACE_GEOMETRY::LINE;
        }
    }

    CSurfaceScript::SURFACE_ROLE ToRuntimeRole(SURFACE_SET_ROLE _Role)
    {
        switch (_Role)
        {
        case SURFACE_SET_ROLE::CORRECTION:
            return CSurfaceScript::SURFACE_ROLE::CORRECTION;
        case SURFACE_SET_ROLE::WALL:
            return CSurfaceScript::SURFACE_ROLE::WALL;
        case SURFACE_SET_ROLE::VERTICAL_ENTRY:
            return CSurfaceScript::SURFACE_ROLE::VERTICAL_ENTRY;
        case SURFACE_SET_ROLE::SURFACE:
        default:
            return CSurfaceScript::SURFACE_ROLE::SURFACE;
        }
    }

    CSurfaceScript::ARC_CORNER ToRuntimeArcCorner(SURFACE_SET_ARC_CORNER _Corner)
    {
        switch (_Corner)
        {
        case SURFACE_SET_ARC_CORNER::TOP_LEFT:
            return CSurfaceScript::ARC_CORNER::TOP_LEFT;
        case SURFACE_SET_ARC_CORNER::TOP_RIGHT:
            return CSurfaceScript::ARC_CORNER::TOP_RIGHT;
        case SURFACE_SET_ARC_CORNER::BOTTOM_RIGHT:
            return CSurfaceScript::ARC_CORNER::BOTTOM_RIGHT;
        case SURFACE_SET_ARC_CORNER::BOTTOM_LEFT:
        default:
            return CSurfaceScript::ARC_CORNER::BOTTOM_LEFT;
        }
    }

    bool IsCircleSurfaceGeometry(SURFACE_SET_GEOMETRY _Geometry)
    {
        return _Geometry == SURFACE_SET_GEOMETRY::CIRCLE ||
            _Geometry == SURFACE_SET_GEOMETRY::FULL_CIRCLE;
    }

    float ComputeFullCircleRadius(const ASurfaceSet::SURFACE_DESC& _Desc)
    {
        float radius = max(fabsf(_Desc.LocalStart.x), fabsf(_Desc.LocalEnd.x));
        radius = max(radius, max(fabsf(_Desc.LocalStart.y), fabsf(_Desc.LocalEnd.y)));
        return max(radius, 1.f);
    }

    std::wstring BuildRuntimeSurfaceName(const ASurfaceSet::SURFACE_DESC& _Desc, UINT _Index)
    {
        if (!_Desc.Name.empty())
            return _Desc.Name;

        wchar_t buffer[64] = {};
        swprintf_s(buffer, L"SurfaceSetSurface_%u", _Index);
        return buffer;
    }

    void SaveSurfaceSetRef(FILE* _File, ASurfaceSet* _SurfaceSet)
    {
        const bool hasAsset = (_SurfaceSet != nullptr);
        fwrite(&hasAsset, sizeof(bool), 1, _File);
        if (!hasAsset)
            return;

        SaveWString(_File, _SurfaceSet->GetKey());

        std::wstring relativePath = _SurfaceSet->GetRelativePath();
        if (relativePath.empty())
            relativePath = _SurfaceSet->GetKey();

        SaveWString(_File, relativePath);
    }

    Ptr<ASurfaceSet> LoadSurfaceSetRef(FILE* _File)
    {
        bool hasAsset = false;
        fread(&hasAsset, sizeof(bool), 1, _File);
        if (!hasAsset)
            return nullptr;

        const std::wstring key = LoadWString(_File);
        const std::wstring relativePath = LoadWString(_File);
        if (key.empty())
            return nullptr;

        if (!relativePath.empty())
            return AssetMgr::GetInst()->Load<ASurfaceSet>(key, relativePath);

        return AssetMgr::GetInst()->Find<ASurfaceSet>(key);
    }
}

CSurfaceSetScript::CSurfaceSetScript()
    : CScript(SCRIPT_TYPE::SURFACESETSCRIPT)
    , m_bRuntimeBuilt(false)
    , m_bBuiltForPlayState(false)
    , m_BuiltSurfaceCount(0)
{
    AddScriptParam(SCRIPT_PARAM::SURFACESET, &m_SurfaceSet, L"Surface Set");
}

CSurfaceSetScript::CSurfaceSetScript(const CSurfaceSetScript& _Origin)
    : CScript(_Origin)
    , m_SurfaceSet(_Origin.m_SurfaceSet)
    , m_bRuntimeBuilt(false)
    , m_bBuiltForPlayState(false)
    , m_BuiltSurfaceCount(0)
{
    AddScriptParam(SCRIPT_PARAM::SURFACESET, &m_SurfaceSet, L"Surface Set");
}

CSurfaceSetScript::~CSurfaceSetScript()
{
}

void CSurfaceSetScript::Begin()
{
    ClearRuntimeSurfaces();
    m_bRuntimeBuilt = false;
    m_bBuiltForPlayState = false;
    m_BuiltSurfaceCount = 0;
    m_vecRuntimeSurfaceObjects.clear();
}

void CSurfaceSetScript::Tick()
{
    const bool bBuildForPlayState = ShouldBuildForPlayState();
    if (m_SurfaceSet == nullptr)
    {
        if (m_bRuntimeBuilt || !m_vecRuntimeSurfaceObjects.empty())
            ClearRuntimeSurfaces();

        m_bRuntimeBuilt = false;
        m_bBuiltForPlayState = bBuildForPlayState;
        m_BuiltSurfaceCount = 0;
        return;
    }

    if (m_bRuntimeBuilt && m_bBuiltForPlayState == bBuildForPlayState)
        return;

    BuildRuntimeSurfaces();
}

void CSurfaceSetScript::SetSurfaceSet(Ptr<ASurfaceSet> _SurfaceSet)
{
    if (m_SurfaceSet == _SurfaceSet)
        return;

    m_SurfaceSet = _SurfaceSet;
    m_bRuntimeBuilt = false;
    m_bBuiltForPlayState = false;
    m_BuiltSurfaceCount = 0;
    ClearRuntimeSurfaces();
}

void CSurfaceSetScript::SaveToLevelFile(FILE* _File)
{
    // Generated preview/runtime children are reconstructed from the asset and
    // must not be serialized into the level file.
    ClearRuntimeSurfaces();
    m_bRuntimeBuilt = false;
    m_bBuiltForPlayState = false;
    m_BuiltSurfaceCount = 0;
    SaveSurfaceSetRef(_File, m_SurfaceSet.Get());
}

void CSurfaceSetScript::LoadFromLevelFile(FILE* _File)
{
    m_SurfaceSet = LoadSurfaceSetRef(_File);
    m_bRuntimeBuilt = false;
    m_bBuiltForPlayState = false;
    m_BuiltSurfaceCount = 0;
    m_vecRuntimeSurfaceObjects.clear();
}

void CSurfaceSetScript::BuildRuntimeSurfaces()
{
    ClearRuntimeSurfaces();

    m_bRuntimeBuilt = false;
    m_bBuiltForPlayState = ShouldBuildForPlayState();
    m_BuiltSurfaceCount = 0;

    if (GetOwner() == nullptr || m_SurfaceSet == nullptr)
        return;

    m_bRuntimeBuilt = true;

    const vector<ASurfaceSet::SURFACE_DESC>& vecSurfaces = m_SurfaceSet->GetSurfaces();
    for (UINT i = 0; i < (UINT)vecSurfaces.size(); ++i)
    {
        const ASurfaceSet::SURFACE_DESC& desc = vecSurfaces[i];

        Ptr<GameObject> pSurfaceObject = new GameObject;
        pSurfaceObject->SetName(BuildRuntimeSurfaceName(desc, i));
        pSurfaceObject->AddComponent(new CTransform);
        pSurfaceObject->Transform()->SetIndependentScale(true);
        pSurfaceObject->Transform()->SetRelativeScale(Vec3(1.f, 1.f, 1.f));
        pSurfaceObject->Transform()->SetRelativePos(desc.LocalPosition);

        if (desc.Role == SURFACE_SET_ROLE::WALL)
            pSurfaceObject->AddComponent(new CCollider2D);

        CSurfaceScript* pSurfaceScript = new CSurfaceScript;
        pSurfaceObject->AddComponent(pSurfaceScript);

        const CSurfaceScript::SURFACE_ROLE role = ToRuntimeRole(desc.Role);
        const CSurfaceScript::ARC_CORNER corner = ToRuntimeArcCorner(desc.ArcCorner);

        switch (ToRuntimeGeometry(desc.Geometry))
        {
        case CSurfaceScript::SURFACE_GEOMETRY::ARC:
            pSurfaceScript->ConfigureArc(desc.LocalStart, desc.LocalEnd, role, corner, desc.FillInside, desc.Attachable);
            break;
        case CSurfaceScript::SURFACE_GEOMETRY::CIRCLE:
            pSurfaceScript->ConfigureCircle(desc.LocalStart, desc.LocalEnd, role, corner, desc.FillInside, desc.Attachable);
            break;
        case CSurfaceScript::SURFACE_GEOMETRY::FULL_CIRCLE:
            pSurfaceScript->ConfigureFullCircle(ComputeFullCircleRadius(desc), role, desc.FillInside, desc.Attachable);
            break;
        case CSurfaceScript::SURFACE_GEOMETRY::LINE:
        default:
            pSurfaceScript->ConfigureLine(desc.LocalStart, desc.LocalEnd, role, desc.FillAbove, desc.Attachable);
            break;
        }

        if (desc.HasCircleGuide && IsCircleSurfaceGeometry(desc.Geometry))
        {
            CSurfaceCircleGuideScript* pGuideScript = new CSurfaceCircleGuideScript;
            pSurfaceObject->AddComponent(pGuideScript);
            pGuideScript->SetEnabled(desc.CircleGuideEnabled);
            pGuideScript->SetEntryAngleDeg(desc.GuideEntryAngleDeg);
            pGuideScript->SetHalfCheckAngleDeg(desc.GuideHalfCheckAngleDeg);
            pGuideScript->SetExitAngleDeg(desc.GuideExitAngleDeg);
            pGuideScript->SetCorrectionLineStartLocal(desc.GuideCorrectionLineStartLocal);
            pGuideScript->SetCorrectionLineEndLocal(desc.GuideCorrectionLineEndLocal);
            pGuideScript->SetLinkedCorrectionLineName(desc.LinkedCorrectionLineName);
        }

        GetOwner()->AddChild(pSurfaceObject);
        m_vecRuntimeSurfaceObjects.push_back(pSurfaceObject);
        ++m_BuiltSurfaceCount;
    }
}

void CSurfaceSetScript::ClearRuntimeSurfaces()
{
    GameObject* pOwner = GetOwner();
    if (pOwner != nullptr)
    {
        const vector<Ptr<GameObject>> vecChild = pOwner->GetChild();
        for (const Ptr<GameObject>& pChild : vecChild)
        {
            if (pChild == nullptr)
                continue;

            if (pChild->GetScript<CSurfaceScript>() == nullptr)
                continue;

            if (pChild->GetParent() == pOwner)
                pChild->DisconnectWithParent();
        }
    }

    for (Ptr<GameObject>& pSurfaceObject : m_vecRuntimeSurfaceObjects)
    {
        if (pSurfaceObject != nullptr && pSurfaceObject->GetParent() != nullptr)
            pSurfaceObject->DisconnectWithParent();
    }

    m_vecRuntimeSurfaceObjects.clear();
}

bool CSurfaceSetScript::ShouldBuildForPlayState() const
{
    const LEVEL_STATE levelState = LevelMgr::GetInst()->GetLevelState();
    return (levelState == LEVEL_STATE::PLAY || levelState == LEVEL_STATE::PAUSE);
}
