#include "pch.h"
#include "MapEditorUI.h"

#include "ASurfaceSet.h"
#include "AssetMgr.h"
#include "Device.h"
#include "EditorMgr.h"
#include "Inspector.h"
#include "KeyMgr.h"
#include "LevelMgr.h"
#include "RenderMgr.h"
#include "SurfaceSetUI.h"
#include "func.h"

#include "CCamera.h"
#include "CCollider2D.h"
#include "CTransform.h"
#include "CTileRender.h"
#include "Source/Scripts/CSurfaceCircleGuideScript.h"

#include "Imgui/imgui.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

namespace
{
    const wchar_t* kSurfaceRootName = L"SurfaceEditorRoot";
    constexpr UINT kMetaMagic = 0x3141544D; // 'MTA1'
    constexpr int kDefaultRow = 10;
    constexpr int kDefaultCol = 30;
    constexpr float kDefaultTileSize = 200.f;
    constexpr float kWallOverlayDepth = 64.f;
    constexpr float kCircleGuideMatchEpsilon = 0.5f;
    constexpr float kCircleGuideMarkerRadius = 7.f;
    constexpr float kCircleGuideHalfCheckAngleDeg = 90.f;

    float Clamp01(float _Value)
    {
        if (_Value < 0.f)
            return 0.f;
        if (_Value > 1.f)
            return 1.f;
        return _Value;
    }

    int ClampInt(int _Value, int _Min, int _Max)
    {
        if (_Value < _Min)
            return _Min;
        if (_Value > _Max)
            return _Max;
        return _Value;
    }

    bool NearlyEqual(float _A, float _B, float _Epsilon = 0.001f)
    {
        return fabsf(_A - _B) <= _Epsilon;
    }

    float NormalizeAngleDegrees(float _AngleDeg)
    {
        while (_AngleDeg <= -180.f)
            _AngleDeg += 360.f;

        while (_AngleDeg > 180.f)
            _AngleDeg -= 360.f;

        return _AngleDeg;
    }

    float DegreesToRadians(float _AngleDeg)
    {
        return _AngleDeg * XM_PI / 180.f;
    }

    float RadiansToDegrees(float _AngleRad)
    {
        return _AngleRad * 180.f / XM_PI;
    }

    float NormalizeAngleDegreesPositive(float _AngleDeg)
    {
        while (_AngleDeg < 0.f)
            _AngleDeg += 360.f;

        while (_AngleDeg >= 360.f)
            _AngleDeg -= 360.f;

        return _AngleDeg;
    }

    float DeltaAngleCCWDeg(float _FromDeg, float _ToDeg)
    {
        const float from = NormalizeAngleDegreesPositive(_FromDeg);
        const float to = NormalizeAngleDegreesPositive(_ToDeg);
        float delta = to - from;
        if (delta < 0.f)
            delta += 360.f;
        return delta;
    }

    float LerpAngleDegreesAlong(bool _CounterClockwise, float _FromDeg, float _ToDeg, float _T)
    {
        const float delta = _CounterClockwise ? DeltaAngleCCWDeg(_FromDeg, _ToDeg) : DeltaAngleCCWDeg(_ToDeg, _FromDeg);
        const float angle = _CounterClockwise ? (_FromDeg + delta * _T) : (_FromDeg - delta * _T);
        return NormalizeAngleDegrees(angle);
    }

    bool TryResolveGuideArcDirection(float _StartAngleDeg, float _EndAngleDeg, float _HalfAngleDeg, bool& _OutCounterClockwise)
    {
        if (DeltaAngleCCWDeg(_StartAngleDeg, _HalfAngleDeg) <= DeltaAngleCCWDeg(_StartAngleDeg, _EndAngleDeg) + 0.001f)
        {
            _OutCounterClockwise = true;
            return true;
        }

        if (DeltaAngleCCWDeg(_EndAngleDeg, _HalfAngleDeg) <= DeltaAngleCCWDeg(_EndAngleDeg, _StartAngleDeg) + 0.001f)
        {
            _OutCounterClockwise = false;
            return true;
        }

        return false;
    }

    Vec2 MakeCirclePointFromAngle(const Vec2& _Center, float _Radius, float _AngleDeg)
    {
        const float angleRad = DegreesToRadians(_AngleDeg);
        return Vec2(_Center.x + cosf(angleRad) * _Radius,
                    _Center.y + sinf(angleRad) * _Radius);
    }

    float ComputeCircleAngleDegrees(const Vec2& _Center, const Vec2& _Point)
    {
        const Vec2 delta = _Point - _Center;
        if (fabsf(delta.x) <= 0.0001f && fabsf(delta.y) <= 0.0001f)
            return 90.f;

        return NormalizeAngleDegrees(RadiansToDegrees(atan2f(delta.y, delta.x)));
    }

    bool IsCircleGeometry(CSurfaceScript::SURFACE_GEOMETRY _Geometry)
    {
        return _Geometry == CSurfaceScript::SURFACE_GEOMETRY::CIRCLE ||
               _Geometry == CSurfaceScript::SURFACE_GEOMETRY::FULL_CIRCLE;
    }

    int CellIndex(int _Row, int _Col, int _ColCount)
    {
        return _Row * _ColCount + _Col;
    }

    wstring GetPresetPath()
    {
        return wstring(CONTENT_PATH) + L"TileMap\\TileScriptPreset.txt";
    }

    wstring GetMetaPath()
    {
        return wstring(CONTENT_PATH) + L"TileMap\\TileEditorMeta.bin";
    }

    wstring GetTileMapAssetPath()
    {
        return wstring(CONTENT_PATH) + L"TileMap\\TestTileMap.tile";
    }

    bool SaveMetaFile(const wstring& _FilePath, int _Row, int _Col, const vector<unsigned char>& _Kinds)
    {
        if (_Row <= 0 || _Col <= 0)
            return false;

        if (_Kinds.size() != (size_t)(_Row * _Col))
            return false;

        std::filesystem::path metaPath(_FilePath);
        if (!metaPath.parent_path().empty())
        {
            std::filesystem::create_directories(metaPath.parent_path());
        }

        FILE* pFile = nullptr;
        _wfopen_s(&pFile, _FilePath.c_str(), L"wb");
        if (nullptr == pFile)
            return false;

        fwrite(&kMetaMagic, sizeof(UINT), 1, pFile);
        fwrite(&_Row, sizeof(int), 1, pFile);
        fwrite(&_Col, sizeof(int), 1, pFile);

        UINT count = (UINT)_Kinds.size();
        fwrite(&count, sizeof(UINT), 1, pFile);
        fwrite(_Kinds.data(), sizeof(unsigned char), count, pFile);
        fclose(pFile);
        return true;
    }

    bool LoadMetaFile(const wstring& _FilePath, int _ExpectedRow, int _ExpectedCol, vector<unsigned char>& _OutKinds)
    {
        FILE* pFile = nullptr;
        _wfopen_s(&pFile, _FilePath.c_str(), L"rb");
        if (nullptr == pFile)
            return false;

        UINT magic = 0;
        int row = 0;
        int col = 0;
        UINT count = 0;

        fread(&magic, sizeof(UINT), 1, pFile);
        fread(&row, sizeof(int), 1, pFile);
        fread(&col, sizeof(int), 1, pFile);
        fread(&count, sizeof(UINT), 1, pFile);

        if (magic != kMetaMagic || row != _ExpectedRow || col != _ExpectedCol || count != (UINT)(row * col))
        {
            fclose(pFile);
            return false;
        }

        _OutKinds.assign(count, 0);
        fread(_OutKinds.data(), sizeof(unsigned char), count, pFile);
        fclose(pFile);
        return true;
    }

    bool SameShape(const CTileScript::TILE_FORMULA_CONFIG& _A, const CTileScript::TILE_FORMULA_CONFIG& _B)
    {
        return NearlyEqual(_A.a, _B.a)
            && NearlyEqual(_A.b, _B.b)
            && NearlyEqual(_A.c, _B.c)
            && NearlyEqual(_A.r, _B.r)
            && NearlyEqual(_A.center_x, _B.center_x)
            && NearlyEqual(_A.center_y, _B.center_y);
    }

    float EvalLineY(const MapEditorUI::GRID_VERTEX& _Start, const MapEditorUI::GRID_VERTEX& _End, float _X)
    {
        float dx = (float)(_End.x - _Start.x);
        if (fabsf(dx) <= 0.0001f)
            return (float)_Start.y;

        float t = (_X - (float)_Start.x) / dx;
        return (float)_Start.y + ((float)_End.y - (float)_Start.y) * t;
    }

    float DotVec2(const Vec2& _A, const Vec2& _B)
    {
        return _A.x * _B.x + _A.y * _B.y;
    }

    float LengthVec2(const Vec2& _V)
    {
        return sqrtf(_V.x * _V.x + _V.y * _V.y);
    }

    Vec2 NormalizeVec2(const Vec2& _V, const Vec2& _Fallback = Vec2(1.f, 0.f))
    {
        float len = LengthVec2(_V);
        if (len <= 0.0001f)
            return _Fallback;

        return Vec2(_V.x / len, _V.y / len);
    }

    void CanonicalizeLineWorld(Vec2& _A, Vec2& _B)
    {
        Vec2 delta = _B - _A;

        if (fabsf(delta.x) >= fabsf(delta.y))
        {
            if (_A.x > _B.x)
                std::swap(_A, _B);
        }
        else
        {
            if (_A.y > _B.y)
                std::swap(_A, _B);
        }
    }

    float DistancePointToSegment(const Vec2& _Point, const Vec2& _A, const Vec2& _B)
    {
        Vec2 ab = _B - _A;
        float len2 = DotVec2(ab, ab);
        if (len2 <= 0.0001f)
            return LengthVec2(_Point - _A);

        float t = DotVec2(_Point - _A, ab) / len2;
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;

        Vec2 q = _A + ab * t;
        return LengthVec2(_Point - q);
    }
}

MapEditorUI::MapEditorUI()
    : EditorUI("MapEditor")
    , m_ShowGrid(false)
    , m_ShowNormals(false)
    , m_AutoSave(false)
    , m_GridOpacity(0.35f)
    , m_FreeformMode(true)
    , m_Tool(MAP_TOOL::SURFACE_LINE)
    , m_ArcCorner(ARC_CORNER::BOTTOM_LEFT)
    , m_WallSide(WALL_SIDE::RIGHT)
    , m_FillAbove(false)
    , m_ArcFillInside(false)
    , m_Attachable(true)
    , m_EnablePointSnap(false)
    , m_PointSnapDistance(36.f)
    , m_FreeformShape(FREEFORM_SHAPE::LINE)
    , m_FreeformRole(FREEFORM_ROLE::SURFACE)
    , m_UseCircleRadius(false)
    , m_CircleRadius(200.f)
    , m_SurfaceSetAsset(nullptr)
    , m_HasStartVertex(false)
    , m_StartVertex{ 0, 0 }
    , m_HasStartPoint(false)
    , m_StartPoint(Vec2(0.f, 0.f))
    , m_HasLastCreatedSegment(false)
    , m_LastCreatedStart(Vec2(0.f, 0.f))
    , m_LastCreatedEnd(Vec2(0.f, 0.f))
    , m_GuideDragHandle(GUIDE_DRAG_HANDLE::NONE)
    , m_Row(kDefaultRow)
    , m_Col(kDefaultCol)
    , m_TileSize(kDefaultTileSize, kDefaultTileSize)
    , m_LastTargetObject(nullptr)
{
    SetActive(false);
}

MapEditorUI::~MapEditorUI()
{
}

void MapEditorUI::Activate()
{
    m_HasStartVertex = false;
    m_HasStartPoint = false;
    m_LastTargetObject = nullptr;
    m_StatusText = "Freeform surface editor active.";
    EnsureSurfaceRoot();
}

void MapEditorUI::Deactivate()
{
    m_HasStartVertex = false;
    m_HasStartPoint = false;
}

Ptr<CCamera> MapEditorUI::GetSceneCamera() const
{
    if (LEVEL_STATE::PLAY == LevelMgr::GetInst()->GetLevelState())
    {
        Ptr<CCamera> pPOVCamera = RenderMgr::GetInst()->GetPOVCamera();
        if (pPOVCamera != nullptr)
            return pPOVCamera;
    }

    return RenderMgr::GetInst()->GetEditorCamera();
}

Ptr<GameObject> MapEditorUI::ResolveSurfaceRoot() const
{
    Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();
    if (pLevel == nullptr)
        return nullptr;

    return pLevel->FindObjectByName(kSurfaceRootName);
}

Ptr<GameObject> MapEditorUI::EnsureSurfaceRoot()
{
    Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();
    if (pLevel == nullptr)
        return nullptr;

    Ptr<GameObject> pRoot = ResolveSurfaceRoot();
    if (pRoot != nullptr)
        return pRoot;

    pRoot = new GameObject;
    pRoot->SetName(kSurfaceRootName);
    pRoot->AddComponent(new CTransform);
    pRoot->Transform()->SetRelativePos(Vec3(0.f, 0.f, 0.f));
    pLevel->AddObject(2, pRoot);
    return pRoot;
}

void MapEditorUI::CollectSurfaceObjects(vector<Ptr<GameObject>>& _OutObjects) const
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

Ptr<GameObject> MapEditorUI::FindSurfaceObjectByName(const wstring& _Name) const
{
    if (_Name.empty())
        return nullptr;

    vector<Ptr<GameObject>> vecSurfaces;
    CollectSurfaceObjects(vecSurfaces);

    for (size_t i = 0; i < vecSurfaces.size(); ++i)
    {
        if (vecSurfaces[i] != nullptr && vecSurfaces[i]->GetName() == _Name)
            return vecSurfaces[i];
    }

    return nullptr;
}

Ptr<GameObject> MapEditorUI::GetSelectedSurfaceObject() const
{
    Ptr<Inspector> pInspector = (Inspector*)EditorMgr::GetInst()->FindUI("Inspector").Get();
    if (pInspector == nullptr)
        return nullptr;

    Ptr<GameObject> pSelected = pInspector->GetTargetObject();
    if (pSelected == nullptr || pSelected->GetScript<CSurfaceScript>() == nullptr)
        return nullptr;

    return pSelected;
}

void MapEditorUI::SelectSurfaceObject(Ptr<GameObject> _Object) const
{
    Ptr<Inspector> pInspector = (Inspector*)EditorMgr::GetInst()->FindUI("Inspector").Get();
    if (pInspector != nullptr)
        pInspector->SetTargetObject(_Object);
}

CSurfaceCircleGuideScript* MapEditorUI::EnsureCircleGuideScript(GameObject* _SurfaceObject, bool _ApplyDefaults) const
{
    if (_SurfaceObject == nullptr)
        return nullptr;

    auto pSurface = _SurfaceObject->GetScript<CSurfaceScript>();
    if (pSurface == nullptr || !IsCircleGeometry(pSurface->GetGeometry()))
        return nullptr;

    Ptr<CSurfaceCircleGuideScript> pGuide = _SurfaceObject->GetScript<CSurfaceCircleGuideScript>();
    if (pGuide != nullptr)
        return pGuide.Get();

    CSurfaceCircleGuideScript* pNewGuide = new CSurfaceCircleGuideScript;
    _SurfaceObject->AddComponent(pNewGuide);

    if (_ApplyDefaults)
    {
        Vec2 center = {};
        float radius = 0.f;
        Vec2 boxMin = {};
        Vec2 boxMax = {};
        pSurface->GetArcWorldData(center, radius, boxMin, boxMax);
        pNewGuide->ResetToDefaultGuide(radius);
        pNewGuide->SetEnabled(true);
    }

    Ptr<GameObject> pSelectedSurface = GetSelectedSurfaceObject();
    if (pSelectedSurface != nullptr && pSelectedSurface.Get() == _SurfaceObject)
        SelectSurfaceObject(pSelectedSurface);

    return pNewGuide;
}

