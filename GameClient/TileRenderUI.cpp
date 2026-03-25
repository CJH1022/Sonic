#include "pch.h"
#include "TileRenderUI.h"

#include "AssetMgr.h"
#include "EditorMgr.h"
#include "LevelMgr.h"
#include "ListUI.h"
#include "func.h"

#include "assets.h"
#include "Source/Scripts/CTileScript.h"

namespace
{
    constexpr UINT EMPTY_TILE_TYPE_INDEX = (UINT)TILETYPE::EMPTY_BLOCK;

    TileShapeDesc MakeLineShape(float _A, float _B, float _C)
    {
        TileShapeDesc shape = {};
        shape.Mode = TILE_DRAW_MODE::LINE;
        shape.a = _A;
        shape.b = _B;
        shape.c = _C;
        return shape;
    }

    TileShapeDesc MakeCircleShape(float _Radius, float _CenterX, float _CenterY)
    {
        TileShapeDesc shape = {};
        shape.Mode = TILE_DRAW_MODE::CIRCLE;
        shape.r = _Radius;
        shape.centerX = _CenterX;
        shape.centerY = _CenterY;
        return shape;
    }

    TileShapeDesc MakeVerticalShape(float _X, float _C)
    {
        TileShapeDesc shape = {};
        shape.Mode = TILE_DRAW_MODE::VERTICAL;
        shape.a = _X;
        shape.c = _C;
        return shape;
    }

    string ToStringA(const wstring& _Text)
    {
        if (_Text.empty())
            return string();

        int size = WideCharToMultiByte(CP_UTF8, 0, _Text.c_str(), (int)_Text.size(), nullptr, 0, nullptr, nullptr);
        if (size <= 0)
            return string();

        string result;
        result.resize(size);
        WideCharToMultiByte(CP_UTF8, 0, _Text.c_str(), (int)_Text.size(), result.data(), size, nullptr, nullptr);
        return result;
    }

    wstring ToWStringA(const string& _Text)
    {
        if (_Text.empty())
            return wstring();

        int size = MultiByteToWideChar(CP_UTF8, 0, _Text.c_str(), (int)_Text.size(), nullptr, 0);
        if (size <= 0)
            return wstring();

        wstring result;
        result.resize(size);
        MultiByteToWideChar(CP_UTF8, 0, _Text.c_str(), (int)_Text.size(), result.data(), size);
        return result;
    }

    void CopyNameToBuffer(const wstring& _Name, char* _Buffer, size_t _BufferCount)
    {
        string text = ToStringA(_Name);
        strncpy_s(_Buffer, _BufferCount, text.c_str(), _TRUNCATE);
    }

    wstring ExtractFileStem(const wstring& _Path)
    {
        wstring stem = _Path;

        size_t slashPos = stem.find_last_of(L"\\/");
        if (slashPos != wstring::npos)
        {
            stem = stem.substr(slashPos + 1);
        }

        size_t dotPos = stem.find_last_of(L'.');
        if (dotPos != wstring::npos)
        {
            stem = stem.substr(0, dotPos);
        }

        if (stem.empty())
        {
            stem = L"TileMap";
        }

        return stem;
    }

    wstring BuildUniqueTileMapKey()
    {
        int idx = 0;
        while (true)
        {
            wchar_t buffer[64] = {};
            swprintf_s(buffer, L"CustomTileMap_%d", idx);

            if (nullptr == AssetMgr::GetInst()->FindAsset(ASSET_TYPE::TILEMAP, buffer))
            {
                return buffer;
            }

            ++idx;
        }
    }

    wstring BuildTileMapRelativePath(const wstring& _Key)
    {
        wstring stem = ExtractFileStem(_Key);
        return L"TileMap\\" + stem + L".tile";
    }

    void ClampShapeForEditor(TileShapeDesc& _Shape)
    {
        if (_Shape.Mode == TILE_DRAW_MODE::LINE)
        {
            if (_Shape.a < -1.f) _Shape.a = -1.f;
            if (_Shape.a > 2.f) _Shape.a = 2.f;
            if (_Shape.b < -1.f) _Shape.b = -1.f;
            if (_Shape.b > 2.f) _Shape.b = 2.f;

            if (_Shape.c > -0.001f && _Shape.c < 0.001f)
            {
                _Shape.c = -1.f;
            }
            else if (_Shape.c > 2.f)
            {
                _Shape.c = 2.f;
            }
            else if (_Shape.c < -2.f)
            {
                _Shape.c = -2.f;
            }
        }
        else if (_Shape.Mode == TILE_DRAW_MODE::CIRCLE)
        {
            if (_Shape.r < 0.f) _Shape.r = 0.f;
            if (_Shape.r > 2.f) _Shape.r = 2.f;
            if (_Shape.centerX < -1.f) _Shape.centerX = -1.f;
            if (_Shape.centerX > 2.f) _Shape.centerX = 2.f;
            if (_Shape.centerY < -1.f) _Shape.centerY = -1.f;
            if (_Shape.centerY > 2.f) _Shape.centerY = 2.f;
        }
        else if (_Shape.Mode == TILE_DRAW_MODE::VERTICAL)
        {
            if (_Shape.a < -1.f) _Shape.a = -1.f;
            if (_Shape.a > 2.f) _Shape.a = 2.f;

            if (_Shape.c > -0.001f && _Shape.c < 0.001f)
            {
                _Shape.c = 1.f;
            }
            else if (_Shape.c > 2.f)
            {
                _Shape.c = 2.f;
            }
            else if (_Shape.c < -2.f)
            {
                _Shape.c = -2.f;
            }
        }
    }

