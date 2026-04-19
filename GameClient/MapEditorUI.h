#pragma once

#include "EditorUI.h"

#include "ATileMap.h"
#include "GameObject.h"
#include "Source/Scripts/CSurfaceScript.h"
#include "Source/Scripts/CTileScript.h"

#include <vector>

class CTileRender;
class CCamera;
class CSurfaceCircleGuideScript;
class ASurfaceSet;
struct ImDrawList;

class MapEditorUI
    : public EditorUI
{
public:
    struct GRID_VERTEX
    {
        int x;
        int y;
    };

private:
    enum class MAP_TOOL
    {
        SURFACE_LINE = 0,
        ARC,
        WALL,
        CORRECTION,
        ERASE,
    };

    enum class ARC_CORNER
    {
        TOP_LEFT = 0,
        TOP_RIGHT,
        BOTTOM_LEFT,
        BOTTOM_RIGHT,
    };

    enum class WALL_SIDE
    {
        RIGHT = 0,
        LEFT,
        BELOW,
        ABOVE,
    };

    enum class CELL_KIND : unsigned char
    {
        NONE = 0,
        SURFACE,
        CORRECTION,
        WALL,
    };

    enum class FREEFORM_SHAPE
    {
        LINE = 0,
        FLAT,
        ARC,
        CIRCLE,
        FULL_CIRCLE,
    };

    enum class FREEFORM_ROLE
    {
        SURFACE = 0,
        CORRECTION,
        WALL,
        VERTICAL_ENTRY,
        VERTICAL_STICKY_ENTRY,
        ERASE,
    };

    enum class GUIDE_DRAG_HANDLE
    {
        NONE = 0,
        CORR_START,
        CORR_END,
    };

private:
    bool                    m_ShowGrid;
    bool                    m_ShowNormals;
    bool                    m_AutoSave;
    float                   m_GridOpacity;
    bool                    m_FreeformMode;

    MAP_TOOL                m_Tool;
    ARC_CORNER              m_ArcCorner;
    WALL_SIDE               m_WallSide;
    bool                    m_FillAbove;
    bool                    m_ArcFillInside;
    bool                    m_Attachable;
    bool                    m_EnablePointSnap;
    float                   m_PointSnapDistance;
    FREEFORM_SHAPE          m_FreeformShape;
    FREEFORM_ROLE           m_FreeformRole;
    bool                    m_UseCircleRadius;
    float                   m_CircleRadius;
    Ptr<ASurfaceSet>        m_SurfaceSetAsset;

    bool                    m_HasStartVertex;
    GRID_VERTEX             m_StartVertex;
    bool                    m_HasStartPoint;
    Vec2                    m_StartPoint;
    bool                    m_HasLastCreatedSegment;
    Vec2                    m_LastCreatedStart;
    Vec2                    m_LastCreatedEnd;
    GUIDE_DRAG_HANDLE       m_GuideDragHandle;

    int                     m_Row;
    int                     m_Col;
    Vec2                    m_TileSize;

    GameObject*             m_LastTargetObject;
    std::vector<int>        m_TileValues;
    std::vector<unsigned char> m_CellKinds;
    string                  m_StatusText;

public:
    virtual void Tick_UI() override;
    virtual void Activate() override;
    virtual void Deactivate() override;

private:
    Ptr<CCamera> GetSceneCamera() const;
    Ptr<GameObject> ResolveSurfaceRoot() const;
    Ptr<GameObject> EnsureSurfaceRoot();
    void CollectSurfaceObjects(vector<Ptr<GameObject>>& _OutObjects) const;
    Ptr<GameObject> FindSurfaceObjectByName(const wstring& _Name) const;
    bool GetSurfaceEndpointPair(CSurfaceScript* _Script, Vec2& _OutStart, Vec2& _OutEnd) const;
    Ptr<GameObject> FindNearestSurfaceObject(const Vec2& _WorldPos, float& _OutDistance) const;
    bool FindSnapPoint(const Vec2& _WorldPos, Vec2& _OutSnapPoint) const;
    void RefreshCircleGuideCorrectionLines();
    void BuildArcSurfaceBounds(const Vec2& _StartWorld, const Vec2& _EndWorld, Vec2& _OutMinBox, Vec2& _OutMaxBox, Vec2& _OutCenter) const;
    void BuildQuarterSurfaceBounds(const Vec2& _StartWorld, const Vec2& _EndWorld, bool _UseCircleRadius, Vec2& _OutMinBox, Vec2& _OutMaxBox, Vec2& _OutCenter, float& _OutRadius) const;
    void BuildFullCircleBounds(const Vec2& _StartWorld, const Vec2& _EndWorld, Vec2& _OutMinBox, Vec2& _OutMaxBox, Vec2& _OutCenter, float& _OutRadius) const;
    Ptr<GameObject> GetSelectedSurfaceObject() const;
    void SelectSurfaceObject(Ptr<GameObject> _Object) const;
    CSurfaceCircleGuideScript* EnsureCircleGuideScript(GameObject* _SurfaceObject, bool _ApplyDefaults = false) const;
    void ApplyCircleGuideToMatchingSurfaces(GameObject* _AnchorObject);
    void CreateOrUpdateCircleGuideCorrectionLine(GameObject* _CircleSurfaceObject);
    bool HandleSelectedCircleGuideDrag(const Vec2& _MouseWorld);
    void HandleFreeformInput();
    void DrawFreeformOverlay();
    void DrawCircleGuideOverlayForSurface(ImDrawList* _Draw, GameObject* _SurfaceObject, bool _ShowLabels);
    void DrawSelectedCircleGuideOverlay(ImDrawList* _Draw);
    void CreateLineSurfaceObject(const Vec2& _StartWorld, const Vec2& _EndWorld);
    void CreateArcSurfaceObject(const Vec2& _StartWorld, const Vec2& _EndWorld);
    void CreateCircleSurfaceObject(const Vec2& _StartWorld, const Vec2& _EndWorld);
    void CreateFullCircleSurfaceObject(const Vec2& _StartWorld, const Vec2& _EndWorld);
    void DeleteNearestSurfaceObject(const Vec2& _WorldPos);

    Ptr<GameObject> ResolveTargetObject() const;
    bool EnsureWorkingSetLoaded();
    bool CreateFreshTileMap();
    void ResetWorkingSet(int _Row, int _Col);
    void LoadWorkingSetFromDisk();
    void SaveWorkingSetToDisk();
    void ApplyWorkingSetToScene(bool _RebuildCollision = true);
    void RebuildCollisionChildren(GameObject* _TileMapObject);

    void HandleSceneInput();
    void DrawSceneOverlay();

    bool GetMouseWorldPos(Vec2& _OutWorldPos) const;
    bool WorldToGridVertex(const Vec2& _WorldPos, GRID_VERTEX& _OutVertex) const;
    bool GridVertexToWorld(const GRID_VERTEX& _Vertex, Vec2& _OutWorldPos) const;
    bool WorldToScreen(const Vec2& _WorldPos, ImVec2& _OutScreenPos) const;

    void ApplyStroke(const GRID_VERTEX& _Start, const GRID_VERTEX& _End);
    void ApplyLineStroke(const GRID_VERTEX& _Start, const GRID_VERTEX& _End, CELL_KIND _Kind);
    void ApplyArcStroke(const GRID_VERTEX& _Start, const GRID_VERTEX& _End);
    void ApplyWallStroke(const GRID_VERTEX& _Start, const GRID_VERTEX& _End);
    void ApplyEraseStroke(const GRID_VERTEX& _Start, const GRID_VERTEX& _End);

    void SetCell(int _Row, int _Col, int _TypeValue, CELL_KIND _Kind);
    void ClearCell(int _Row, int _Col);
    bool IsCellInRange(int _Row, int _Col) const;

    int FindOrCreateLineType(float _A, float _B, float _C);
    int FindOrCreateCircleType(float _Radius, float _CenterX, float _CenterY, float _CSign);

    Vec4 GetCellColor(int _Row, int _Col) const;
    bool GetCellNormal(int _Row, int _Col, Vec2& _OutNormal) const;

public:
    MapEditorUI();
    virtual ~MapEditorUI();
};