void MapEditorUI::ApplyCircleGuideToMatchingSurfaces(GameObject* _AnchorObject)
{
    if (_AnchorObject == nullptr)
        return;

    auto pAnchorSurface = _AnchorObject->GetScript<CSurfaceScript>();
    auto pAnchorGuide = _AnchorObject->GetScript<CSurfaceCircleGuideScript>();
    if (pAnchorSurface == nullptr || pAnchorGuide == nullptr || !IsCircleGeometry(pAnchorSurface->GetGeometry()))
    {
        return;
    }

    Vec2 anchorCenter = {};
    float anchorRadius = 0.f;
    Vec2 anchorBoxMin = {};
    Vec2 anchorBoxMax = {};
    pAnchorSurface->GetArcWorldData(anchorCenter, anchorRadius, anchorBoxMin, anchorBoxMax);

    vector<Ptr<GameObject>> vecSurfaces;
    CollectSurfaceObjects(vecSurfaces);

    for (size_t i = 0; i < vecSurfaces.size(); ++i)
    {
        Ptr<GameObject> pSurfaceObject = vecSurfaces[i];
        if (pSurfaceObject == nullptr || pSurfaceObject.Get() == _AnchorObject)
            continue;

        auto pSurface = pSurfaceObject->GetScript<CSurfaceScript>();
        if (pSurface == nullptr ||
            pSurface->GetGeometry() != pAnchorSurface->GetGeometry() ||
            pSurface->GetFillInside() != pAnchorSurface->GetFillInside())
        {
            continue;
        }

        Vec2 center = {};
        float radius = 0.f;
        Vec2 boxMin = {};
        Vec2 boxMax = {};
        pSurface->GetArcWorldData(center, radius, boxMin, boxMax);

        if (!NearlyEqual(center.x, anchorCenter.x, kCircleGuideMatchEpsilon) ||
            !NearlyEqual(center.y, anchorCenter.y, kCircleGuideMatchEpsilon) ||
            !NearlyEqual(radius, anchorRadius, kCircleGuideMatchEpsilon))
        {
            continue;
        }

        auto pGuide = EnsureCircleGuideScript(pSurfaceObject.Get(), false);
        if (pGuide == nullptr)
            continue;

        pGuide->CopyFrom(*pAnchorGuide.Get());
    }
}

void MapEditorUI::CreateOrUpdateCircleGuideCorrectionLine(GameObject* _CircleSurfaceObject)
{
    if (_CircleSurfaceObject == nullptr)
        return;

    auto pSurface = _CircleSurfaceObject->GetScript<CSurfaceScript>();
    auto pGuide = EnsureCircleGuideScript(_CircleSurfaceObject, true);
    if (pSurface == nullptr || pGuide == nullptr)
        return;

    Vec2 center = {};
    float radius = 0.f;
    Vec2 boxMin = {};
    Vec2 boxMax = {};
    pSurface->GetArcWorldData(center, radius, boxMin, boxMax);

    if (pGuide->GetLinkedCorrectionLineName().empty())
    {
        pGuide->SetLinkedCorrectionLineName(_CircleSurfaceObject->GetName() + wstring(L"_CorrectionLine"));
        ApplyCircleGuideToMatchingSurfaces(_CircleSurfaceObject);
    }

    const Vec2 worldStart = center + pGuide->GetCorrectionLineStartLocal();
    const Vec2 worldEnd = center + pGuide->GetCorrectionLineEndLocal();
    if (LengthVec2(worldEnd - worldStart) <= 1.f)
    {
        m_StatusText = "Correction line start and end must be different.";
        return;
    }

    Ptr<GameObject> pRoot = EnsureSurfaceRoot();
    if (pRoot == nullptr)
        return;

    Ptr<GameObject> pLineObject = FindSurfaceObjectByName(pGuide->GetLinkedCorrectionLineName());
    if (pLineObject == nullptr)
    {
        pLineObject = new GameObject;
        pLineObject->SetName(pGuide->GetLinkedCorrectionLineName());
        pLineObject->AddComponent(new CTransform);
        pLineObject->AddComponent(new CCollider2D);
        pLineObject->AddComponent(new CSurfaceScript);
        pRoot->AddChild(pLineObject);
    }

    auto pLineSurface = pLineObject->GetScript<CSurfaceScript>();
    if (pLineSurface == nullptr)
    {
        m_StatusText = "Linked correction line exists but is not a surface.";
        return;
    }

    const Vec2 lineCenter = (worldStart + worldEnd) * 0.5f;
    pLineObject->Transform()->SetRelativePos(Vec3(lineCenter.x, lineCenter.y, 10.f));
    pLineSurface->ConfigureLine(worldStart - lineCenter,
                                worldEnd - lineCenter,
                                CSurfaceScript::SURFACE_ROLE::SURFACE,
                                false,
                                true);

    RefreshCircleGuideCorrectionLines();
    m_StatusText = "Circle correction line updated from the selected guide.";
}

bool MapEditorUI::GetSurfaceEndpointPair(CSurfaceScript* _Script, Vec2& _OutStart, Vec2& _OutEnd) const
{
    if (_Script == nullptr)
        return false;

    if (_Script->GetGeometry() == CSurfaceScript::SURFACE_GEOMETRY::LINE)
    {
        _Script->GetWorldEndpoints(_OutStart, _OutEnd);
        return true;
    }

    Vec2 center = {};
    Vec2 boxMin = {};
    Vec2 boxMax = {};
    float radius = 0.f;
    _Script->GetArcWorldData(center, radius, boxMin, boxMax);
    if (radius <= 0.001f)
        return false;

    if (_Script->GetGeometry() == CSurfaceScript::SURFACE_GEOMETRY::FULL_CIRCLE)
    {
        _OutStart = Vec2(center.x - radius, center.y);
        _OutEnd = Vec2(center.x + radius, center.y);
        return true;
    }

    switch (_Script->GetArcCorner())
    {
    case CSurfaceScript::ARC_CORNER::TOP_LEFT:
        _OutStart = Vec2(center.x + radius, center.y);
        _OutEnd = Vec2(center.x, center.y - radius);
        return true;
    case CSurfaceScript::ARC_CORNER::TOP_RIGHT:
        _OutStart = Vec2(center.x - radius, center.y);
        _OutEnd = Vec2(center.x, center.y - radius);
        return true;
    case CSurfaceScript::ARC_CORNER::BOTTOM_LEFT:
        _OutStart = Vec2(center.x + radius, center.y);
        _OutEnd = Vec2(center.x, center.y + radius);
        return true;
    case CSurfaceScript::ARC_CORNER::BOTTOM_RIGHT:
    default:
        _OutStart = Vec2(center.x - radius, center.y);
        _OutEnd = Vec2(center.x, center.y + radius);
        return true;
    }
}

Ptr<GameObject> MapEditorUI::FindNearestSurfaceObject(const Vec2& _WorldPos, float& _OutDistance) const
{
    _OutDistance = FLT_MAX;

    vector<Ptr<GameObject>> vecSurfaces;
    CollectSurfaceObjects(vecSurfaces);

    Ptr<GameObject> pBest = nullptr;

    for (size_t i = 0; i < vecSurfaces.size(); ++i)
    {
        Ptr<CSurfaceScript> pScript = vecSurfaces[i]->GetScript<CSurfaceScript>();
        if (pScript == nullptr)
            continue;

        float dist = FLT_MAX;
        if (pScript->GetGeometry() == CSurfaceScript::SURFACE_GEOMETRY::LINE)
        {
            Vec2 a = {};
            Vec2 b = {};
            pScript->GetWorldEndpoints(a, b);
            dist = DistancePointToSegment(_WorldPos, a, b);
        }
        else
        {
            Vec2 center = {};
            Vec2 boxMin = {};
            Vec2 boxMax = {};
            float radius = 0.f;
            pScript->GetArcWorldData(center, radius, boxMin, boxMax);
            float radialDist = fabsf(LengthVec2(_WorldPos - center) - radius);
            bool inBox = boxMin.x <= _WorldPos.x && _WorldPos.x <= boxMax.x && boxMin.y <= _WorldPos.y && _WorldPos.y <= boxMax.y;
            dist = inBox ? radialDist : FLT_MAX;
        }

        if (dist < _OutDistance)
        {
            _OutDistance = dist;
            pBest = vecSurfaces[i];
        }
    }

    return pBest;
}

bool MapEditorUI::FindSnapPoint(const Vec2& _WorldPos, Vec2& _OutSnapPoint) const
{
    if (!m_EnablePointSnap || m_PointSnapDistance <= 0.f)
        return false;

    vector<Ptr<GameObject>> vecSurfaces;
    CollectSurfaceObjects(vecSurfaces);

    float bestDistSq = m_PointSnapDistance * m_PointSnapDistance;
    bool found = false;

    for (size_t i = 0; i < vecSurfaces.size(); ++i)
    {
        Ptr<CSurfaceScript> pScript = vecSurfaces[i]->GetScript<CSurfaceScript>();
        if (pScript == nullptr)
            continue;

        Vec2 pointA = {};
        Vec2 pointB = {};
        if (!GetSurfaceEndpointPair(pScript.Get(), pointA, pointB))
            continue;

        const Vec2 candidates[2] = { pointA, pointB };
        for (int candidateIdx = 0; candidateIdx < 2; ++candidateIdx)
        {
            Vec2 delta = candidates[candidateIdx] - _WorldPos;
            float distSq = DotVec2(delta, delta);
            if (distSq <= bestDistSq)
            {
                bestDistSq = distSq;
                _OutSnapPoint = candidates[candidateIdx];
                found = true;
            }
        }

        if (pScript->GetGeometry() != CSurfaceScript::SURFACE_GEOMETRY::LINE)
        {
            Vec2 center = {};
            Vec2 boxMin = {};
            Vec2 boxMax = {};
            float radius = 0.f;
            pScript->GetArcWorldData(center, radius, boxMin, boxMax);

            Vec2 delta = center - _WorldPos;
            float distSq = DotVec2(delta, delta);
            if (distSq <= bestDistSq)
            {
                bestDistSq = distSq;
                _OutSnapPoint = center;
                found = true;
            }
        }

        auto pGuide = vecSurfaces[i]->GetScript<CSurfaceCircleGuideScript>();
        if (pGuide != nullptr && pGuide->IsEnabled() && IsCircleGeometry(pScript->GetGeometry()))
        {
            Vec2 center = {};
            float radius = 0.f;
            Vec2 boxMin = {};
            Vec2 boxMax = {};
            pScript->GetArcWorldData(center, radius, boxMin, boxMax);

            const Vec2 guidePoints[2] =
            {
                center + pGuide->GetCorrectionLineStartLocal(),
                center + pGuide->GetCorrectionLineEndLocal(),
            };

            for (int guideIdx = 0; guideIdx < 2; ++guideIdx)
            {
                const Vec2 delta = guidePoints[guideIdx] - _WorldPos;
                const float distSq = DotVec2(delta, delta);
                if (distSq <= bestDistSq)
                {
                    bestDistSq = distSq;
                    _OutSnapPoint = guidePoints[guideIdx];
                    found = true;
                }
            }
        }
    }

    return found;
}

void MapEditorUI::RefreshCircleGuideCorrectionLines()
{
    vector<Ptr<GameObject>> vecSurfaces;
    CollectSurfaceObjects(vecSurfaces);

    vector<wstring> vecLinkedNames;
    vecLinkedNames.reserve(vecSurfaces.size());

    for (size_t i = 0; i < vecSurfaces.size(); ++i)
    {
        if (vecSurfaces[i] == nullptr)
            continue;

        auto pSurface = vecSurfaces[i]->GetScript<CSurfaceScript>();
        auto pGuide = vecSurfaces[i]->GetScript<CSurfaceCircleGuideScript>();
        if (pSurface == nullptr ||
            pGuide == nullptr ||
            !IsCircleGeometry(pSurface->GetGeometry()) ||
            pGuide->GetLinkedCorrectionLineName().empty())
        {
            continue;
        }

        vecLinkedNames.push_back(pGuide->GetLinkedCorrectionLineName());

        Ptr<GameObject> pLineObject = FindSurfaceObjectByName(pGuide->GetLinkedCorrectionLineName());
        if (pLineObject == nullptr)
            continue;

        auto pLineSurface = pLineObject->GetScript<CSurfaceScript>();
        if (pLineSurface == nullptr ||
            pLineSurface->GetGeometry() != CSurfaceScript::SURFACE_GEOMETRY::LINE)
        {
            continue;
        }

        Vec2 center = {};
        float radius = 0.f;
        Vec2 boxMin = {};
        Vec2 boxMax = {};
        pSurface->GetArcWorldData(center, radius, boxMin, boxMax);

        const Vec2 worldStart = center + pGuide->GetCorrectionLineStartLocal();
        const Vec2 worldEnd = center + pGuide->GetCorrectionLineEndLocal();
        if (LengthVec2(worldEnd - worldStart) <= 1.f)
            continue;

        const Vec2 lineCenter = (worldStart + worldEnd) * 0.5f;
        pLineObject->Transform()->SetRelativePos(Vec3(lineCenter.x, lineCenter.y, 10.f));
        pLineSurface->ConfigureLine(worldStart - lineCenter,
                                    worldEnd - lineCenter,
                                    CSurfaceScript::SURFACE_ROLE::SURFACE,
                                    false,
                                    true);
    }

    const auto isLinkedCorrectionLineName = [&](const wstring& _Name)
    {
        for (size_t i = 0; i < vecLinkedNames.size(); ++i)
        {
            if (vecLinkedNames[i] == _Name)
                return true;
        }

        return false;
    };

    const auto hasCorrectionLineSuffix = [](const wstring& _Name)
    {
        static const wstring suffix = L"_CorrectionLine";
        if (_Name.length() < suffix.length())
            return false;

        return (_Name.compare(_Name.length() - suffix.length(), suffix.length(), suffix) == 0);
    };

    for (size_t i = 0; i < vecSurfaces.size(); ++i)
    {
        if (vecSurfaces[i] == nullptr)
            continue;

        auto pSurface = vecSurfaces[i]->GetScript<CSurfaceScript>();
        if (pSurface == nullptr ||
            pSurface->GetGeometry() != CSurfaceScript::SURFACE_GEOMETRY::LINE)
        {
            continue;
        }

        const wstring& lineName = vecSurfaces[i]->GetName();
        if (!hasCorrectionLineSuffix(lineName) || isLinkedCorrectionLineName(lineName))
            continue;

        vecSurfaces[i]->Destroy();
    }
}

void MapEditorUI::BuildQuarterSurfaceBounds(const Vec2& _StartWorld, const Vec2& _EndWorld, bool _UseCircleRadius, Vec2& _OutMinBox, Vec2& _OutMaxBox, Vec2& _OutCenter, float& _OutRadius) const
{
    Vec2 leftTop = Vec2(min(_StartWorld.x, _EndWorld.x), max(_StartWorld.y, _EndWorld.y));
    Vec2 rightBottom = Vec2(max(_StartWorld.x, _EndWorld.x), min(_StartWorld.y, _EndWorld.y));

    float width = rightBottom.x - leftTop.x;
    float height = leftTop.y - rightBottom.y;
    float side = min(fabsf(width), fabsf(height));

    if (side <= 0.001f)
        side = max(fabsf(width), fabsf(height));

    if (_UseCircleRadius && m_UseCircleRadius)
        side = max(1.f, m_CircleRadius);

    if (side <= 0.001f)
        side = 1.f;

    switch (m_ArcCorner)
    {
    case ARC_CORNER::TOP_LEFT:
        _OutMinBox = Vec2(leftTop.x, leftTop.y - side);
        _OutMaxBox = Vec2(leftTop.x + side, leftTop.y);
        _OutCenter = Vec2(_OutMinBox.x, _OutMaxBox.y);
        break;
    case ARC_CORNER::TOP_RIGHT:
        _OutMinBox = Vec2(rightBottom.x - side, leftTop.y - side);
        _OutMaxBox = Vec2(rightBottom.x, leftTop.y);
        _OutCenter = Vec2(_OutMaxBox.x, _OutMaxBox.y);
        break;
    case ARC_CORNER::BOTTOM_LEFT:
        _OutMinBox = Vec2(leftTop.x, rightBottom.y);
        _OutMaxBox = Vec2(leftTop.x + side, rightBottom.y + side);
        _OutCenter = Vec2(_OutMinBox.x, _OutMinBox.y);
        break;
    case ARC_CORNER::BOTTOM_RIGHT:
    default:
        _OutMinBox = Vec2(rightBottom.x - side, rightBottom.y);
        _OutMaxBox = Vec2(rightBottom.x, rightBottom.y + side);
        _OutCenter = Vec2(_OutMaxBox.x, _OutMinBox.y);
        break;
    }

    _OutRadius = side;
}