    void MarkCurrentLevelChanged()
    {
        Ptr<ALevel> pCurLevel = LevelMgr::GetInst()->GetCurLevel();
        if (nullptr != pCurLevel)
        {
            pCurLevel->SetChanged();
        }
    }

    bool IsReservedTileTypeIndex(int _Idx)
    {
        return _Idx <= (int)EMPTY_TILE_TYPE_INDEX;
    }

    const char* GetModeLabel(TILE_DRAW_MODE _Mode)
    {
        switch (_Mode)
        {
        case TILE_DRAW_MODE::LINE:
            return "Line";
        case TILE_DRAW_MODE::CIRCLE:
            return "Circle";
        case TILE_DRAW_MODE::VERTICAL:
            return "Vertical";
        default:
            return "Empty";
        }
    }

    ImU32 GetTileFillColor(const TileDrawInfo& _Info)
    {
        if ((_Info.flags & TILE_FLAG_SOLID) == 0)
            return IM_COL32(35, 39, 46, 255);

        switch ((TILE_DRAW_MODE)_Info.mode)
        {
        case TILE_DRAW_MODE::LINE:
            return IM_COL32(45, 92, 156, 255);
        case TILE_DRAW_MODE::CIRCLE:
            return IM_COL32(37, 121, 84, 255);
        case TILE_DRAW_MODE::VERTICAL:
            return IM_COL32(137, 92, 38, 255);
        default:
            return IM_COL32(35, 39, 46, 255);
        }
    }

    void DrawTilePreview(ImDrawList* _DrawList, const ImVec2& _Min, const ImVec2& _Max, const TileDrawInfo& _Info, const Vec2& _Scale, bool _Selected)
    {
        const float cellWidth = _Max.x - _Min.x;
        const float cellHeight = _Max.y - _Min.y;

        Vec2 tileScale = _Scale;
        if (tileScale.x < 0.1f) tileScale.x = 0.1f;
        else if (tileScale.x > 1.f) tileScale.x = 1.f;
        if (tileScale.y < 0.1f) tileScale.y = 0.1f;
        else if (tileScale.y > 1.f) tileScale.y = 1.f;

        const float width = cellWidth * tileScale.x;
        const float height = cellHeight * tileScale.y;
        const float centerX = (_Min.x + _Max.x) * 0.5f;
        const float centerY = (_Min.y + _Max.y) * 0.5f;
        const ImVec2 shapeMin(centerX - width * 0.5f, centerY - height * 0.5f);
        const ImVec2 shapeMax(centerX + width * 0.5f, centerY + height * 0.5f);

        const ImU32 emptyColor = IM_COL32(28, 31, 37, 255);
        const ImU32 fillColor = GetTileFillColor(_Info);
        const ImU32 contourColor = IM_COL32(255, 215, 80, 255);
        const ImU32 borderColor = _Selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(80, 88, 96, 255);

        _DrawList->AddRectFilled(_Min, _Max, emptyColor, 2.f);

        switch ((TILE_DRAW_MODE)_Info.mode)
        {
        case TILE_DRAW_MODE::LINE:
        {
            const float y0 = shapeMin.y + _Info.a * height;
            const float y1 = shapeMin.y + _Info.b * height;
            ImVec2 p0(shapeMin.x, y0);
            ImVec2 p1(shapeMax.x, y1);

            if (_Info.c < 0.f)
            {
                const ImVec2 poly[4] = { p0, p1, ImVec2(shapeMax.x, shapeMax.y), ImVec2(shapeMin.x, shapeMax.y) };
                _DrawList->AddConvexPolyFilled(poly, 4, fillColor);
            }
            else
            {
                const ImVec2 poly[4] = { ImVec2(shapeMin.x, shapeMin.y), ImVec2(shapeMax.x, shapeMin.y), p1, p0 };
                _DrawList->AddConvexPolyFilled(poly, 4, fillColor);
            }

            _DrawList->AddLine(p0, p1, contourColor, 2.f);
            break;
        }
        case TILE_DRAW_MODE::CIRCLE:
        {
            _DrawList->AddRectFilled(shapeMin, shapeMax, fillColor, 2.f);

            const ImVec2 center(
                shapeMin.x + _Info.center_x * width,
                shapeMin.y + _Info.center_y * height
            );
            const float radius = _Info.r * ((width < height) ? width : height);
            _DrawList->AddCircleFilled(center, radius, emptyColor, 28);
            _DrawList->AddCircle(center, radius, contourColor, 28, 2.f);
            break;
        }
        case TILE_DRAW_MODE::VERTICAL:
        {
            const float x = shapeMin.x + _Info.a * width;

            if (_Info.c >= 0.f)
            {
                _DrawList->AddRectFilled(shapeMin, ImVec2(x, shapeMax.y), fillColor, 2.f);
            }
            else
            {
                _DrawList->AddRectFilled(ImVec2(x, shapeMin.y), shapeMax, fillColor, 2.f);
            }

            _DrawList->AddLine(ImVec2(x, shapeMin.y), ImVec2(x, shapeMax.y), contourColor, 2.f);
            break;
        }
        default:
            break;
        }

        if ((_Info.flags & TILE_FLAG_USE_CUSTOM_NORMAL) != 0)
        {
            Vec2 customNormal = _Info.customNormal;
            if (customNormal.LengthSquared() > 0.0001f)
            {
                customNormal.Normalize();

                const ImVec2 start((_Min.x + _Max.x) * 0.5f, (_Min.y + _Max.y) * 0.5f);
                const ImVec2 end(
                    start.x + customNormal.x * width * 0.25f,
                    start.y - customNormal.y * height * 0.25f
                );

                _DrawList->AddLine(start, end, IM_COL32(80, 255, 140, 255), 2.f);
            }
        }

        _DrawList->AddRect(_Min, _Max, borderColor, 2.f, 0, _Selected ? 2.f : 1.f);
        _DrawList->AddRect(shapeMin, shapeMax, IM_COL32(180, 188, 198, 120), 2.f, 0, 1.f);
    }
}