void MapEditorUI::BuildFullCircleBounds(const Vec2& _StartWorld, const Vec2& _EndWorld,
                                        Vec2& _OutMinBox, Vec2& _OutMaxBox, Vec2& _OutCenter, float& _OutRadius) const
{
    _OutCenter = _StartWorld;

    float radius = 0.f;
    if (m_UseCircleRadius)
        radius = max(1.f, m_CircleRadius);
    else
        radius = LengthVec2(_EndWorld - _StartWorld);

    if (radius <= 0.001f)
        radius = 1.f;

    _OutRadius = radius;
    _OutMinBox = _OutCenter - Vec2(radius, radius);
    _OutMaxBox = _OutCenter + Vec2(radius, radius);
}

void MapEditorUI::CreateLineSurfaceObject(const Vec2& _StartWorld, const Vec2& _EndWorld)
{
    Ptr<GameObject> pRoot = EnsureSurfaceRoot();
    if (pRoot == nullptr)
        return;

    GameObject* pSurface = new GameObject;
    pSurface->SetName(L"Surface");
    pSurface->AddComponent(new CTransform);
    pSurface->AddComponent(new CCollider2D);

    CSurfaceScript* pScript = new CSurfaceScript;
    pSurface->AddComponent(pScript);

    Vec2 center = (_StartWorld + _EndWorld) * 0.5f;
    pSurface->Transform()->SetRelativePos(Vec3(center.x, center.y, 10.f));

    Vec2 localStart = _StartWorld - center;
    Vec2 localEnd = _EndWorld - center;

    CSurfaceScript::SURFACE_ROLE role = CSurfaceScript::SURFACE_ROLE::SURFACE;
    if (m_FreeformRole == FREEFORM_ROLE::CORRECTION)
        role = CSurfaceScript::SURFACE_ROLE::CORRECTION;
    else if (m_FreeformRole == FREEFORM_ROLE::WALL)
        role = CSurfaceScript::SURFACE_ROLE::WALL;
    else if (m_FreeformRole == FREEFORM_ROLE::VERTICAL_ENTRY)
        role = CSurfaceScript::SURFACE_ROLE::VERTICAL_ENTRY;

    pScript->ConfigureLine(localStart, localEnd, role, m_FillAbove, m_Attachable);

    pRoot->AddChild(pSurface);
    m_HasLastCreatedSegment = true;
    m_LastCreatedStart = _StartWorld;
    m_LastCreatedEnd = _EndWorld;
    m_StatusText = "Line surface created from the clicked world endpoints.";
}

void MapEditorUI::CreateArcSurfaceObject(const Vec2& _StartWorld, const Vec2& _EndWorld)
{
    Ptr<GameObject> pRoot = EnsureSurfaceRoot();
    if (pRoot == nullptr)
        return;

    GameObject* pSurface = new GameObject;
    pSurface->SetName(L"SurfaceArc");
    pSurface->AddComponent(new CTransform);
    pSurface->AddComponent(new CCollider2D);

    CSurfaceScript* pScript = new CSurfaceScript;
    pSurface->AddComponent(pScript);

    Vec2 minBox = {};
    Vec2 maxBox = {};
    Vec2 center = {};
    float radius = 0.f;
    BuildQuarterSurfaceBounds(_StartWorld, _EndWorld, false, minBox, maxBox, center, radius);
    pSurface->Transform()->SetRelativePos(Vec3(center.x, center.y, 10.f));

    Vec2 localStart = minBox - center;
    Vec2 localEnd = maxBox - center;

    CSurfaceScript::SURFACE_ROLE role = CSurfaceScript::SURFACE_ROLE::SURFACE;
    if (m_FreeformRole == FREEFORM_ROLE::CORRECTION)
        role = CSurfaceScript::SURFACE_ROLE::CORRECTION;
    else if (m_FreeformRole == FREEFORM_ROLE::WALL)
        role = CSurfaceScript::SURFACE_ROLE::WALL;
    else if (m_FreeformRole == FREEFORM_ROLE::VERTICAL_ENTRY)
        role = CSurfaceScript::SURFACE_ROLE::VERTICAL_ENTRY;

    pScript->ConfigureArc(localStart, localEnd, role, (CSurfaceScript::ARC_CORNER)(int)m_ArcCorner, m_ArcFillInside, m_Attachable);

    pRoot->AddChild(pSurface);
    m_HasLastCreatedSegment = true;
    m_LastCreatedStart = minBox;
    m_LastCreatedEnd = maxBox;
    m_StatusText = "Arc surface created from the clicked world span.";
}

void MapEditorUI::CreateCircleSurfaceObject(const Vec2& _StartWorld, const Vec2& _EndWorld)
{
    Ptr<GameObject> pRoot = EnsureSurfaceRoot();
    if (pRoot == nullptr)
        return;

    GameObject* pSurface = new GameObject;
    pSurface->SetName(L"SurfaceCircle");
    pSurface->AddComponent(new CTransform);
    pSurface->AddComponent(new CCollider2D);

    CSurfaceScript* pScript = new CSurfaceScript;
    pSurface->AddComponent(pScript);

    Vec2 minBox = {};
    Vec2 maxBox = {};
    Vec2 center = {};
    float radius = 0.f;
    BuildQuarterSurfaceBounds(_StartWorld, _EndWorld, true, minBox, maxBox, center, radius);
    pSurface->Transform()->SetRelativePos(Vec3(center.x, center.y, 10.f));

    Vec2 localStart = minBox - center;
    Vec2 localEnd = maxBox - center;

    CSurfaceScript::SURFACE_ROLE role = CSurfaceScript::SURFACE_ROLE::SURFACE;
    if (m_FreeformRole == FREEFORM_ROLE::CORRECTION)
        role = CSurfaceScript::SURFACE_ROLE::CORRECTION;
    else if (m_FreeformRole == FREEFORM_ROLE::WALL)
        role = CSurfaceScript::SURFACE_ROLE::WALL;
    else if (m_FreeformRole == FREEFORM_ROLE::VERTICAL_ENTRY)
        role = CSurfaceScript::SURFACE_ROLE::VERTICAL_ENTRY;

    pScript->ConfigureCircle(localStart, localEnd, role, (CSurfaceScript::ARC_CORNER)(int)m_ArcCorner, m_ArcFillInside, m_Attachable);

    if (m_ArcFillInside && role == CSurfaceScript::SURFACE_ROLE::SURFACE)
    {
        CSurfaceCircleGuideScript* pGuide = new CSurfaceCircleGuideScript;
        pGuide->SetEnabled(true);
        pGuide->ResetToDefaultGuide(radius);
        pSurface->AddComponent(pGuide);
    }

    pRoot->AddChild(pSurface);

    const bool createdGuideLine = (m_ArcFillInside && role == CSurfaceScript::SURFACE_ROLE::SURFACE);
    if (createdGuideLine)
        CreateOrUpdateCircleGuideCorrectionLine(pSurface);

    m_HasLastCreatedSegment = true;
    m_LastCreatedStart = minBox;
    m_LastCreatedEnd = maxBox;
    if (createdGuideLine)
    {
        m_StatusText = m_UseCircleRadius
            ? "Quarter circle and inside correction line created with explicit radius."
            : "Quarter circle and inside correction line created from the clicked span.";
    }
    else
    {
        m_StatusText = m_UseCircleRadius
            ? "Quarter circle created with explicit radius."
            : "Quarter circle created from the clicked span.";
    }
}

void MapEditorUI::CreateFullCircleSurfaceObject(const Vec2& _StartWorld, const Vec2& _EndWorld)
{
    Ptr<GameObject> pRoot = EnsureSurfaceRoot();
    if (pRoot == nullptr)
        return;

    Vec2 minBox = {};
    Vec2 maxBox = {};
    Vec2 center = {};
    float radius = 0.f;
    BuildFullCircleBounds(_StartWorld, _EndWorld, minBox, maxBox, center, radius);
    if (radius <= 0.001f)
        return;

    CSurfaceScript::SURFACE_ROLE role = CSurfaceScript::SURFACE_ROLE::SURFACE;
    if (m_FreeformRole == FREEFORM_ROLE::CORRECTION)
        role = CSurfaceScript::SURFACE_ROLE::CORRECTION;
    else if (m_FreeformRole == FREEFORM_ROLE::WALL)
        role = CSurfaceScript::SURFACE_ROLE::WALL;
    else if (m_FreeformRole == FREEFORM_ROLE::VERTICAL_ENTRY)
        role = CSurfaceScript::SURFACE_ROLE::VERTICAL_ENTRY;

    GameObject* pSurface = new GameObject;
    pSurface->SetName(
        L"SurfaceFullCircle_" +
        std::to_wstring((int)roundf(center.x)) + L"_" +
        std::to_wstring((int)roundf(center.y)) + L"_" +
        std::to_wstring((int)roundf(radius)));
    pSurface->AddComponent(new CTransform);
    pSurface->AddComponent(new CCollider2D);

    CSurfaceScript* pScript = new CSurfaceScript;
    pSurface->AddComponent(pScript);
    pSurface->Transform()->SetRelativePos(Vec3(center.x, center.y, 10.f));
    pScript->ConfigureFullCircle(radius, role, m_ArcFillInside, m_Attachable);

    if (m_ArcFillInside && role == CSurfaceScript::SURFACE_ROLE::SURFACE)
    {
        CSurfaceCircleGuideScript* pGuide = new CSurfaceCircleGuideScript;
        pGuide->SetEnabled(true);
        pGuide->ResetToDefaultGuide(radius);
        pSurface->AddComponent(pGuide);
    }

    pRoot->AddChild(pSurface);

    const bool createdGuideLine = (m_ArcFillInside && role == CSurfaceScript::SURFACE_ROLE::SURFACE);
    if (createdGuideLine)
    {
        CreateOrUpdateCircleGuideCorrectionLine(pSurface);
    }
    SelectSurfaceObject(pSurface);

    m_HasLastCreatedSegment = true;
    m_LastCreatedStart = minBox;
    m_LastCreatedEnd = maxBox;
    if (createdGuideLine)
    {
        m_StatusText = m_UseCircleRadius
            ? "Full circle and inside correction line created with explicit radius."
            : "Full circle and inside correction line created from the clicked span.";
    }
    else
    {
        m_StatusText = m_UseCircleRadius
            ? "Full circle created with explicit radius."
            : "Full circle created from the clicked span.";
    }
}

void MapEditorUI::DeleteNearestSurfaceObject(const Vec2& _WorldPos)
{
    float bestDist = FLT_MAX;
    Ptr<GameObject> pBest = FindNearestSurfaceObject(_WorldPos, bestDist);

    if (pBest != nullptr && bestDist <= 48.f)
    {
        auto pGuide = pBest->GetScript<CSurfaceCircleGuideScript>();
        if (pGuide != nullptr && !pGuide->GetLinkedCorrectionLineName().empty())
        {
            Ptr<GameObject> pLinkedLine = FindSurfaceObjectByName(pGuide->GetLinkedCorrectionLineName());
            if (pLinkedLine != nullptr && pLinkedLine.Get() != pBest.Get())
                pLinkedLine->Destroy();
        }

        pBest->Destroy();
        RefreshCircleGuideCorrectionLines();
        m_StatusText = "Nearest surface deleted.";
    }
    else
    {
        m_StatusText = "No nearby surface to delete.";
    }
}

Ptr<GameObject> MapEditorUI::ResolveTargetObject() const
{
    Ptr<Inspector> pInspector = (Inspector*)EditorMgr::GetInst()->FindUI("Inspector").Get();
    if (pInspector != nullptr)
    {
        Ptr<GameObject> pSelected = pInspector->GetTargetObject();
        if (pSelected != nullptr && pSelected->TileRender() != nullptr)
            return pSelected;
    }

    Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();
    if (pLevel == nullptr)
        return nullptr;

    return pLevel->FindObjectByName(L"TileMapRender");
}

void MapEditorUI::ResetWorkingSet(int _Row, int _Col)
{
    m_Row = max(1, _Row);
    m_Col = max(1, _Col);
    m_TileValues.assign((size_t)m_Row * (size_t)m_Col, (int)TILETYPE::EMPTY_BLOCK);
    m_CellKinds.assign((size_t)m_Row * (size_t)m_Col, (unsigned char)CELL_KIND::NONE);
}

bool MapEditorUI::EnsureWorkingSetLoaded()
{
    Ptr<GameObject> pTarget = ResolveTargetObject();
    if (pTarget == nullptr || pTarget->TileRender() == nullptr)
        return false;

    Ptr<ATileMap> pTileMap = pTarget->TileRender()->GetTileMap();
    if (pTileMap == nullptr)
        return false;

    if (m_LastTargetObject == pTarget.Get() && !m_TileValues.empty())
        return true;

    m_LastTargetObject = pTarget.Get();
    m_TileSize = pTileMap->GetTileSize();

    UINT row = pTileMap->GetRow();
    UINT col = pTileMap->GetCol();
    vector<int> tileValues;

    if (CTileScript::LoadTileScriptPreset(GetPresetPath(), row, col, tileValues, true))
    {
        m_Row = (int)row;
        m_Col = (int)col;
        m_TileValues = tileValues;
    }
    else
    {
        if (0 == row || 0 == col)
        {
            row = kDefaultRow;
            col = kDefaultCol;
        }

        ResetWorkingSet((int)row, (int)col);
    }

    if (m_TileValues.size() != (size_t)m_Row * (size_t)m_Col)
    {
        ResetWorkingSet(m_Row, m_Col);
    }

    if (!LoadMetaFile(GetMetaPath(), m_Row, m_Col, m_CellKinds))
    {
        m_CellKinds.assign((size_t)m_Row * (size_t)m_Col, (unsigned char)CELL_KIND::NONE);
    }

    if (m_TileSize.x <= 0.f || m_TileSize.y <= 0.f)
    {
        m_TileSize = Vec2(kDefaultTileSize, kDefaultTileSize);
    }

    return true;
}

void MapEditorUI::LoadWorkingSetFromDisk()
{
    m_LastTargetObject = nullptr;
    EnsureWorkingSetLoaded();
    m_StatusText = "Preset reloaded from disk.";
}

void MapEditorUI::SaveWorkingSetToDisk()
{
    if (m_Row <= 0 || m_Col <= 0)
        return;

    std::filesystem::create_directories(std::filesystem::path(GetPresetPath()).parent_path());

    CTileScript::SaveTileScriptPreset(GetPresetPath(), (UINT)m_Row, (UINT)m_Col, m_TileValues);
    SaveMetaFile(GetMetaPath(), m_Row, m_Col, m_CellKinds);

    Ptr<GameObject> pTarget = ResolveTargetObject();
    if (pTarget != nullptr && pTarget->TileRender() != nullptr)
    {
        Ptr<ATileMap> pTileMap = pTarget->TileRender()->GetTileMap();
        if (pTileMap != nullptr)
        {
            pTileMap->Save(GetTileMapAssetPath());
        }
    }
}

bool MapEditorUI::CreateFreshTileMap()
{
    Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();
    if (pLevel == nullptr)
    {
        m_StatusText = "No level is loaded.";
        return false;
    }

    Ptr<ATexture> pAtlas = FIND(ATexture, L"MapTest");
    if (pAtlas == nullptr)
    {
        m_StatusText = "MapTest atlas is missing.";
        return false;
    }

    Ptr<ATileMap> pTileMap = FIND(ATileMap, L"TestTileMap");
    if (pTileMap == nullptr)
    {
        pTileMap = new ATileMap;
        pTileMap->SetName(L"TestTileMap");
        AssetMgr::GetInst()->AddAsset(L"TestTileMap", pTileMap.Get());
    }

    pTileMap->SetAtlas(pAtlas);
    pTileMap->SetTileSize(m_TileSize);
    pTileMap->SetRowCol((UINT)m_Row, (UINT)m_Col);

    Ptr<ASprite> pEmptySprite = AssetMgr::GetInst()->Load<ASprite>(L"MapTest_0", L"Sprite\\MapTest_0.sprite");
    if (pEmptySprite != nullptr)
    {
        for (int row = 0; row < m_Row; ++row)
        {
            for (int col = 0; col < m_Col; ++col)
            {
                pTileMap->SetSprite((UINT)row, (UINT)col, pEmptySprite);
            }
        }
    }

    Ptr<GameObject> pTileMapObj = pLevel->FindObjectByName(L"TileMapRender");
    if (pTileMapObj == nullptr)
    {
        pTileMapObj = new GameObject;
        pTileMapObj->SetName(L"TileMapRender");
        pTileMapObj->AddComponent(new CTransform);
        pTileMapObj->AddComponent(new CTileRender);

        CTileScript::TILE_MAP_PLACEMENT placement = {};
        CTileScript::GetTileMapPlacement(placement);
        pTileMapObj->Transform()->SetRelativePos(Vec3(placement.map_pos_x, placement.map_pos_y, placement.map_pos_z));
        pLevel->AddObject(2, pTileMapObj);
    }

    pTileMapObj->TileRender()->SetTileMap(pTileMap);
    pTileMapObj->TileRender()->SetOpacity(0.45f);

    ResetWorkingSet(m_Row, m_Col);
    ApplyWorkingSetToScene();
    SaveWorkingSetToDisk();

    m_StatusText = "Created fresh grid-based TileMapRender.";
    return true;
}

void MapEditorUI::SetCell(int _Row, int _Col, int _TypeValue, CELL_KIND _Kind)
{
    if (!IsCellInRange(_Row, _Col))
        return;

    const int idx = CellIndex(_Row, _Col, m_Col);
    m_TileValues[idx] = _TypeValue;
    m_CellKinds[idx] = (unsigned char)_Kind;
}

void MapEditorUI::ClearCell(int _Row, int _Col)
{
    if (!IsCellInRange(_Row, _Col))
        return;

    const int idx = CellIndex(_Row, _Col, m_Col);
    m_TileValues[idx] = (int)TILETYPE::EMPTY_BLOCK;
    m_CellKinds[idx] = (unsigned char)CELL_KIND::NONE;
}

bool MapEditorUI::IsCellInRange(int _Row, int _Col) const
{
    return 0 <= _Row && _Row < m_Row && 0 <= _Col && _Col < m_Col;
}

int MapEditorUI::FindOrCreateLineType(float _A, float _B, float _C)
{
    CTileScript::TILE_FORMULA_CONFIG wanted = {};
    wanted.a = Clamp01(_A);
    wanted.b = Clamp01(_B);
    wanted.c = _C;
    wanted.r = 0.8f;
    wanted.center_x = 0.5f;
    wanted.center_y = 0.5f;

    vector<int> editableTypes;
    CTileScript::GetEditableTileTypeValues(editableTypes, false, true);

    for (size_t i = 0; i < editableTypes.size(); ++i)
    {
        const int typeValue = editableTypes[i];
        if (!CTileScript::IsLineTileTypeValue(typeValue))
            continue;

        CTileScript::TILE_FORMULA_CONFIG existing = {};
        if (CTileScript::GetTileFormulaConfigByValue(typeValue, existing) && SameShape(existing, wanted))
        {
            return typeValue;
        }
    }

    TILETYPE newType = TILETYPE::EMPTY_BLOCK;
    if (CTileScript::CreateCustomTileType(false, TILETYPE::LINE_BLOCK_3, newType))
    {
        CTileScript::SetTileFormulaConfig(newType, wanted);
        return (int)newType;
    }

    m_StatusText = "No free custom line slots left. Using LINE_3 preview.";
    return (int)TILETYPE::LINE_BLOCK_3;
}

int MapEditorUI::FindOrCreateCircleType(float _Radius, float _CenterX, float _CenterY, float _CSign)
{
    CTileScript::TILE_FORMULA_CONFIG wanted = {};
    wanted.a = 0.8f;
    wanted.b = 0.8f;
    wanted.c = _CSign;
    wanted.r = _Radius;
    wanted.center_x = _CenterX;
    wanted.center_y = _CenterY;

    vector<int> editableTypes;
    CTileScript::GetEditableTileTypeValues(editableTypes, false, true);

    for (size_t i = 0; i < editableTypes.size(); ++i)
    {
        const int typeValue = editableTypes[i];
        if (!CTileScript::IsCircleTileTypeValue(typeValue))
            continue;

        CTileScript::TILE_FORMULA_CONFIG existing = {};
        if (CTileScript::GetTileFormulaConfigByValue(typeValue, existing) && SameShape(existing, wanted))
        {
            return typeValue;
        }
    }

    TILETYPE newType = TILETYPE::EMPTY_BLOCK;
    if (CTileScript::CreateCustomTileType(true, TILETYPE::CIRCLE_BLOCK_1, newType))
    {
        CTileScript::SetTileFormulaConfig(newType, wanted);
        return (int)newType;
    }

    m_StatusText = "No free custom circle slots left. Using CIRCLE_1 preview.";
    return (int)TILETYPE::CIRCLE_BLOCK_1;
}

void MapEditorUI::ApplyLineStroke(const GRID_VERTEX& _Start, const GRID_VERTEX& _End, CELL_KIND _Kind)
{
    int leftCol = min(_Start.x, _End.x);
    int rightCol = max(_Start.x, _End.x);
    if (leftCol == rightCol)
    {
        m_StatusText = "Surface / correction stroke needs different X endpoints. Use Wall for vertical fills.";
        return;
    }

    for (int col = leftCol; col < rightCol; ++col)
    {
        const float yLeft = EvalLineY(_Start, _End, (float)col);
        const float yRight = EvalLineY(_Start, _End, (float)(col + 1));

        for (int row = 0; row < m_Row; ++row)
        {
            const float top = (float)row;
            const float bottom = (float)(row + 1);

            const float cornerY[4] = { top, top, bottom, bottom };
            const float lineY[4] = { yLeft, yRight, yLeft, yRight };

            int insideCount = 0;
            for (int i = 0; i < 4; ++i)
            {
                const bool inside = m_FillAbove ? (cornerY[i] <= lineY[i] + 0.0001f)
                                                : (cornerY[i] >= lineY[i] - 0.0001f);
                if (inside)
                    ++insideCount;
            }

            if (insideCount == 0)
                continue;

            if (insideCount == 4)
            {
                SetCell(row, col, (int)TILETYPE::FULL_BLOCK, _Kind);
                continue;
            }

            const float localA = Clamp01(yLeft - (float)row);
            const float localB = Clamp01(yRight - (float)row);
            const float sign = m_FillAbove ? 1.f : -1.f;
            SetCell(row, col, FindOrCreateLineType(localA, localB, sign), _Kind);
        }
    }

    m_StatusText = (_Kind == CELL_KIND::CORRECTION)
        ? "Correction stroke baked from clicked endpoints."
        : "Surface stroke baked from clicked endpoints.";
}

void MapEditorUI::ApplyArcStroke(const GRID_VERTEX& _Start, const GRID_VERTEX& _End)
{
    const int minX = min(_Start.x, _End.x);
    const int maxX = max(_Start.x, _End.x);
    const int minY = min(_Start.y, _End.y);
    const int maxY = max(_Start.y, _End.y);

    const int width = maxX - minX;
    const int height = maxY - minY;
    const int side = min(width, height);

    if (side <= 0)
    {
        m_StatusText = "Arc stroke needs a square-like span.";
        return;
    }

    int left = minX;
    int right = minX + side;
    int top = minY;
    int bottom = minY + side;

    switch (m_ArcCorner)
    {
    case ARC_CORNER::TOP_LEFT:
        left = minX;     right = minX + side;
        top = minY;      bottom = minY + side;
        break;
    case ARC_CORNER::TOP_RIGHT:
        right = maxX;    left = maxX - side;
        top = minY;      bottom = minY + side;
        break;
    case ARC_CORNER::BOTTOM_LEFT:
        left = minX;     right = minX + side;
        bottom = maxY;   top = maxY - side;
        break;
    case ARC_CORNER::BOTTOM_RIGHT:
        right = maxX;    left = maxX - side;
        bottom = maxY;   top = maxY - side;
        break;
    }

    float centerX = 0.f;
    float centerY = 0.f;

    switch (m_ArcCorner)
    {
    case ARC_CORNER::TOP_LEFT:
        centerX = (float)left;
        centerY = (float)top;
        break;
    case ARC_CORNER::TOP_RIGHT:
        centerX = (float)right;
        centerY = (float)top;
        break;
    case ARC_CORNER::BOTTOM_LEFT:
        centerX = (float)left;
        centerY = (float)bottom;
        break;
    case ARC_CORNER::BOTTOM_RIGHT:
        centerX = (float)right;
        centerY = (float)bottom;
        break;
    }

    const float radius = (float)side;
    const float sign = m_ArcFillInside ? 1.f : -1.f;

    for (int row = top; row < bottom; ++row)
    {
        for (int col = left; col < right; ++col)
        {
            if (!IsCellInRange(row, col))
                continue;

            const float cornerX[4] = { (float)col, (float)(col + 1), (float)col, (float)(col + 1) };
            const float cornerY[4] = { (float)row, (float)row, (float)(row + 1), (float)(row + 1) };

            int insideCount = 0;
            for (int i = 0; i < 4; ++i)
            {
                const float dx = cornerX[i] - centerX;
                const float dy = cornerY[i] - centerY;
                const float dist2 = dx * dx + dy * dy;
                const bool inside = m_ArcFillInside ? (dist2 <= radius * radius + 0.0001f)
                                                    : (dist2 >= radius * radius - 0.0001f);
                if (inside)
                    ++insideCount;
            }

            if (insideCount == 0)
                continue;

            if (insideCount == 4)
            {
                SetCell(row, col, (int)TILETYPE::FULL_BLOCK, CELL_KIND::SURFACE);
                continue;
            }

            const float localCenterX = centerX - (float)col;
            const float localCenterY = centerY - (float)row;
            SetCell(row, col, FindOrCreateCircleType(radius, localCenterX, localCenterY, sign), CELL_KIND::SURFACE);
        }
    }

    m_StatusText = "Arc stroke baked from the selected square span.";
}

void MapEditorUI::ApplyWallStroke(const GRID_VERTEX& _Start, const GRID_VERTEX& _End)
{
    int left = min(_Start.x, _End.x);
    int right = max(_Start.x, _End.x);
    int top = min(_Start.y, _End.y);
    int bottom = max(_Start.y, _End.y);

    if (left == right)
    {
        if (m_WallSide == WALL_SIDE::LEFT)
            --left;
        else
            ++right;
    }

    if (top == bottom)
    {
        if (m_WallSide == WALL_SIDE::ABOVE)
            --top;
        else
            ++bottom;
    }

    left = ClampInt(left, 0, m_Col);
    right = ClampInt(right, 0, m_Col);
    top = ClampInt(top, 0, m_Row);
    bottom = ClampInt(bottom, 0, m_Row);

    for (int row = top; row < bottom; ++row)
    {
        for (int col = left; col < right; ++col)
        {
            SetCell(row, col, (int)TILETYPE::FULL_BLOCK, CELL_KIND::WALL);
        }
    }

    m_StatusText = "Wall stroke baked as solid full cells.";
}

void MapEditorUI::ApplyEraseStroke(const GRID_VERTEX& _Start, const GRID_VERTEX& _End)
{
    int left = min(_Start.x, _End.x);
    int right = max(_Start.x, _End.x);
    int top = min(_Start.y, _End.y);
    int bottom = max(_Start.y, _End.y);

    if (left == right)
        right = min(m_Col, right + 1);
    if (top == bottom)
        bottom = min(m_Row, bottom + 1);

    for (int row = top; row < bottom; ++row)
    {
        for (int col = left; col < right; ++col)
        {
            ClearCell(row, col);
        }
    }

    m_StatusText = "Selected cells were erased.";
}

void MapEditorUI::ApplyStroke(const GRID_VERTEX& _Start, const GRID_VERTEX& _End)
{
    switch (m_Tool)
    {
    case MAP_TOOL::SURFACE_LINE:
        ApplyLineStroke(_Start, _End, CELL_KIND::SURFACE);
        break;
    case MAP_TOOL::ARC:
        ApplyArcStroke(_Start, _End);
        break;
    case MAP_TOOL::WALL:
        ApplyWallStroke(_Start, _End);
        break;
    case MAP_TOOL::CORRECTION:
        ApplyLineStroke(_Start, _End, CELL_KIND::CORRECTION);
        break;
    case MAP_TOOL::ERASE:
        ApplyEraseStroke(_Start, _End);
        break;
    }

    ApplyWorkingSetToScene();
    if (m_AutoSave)
        SaveWorkingSetToDisk();
}

void MapEditorUI::RebuildCollisionChildren(GameObject* _TileMapObject)
{
    if (nullptr == _TileMapObject || nullptr == _TileMapObject->TileRender())
        return;

    const vector<Ptr<GameObject>>& vecChild = _TileMapObject->GetChild();
    for (size_t i = 0; i < vecChild.size(); ++i)
    {
        if (vecChild[i] == nullptr)
            continue;

        if (vecChild[i]->GetScript<CTileScript>() != nullptr)
            vecChild[i]->Destroy();
    }

    CTileScript::TILE_MAP_PLACEMENT placement = {};
    CTileScript::GetTileMapPlacement(placement);

    const float tileW = m_TileSize.x;
    const float tileH = m_TileSize.y;
    const float startLocalX = tileW * 0.5f + placement.collision_offset_x;
    const float startLocalY = -tileH * 0.5f + placement.collision_offset_y;
    const float localZ = 10.f - _TileMapObject->Transform()->GetRelativePos().z;

    for (int row = 0; row < m_Row; ++row)
    {
        for (int col = 0; col < m_Col; ++col)
        {
            const int typeValue = m_TileValues[CellIndex(row, col, m_Col)];
            if (!CTileScript::IsValidTileTypeValue(typeValue))
                continue;
            if (typeValue == (int)TILETYPE::EMPTY_BLOCK)
                continue;

            unsigned char fullBlockFaceMask = CTileScript::FULL_FACE_ALL;
            if (typeValue == (int)TILETYPE::FULL_BLOCK)
            {
                fullBlockFaceMask = CTileScript::FULL_FACE_NONE;

                const bool emptyAbove = (row == 0) || (m_TileValues[CellIndex(row - 1, col, m_Col)] == (int)TILETYPE::EMPTY_BLOCK);
                const bool emptyBelow = (row == m_Row - 1) || (m_TileValues[CellIndex(row + 1, col, m_Col)] == (int)TILETYPE::EMPTY_BLOCK);
                const bool emptyLeft = (col == 0) || (m_TileValues[CellIndex(row, col - 1, m_Col)] == (int)TILETYPE::EMPTY_BLOCK);
                const bool emptyRight = (col == m_Col - 1) || (m_TileValues[CellIndex(row, col + 1, m_Col)] == (int)TILETYPE::EMPTY_BLOCK);

                if (emptyAbove)
                    fullBlockFaceMask |= CTileScript::FULL_FACE_TOP;
                if (emptyBelow)
                    fullBlockFaceMask |= CTileScript::FULL_FACE_BOTTOM;
                if (emptyLeft)
                    fullBlockFaceMask |= CTileScript::FULL_FACE_LEFT;
                if (emptyRight)
                    fullBlockFaceMask |= CTileScript::FULL_FACE_RIGHT;

                // Interior solid cells should not spawn their own collider, otherwise
                // adjacent full tiles stack corrective pushes and can fling the player.
                if (fullBlockFaceMask == CTileScript::FULL_FACE_NONE)
                    continue;
            }

            GameObject* pTileObj = new GameObject;
            pTileObj->SetName(L"Tile");
            pTileObj->AddComponent(new CTransform);
            pTileObj->AddComponent(new CCollider2D);
            pTileObj->Transform()->SetIndependentScale(true);
            pTileObj->Transform()->SetRelativePos(
                Vec3(startLocalX + col * tileW, startLocalY - row * tileH, localZ));
            pTileObj->Transform()->SetRelativeScale(Vec3(tileW, tileH, 1.f));

            pTileObj->Collider2D()->SetOffset(Vec2(0.f, 0.f));
            pTileObj->Collider2D()->SetScale(Vec2(1.f, 1.f));

            CTileScript* pTileScript = new CTileScript;
            pTileObj->AddComponent(pTileScript);
            pTileScript->TileMapSetting(typeValue);
            if (typeValue == (int)TILETYPE::FULL_BLOCK)
                pTileScript->SetFullBlockFaceMask(fullBlockFaceMask);

            _TileMapObject->AddChild(pTileObj);
        }
    }
}