TileRenderUI::TileRenderUI()
    : ComponentUI(COMPONENT_TYPE::TILE_RENDER, "TileRenderUI")
    , m_SelectedTypeIdx((int)TILETYPE::LINE_BLOCK_1)
    , m_BrushTypeIdx((int)TILETYPE::LINE_BLOCK_1)
    , m_EditRow(0)
    , m_EditCol(0)
    , m_EditTileSize(Vec2(0.f, 0.f))
    , m_SelectedCellRow(-1)
    , m_SelectedCellCol(-1)
    , m_LastTileMap(nullptr)
    , m_LastSyncedTypeIdx(-1)
    , m_RequestCollisionRebuild(false)
{
    m_TileTypeName[0] = '\0';
}

TileRenderUI::~TileRenderUI()
{
}

void TileRenderUI::SyncUIState(ATileMap* _TileMap)
{
    if (m_LastTileMap != _TileMap)
    {
        m_LastTileMap = _TileMap;
        m_LastSyncedTypeIdx = -1;

        if (nullptr != _TileMap)
        {
            m_EditRow = (int)_TileMap->GetRow();
            m_EditCol = (int)_TileMap->GetCol();
            m_EditTileSize = _TileMap->GetTileSize();
            m_SelectedCellRow = -1;
            m_SelectedCellCol = -1;
        }
    }

    if (nullptr == _TileMap)
        return;

    const vector<TileTypeDesc>& defs = _TileMap->GetTileTypeDescs();
    if (defs.empty())
        return;

    if (m_SelectedTypeIdx < 0 || defs.size() <= (size_t)m_SelectedTypeIdx)
        m_SelectedTypeIdx = (int)EMPTY_TILE_TYPE_INDEX;

    if (m_BrushTypeIdx < 0 || defs.size() <= (size_t)m_BrushTypeIdx)
        m_BrushTypeIdx = m_SelectedTypeIdx;

    if (m_LastSyncedTypeIdx != m_SelectedTypeIdx)
    {
        CopyNameToBuffer(defs[m_SelectedTypeIdx].Name, m_TileTypeName, sizeof(m_TileTypeName));
        m_LastSyncedTypeIdx = m_SelectedTypeIdx;
    }
}

void TileRenderUI::RequestTileMapRefresh(bool _NeedRenderReset, bool _NeedCollisionRebuild)
{
    Ptr<CTileRender> pTileRender = GetTarget()->TileRender();
    Ptr<ATileMap> pTileMap = pTileRender->GetTileMap();

    if (_NeedRenderReset && nullptr != pTileMap)
    {
        // Row/Col, TileSize 처럼 메쉬/버퍼 크기까지 다시 잡아야 하는 수정만
        // SetTileMap 으로 재초기화한다.
        pTileRender->SetTileMap(pTileMap);
    }

    if (_NeedCollisionRebuild)
    {
        m_RequestCollisionRebuild = true;
    }

    MarkCurrentLevelChanged();
}