void MapEditorUI::ApplyWorkingSetToScene(bool _RebuildCollision)
{
    Ptr<GameObject> pTarget = ResolveTargetObject();
    if (pTarget == nullptr || pTarget->TileRender() == nullptr)
        return;

    Ptr<ATileMap> pTileMap = pTarget->TileRender()->GetTileMap();
    if (pTileMap == nullptr)
        return;

    if (pTileMap->GetAtlas() == nullptr)
    {
        pTileMap->SetAtlas(FIND(ATexture, L"MapTest"));
    }

    pTileMap->SetRowCol((UINT)m_Row, (UINT)m_Col);
    pTileMap->SetTileSize(m_TileSize);

    CTileScript::TILE_MAP_PLACEMENT placement = {};
    CTileScript::GetTileMapPlacement(placement);
    placement.map_pos_x = pTarget->Transform()->GetRelativePos().x;
    placement.map_pos_y = pTarget->Transform()->GetRelativePos().y;
    placement.map_pos_z = pTarget->Transform()->GetRelativePos().z;
    CTileScript::SetTileMapPlacement(placement);

    for (int row = 0; row < m_Row; ++row)
    {
        for (int col = 0; col < m_Col; ++col)
        {
            int typeValue = m_TileValues[CellIndex(row, col, m_Col)];
            int previewType = typeValue;

            if (typeValue == (int)TILETYPE::FULL_BLOCK)
            {
                previewType = (int)TILETYPE::LINE_BLOCK_6;
            }
            else if (CTileScript::IsLineTileTypeValue(typeValue) && typeValue > (int)TILETYPE::LINE_BLOCK_6)
            {
                CTileScript::TILE_FORMULA_CONFIG cfg = {};
                CTileScript::GetTileFormulaConfigByValue(typeValue, cfg);

                struct Candidate
                {
                    int TypeValue;
                    CTileScript::TILE_FORMULA_CONFIG Config;
                };

                Candidate builtins[6] =
                {
                    { (int)TILETYPE::LINE_BLOCK_1, {} },
                    { (int)TILETYPE::LINE_BLOCK_2, {} },
                    { (int)TILETYPE::LINE_BLOCK_3, {} },
                    { (int)TILETYPE::LINE_BLOCK_4, {} },
                    { (int)TILETYPE::LINE_BLOCK_5, {} },
                    { (int)TILETYPE::LINE_BLOCK_6, {} },
                };

                float bestScore = FLT_MAX;
                previewType = (int)TILETYPE::LINE_BLOCK_3;

                for (int i = 0; i < 6; ++i)
                {
                    CTileScript::GetTileFormulaConfigByValue(builtins[i].TypeValue, builtins[i].Config);
                    const float diff = fabsf(builtins[i].Config.a - cfg.a) + fabsf(builtins[i].Config.b - cfg.b);
                    if (diff < bestScore)
                    {
                        bestScore = diff;
                        previewType = builtins[i].TypeValue;
                    }
                }
            }
            else if (CTileScript::IsCircleTileTypeValue(typeValue) && typeValue > (int)TILETYPE::CIRCLE_BLOCK_4)
            {
                CTileScript::TILE_FORMULA_CONFIG cfg = {};
                CTileScript::GetTileFormulaConfigByValue(typeValue, cfg);
                previewType = (cfg.center_x >= 0.5f)
                    ? ((cfg.center_y >= 0.5f) ? (int)TILETYPE::CIRCLE_BLOCK_1 : (int)TILETYPE::CIRCLE_BLOCK_3)
                    : ((cfg.center_y >= 0.5f) ? (int)TILETYPE::CIRCLE_BLOCK_2 : (int)TILETYPE::CIRCLE_BLOCK_4);
            }

            int spriteIdx = max(0, previewType - 1);
            wchar_t key[64] = {};
            wchar_t relativePath[128] = {};
            swprintf_s(key, L"MapTest_%d", spriteIdx);
            swprintf_s(relativePath, L"Sprite\\MapTest_%d.sprite", spriteIdx);

            Ptr<ASprite> pSprite = AssetMgr::GetInst()->Load<ASprite>(key, relativePath);
            if (pSprite == nullptr)
            {
                pSprite = AssetMgr::GetInst()->Load<ASprite>(L"MapTest_0", L"Sprite\\MapTest_0.sprite");
            }

            if (pSprite != nullptr)
            {
                pTileMap->SetSprite((UINT)row, (UINT)col, pSprite);
            }
        }
    }

    pTarget->TileRender()->SetTileMap(pTileMap);
    if (_RebuildCollision)
    {
        RebuildCollisionChildren(pTarget.Get());
    }
}

bool MapEditorUI::GetMouseWorldPos(Vec2& _OutWorldPos) const
{
    Ptr<CCamera> pCamera = GetSceneCamera();
    if (pCamera == nullptr)
        return false;

    Vec2 mousePos = KeyMgr::GetInst()->GetMousePos();
    Vec2 resolution = Device::GetInst()->GetRenderResolution();
    if (resolution.x <= 0.f || resolution.y <= 0.f)
        return false;

    Vec3 camPos = pCamera->Transform()->GetRelativePos();
    const float worldWidth = pCamera->GetWidth() * pCamera->GetOrthoScale();
    const float worldHeight = (pCamera->GetWidth() / pCamera->GetAspectRatio()) * pCamera->GetOrthoScale();

    _OutWorldPos.x = camPos.x + ((mousePos.x / resolution.x) - 0.5f) * worldWidth;
    _OutWorldPos.y = camPos.y - ((mousePos.y / resolution.y) - 0.5f) * worldHeight;
    return true;
}

bool MapEditorUI::WorldToGridVertex(const Vec2& _WorldPos, GRID_VERTEX& _OutVertex) const
{
    Ptr<GameObject> pTarget = ResolveTargetObject();
    if (pTarget == nullptr || m_TileSize.x <= 0.f || m_TileSize.y <= 0.f)
        return false;

    Vec3 mapPos = pTarget->Transform()->GetWorldPos();
    const float localX = (_WorldPos.x - mapPos.x) / m_TileSize.x;
    const float localY = (mapPos.y - _WorldPos.y) / m_TileSize.y;

    if (localX < -0.5f || localX > (float)m_Col + 0.5f || localY < -0.5f || localY > (float)m_Row + 0.5f)
        return false;

    _OutVertex.x = ClampInt((int)roundf(localX), 0, m_Col);
    _OutVertex.y = ClampInt((int)roundf(localY), 0, m_Row);
    return true;
}

bool MapEditorUI::GridVertexToWorld(const GRID_VERTEX& _Vertex, Vec2& _OutWorldPos) const
{
    Ptr<GameObject> pTarget = ResolveTargetObject();
    if (pTarget == nullptr)
        return false;

    Vec3 mapPos = pTarget->Transform()->GetWorldPos();
    _OutWorldPos.x = mapPos.x + _Vertex.x * m_TileSize.x;
    _OutWorldPos.y = mapPos.y - _Vertex.y * m_TileSize.y;
    return true;
}

bool MapEditorUI::WorldToScreen(const Vec2& _WorldPos, ImVec2& _OutScreenPos) const
{
    Ptr<CCamera> pCamera = GetSceneCamera();
    if (pCamera == nullptr)
        return false;

    ImGuiViewport* pViewport = ImGui::GetMainViewport();
    if (pViewport == nullptr)
        return false;

    Vec2 resolution = Device::GetInst()->GetRenderResolution();
    Vec3 camPos = pCamera->Transform()->GetRelativePos();
    const float worldWidth = pCamera->GetWidth() * pCamera->GetOrthoScale();
    const float worldHeight = (pCamera->GetWidth() / pCamera->GetAspectRatio()) * pCamera->GetOrthoScale();

    _OutScreenPos.x = pViewport->Pos.x + (((_WorldPos.x - camPos.x) / worldWidth + 0.5f) * resolution.x);
    _OutScreenPos.y = pViewport->Pos.y + (((camPos.y - _WorldPos.y) / worldHeight + 0.5f) * resolution.y);
    return true;
}

Vec4 MapEditorUI::GetCellColor(int _Row, int _Col) const
{
    if (!IsCellInRange(_Row, _Col))
        return Vec4(0.f, 0.f, 0.f, 0.f);

    int idx = CellIndex(_Row, _Col, m_Col);
    const int typeValue = m_TileValues[idx];
    if (typeValue == (int)TILETYPE::EMPTY_BLOCK)
        return Vec4(0.f, 0.f, 0.f, 0.f);

    CELL_KIND kind = (CELL_KIND)m_CellKinds[idx];
    switch (kind)
    {
    case CELL_KIND::SURFACE:    return Vec4(0.2f, 0.75f, 1.f, 0.24f);
    case CELL_KIND::CORRECTION: return Vec4(0.2f, 1.f, 0.45f, 0.24f);
    case CELL_KIND::WALL:       return Vec4(1.f, 0.45f, 0.2f, 0.24f);
    default:                    return Vec4(0.8f, 0.8f, 0.8f, 0.16f);
    }
}

bool MapEditorUI::GetCellNormal(int _Row, int _Col, Vec2& _OutNormal) const
{
    if (!IsCellInRange(_Row, _Col))
        return false;

    const int typeValue = m_TileValues[CellIndex(_Row, _Col, m_Col)];
    if (typeValue == (int)TILETYPE::EMPTY_BLOCK)
        return false;

    if (typeValue == (int)TILETYPE::FULL_BLOCK)
    {
        const bool emptyAbove = (_Row == 0) || (m_TileValues[CellIndex(_Row - 1, _Col, m_Col)] == (int)TILETYPE::EMPTY_BLOCK);
        const bool emptyBelow = (_Row == m_Row - 1) || (m_TileValues[CellIndex(_Row + 1, _Col, m_Col)] == (int)TILETYPE::EMPTY_BLOCK);
        const bool emptyLeft = (_Col == 0) || (m_TileValues[CellIndex(_Row, _Col - 1, m_Col)] == (int)TILETYPE::EMPTY_BLOCK);
        const bool emptyRight = (_Col == m_Col - 1) || (m_TileValues[CellIndex(_Row, _Col + 1, m_Col)] == (int)TILETYPE::EMPTY_BLOCK);

        if (emptyAbove)      _OutNormal = Vec2(0.f, 1.f);
        else if (emptyBelow) _OutNormal = Vec2(0.f, -1.f);
        else if (emptyLeft)  _OutNormal = Vec2(-1.f, 0.f);
        else if (emptyRight) _OutNormal = Vec2(1.f, 0.f);
        else                 _OutNormal = Vec2(0.f, 1.f);

        return true;
    }

    CTileScript::TILE_FORMULA_CONFIG cfg = {};
    if (!CTileScript::GetTileFormulaConfigByValue(typeValue, cfg))
        return false;

    Vec2 localNormal = Vec2(0.f, 1.f);

    if (CTileScript::IsLineTileTypeValue(typeValue))
    {
        const float dx = cfg.c * (cfg.a - cfg.b);
        const float dy = cfg.c;
        const float len = sqrtf(dx * dx + dy * dy);
        if (len <= 0.0001f)
            return false;
        localNormal = Vec2(dx / len, dy / len);
    }
    else if (CTileScript::IsCircleTileTypeValue(typeValue))
    {
        const float sampleX = 0.5f;
        const float sampleY = 0.5f;
        const float dx = cfg.c * 2.f * (sampleX - cfg.center_x);
        const float dy = cfg.c * 2.f * (sampleY - cfg.center_y);
        const float len = sqrtf(dx * dx + dy * dy);
        if (len <= 0.0001f)
            return false;
        localNormal = Vec2(dx / len, dy / len);
    }
    else
    {
        return false;
    }

    Vec2 worldNormal = Vec2(localNormal.x * m_TileSize.x, -localNormal.y * m_TileSize.y);
    const float worldLen = sqrtf(worldNormal.x * worldNormal.x + worldNormal.y * worldNormal.y);
    if (worldLen <= 0.0001f)
        return false;

    _OutNormal = Vec2(worldNormal.x / worldLen, worldNormal.y / worldLen);
    return true;
}

void MapEditorUI::HandleFreeformInput()
{
    if (LevelMgr::GetInst()->GetLevelState() != LEVEL_STATE::STOP)
        return;

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsAnyItemActive())
        return;

    Vec2 mouseWorld = {};
    if (!GetMouseWorldPos(mouseWorld))
        return;

    if (HandleSelectedCircleGuideDrag(mouseWorld))
        return;

    Vec2 snappedMouseWorld = mouseWorld;
    if (m_FreeformRole != FREEFORM_ROLE::ERASE)
    {
        Vec2 snapPoint = {};
        if (FindSnapPoint(mouseWorld, snapPoint))
            snappedMouseWorld = snapPoint;
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        m_HasStartPoint = false;
        m_StatusText = "Stroke cancelled.";
        return;
    }

    if (m_FreeformRole == FREEFORM_ROLE::ERASE && ImGui::IsKeyPressed(ImGuiKey_Delete))
    {
        DeleteNearestSurfaceObject(mouseWorld);
        return;
    }

    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        return;

    if (ImGui::GetIO().KeyCtrl)
    {
        float bestDist = FLT_MAX;
        Ptr<GameObject> pNearest = FindNearestSurfaceObject(mouseWorld, bestDist);
        if (pNearest != nullptr && bestDist <= 48.f)
        {
            SelectSurfaceObject(pNearest);
            m_StatusText = "Nearest surface selected in Inspector.";
        }
        else
        {
            m_StatusText = "No nearby surface to select.";
        }
        return;
    }

    if (m_FreeformRole == FREEFORM_ROLE::ERASE)
    {
        DeleteNearestSurfaceObject(mouseWorld);
        return;
    }

    if (!m_HasStartPoint)
    {
        m_StartPoint = snappedMouseWorld;
        m_HasStartPoint = true;
        if (m_FreeformShape == FREEFORM_SHAPE::FULL_CIRCLE)
        {
            m_StatusText = m_UseCircleRadius
                ? "Center selected. Click again to place the full circle."
                : "Center selected. Click a radius point in the game view.";
        }
        else
        {
            m_StatusText = "Start point selected. Click the end point in the game view.";
        }
        return;
    }

    if (DotVec2(m_StartPoint - snappedMouseWorld, m_StartPoint - snappedMouseWorld) <= 1.f)
    {
        m_StatusText = "End point must be different.";
        return;
    }

    if (m_FreeformShape == FREEFORM_SHAPE::LINE)
        CreateLineSurfaceObject(m_StartPoint, snappedMouseWorld);
    else if (m_FreeformShape == FREEFORM_SHAPE::FLAT)
        CreateLineSurfaceObject(m_StartPoint, Vec2(snappedMouseWorld.x, m_StartPoint.y));
    else if (m_FreeformShape == FREEFORM_SHAPE::ARC)
        CreateArcSurfaceObject(m_StartPoint, snappedMouseWorld);
    else if (m_FreeformShape == FREEFORM_SHAPE::CIRCLE)
        CreateCircleSurfaceObject(m_StartPoint, snappedMouseWorld);
    else
        CreateFullCircleSurfaceObject(m_StartPoint, snappedMouseWorld);

    m_HasStartPoint = false;
}

void MapEditorUI::DrawFreeformOverlay()
{
    ImGuiViewport* pViewport = ImGui::GetMainViewport();
    if (pViewport == nullptr)
        return;

    // Draw on the main viewport instead of the editor window draw list so the
    // helper lines stay visible across the actual game view even when no ImGui
    // window is overlapping the scene.
    ImDrawList* pDraw = ImGui::GetForegroundDrawList(pViewport);

    vector<Ptr<GameObject>> vecSurfaces;
    CollectSurfaceObjects(vecSurfaces);

    vector<GameObject*> vecGuidedCircleObjects;
    vector<wstring> vecGuidedCorrectionLineNames;
    GameObject* pSelectedGuideCircleObject = nullptr;
    {
        for (size_t i = 0; i < vecSurfaces.size(); ++i)
        {
            if (vecSurfaces[i] == nullptr)
                continue;

            auto pSurfaceScript = vecSurfaces[i]->GetScript<CSurfaceScript>();
            auto pGuide = vecSurfaces[i]->GetScript<CSurfaceCircleGuideScript>();
            if (pSurfaceScript == nullptr ||
                pGuide == nullptr ||
                !pGuide->IsEnabled() ||
                !IsCircleGeometry(pSurfaceScript->GetGeometry()))
            {
                continue;
            }

            vecGuidedCircleObjects.push_back(vecSurfaces[i].Get());
            if (!pGuide->GetLinkedCorrectionLineName().empty())
                vecGuidedCorrectionLineNames.push_back(pGuide->GetLinkedCorrectionLineName());
        }

        Ptr<GameObject> pSelectedSurface = GetSelectedSurfaceObject();
        if (pSelectedSurface != nullptr)
        {
            for (size_t i = 0; i < vecGuidedCircleObjects.size(); ++i)
            {
                if (vecGuidedCircleObjects[i] == pSelectedSurface.Get())
                {
                    pSelectedGuideCircleObject = pSelectedSurface.Get();
                    break;
                }
            }
        }
    }

    const auto isGuidedCircleObject = [&](GameObject* _Object)
    {
        if (_Object == nullptr)
            return false;

        for (size_t i = 0; i < vecGuidedCircleObjects.size(); ++i)
        {
            if (vecGuidedCircleObjects[i] == _Object)
                return true;
        }

        return false;
    };

    const auto isGuidedCorrectionLineName = [&](const wstring& _Name)
    {
        if (_Name.empty())
            return false;

        for (size_t i = 0; i < vecGuidedCorrectionLineNames.size(); ++i)
        {
            if (vecGuidedCorrectionLineNames[i] == _Name)
                return true;
        }

        return false;
    };

    for (size_t i = 0; i < vecSurfaces.size(); ++i)
    {
        if (vecSurfaces[i] != nullptr && isGuidedCircleObject(vecSurfaces[i].Get()))
            continue;

        if (vecSurfaces[i] != nullptr &&
            isGuidedCorrectionLineName(vecSurfaces[i]->GetName()))
        {
            continue;
        }

        Ptr<CSurfaceScript> pScript = vecSurfaces[i]->GetScript<CSurfaceScript>();
        if (pScript == nullptr)
            continue;

        Vec4 color = pScript->GetEditorColor();
        ImU32 lineColor = ImColor(color.x, color.y, color.z, 1.f);

        if (pScript->GetGeometry() == CSurfaceScript::SURFACE_GEOMETRY::LINE)
        {
            Vec2 worldA = {};
            Vec2 worldB = {};
            pScript->GetWorldEndpoints(worldA, worldB);

            ImVec2 screenA = {};
            ImVec2 screenB = {};
            if (!WorldToScreen(worldA, screenA) || !WorldToScreen(worldB, screenB))
                continue;

            if (pScript->GetRole() == CSurfaceScript::SURFACE_ROLE::WALL)
            {
                Vec2 a = worldA;
                Vec2 b = worldB;
                CanonicalizeLineWorld(a, b);
                Vec2 tangent = NormalizeVec2(b - a);
                Vec2 openNormal = pScript->GetFillAbove() ? Vec2(tangent.y, -tangent.x) : Vec2(-tangent.y, tangent.x);
                Vec2 blockedDir = -openNormal;

                Vec2 fill0 = worldA;
                Vec2 fill1 = worldB;
                Vec2 fill2 = worldB + blockedDir * kWallOverlayDepth;
                Vec2 fill3 = worldA + blockedDir * kWallOverlayDepth;

                ImVec2 screenFill[4] = {};
                if (WorldToScreen(fill0, screenFill[0]) &&
                    WorldToScreen(fill1, screenFill[1]) &&
                    WorldToScreen(fill2, screenFill[2]) &&
                    WorldToScreen(fill3, screenFill[3]))
                {
                    pDraw->AddConvexPolyFilled(screenFill, 4, IM_COL32(255, 128, 64, 72));
                }
            }

            pDraw->AddLine(screenA, screenB, lineColor, 3.f);
            pDraw->AddCircleFilled(screenA, 4.f, IM_COL32(255, 255, 255, 220));
            pDraw->AddCircleFilled(screenB, 4.f, IM_COL32(255, 255, 255, 220));

            if (m_ShowNormals)
            {
                Vec2 a = worldA;
                Vec2 b = worldB;
                CanonicalizeLineWorld(a, b);
                Vec2 tangent = NormalizeVec2(b - a);
                Vec2 upNormal = Vec2(-tangent.y, tangent.x);
                Vec2 normal = pScript->GetFillAbove() ? -upNormal : upNormal;

                int sampleCount = max(2, (int)(LengthVec2(b - a) / 120.f));
                for (int sample = 0; sample <= sampleCount; ++sample)
                {
                    float t = (float)sample / (float)sampleCount;
                    Vec2 worldPos = a + (b - a) * t;
                    Vec2 worldEnd = worldPos + normal * 36.f;

                    ImVec2 screenPos = {};
                    ImVec2 screenEnd = {};
                    if (WorldToScreen(worldPos, screenPos) && WorldToScreen(worldEnd, screenEnd))
                    {
                        pDraw->AddLine(screenPos, screenEnd, IM_COL32(255, 235, 80, 220), 1.5f);
                    }
                }
            }
        }
        else
        {
            Vec2 center = {};
            Vec2 boxMin = {};
            Vec2 boxMax = {};
            float radius = 0.f;
            pScript->GetArcWorldData(center, radius, boxMin, boxMax);
            const bool fullCircle = (pScript->GetGeometry() == CSurfaceScript::SURFACE_GEOMETRY::FULL_CIRCLE);

            if (pScript->GetRole() == CSurfaceScript::SURFACE_ROLE::WALL && radius > 0.001f)
            {
                const int fillSegmentCount = fullCircle ? 48 : 24;
                vector<ImVec2> fillPoints;

                if (pScript->GetFillInside())
                {
                    ImVec2 screenCenter = {};
                    if (WorldToScreen(center, screenCenter))
                        fillPoints.push_back(screenCenter);
                }

                vector<Vec2> arcPoints;
                vector<Vec2> outerPoints;
                arcPoints.reserve(fillSegmentCount + 1);
                outerPoints.reserve(fillSegmentCount + 1);

                for (int segment = 0; segment <= fillSegmentCount; ++segment)
                {
                    float t = (float)segment / (float)fillSegmentCount;
                    float angle = 0.f;

                    if (fullCircle)
                    {
                        angle = t * XM_2PI;
                    }
                    else switch (pScript->GetArcCorner())
                    {
                    case CSurfaceScript::ARC_CORNER::TOP_LEFT:
                        angle = 0.f + t * (-XM_PIDIV2);
                        break;
                    case CSurfaceScript::ARC_CORNER::TOP_RIGHT:
                        angle = XM_PI + t * XM_PIDIV2;
                        break;
                    case CSurfaceScript::ARC_CORNER::BOTTOM_LEFT:
                        angle = 0.f + t * XM_PIDIV2;
                        break;
                    case CSurfaceScript::ARC_CORNER::BOTTOM_RIGHT:
                    default:
                        angle = XM_PI - t * XM_PIDIV2;
                        break;
                    }

                    Vec2 radial = Vec2(cosf(angle), sinf(angle));
                    Vec2 arcWorld = Vec2(center.x + radial.x * radius, center.y + radial.y * radius);
                    arcPoints.push_back(arcWorld);

                    if (!pScript->GetFillInside())
                        outerPoints.push_back(arcWorld + radial * kWallOverlayDepth);
                }

                for (size_t i = 0; i < arcPoints.size(); ++i)
                {
                    ImVec2 screenPos = {};
                    if (WorldToScreen(arcPoints[i], screenPos))
                        fillPoints.push_back(screenPos);
                }

                if (!pScript->GetFillInside())
                {
                    for (int i = (int)outerPoints.size() - 1; i >= 0; --i)
                    {
                        ImVec2 screenPos = {};
                        if (WorldToScreen(outerPoints[i], screenPos))
                            fillPoints.push_back(screenPos);
                    }
                }

                if (fillPoints.size() >= 3)
                    pDraw->AddConvexPolyFilled(fillPoints.data(), (int)fillPoints.size(), IM_COL32(255, 128, 64, 72));
            }

            const int segmentCount = fullCircle ? 48 : 24;
            ImVec2 prev = {};
            bool hasPrev = false;

            for (int segment = 0; segment <= segmentCount; ++segment)
            {
                float t = (float)segment / (float)segmentCount;
                float angle = 0.f;

                if (fullCircle)
                {
                    angle = t * XM_2PI;
                }
                else switch (pScript->GetArcCorner())
                {
                case CSurfaceScript::ARC_CORNER::TOP_LEFT:
                    angle = 0.f + t * (-XM_PIDIV2);
                    break;
                case CSurfaceScript::ARC_CORNER::TOP_RIGHT:
                    angle = XM_PI + t * XM_PIDIV2;
                    break;
                case CSurfaceScript::ARC_CORNER::BOTTOM_LEFT:
                    angle = 0.f + t * XM_PIDIV2;
                    break;
                case CSurfaceScript::ARC_CORNER::BOTTOM_RIGHT:
                default:
                    angle = XM_PI - t * XM_PIDIV2;
                    break;
                }

                Vec2 worldPos = Vec2(center.x + cosf(angle) * radius, center.y + sinf(angle) * radius);
                ImVec2 screenPos = {};
                if (!WorldToScreen(worldPos, screenPos))
                    continue;

                if (hasPrev)
                    pDraw->AddLine(prev, screenPos, lineColor, 3.f);

                if (m_ShowNormals && segment < segmentCount)
                {
                    Vec2 radial = NormalizeVec2(worldPos - center, Vec2(0.f, 1.f));
                    Vec2 normal = pScript->GetFillInside() ? -radial : radial;
                    Vec2 worldEnd = worldPos + normal * 36.f;

                    ImVec2 screenEnd = {};
                    if (WorldToScreen(worldEnd, screenEnd))
                        pDraw->AddLine(screenPos, screenEnd, IM_COL32(255, 235, 80, 220), 1.5f);
                }

                prev = screenPos;
                hasPrev = true;
            }

            ImVec2 screenCenter = {};
            if (!fullCircle && WorldToScreen(center, screenCenter))
            {
                pDraw->AddCircleFilled(screenCenter, 4.f, IM_COL32(255, 120, 255, 220));
                pDraw->AddCircle(screenCenter, 8.f, IM_COL32(255, 120, 255, 120), 0, 1.5f);
            }
        }
    }

    for (size_t i = 0; i < vecGuidedCircleObjects.size(); ++i)
    {
        if (vecGuidedCircleObjects[i] == nullptr || vecGuidedCircleObjects[i] == pSelectedGuideCircleObject)
            continue;

        DrawCircleGuideOverlayForSurface(pDraw, vecGuidedCircleObjects[i], false);
    }

    if (pSelectedGuideCircleObject != nullptr)
        DrawCircleGuideOverlayForSurface(pDraw, pSelectedGuideCircleObject, true);

    if (m_FreeformRole == FREEFORM_ROLE::ERASE)
    {
        Vec2 mouseWorld = {};
        if (GetMouseWorldPos(mouseWorld))
        {
            float nearestDist = FLT_MAX;
            Ptr<GameObject> pNearest = FindNearestSurfaceObject(mouseWorld, nearestDist);
            if (pNearest != nullptr && nearestDist <= 48.f)
            {
                Ptr<CSurfaceScript> pScript = pNearest->GetScript<CSurfaceScript>();
                if (pScript != nullptr)
                {
                    ImU32 eraseColor = IM_COL32(255, 80, 80, 255);
                    if (pScript->GetGeometry() == CSurfaceScript::SURFACE_GEOMETRY::LINE)
                    {
                        Vec2 worldA = {};
                        Vec2 worldB = {};
                        pScript->GetWorldEndpoints(worldA, worldB);

                        ImVec2 screenA = {};
                        ImVec2 screenB = {};
                        if (WorldToScreen(worldA, screenA) && WorldToScreen(worldB, screenB))
                        {
                            pDraw->AddLine(screenA, screenB, eraseColor, 5.f);
                            pDraw->AddCircle(screenA, 8.f, eraseColor, 0, 2.5f);
                            pDraw->AddCircle(screenB, 8.f, eraseColor, 0, 2.5f);
                        }
                    }
                    else
                    {
                        Vec2 center = {};
                        Vec2 boxMin = {};
                        Vec2 boxMax = {};
                        float radius = 0.f;
                        pScript->GetArcWorldData(center, radius, boxMin, boxMax);

                        const bool fullCircle = (pScript->GetGeometry() == CSurfaceScript::SURFACE_GEOMETRY::FULL_CIRCLE);
                        const int segmentCount = fullCircle ? 48 : 24;
                        ImVec2 prev = {};
                        bool hasPrev = false;

                        for (int segment = 0; segment <= segmentCount; ++segment)
                        {
                            float t = (float)segment / (float)segmentCount;
                            float angle = 0.f;

                            if (fullCircle)
                            {
                                angle = t * XM_2PI;
                            }
                            else switch (pScript->GetArcCorner())
                            {
                            case CSurfaceScript::ARC_CORNER::TOP_LEFT:
                                angle = 0.f + t * (-XM_PIDIV2);
                                break;
                            case CSurfaceScript::ARC_CORNER::TOP_RIGHT:
                                angle = XM_PI + t * XM_PIDIV2;
                                break;
                            case CSurfaceScript::ARC_CORNER::BOTTOM_LEFT:
                                angle = 0.f + t * XM_PIDIV2;
                                break;
                            case CSurfaceScript::ARC_CORNER::BOTTOM_RIGHT:
                            default:
                                angle = XM_PI - t * XM_PIDIV2;
                                break;
                            }

                            Vec2 worldPos = Vec2(center.x + cosf(angle) * radius, center.y + sinf(angle) * radius);
                            ImVec2 screenPos = {};
                            if (!WorldToScreen(worldPos, screenPos))
                                continue;

                            if (hasPrev)
                                pDraw->AddLine(prev, screenPos, eraseColor, 5.f);

                            prev = screenPos;
                            hasPrev = true;
                        }
                    }
                }
            }
        }
    }

    // Existing endpoints are always shown so new segments can be snapped and
    // connected without manually matching coordinates.
    for (size_t i = 0; i < vecSurfaces.size(); ++i)
    {
        if (vecSurfaces[i] != nullptr && isGuidedCircleObject(vecSurfaces[i].Get()))
            continue;

        if (vecSurfaces[i] != nullptr &&
            isGuidedCorrectionLineName(vecSurfaces[i]->GetName()))
        {
            continue;
        }

        Ptr<CSurfaceScript> pScript = vecSurfaces[i]->GetScript<CSurfaceScript>();
        if (pScript == nullptr)
            continue;

        Vec2 pointA = {};
        Vec2 pointB = {};
        if (!GetSurfaceEndpointPair(pScript.Get(), pointA, pointB))
            continue;

        ImVec2 screenA = {};
        ImVec2 screenB = {};
        if (WorldToScreen(pointA, screenA))
            pDraw->AddCircleFilled(screenA, 5.f, IM_COL32(255, 255, 255, 220));
        if (WorldToScreen(pointB, screenB))
            pDraw->AddCircleFilled(screenB, 5.f, IM_COL32(255, 255, 255, 220));
    }

    if (!m_HasStartPoint)
    {
        Vec2 mouseWorld = {};
        Vec2 snapPoint = {};
        const bool hasMouseWorld = GetMouseWorldPos(mouseWorld);
        const bool hasSnapPoint = hasMouseWorld && FindSnapPoint(mouseWorld, snapPoint);
        if (hasSnapPoint)
        {
            ImVec2 snapScreen = {};
            if (WorldToScreen(snapPoint, snapScreen))
            {
                pDraw->AddCircle(snapScreen, 9.f, IM_COL32(80, 255, 255, 255), 0, 2.5f);
                pDraw->AddCircle(snapScreen, 14.f, IM_COL32(80, 255, 255, 120), 0, 1.5f);
            }
        }

        if (hasMouseWorld &&
            m_FreeformShape == FREEFORM_SHAPE::FULL_CIRCLE &&
            m_UseCircleRadius)
        {
            const Vec2 previewCenter = hasSnapPoint ? snapPoint : mouseWorld;
            const float radius = max(1.f, m_CircleRadius);
            const int segmentCount = 48;
            ImVec2 prev = {};
            bool hasPrev = false;
            for (int segment = 0; segment <= segmentCount; ++segment)
            {
                const float t = (float)segment / (float)segmentCount;
                const float angle = t * XM_2PI;
                const Vec2 worldPos = Vec2(previewCenter.x + cosf(angle) * radius,
                                           previewCenter.y + sinf(angle) * radius);
                ImVec2 screenPos = {};
                if (!WorldToScreen(worldPos, screenPos))
                    continue;

                if (hasPrev)
                    pDraw->AddLine(prev, screenPos, IM_COL32(255, 160, 80, 220), 2.f);

                prev = screenPos;
                hasPrev = true;
            }
        }

        return;
    }

    Vec2 mouseWorld = {};
    if (!GetMouseWorldPos(mouseWorld))
        return;

    Vec2 previewEnd = mouseWorld;
    Vec2 snapPoint = {};
    const bool snapped = FindSnapPoint(mouseWorld, snapPoint);
    if (snapped)
        previewEnd = snapPoint;

    ImVec2 startScreen = {};
    ImVec2 endScreen = {};
    if (!WorldToScreen(m_StartPoint, startScreen) || !WorldToScreen(previewEnd, endScreen))
        return;

    pDraw->AddCircleFilled(startScreen, 5.f, IM_COL32(80, 255, 160, 255));
    pDraw->AddCircle(endScreen, snapped ? 8.f : 5.f, snapped ? IM_COL32(80, 255, 255, 255) : IM_COL32(255, 120, 120, 255), 0, 2.f);
    if (snapped)
        pDraw->AddCircle(endScreen, 13.f, IM_COL32(80, 255, 255, 120), 0, 1.5f);

    if (m_FreeformShape == FREEFORM_SHAPE::LINE || m_FreeformShape == FREEFORM_SHAPE::FLAT)
    {
        if (m_FreeformShape == FREEFORM_SHAPE::FLAT)
        {
            ImVec2 flatEndScreen = {};
            Vec2 flatEndWorld = Vec2(previewEnd.x, m_StartPoint.y);
            if (WorldToScreen(flatEndWorld, flatEndScreen))
                pDraw->AddLine(startScreen, flatEndScreen, IM_COL32(255, 160, 80, 255), 2.f);
        }
        else
        {
            pDraw->AddLine(startScreen, endScreen, IM_COL32(255, 160, 80, 255), 2.f);
        }
    }
    else if (m_FreeformShape == FREEFORM_SHAPE::FULL_CIRCLE)
    {
        Vec2 minBox = {};
        Vec2 maxBox = {};
        Vec2 center = {};
        float radius = 0.f;
        BuildFullCircleBounds(m_StartPoint, previewEnd, minBox, maxBox, center, radius);

        const int segmentCount = 48;
        ImVec2 prev = {};
        bool hasPrev = false;
        for (int segment = 0; segment <= segmentCount; ++segment)
        {
            const float t = (float)segment / (float)segmentCount;
            const float angle = t * XM_2PI;
            const Vec2 worldPos = Vec2(center.x + cosf(angle) * radius, center.y + sinf(angle) * radius);
            ImVec2 screenPos = {};
            if (!WorldToScreen(worldPos, screenPos))
                continue;

            if (hasPrev)
                pDraw->AddLine(prev, screenPos, IM_COL32(255, 160, 80, 255), 2.f);

            prev = screenPos;
            hasPrev = true;
        }

        if (m_ArcFillInside && m_FreeformRole == FREEFORM_ROLE::SURFACE)
        {
            const Vec2 corrStart = MakeCirclePointFromAngle(center, radius, -145.f);
            const Vec2 corrEnd = MakeCirclePointFromAngle(center, radius, -35.f);
            ImVec2 screenCorrStart = {};
            ImVec2 screenCorrEnd = {};
            if (WorldToScreen(corrStart, screenCorrStart) && WorldToScreen(corrEnd, screenCorrEnd))
                pDraw->AddLine(screenCorrStart, screenCorrEnd, IM_COL32(80, 255, 255, 220), 2.f);
        }
    }
    else
    {
        Vec2 minBox = {};
        Vec2 maxBox = {};
        Vec2 center = {};
        float side = 0.f;
        BuildQuarterSurfaceBounds(m_StartPoint, previewEnd, m_FreeformShape == FREEFORM_SHAPE::CIRCLE, minBox, maxBox, center, side);

        const int segmentCount = 24;
        ImVec2 prev = {};
        bool hasPrev = false;
        for (int segment = 0; segment <= segmentCount; ++segment)
        {
            float t = (float)segment / (float)segmentCount;
            float angle = 0.f;

            switch (m_ArcCorner)
            {
            case ARC_CORNER::TOP_LEFT:
                angle = 0.f + t * (-XM_PIDIV2);
                break;
            case ARC_CORNER::TOP_RIGHT:
                angle = XM_PI + t * XM_PIDIV2;
                break;
            case ARC_CORNER::BOTTOM_LEFT:
                angle = 0.f + t * XM_PIDIV2;
                break;
            case ARC_CORNER::BOTTOM_RIGHT:
            default:
                angle = XM_PI - t * XM_PIDIV2;
                break;
            }

            Vec2 worldPos = Vec2(center.x + cosf(angle) * side, center.y + sinf(angle) * side);
            ImVec2 screenPos = {};
            if (!WorldToScreen(worldPos, screenPos))
                continue;

            if (hasPrev)
                pDraw->AddLine(prev, screenPos, IM_COL32(255, 160, 80, 255), 2.f);

            prev = screenPos;
            hasPrev = true;
        }

    }
}