bool TileRenderUI::DrawShapeEditor(const char* _Label, TileShapeDesc& _Shape)
{
    bool changed = false;

    if (ImGui::TreeNode(_Label))
    {
        ImGui::PushID(_Label);

        const char* modeLabels[] = { "Empty", "Line", "Circle", "Vertical" };
        int mode = (int)_Shape.Mode;
        if (ImGui::Combo("Mode", &mode, modeLabels, IM_ARRAYSIZE(modeLabels)))
        {
            _Shape.Mode = (TILE_DRAW_MODE)mode;
            changed = true;
        }

        switch (_Shape.Mode)
        {
        case TILE_DRAW_MODE::LINE:
            ImGui::TextWrapped("Line mode uses f(x, y) = c * (y - (b - a) * x - a).");
            ImGui::TextWrapped("Preset buttons help you start from the stock Sonic slopes, then refine a/b/c for custom ones.");

            if (ImGui::Button("Line 1")) { _Shape = MakeLineShape(0.1f, 0.4f, -1.f); changed = true; }
            ImGui::SameLine();
            if (ImGui::Button("Line 2")) { _Shape = MakeLineShape(0.4f, 0.8f, -1.f); changed = true; }
            ImGui::SameLine();
            if (ImGui::Button("Line 3")) { _Shape = MakeLineShape(0.8f, 0.8f, -1.f); changed = true; }

            if (ImGui::Button("Line 4")) { _Shape = MakeLineShape(0.8f, 0.4f, -1.f); changed = true; }
            ImGui::SameLine();
            if (ImGui::Button("Line 5")) { _Shape = MakeLineShape(0.4f, 0.1f, -1.f); changed = true; }
            ImGui::SameLine();
            if (ImGui::Button("Line 6")) { _Shape = MakeLineShape(0.1f, 0.1f, -1.f); changed = true; }

            if (ImGui::Button("Swap Ends"))
            {
                float temp = _Shape.a;
                _Shape.a = _Shape.b;
                _Shape.b = temp;
                changed = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Flip Solid Side"))
            {
                _Shape.c = -_Shape.c;
                if (_Shape.c > -0.001f && _Shape.c < 0.001f)
                    _Shape.c = -1.f;
                changed = true;
            }

            changed |= ImGui::DragFloat("a", &_Shape.a, 0.01f, -1.f, 2.f);
            changed |= ImGui::DragFloat("b", &_Shape.b, 0.01f, -1.f, 2.f);
            changed |= ImGui::DragFloat("c", &_Shape.c, 0.01f, -2.f, 2.f);
            break;
        case TILE_DRAW_MODE::CIRCLE:
            ImGui::TextWrapped("Circle mode keeps the outside of the circle solid, so corner arcs are easy to build.");
            ImGui::TextWrapped("Corner presets place quarter-circle tiles fast, and you can still refine radius/center for custom arcs.");

            if (ImGui::Button("Top Left")) { _Shape = MakeCircleShape((_Shape.r <= 0.f) ? 0.8f : _Shape.r, 0.f, 0.f); changed = true; }
            ImGui::SameLine();
            if (ImGui::Button("Top Right")) { _Shape = MakeCircleShape((_Shape.r <= 0.f) ? 0.8f : _Shape.r, 1.f, 0.f); changed = true; }
            ImGui::SameLine();
            if (ImGui::Button("Bottom Left")) { _Shape = MakeCircleShape((_Shape.r <= 0.f) ? 0.8f : _Shape.r, 0.f, 1.f); changed = true; }
            ImGui::SameLine();
            if (ImGui::Button("Bottom Right")) { _Shape = MakeCircleShape((_Shape.r <= 0.f) ? 0.8f : _Shape.r, 1.f, 1.f); changed = true; }

            if (ImGui::Button("R 0.50")) { _Shape.r = 0.5f; changed = true; }
            ImGui::SameLine();
            if (ImGui::Button("R 0.80")) { _Shape.r = 0.8f; changed = true; }
            ImGui::SameLine();
            if (ImGui::Button("R 1.00")) { _Shape.r = 1.f; changed = true; }

            changed |= ImGui::DragFloat("radius", &_Shape.r, 0.01f, 0.f, 2.f);
            changed |= ImGui::DragFloat("centerX", &_Shape.centerX, 0.01f, -1.f, 2.f);
            changed |= ImGui::DragFloat("centerY", &_Shape.centerY, 0.01f, -1.f, 2.f);
            break;
        case TILE_DRAW_MODE::VERTICAL:
            ImGui::TextWrapped("Vertical mode uses f(x, y) = c * (x - a). a is the wall position, c picks the solid side.");

            if (ImGui::Button("Left Wall")) { _Shape = MakeVerticalShape(0.15f, 1.f); changed = true; }
            ImGui::SameLine();
            if (ImGui::Button("Center Wall")) { _Shape = MakeVerticalShape(0.5f, 1.f); changed = true; }
            ImGui::SameLine();
            if (ImGui::Button("Right Wall")) { _Shape = MakeVerticalShape(0.85f, 1.f); changed = true; }
            ImGui::SameLine();
            if (ImGui::Button("Flip Solid Side")) { _Shape.c = -_Shape.c; changed = true; }

            changed |= ImGui::DragFloat("wallX", &_Shape.a, 0.01f, -1.f, 2.f);
            changed |= ImGui::DragFloat("c", &_Shape.c, 0.01f, -2.f, 2.f);
            break;
        default:
            ImGui::TextWrapped("Empty mode leaves the cell open and skips collision.");
            break;
        }

        ClampShapeForEditor(_Shape);

        ImGui::Spacing();
        ImGui::Text("Shape Preview");
        {
            TileDrawInfo preview = {};
            preview.a = _Shape.a;
            preview.b = _Shape.b;
            preview.c = _Shape.c;
            preview.r = _Shape.r;
            preview.center_x = _Shape.centerX;
            preview.center_y = _Shape.centerY;
            preview.mode = (int)_Shape.Mode;
            preview.flags = TILE_FLAG_SOLID;

            const ImVec2 previewSize(92.f, 92.f);
            const ImVec2 previewMin = ImGui::GetCursorScreenPos();
            ImGui::InvisibleButton("##ShapePreview", previewSize);
            const ImVec2 previewMax = ImGui::GetItemRectMax();

            DrawTilePreview(ImGui::GetWindowDrawList(), previewMin, previewMax, preview, Vec2(1.f, 1.f), true);
        }

        ImGui::PopID();
        ImGui::TreePop();
    }

    return changed;
}

bool TileRenderUI::DrawTileTypeEditor(ATileMap* _TileMap)
{
    if (nullptr == _TileMap)
        return false;

    TileTypeDesc* pDesc = _TileMap->GetTileTypeDesc((UINT)m_SelectedTypeIdx);
    if (nullptr == pDesc)
        return false;

    bool changed = false;
    bool bReservedType = IsReservedTileTypeIndex(m_SelectedTypeIdx);

    if (bReservedType)
    {
        ImGui::TextWrapped("Empty/Reserved slots stay as the map's safety fallback, so deletion is blocked.");
    }

    if (ImGui::InputText("Tile Name", m_TileTypeName, sizeof(m_TileTypeName)))
    {
        pDesc->Name = ToWStringA(m_TileTypeName);
        changed = true;
    }

    bool solid = (pDesc->Flags & TILE_FLAG_SOLID) != 0;
    if (ImGui::Checkbox("Solid", &solid))
    {
        if (solid) pDesc->Flags |= TILE_FLAG_SOLID;
        else pDesc->Flags &= ~TILE_FLAG_SOLID;
        changed = true;
    }

    bool attachable = (pDesc->Flags & TILE_FLAG_ATTACHABLE) != 0;
    if (ImGui::Checkbox("Attachable", &attachable))
    {
        if (attachable) pDesc->Flags |= TILE_FLAG_ATTACHABLE;
        else pDesc->Flags &= ~TILE_FLAG_ATTACHABLE;
        changed = true;
    }

    bool useCustomNormal = (pDesc->Flags & TILE_FLAG_USE_CUSTOM_NORMAL) != 0;
    if (ImGui::Checkbox("Use Custom Normal", &useCustomNormal))
    {
        if (useCustomNormal) pDesc->Flags |= TILE_FLAG_USE_CUSTOM_NORMAL;
        else pDesc->Flags &= ~TILE_FLAG_USE_CUSTOM_NORMAL;
        changed = true;
    }

    if (useCustomNormal)
    {
        ImGui::TextWrapped("Custom normal is authored in world-like axes: (0, 1) means upward, (-1, 0) means left wall.");
        if (ImGui::Button("Up")) { pDesc->CustomNormal = Vec2(0.f, 1.f); changed = true; }
        ImGui::SameLine();
        if (ImGui::Button("Down")) { pDesc->CustomNormal = Vec2(0.f, -1.f); changed = true; }
        ImGui::SameLine();
        if (ImGui::Button("Left")) { pDesc->CustomNormal = Vec2(-1.f, 0.f); changed = true; }
        ImGui::SameLine();
        if (ImGui::Button("Right")) { pDesc->CustomNormal = Vec2(1.f, 0.f); changed = true; }
        changed |= ImGui::DragFloat2("Custom Normal", (float*)&pDesc->CustomNormal, 0.01f, -1.f, 1.f);
    }

    ImGui::Separator();

    changed |= DrawShapeEditor("Main Shape", pDesc->MainShape);

    const char* correctionLabels[] =
    {
        "None",
        "Use Correction When HalfChecker = false",
        "Use Correction When HalfChecker = true",
    };

    int correctionTrigger = (int)pDesc->CorrectionTrigger;
    if (ImGui::Combo("Correction Trigger", &correctionTrigger, correctionLabels, IM_ARRAYSIZE(correctionLabels)))
    {
        pDesc->CorrectionTrigger = (TILE_CORRECTION_TRIGGER)correctionTrigger;
        changed = true;
    }

    if (pDesc->CorrectionTrigger != TILE_CORRECTION_TRIGGER::NONE)
    {
        ImGui::TextWrapped("Circle tiles often use a correction slope when half-checker changes, but this correction shape can also be fully custom.");

        if (ImGui::Button("Copy Main -> Correction"))
        {
            pDesc->CorrectionShape = pDesc->MainShape;
            changed = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Correction = Flat"))
        {
            pDesc->CorrectionShape = MakeLineShape(0.8f, 0.8f, -1.f);
            changed = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Correction = Diag"))
        {
            pDesc->CorrectionShape = MakeLineShape(0.2f, 0.8f, -1.f);
            changed = true;
        }

        ImGui::TextWrapped("Correction Shape is the backup slope/curve used when the global half-checker flips.");
        changed |= DrawShapeEditor("Correction Shape", pDesc->CorrectionShape);
    }

    return changed;
}

void TileRenderUI::DrawTilePaintCanvas(ATileMap* _TileMap)
{
    if (nullptr == _TileMap)
        return;

    const UINT row = _TileMap->GetRow();
    const UINT col = _TileMap->GetCol();
    const vector<UINT>& tileTypes = _TileMap->GetTileTypes();
    const vector<Vec2>& tileScales = _TileMap->GetTileScales();
    const vector<TileTypeDesc>& tileDefs = _TileMap->GetTileTypeDescs();

    if (0 == row || 0 == col)
        return;

    if (ImGui::BeginChild("TilePaintCanvas", Vec2(0.f, 320.f), true, ImGuiWindowFlags_HorizontalScrollbar))
    {
        const float availWidth = ImGui::GetContentRegionAvail().x;
        float cellSize = availWidth / (float)col;
        if (cellSize < 24.f) cellSize = 24.f;
        if (cellSize > 56.f) cellSize = 56.f;

        ImVec2 canvasSize(cellSize * col, cellSize * row);
        ImGui::InvisibleButton("##TileCanvasButton", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

        const ImVec2 canvasMin = ImGui::GetItemRectMin();
        const ImVec2 canvasMax = ImGui::GetItemRectMax();
        ImDrawList* pDrawList = ImGui::GetWindowDrawList();

        pDrawList->AddRectFilled(canvasMin, canvasMax, IM_COL32(17, 19, 23, 255), 3.f);

        bool hoveredCanvas = ImGui::IsItemHovered();
        int hoveredRow = -1;
        int hoveredCol = -1;

        if (hoveredCanvas)
        {
            ImVec2 mousePos = ImGui::GetMousePos();
            hoveredCol = (int)((mousePos.x - canvasMin.x) / cellSize);
            hoveredRow = (int)((mousePos.y - canvasMin.y) / cellSize);

            if (hoveredCol < 0 || col <= (UINT)hoveredCol || hoveredRow < 0 || row <= (UINT)hoveredRow)
            {
                hoveredCol = -1;
                hoveredRow = -1;
            }
        }

        const bool halfChecker = CTileScript::GetHalfCheckerState();

        for (UINT y = 0; y < row; ++y)
        {
            for (UINT x = 0; x < col; ++x)
            {
                const UINT idx = y * col + x;
                const UINT typeIdx = (idx < tileTypes.size()) ? tileTypes[idx] : EMPTY_TILE_TYPE_INDEX;

                ImVec2 cellMin(canvasMin.x + x * cellSize, canvasMin.y + y * cellSize);
                ImVec2 cellMax(cellMin.x + cellSize, cellMin.y + cellSize);

                TileDrawInfo preview = {};
                if (typeIdx < tileDefs.size())
                {
                    CTileScript::ResolveTileTypeDesc(tileDefs[typeIdx], halfChecker, preview);
                }

                Vec2 cellScale = Vec2(1.f, 1.f);
                if (idx < tileScales.size())
                {
                    cellScale = tileScales[idx];
                }

                const bool hoveredCell = ((int)y == hoveredRow && (int)x == hoveredCol);
                const bool selectedCell = ((int)y == m_SelectedCellRow && (int)x == m_SelectedCellCol);
                DrawTilePreview(pDrawList, cellMin, cellMax, preview, cellScale, hoveredCell || selectedCell);

                if (cellSize >= 30.f)
                {
                    char numberText[16] = {};
                    sprintf_s(numberText, "%u", typeIdx);
                    pDrawList->AddText(ImVec2(cellMin.x + 4.f, cellMin.y + 3.f), IM_COL32(220, 226, 230, 255), numberText);
                }
            }
        }

        if (hoveredRow >= 0 && hoveredCol >= 0)
        {
            const bool selectOnly = ImGui::GetIO().KeyCtrl && ImGui::IsMouseDown(ImGuiMouseButton_Left);
            UINT desiredType = (UINT)m_BrushTypeIdx;

            if (selectOnly)
            {
                m_SelectedCellRow = hoveredRow;
                m_SelectedCellCol = hoveredCol;
            }
            else if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                m_SelectedCellRow = hoveredRow;
                m_SelectedCellCol = hoveredCol;
                const UINT currentType = _TileMap->GetTileType((UINT)hoveredRow, (UINT)hoveredCol);
                if (currentType != desiredType)
                {
                    _TileMap->SetTileType((UINT)hoveredRow, (UINT)hoveredCol, desiredType);
                    RequestTileMapRefresh(false, true);
                }
            }
            else if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
            {
                m_SelectedCellRow = hoveredRow;
                m_SelectedCellCol = hoveredCol;
                const UINT currentType = _TileMap->GetTileType((UINT)hoveredRow, (UINT)hoveredCol);
                if (currentType != EMPTY_TILE_TYPE_INDEX)
                {
                    _TileMap->SetTileType((UINT)hoveredRow, (UINT)hoveredCol, EMPTY_TILE_TYPE_INDEX);
                    RequestTileMapRefresh(false, true);
                }
            }

            ImGui::SetCursorScreenPos(ImVec2(canvasMin.x, canvasMax.y + 8.f));
            ImGui::Text("Hover : Row %d  Col %d", hoveredRow, hoveredCol);
        }
    }
    ImGui::EndChild();
}

void TileRenderUI::SaveTileMapAsset(ATileMap* _TileMap)
{
    if (nullptr == _TileMap)
        return;

    wstring key = _TileMap->GetKey();
    if (key.empty())
    {
        key = BuildUniqueTileMapKey();
        AssetMgr::GetInst()->AddAsset(key, _TileMap);
    }

    wstring relativePath = _TileMap->GetRelativePath();
    if (relativePath.empty())
    {
        relativePath = BuildTileMapRelativePath(key);
    }

    CreateDirectoryW((wstring(CONTENT_PATH) + L"TileMap").c_str(), nullptr);

    wstring filePath = wstring(CONTENT_PATH) + relativePath;
    if (SUCCEEDED(_TileMap->Save(filePath)))
    {
        Ptr<ATileMap> pLoaded = AssetMgr::GetInst()->Load<ATileMap>(key, relativePath);
        if (nullptr != pLoaded)
        {
            GetTarget()->TileRender()->SetTileMap(pLoaded);
            SyncUIState(pLoaded.Get());
        }
    }
}

Ptr<ATileMap> TileRenderUI::CreateTileMapAsset()
{
    Ptr<ATileMap> pTileMap = new ATileMap;
    pTileMap->SetRowCol(8, 16);
    pTileMap->SetTileSize(Vec2(96.f, 96.f));

    wstring key = BuildUniqueTileMapKey();
    AssetMgr::GetInst()->AddAsset(key, pTileMap.Get());

    SaveTileMapAsset(pTileMap.Get());
    return AssetMgr::GetInst()->Find<ATileMap>(key);
}

void TileRenderUI::Tick_UI()
{
    OutputTitle("TileRender");

    Ptr<CTileRender> pTileRender = GetTarget()->TileRender();
    Ptr<ATileMap> pTileMap = pTileRender->GetTileMap();

    ImGui::Text("TileMap");
    ImGui::SameLine(120);

    string tileMapKey = (nullptr == pTileMap) ? "None" : ToStringA(pTileMap->GetKey());
    ImGui::TextWrapped("%s", tileMapKey.c_str());

    if (ImGui::BeginDragDropTarget())
    {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content");
        if (payload)
        {
            DWORD_PTR data = *((DWORD_PTR*)payload->Data);
            Ptr<Asset> pAsset = (Asset*)data;

            if (ASSET_TYPE::TILEMAP == pAsset->GetType())
            {
                pTileRender->SetTileMap((ATileMap*)pAsset.Get());
                pTileMap = pTileRender->GetTileMap();
                SyncUIState(pTileMap.Get());
                RequestTileMapRefresh(true, true);
            }
        }

        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();
    if (ImGui::Button("...", Vec2(28.f, 20.f)))
    {
        Ptr<ListUI> pUI = dynamic_cast<ListUI*>(EditorMgr::GetInst()->FindUI("ListUI").Get());
        assert(pUI.Get());

        pUI->SetUIName("TileMap List");

        vector<wstring> vecTileMapNames;
        AssetMgr::GetInst()->GetAssetNames(ASSET_TYPE::TILEMAP, vecTileMapNames);
        pUI->AddString(vecTileMapNames);
        pUI->AddDelegate(this, (DELEGATE_1)&TileRenderUI::SelectTileMap);
        pUI->SetActive(true);
    }

    ImGui::SameLine();
    if (ImGui::Button("New TileMap"))
    {
        Ptr<ATileMap> pNewTileMap = CreateTileMapAsset();
        if (nullptr != pNewTileMap)
        {
            pTileRender->SetTileMap(pNewTileMap);
            pTileMap = pNewTileMap;
            SyncUIState(pTileMap.Get());
            RequestTileMapRefresh(true, true);
        }
    }

    float opacity = pTileRender->GetOpacity();
    ImGui::Text("Opacity");
    ImGui::SameLine(120);
    if (ImGui::DragFloat("##TileOpacity", &opacity, 0.01f, 0.f, 1.f))
    {
        pTileRender->SetOpacity(opacity);
    }

    if (nullptr == pTileMap)
    {
        ImGui::Spacing();
        ImGui::TextWrapped("Attach or create a TileMap asset first. After that, this inspector becomes your tile type library + paint tool.");
        return;
    }

    SyncUIState(pTileMap.Get());

    ImGui::Separator();
    ImGui::Text("Grid Setup");

    if (ImGui::InputInt("Row", &m_EditRow))
    {
        if (m_EditRow < 1) m_EditRow = 1;
    }

    if (ImGui::InputInt("Col", &m_EditCol))
    {
        if (m_EditCol < 1) m_EditCol = 1;
    }

    ImGui::DragFloat2("Tile Size", (float*)&m_EditTileSize, 1.f, 1.f, 4096.f);
    if (m_EditTileSize.x < 1.f) m_EditTileSize.x = 1.f;
    if (m_EditTileSize.y < 1.f) m_EditTileSize.y = 1.f;

    if (ImGui::Button("Apply Grid"))
    {
        pTileMap->Resize((UINT)m_EditRow, (UINT)m_EditCol);
        pTileMap->SetTileSize(m_EditTileSize);
        RequestTileMapRefresh(true, true);
    }

    ImGui::SameLine();
    if (ImGui::Button("Save TileMap"))
    {
        SaveTileMapAsset(pTileMap.Get());
    }

    ImGui::SameLine();
    if (ImGui::Button("Rebuild Collision"))
    {
        m_RequestCollisionRebuild = true;
    }

    ImGui::Separator();
    ImGui::Text("Tile Type Library");

    if (ImGui::Button("Add Tile Type"))
    {
        TileTypeDesc newDesc = {};
        newDesc.Name = L"Custom Tile";
        newDesc.LegacyType = (UINT)TILETYPE::EMPTY_BLOCK;
        newDesc.Flags = TILE_FLAG_SOLID | TILE_FLAG_ATTACHABLE;
        newDesc.MainShape.Mode = TILE_DRAW_MODE::LINE;
        newDesc.MainShape.a = 0.2f;
        newDesc.MainShape.b = 0.8f;
        newDesc.MainShape.c = -1.f;

        m_SelectedTypeIdx = pTileMap->AddTileType(newDesc);
        m_BrushTypeIdx = m_SelectedTypeIdx;
        m_LastSyncedTypeIdx = -1;
        SyncUIState(pTileMap.Get());
        RequestTileMapRefresh(false, true);
    }

    ImGui::SameLine();
    if (ImGui::Button("Duplicate Selected"))
    {
        m_SelectedTypeIdx = pTileMap->DuplicateTileType((UINT)m_SelectedTypeIdx);
        if (m_SelectedTypeIdx >= 0)
        {
            m_BrushTypeIdx = m_SelectedTypeIdx;
            m_LastSyncedTypeIdx = -1;
            SyncUIState(pTileMap.Get());
            RequestTileMapRefresh(false, true);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Delete Selected"))
    {
        if (!IsReservedTileTypeIndex(m_SelectedTypeIdx))
        {
            pTileMap->RemoveTileType((UINT)m_SelectedTypeIdx);
            if (m_SelectedTypeIdx >= (int)pTileMap->GetTileTypeDescs().size())
                m_SelectedTypeIdx = (int)pTileMap->GetTileTypeDescs().size() - 1;

            m_BrushTypeIdx = m_SelectedTypeIdx;
            m_LastSyncedTypeIdx = -1;
            SyncUIState(pTileMap.Get());
            RequestTileMapRefresh(false, true);
        }
    }

    if (ImGui::BeginListBox("##TileTypeList", Vec2(0.f, 160.f)))
    {
        const vector<TileTypeDesc>& defs = pTileMap->GetTileTypeDescs();
        for (size_t i = 0; i < defs.size(); ++i)
        {
            string label = to_string((int)i) + " : " + ToStringA(defs[i].Name);
            if (ImGui::Selectable(label.c_str(), m_SelectedTypeIdx == (int)i))
            {
                m_SelectedTypeIdx = (int)i;
                m_LastSyncedTypeIdx = -1;
            }
        }

        ImGui::EndListBox();
    }

    if (ImGui::Button("Use Selected As Brush"))
    {
        m_BrushTypeIdx = m_SelectedTypeIdx;
    }

    ImGui::SameLine();
    {
        const TileTypeDesc* pBrushDesc = pTileMap->GetTileTypeDesc((UINT)m_BrushTypeIdx);
        string brushLabel = (nullptr == pBrushDesc) ? "None" : ToStringA(pBrushDesc->Name);
        ImGui::Text("Brush : %s (%s)", brushLabel.c_str(), (nullptr == pBrushDesc) ? "-" : GetModeLabel(pBrushDesc->MainShape.Mode));
    }

    if (DrawTileTypeEditor(pTileMap.Get()))
    {
        RequestTileMapRefresh(false, true);
    }

    ImGui::Separator();
    ImGui::Text("Paint Grid");
    ImGui::TextWrapped("Left drag paints the current brush. Right drag erases back to the empty tile. Ctrl + Left Click only selects a cell for per-cell size editing.");
    DrawTilePaintCanvas(pTileMap.Get());

    ImGui::Separator();
    ImGui::Text("Selected Cell Size");

    if (m_SelectedCellRow >= 0 && m_SelectedCellCol >= 0 &&
        m_SelectedCellRow < (int)pTileMap->GetRow() &&
        m_SelectedCellCol < (int)pTileMap->GetCol())
    {
        Vec2 cellScale = pTileMap->GetTileScale((UINT)m_SelectedCellRow, (UINT)m_SelectedCellCol);
        const UINT cellType = pTileMap->GetTileType((UINT)m_SelectedCellRow, (UINT)m_SelectedCellCol);
        const TileTypeDesc* pCellDesc = pTileMap->GetTileTypeDesc(cellType);

        ImGui::Text("Cell");
        ImGui::SameLine(120);
        ImGui::Text("Row %d  Col %d", m_SelectedCellRow, m_SelectedCellCol);

        ImGui::Text("Type");
        ImGui::SameLine(120);
        ImGui::Text("%s", (nullptr == pCellDesc) ? "None" : ToStringA(pCellDesc->Name).c_str());

        bool changedScale = false;
        changedScale |= ImGui::DragFloat2("Cell Scale", (float*)&cellScale, 0.01f, 0.1f, 1.f);

        if (ImGui::Button("50%"))
        {
            cellScale = Vec2(0.5f, 0.5f);
            changedScale = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("75%"))
        {
            cellScale = Vec2(0.75f, 0.75f);
            changedScale = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("100%"))
        {
            cellScale = Vec2(1.f, 1.f);
            changedScale = true;
        }

        if (ImGui::Button("Wide"))
        {
            cellScale = Vec2(1.f, 0.5f);
            changedScale = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Tall"))
        {
            cellScale = Vec2(0.5f, 1.f);
            changedScale = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Cell Size"))
        {
            cellScale = Vec2(1.f, 1.f);
            changedScale = true;
        }

        if (changedScale)
        {
            pTileMap->SetTileScale((UINT)m_SelectedCellRow, (UINT)m_SelectedCellCol, cellScale);
            RequestTileMapRefresh(false, true);
        }
    }
    else
    {
        ImGui::TextWrapped("No cell selected yet. Use Ctrl + Left Click on the paint grid to pick one cell without repainting it.");
    }

    if (m_RequestCollisionRebuild && !ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        RebuildTileCollision(GetTarget().Get());
        m_RequestCollisionRebuild = false;
    }
}

void TileRenderUI::SelectTileMap(DWORD_PTR _ListUI)
{ 
    Ptr<ListUI> pListUI = ((ListUI*)_ListUI);
    wstring key = wstring(pListUI->GetSelectedString().begin(), pListUI->GetSelectedString().end());

    Ptr<ATileMap> pTileMap = FIND(ATileMap, key);
    GetTarget()->TileRender()->SetTileMap(pTileMap);
    SyncUIState(pTileMap.Get());
    RequestTileMapRefresh(true, true);
}