void MapEditorUI::HandleSceneInput()
{
    if (m_FreeformMode)
    {
        HandleFreeformInput();
        return;
    }
}

bool MapEditorUI::HandleSelectedCircleGuideDrag(const Vec2& _MouseWorld)
{
    Ptr<GameObject> pSelectedSurface = GetSelectedSurfaceObject();
    if (pSelectedSurface == nullptr)
    {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            m_GuideDragHandle = GUIDE_DRAG_HANDLE::NONE;
        return false;
    }

    auto pSurface = pSelectedSurface->GetScript<CSurfaceScript>();
    auto pGuide = pSelectedSurface->GetScript<CSurfaceCircleGuideScript>();
    if (pSurface == nullptr || pGuide == nullptr || !pGuide->IsEnabled() || !IsCircleGeometry(pSurface->GetGeometry()))
    {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            m_GuideDragHandle = GUIDE_DRAG_HANDLE::NONE;
        return false;
    }

    Vec2 center = {};
    float radius = 0.f;
    Vec2 boxMin = {};
    Vec2 boxMax = {};
    pSurface->GetArcWorldData(center, radius, boxMin, boxMax);
    if (radius <= 0.001f)
    {
        m_GuideDragHandle = GUIDE_DRAG_HANDLE::NONE;
        return false;
    }

    const Vec2 corrStartWorld = center + pGuide->GetCorrectionLineStartLocal();
    const Vec2 corrEndWorld = center + pGuide->GetCorrectionLineEndLocal();
    const float handlePickRadius = max(18.f, radius * 0.06f);
    const float distToStart = LengthVec2(_MouseWorld - corrStartWorld);
    const float distToEnd = LengthVec2(_MouseWorld - corrEndWorld);

    if (m_GuideDragHandle != GUIDE_DRAG_HANDLE::NONE)
    {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            m_GuideDragHandle = GUIDE_DRAG_HANDLE::NONE;
            m_StatusText = "Guide correction handle released.";
            return true;
        }

        if (m_GuideDragHandle == GUIDE_DRAG_HANDLE::CORR_START)
            pGuide->SetCorrectionLineStartLocal(_MouseWorld - center);
        else
            pGuide->SetCorrectionLineEndLocal(_MouseWorld - center);

        ApplyCircleGuideToMatchingSurfaces(pSelectedSurface.Get());
        CreateOrUpdateCircleGuideCorrectionLine(pSelectedSurface.Get());
        m_StatusText = "Guide correction line updated from scene handle.";
        return true;
    }

    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        return false;

    if (distToStart > handlePickRadius && distToEnd > handlePickRadius)
        return false;

    m_GuideDragHandle = (distToStart <= distToEnd) ? GUIDE_DRAG_HANDLE::CORR_START : GUIDE_DRAG_HANDLE::CORR_END;
    m_StatusText = (m_GuideDragHandle == GUIDE_DRAG_HANDLE::CORR_START)
        ? "Dragging correction line A handle."
        : "Dragging correction line B handle.";

    if (m_GuideDragHandle == GUIDE_DRAG_HANDLE::CORR_START)
        pGuide->SetCorrectionLineStartLocal(_MouseWorld - center);
    else
        pGuide->SetCorrectionLineEndLocal(_MouseWorld - center);

    ApplyCircleGuideToMatchingSurfaces(pSelectedSurface.Get());
    CreateOrUpdateCircleGuideCorrectionLine(pSelectedSurface.Get());
    return true;
}

void MapEditorUI::DrawCircleGuideOverlayForSurface(ImDrawList* _Draw, GameObject* _SurfaceObject, bool _ShowLabels)
{
    if (_Draw == nullptr || _SurfaceObject == nullptr)
        return;

    auto pSurface = _SurfaceObject->GetScript<CSurfaceScript>();
    auto pGuide = _SurfaceObject->GetScript<CSurfaceCircleGuideScript>();
    if (pSurface == nullptr || pGuide == nullptr || !pGuide->IsEnabled() ||
        !IsCircleGeometry(pSurface->GetGeometry()))
    {
        return;
    }

    Vec2 center = {};
    float radius = 0.f;
    Vec2 boxMin = {};
    Vec2 boxMax = {};
    pSurface->GetArcWorldData(center, radius, boxMin, boxMax);
    if (radius <= 0.001f)
        return;

    const Vec2 halfWorld = MakeCirclePointFromAngle(center, radius, kCircleGuideHalfCheckAngleDeg);
    Vec2 corrStartWorld = center + pGuide->GetCorrectionLineStartLocal();
    Vec2 corrEndWorld = center + pGuide->GetCorrectionLineEndLocal();
    if (corrEndWorld.x < corrStartWorld.x)
    {
        const Vec2 temp = corrStartWorld;
        corrStartWorld = corrEndWorld;
        corrEndWorld = temp;
    }

    const bool entryOnRight = cosf(DegreesToRadians(pGuide->GetEntryAngleDeg())) >= 0.f;
    const bool initialLineOwnsRight = !entryOnRight;
    const Vec2 activeArcStartPoint = initialLineOwnsRight ? halfWorld : corrEndWorld;
    const Vec2 activeArcEndPoint = initialLineOwnsRight ? corrStartWorld : halfWorld;
    const float activeArcStartAngleDeg = ComputeCircleAngleDegrees(center, activeArcStartPoint);
    const float activeArcEndAngleDeg = ComputeCircleAngleDegrees(center, activeArcEndPoint);
    bool activeArcCounterClockwise = false;
    const bool hasActiveArcDirection =
        TryResolveGuideArcDirection(activeArcStartAngleDeg, activeArcEndAngleDeg, kCircleGuideHalfCheckAngleDeg, activeArcCounterClockwise);
    const ImU32 arcColor = _ShowLabels ? IM_COL32(255, 160, 80, 255) : IM_COL32(255, 190, 96, 220);
    const float arcThickness = _ShowLabels ? 3.f : 2.5f;
    const float lineThickness = _ShowLabels ? 2.f : 1.75f;
    const ImU32 halfColor = initialLineOwnsRight ? IM_COL32(255, 0, 80, 255) : IM_COL32(0, 120, 255, 255);

    ImVec2 screenHalf = {};
    ImVec2 screenCorrStart = {};
    ImVec2 screenCorrEnd = {};
    const bool hasHalf = WorldToScreen(halfWorld, screenHalf);
    const bool hasCorrStart = WorldToScreen(corrStartWorld, screenCorrStart);
    const bool hasCorrEnd = WorldToScreen(corrEndWorld, screenCorrEnd);

    const auto drawArcSegment = [&](float _StartAngleDeg, float _EndAngleDeg, bool _CounterClockwise, ImU32 _Color)
    {
        const int segmentCount = 20;
        ImVec2 prev = {};
        bool hasPrev = false;

        for (int segment = 0; segment <= segmentCount; ++segment)
        {
            const float t = (float)segment / (float)segmentCount;
            const float angleDeg = LerpAngleDegreesAlong(_CounterClockwise, _StartAngleDeg, _EndAngleDeg, t);
            const Vec2 worldPos = MakeCirclePointFromAngle(center, radius, angleDeg);
            ImVec2 screenPos = {};
            if (!WorldToScreen(worldPos, screenPos))
                continue;

            if (hasPrev)
                _Draw->AddLine(prev, screenPos, _Color, arcThickness);

            prev = screenPos;
            hasPrev = true;
        }
    };

    const auto drawGuideMarker = [&](const ImVec2& _ScreenPos, ImU32 _Color, const char* _Label)
    {
        const float markerRadius = _ShowLabels ? kCircleGuideMarkerRadius : (kCircleGuideMarkerRadius - 2.f);
        _Draw->AddCircleFilled(_ScreenPos, markerRadius, _Color);
        _Draw->AddCircle(_ScreenPos, markerRadius + 3.f, IM_COL32(255, 255, 255, 200), 0, 1.5f);
        if (_ShowLabels)
            _Draw->AddText(ImVec2(_ScreenPos.x + 10.f, _ScreenPos.y - 8.f), _Color, _Label);
    };

    const auto drawExitArrow = [&](const ImVec2& _ScreenPos, bool _OpenRight, ImU32 _Color)
    {
        const float shaftLength = _ShowLabels ? 34.f : 26.f;
        const float shaftYOffset = _ShowLabels ? -16.f : -12.f;
        const float headLength = _ShowLabels ? 10.f : 8.f;
        const float headWidth = _ShowLabels ? 6.f : 5.f;
        const float dir = _OpenRight ? 1.f : -1.f;
        const ImVec2 start = ImVec2(_ScreenPos.x, _ScreenPos.y + shaftYOffset);
        const ImVec2 end = ImVec2(start.x + shaftLength * dir, start.y);
        const ImVec2 headBase = ImVec2(end.x - headLength * dir, end.y);

        _Draw->AddLine(start, end, _Color, 2.5f);
        _Draw->AddLine(end, ImVec2(headBase.x, headBase.y - headWidth), _Color, 2.5f);
        _Draw->AddLine(end, ImVec2(headBase.x, headBase.y + headWidth), _Color, 2.5f);
        if (_ShowLabels)
            _Draw->AddText(ImVec2(end.x + (_OpenRight ? 8.f : -36.f), end.y - 8.f), _Color, "Exit");
    };

    if (hasActiveArcDirection)
        drawArcSegment(activeArcStartAngleDeg, activeArcEndAngleDeg, activeArcCounterClockwise, arcColor);
    if (hasCorrStart && hasCorrEnd)
        _Draw->AddLine(screenCorrStart, screenCorrEnd, IM_COL32(80, 255, 255, 220), lineThickness);
    if (hasHalf)
    {
        drawGuideMarker(screenHalf, halfColor, "Half");
        drawExitArrow(screenHalf, initialLineOwnsRight, halfColor);
    }
    if (hasCorrStart)
        drawGuideMarker(screenCorrStart, IM_COL32(80, 255, 255, 255), "A");
    if (hasCorrEnd)
        drawGuideMarker(screenCorrEnd, IM_COL32(80, 255, 255, 255), "B");
}

void MapEditorUI::DrawSelectedCircleGuideOverlay(ImDrawList* _Draw)
{
    if (_Draw == nullptr)
        return;

    Ptr<GameObject> pSelectedSurface = GetSelectedSurfaceObject();
    if (pSelectedSurface == nullptr)
        return;

    DrawCircleGuideOverlayForSurface(_Draw, pSelectedSurface.Get(), true);
}

void MapEditorUI::DrawSceneOverlay()
{
    if (m_FreeformMode)
    {
        DrawFreeformOverlay();
        return;
    }
}

void MapEditorUI::Tick_UI()
{
    RefreshCircleGuideCorrectionLines();

    ImGui::TextWrapped("Freeform surface editor. Click a world start point, then an end point directly in the game view.");
    ImGui::TextWrapped("This editor creates persistent scene surface objects instead of baking grid tiles.");
    ImGui::TextWrapped("Right click cancels the current stroke. F10 toggles this editor.");
    ImGui::Separator();

    if (ImGui::Button("Ensure Surface Root"))
    {
        EnsureSurfaceRoot();
        m_StatusText = "Surface root is ready.";
    }

    const char* shapeNames[] = { "Line", "Flat", "Arc", "Quarter Circle", "Full Circle" };
    int shapeIdx = (int)m_FreeformShape;
    if (ImGui::Combo("Shape", &shapeIdx, shapeNames, IM_ARRAYSIZE(shapeNames)))
    {
        m_FreeformShape = (FREEFORM_SHAPE)shapeIdx;
        m_HasStartPoint = false;
    }

    const char* roleNames[] = { "Surface", "Correction", "Pure Wall", "Vertical Entry", "Erase" };
    int roleIdx = (int)m_FreeformRole;
    if (ImGui::Combo("Role", &roleIdx, roleNames, IM_ARRAYSIZE(roleNames)))
    {
        m_FreeformRole = (FREEFORM_ROLE)roleIdx;
        m_HasStartPoint = false;

        if (m_FreeformRole == FREEFORM_ROLE::WALL)
            m_Attachable = false;
        else if (m_FreeformRole == FREEFORM_ROLE::VERTICAL_ENTRY && !m_Attachable)
            m_Attachable = true;
    }

    if (m_FreeformRole != FREEFORM_ROLE::ERASE)
    {
        if (m_FreeformRole == FREEFORM_ROLE::VERTICAL_ENTRY)
            ImGui::TextWrapped("Vertical Entry line only catches when the player's world Y movement is dominant. Side approaches are ignored.");

        if (m_FreeformShape == FREEFORM_SHAPE::LINE || m_FreeformShape == FREEFORM_SHAPE::FLAT)
        {
            ImGui::Checkbox("Solid Above", &m_FillAbove);
            ImGui::Checkbox("Attachable", &m_Attachable);

            if (m_FreeformShape == FREEFORM_SHAPE::FLAT)
                ImGui::TextWrapped("Flat always keeps the start Y and only uses the end X for a perfectly horizontal surface.");
        }
        else
        {
            if (m_FreeformShape != FREEFORM_SHAPE::FULL_CIRCLE)
            {
                const char* cornerNames[] = { "Top Left", "Top Right", "Bottom Left", "Bottom Right" };
                int cornerIdx = (int)m_ArcCorner;
                if (ImGui::Combo("Arc Corner", &cornerIdx, cornerNames, IM_ARRAYSIZE(cornerNames)))
                {
                    m_ArcCorner = (ARC_CORNER)cornerIdx;
                }
            }

            ImGui::Checkbox("Fill Inside", &m_ArcFillInside);
            ImGui::Checkbox("Attachable", &m_Attachable);

            if (m_FreeformShape == FREEFORM_SHAPE::ARC)
                ImGui::TextWrapped("Arc is a pure quarter-curve with no automatic half-check or line fallback.");
            else if (m_FreeformShape == FREEFORM_SHAPE::CIRCLE)
                ImGui::TextWrapped("Quarter Circle creates one curved quarter. If Fill Inside is on, its guide correction line is created automatically.");
            else
                ImGui::TextWrapped("Full Circle uses the first click as center. The second click sets radius, or just confirms placement when Use Radius is on.");

            if (m_FreeformShape == FREEFORM_SHAPE::CIRCLE || m_FreeformShape == FREEFORM_SHAPE::FULL_CIRCLE)
            {
                ImGui::Checkbox("Use Radius", &m_UseCircleRadius);
                if (m_UseCircleRadius)
                {
                    ImGui::DragFloat("Radius", &m_CircleRadius, 1.f, 1.f, 5000.f);
                }
            }
        }
    }

    ImGui::Checkbox("Show Normals", &m_ShowNormals);
    ImGui::Checkbox("Point Snap", &m_EnablePointSnap);
    ImGui::DragFloat("Snap Distance", &m_PointSnapDistance, 1.f, 0.f, 200.f);

    Vec2 mouseWorld = {};
    const bool hasMouseWorld = GetMouseWorldPos(mouseWorld);
    Vec2 snappedMouseWorld = mouseWorld;
    const bool hasSnapPoint = hasMouseWorld && FindSnapPoint(mouseWorld, snappedMouseWorld);
    Vec2 displayEndPos = snappedMouseWorld;
    if (m_HasStartPoint && m_FreeformShape == FREEFORM_SHAPE::FLAT)
        displayEndPos = Vec2(snappedMouseWorld.x, m_StartPoint.y);

    if (hasMouseWorld)
        ImGui::Text("Cursor : %.1f, %.1f", mouseWorld.x, mouseWorld.y);
    else
        ImGui::Text("Cursor : <unavailable>");

    if (m_HasStartPoint)
        ImGui::Text("Start Pos : %.1f, %.1f", m_StartPoint.x, m_StartPoint.y);
    else
        ImGui::Text("Start Pos : <none>");

    if (m_HasStartPoint && hasMouseWorld)
        ImGui::Text("End Pos : %.1f, %.1f", displayEndPos.x, displayEndPos.y);
    else
        ImGui::Text("End Pos : <none>");

    if (m_HasStartPoint && hasMouseWorld &&
        (m_FreeformShape == FREEFORM_SHAPE::CIRCLE || m_FreeformShape == FREEFORM_SHAPE::FULL_CIRCLE))
    {
        Vec2 previewMinBox = {};
        Vec2 previewMaxBox = {};
        Vec2 previewCenter = {};
        float previewRadius = 0.f;
        if (m_FreeformShape == FREEFORM_SHAPE::FULL_CIRCLE)
            BuildFullCircleBounds(m_StartPoint, displayEndPos, previewMinBox, previewMaxBox, previewCenter, previewRadius);
        else
            BuildQuarterSurfaceBounds(m_StartPoint, displayEndPos, true, previewMinBox, previewMaxBox, previewCenter, previewRadius);

        ImGui::Text("Preview Radius : %.1f", previewRadius);
    }

    if (hasSnapPoint)
        ImGui::Text("Snap Target : %.1f, %.1f", displayEndPos.x, displayEndPos.y);
    else
        ImGui::Text("Snap Target : <none>");

    if (m_HasLastCreatedSegment)
    {
        ImGui::Text("Last Start : %.1f, %.1f", m_LastCreatedStart.x, m_LastCreatedStart.y);
        ImGui::Text("Last End : %.1f, %.1f", m_LastCreatedEnd.x, m_LastCreatedEnd.y);
    }
    else
    {
        ImGui::Text("Last Start : <none>");
        ImGui::Text("Last End : <none>");
    }

    vector<Ptr<GameObject>> vecSurfaces;
    CollectSurfaceObjects(vecSurfaces);
    ImGui::Text("Surface Count : %d", (int)vecSurfaces.size());

    std::string surfaceSetName = "None";
    if (m_SurfaceSetAsset != nullptr)
    {
        std::wstring surfaceSetLabel = m_SurfaceSetAsset->GetKey();
        if (surfaceSetLabel.empty())
            surfaceSetLabel = m_SurfaceSetAsset->GetRelativePath();

        if (!surfaceSetLabel.empty())
            surfaceSetName = string(surfaceSetLabel.begin(), surfaceSetLabel.end());
    }

    char surfaceSetBuffer[256] = {};
    strcpy_s(surfaceSetBuffer, surfaceSetName.c_str());
    ImGui::InputText("SurfaceSet Target", surfaceSetBuffer, sizeof(surfaceSetBuffer), ImGuiInputTextFlags_ReadOnly);
    if (ImGui::BeginDragDropTarget())
    {
        const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload("Content");
        if (pPayload != nullptr)
        {
            DWORD_PTR data = *((DWORD_PTR*)pPayload->Data);
            Ptr<Asset> pAsset = (Asset*)data;
            if (pAsset != nullptr && pAsset->GetType() == ASSET_TYPE::SURFACESET)
            {
                m_SurfaceSetAsset = (ASurfaceSet*)pAsset.Get();
                m_StatusText = "SurfaceSet target assigned from Content.";
            }
        }

        ImGui::EndDragDropTarget();
    }

    if (ImGui::Button("Use Inspector SurfaceSet"))
    {
        Ptr<Inspector> pInspector = (Inspector*)EditorMgr::GetInst()->FindUI("Inspector").Get();
        Ptr<Asset> pTargetAsset = (pInspector != nullptr) ? pInspector->GetTargetAsset() : nullptr;
        if (pTargetAsset != nullptr && pTargetAsset->GetType() == ASSET_TYPE::SURFACESET)
        {
            m_SurfaceSetAsset = (ASurfaceSet*)pTargetAsset.Get();
            m_StatusText = "SurfaceSet target copied from Inspector.";
        }
        else
        {
            m_StatusText = "Inspector does not currently have a SurfaceSet asset selected.";
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear SurfaceSet"))
    {
        m_SurfaceSetAsset = nullptr;
        m_StatusText = "SurfaceSet target cleared.";
    }

    if (ImGui::Button("Bake && Replace With Actor"))
    {
        if (LevelMgr::GetInst()->GetLevelState() != LEVEL_STATE::STOP)
        {
            m_StatusText = "Bake / replace is disabled while the level is playing.";
        }
        else
        {
            if (m_SurfaceSetAsset == nullptr)
            {
                std::string autoCreateStatus;
                m_SurfaceSetAsset = SurfaceSetUI::EnsureCurrentLevelSurfaceSetAsset(&autoCreateStatus);
                if (m_SurfaceSetAsset == nullptr)
                {
                    m_StatusText = autoCreateStatus;
                    return;
                }
            }

            UINT bakedCount = 0;
            std::string bakeStatus;
            if (SurfaceSetUI::ReplaceSceneSurfacesWithActorInLevel(m_SurfaceSetAsset.Get(), bakedCount, &bakeStatus))
                m_StatusText = bakeStatus + " Use File -> Level Save to persist the level.";
            else
                m_StatusText = bakeStatus;
        }
    }
    ImGui::TextWrapped("Drag a .sset asset from Content here, or the bake button will auto-create one from the current level name.");

    ImGui::Separator();
    ImGui::TextWrapped("Ctrl + left click in the scene selects the nearest surface for guide editing.");
    if (ImGui::Button("Pick Surface At Cursor"))
    {
        if (hasMouseWorld)
        {
            float nearestDist = FLT_MAX;
            Ptr<GameObject> pNearest = FindNearestSurfaceObject(mouseWorld, nearestDist);
            if (pNearest != nullptr && nearestDist <= 64.f)
            {
                SelectSurfaceObject(pNearest);
                m_StatusText = "Nearest surface selected in Inspector.";
            }
            else
            {
                m_StatusText = "No nearby surface to select.";
            }
        }
        else
        {
            m_StatusText = "Cursor world position is unavailable.";
        }
    }

    Ptr<GameObject> pSelectedSurface = GetSelectedSurfaceObject();
    if (pSelectedSurface != nullptr)
    {
        const wstring selectedName = pSelectedSurface->GetName();
        const string selectedNameUtf8(selectedName.begin(), selectedName.end());
        ImGui::TextWrapped("Selected Surface : %s", selectedNameUtf8.empty() ? "<unnamed>" : selectedNameUtf8.c_str());

        auto pSelectedScript = pSelectedSurface->GetScript<CSurfaceScript>();
        if (pSelectedScript != nullptr && IsCircleGeometry(pSelectedScript->GetGeometry()))
        {
            auto pGuide = EnsureCircleGuideScript(pSelectedSurface.Get(), false);
            if (pGuide == nullptr)
            {
                if (ImGui::Button("Attach Circle Guide"))
                {
                    pGuide = EnsureCircleGuideScript(pSelectedSurface.Get(), true);
                    ApplyCircleGuideToMatchingSurfaces(pSelectedSurface.Get());
                    m_StatusText = "Circle guide attached to the selected circle.";
                }
            }

            if (pGuide != nullptr)
            {
                bool changed = false;

                bool guideEnabled = pGuide->IsEnabled();
                if (ImGui::Checkbox("Enable Circle Guide", &guideEnabled))
                {
                    pGuide->SetEnabled(guideEnabled);
                    changed = true;
                }

                Vec2 circleCenter = {};
                float circleRadius = 0.f;
                Vec2 circleBoxMin = {};
                Vec2 circleBoxMax = {};
                pSelectedScript->GetArcWorldData(circleCenter, circleRadius, circleBoxMin, circleBoxMax);
                ImGui::Text("Guide Radius : %.1f", circleRadius);

                float entryAngle = pGuide->GetEntryAngleDeg();
                float exitAngle = pGuide->GetExitAngleDeg();
                if (ImGui::DragFloat("Entry Angle", &entryAngle, 0.5f, -180.f, 180.f))
                {
                    pGuide->SetEntryAngleDeg(entryAngle);
                    changed = true;
                }
                const bool entryOnRight = cosf(DegreesToRadians(pGuide->GetEntryAngleDeg())) >= 0.f;
                const ImVec4 halfUiColor = entryOnRight ? ImVec4(1.f, 0.35f, 0.35f, 1.f) : ImVec4(0.31f, 0.59f, 1.f, 1.f);
                ImGui::TextColored(halfUiColor, entryOnRight
                    ? "Half Check : red (right line open)"
                    : "Half Check : blue (left line open)");
                if (ImGui::DragFloat("Exit Angle", &exitAngle, 0.5f, -180.f, 180.f))
                {
                    pGuide->SetExitAngleDeg(exitAngle);
                    changed = true;
                }

                if (hasMouseWorld)
                {
                    if (ImGui::Button("Set Entry From Cursor"))
                    {
                        pGuide->SetEntryAngleDeg(ComputeCircleAngleDegrees(circleCenter, mouseWorld));
                        changed = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Set Exit From Cursor"))
                    {
                        pGuide->SetExitAngleDeg(ComputeCircleAngleDegrees(circleCenter, mouseWorld));
                        changed = true;
                    }
                }

                Vec2 corrStartWorld = circleCenter + pGuide->GetCorrectionLineStartLocal();
                Vec2 corrEndWorld = circleCenter + pGuide->GetCorrectionLineEndLocal();
                float corrStart[2] = { corrStartWorld.x, corrStartWorld.y };
                float corrEnd[2] = { corrEndWorld.x, corrEndWorld.y };
                if (ImGui::DragFloat2("Corr Start", corrStart, 1.f))
                {
                    pGuide->SetCorrectionLineStartLocal(Vec2(corrStart[0] - circleCenter.x, corrStart[1] - circleCenter.y));
                    changed = true;
                }
                if (ImGui::DragFloat2("Corr End", corrEnd, 1.f))
                {
                    pGuide->SetCorrectionLineEndLocal(Vec2(corrEnd[0] - circleCenter.x, corrEnd[1] - circleCenter.y));
                    changed = true;
                }

                if (hasMouseWorld)
                {
                    if (ImGui::Button("Set Corr Start From Cursor"))
                    {
                        pGuide->SetCorrectionLineStartLocal(mouseWorld - circleCenter);
                        changed = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Set Corr End From Cursor"))
                    {
                        pGuide->SetCorrectionLineEndLocal(mouseWorld - circleCenter);
                        changed = true;
                    }
                }

                if (ImGui::Button("Reset Guide Defaults"))
                {
                    pGuide->ResetToDefaultGuide(circleRadius);
                    pGuide->SetEnabled(true);
                    changed = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Create / Update Corr Line"))
                {
                    CreateOrUpdateCircleGuideCorrectionLine(pSelectedSurface.Get());
                    changed = true;
                }

                if (!pGuide->GetLinkedCorrectionLineName().empty())
                {
                    const string linkedName(pGuide->GetLinkedCorrectionLineName().begin(), pGuide->GetLinkedCorrectionLineName().end());
                    ImGui::TextWrapped("Linked Corr Line : %s", linkedName.c_str());

                    if (ImGui::Button("Select Corr Line"))
                    {
                        Ptr<GameObject> pLineObject = FindSurfaceObjectByName(pGuide->GetLinkedCorrectionLineName());
                        if (pLineObject != nullptr)
                        {
                            SelectSurfaceObject(pLineObject);
                            m_StatusText = "Linked correction line selected.";
                        }
                        else
                        {
                            m_StatusText = "Linked correction line was not found in the scene.";
                        }
                    }
                }

                ImGui::TextWrapped("Guide overlay shows only the visible arc, the A-B correction line, and blue/red Half state.");

                if (changed)
                {
                    ApplyCircleGuideToMatchingSurfaces(pSelectedSurface.Get());
                    if (!pGuide->GetLinkedCorrectionLineName().empty())
                        CreateOrUpdateCircleGuideCorrectionLine(pSelectedSurface.Get());
                }
            }
        }
        else
        {
            ImGui::TextWrapped("Select a circle surface to edit entry / half / exit and correction line guide data.");
        }
    }
    else
    {
        ImGui::TextWrapped("No surface selected. Use Inspector, Outliner, or the pick button above.");
    }

    if (m_FreeformRole == FREEFORM_ROLE::ERASE)
        ImGui::TextWrapped("Erase mode deletes the nearest surface with left click or Delete key.");

    if (!m_StatusText.empty())
    {
        ImGui::Separator();
        ImGui::TextWrapped("%s", m_StatusText.c_str());
    }

    HandleSceneInput();
    DrawSceneOverlay();
}
