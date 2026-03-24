#pragma once

#include "EditorUI.h"
#include "Source/Scripts/CTileScript.h"

class TileScriptEditor :
    public EditorUI
{
private:
    enum PREVIEW_MODE
    {
        PREVIEW_MAP_EDIT = 0,
        PREVIEW_SELECTED_TYPE = 1,
        PREVIEW_ALL_TYPE = 2,
        PREVIEW_MAP_OVERVIEW = 3,
    };

private:
    UINT        m_Row;
    UINT        m_Col;
    int         m_EditRow;
    int         m_EditCol;
    vector<int> m_vecTileValues;
    CTileScript::TILE_MAP_PLACEMENT m_MapPlacement;

    int         m_BrushTypeValue;
    int         m_EditTypeValue;

    float       m_CellSize;
    bool        m_ShowTypeNumber;
    bool        m_ShowShapeOverlay;
    int         m_PreviewMode;
    int         m_DragLineHandle;

    char        m_szPresetRelativePath[128];
    string      m_StatusText;

private:
    void EnsureTileBuffer();

    int  GetTileValue(UINT _Row, UINT _Col) const;
    void SetTileValue(UINT _Row, UINT _Col, int _Value);

    void DrawToolPanel();
    void DrawTypeConfigPanel();
    void DrawCanvasPanel();
    void DrawSelectedTypePreviewPanel();
    void DrawAllTypePreviewPanel();
    void DrawWholeMapOverviewPanel();

    float EvaluateTypeValue(TILETYPE _Type, float _X, float _Y) const;
    void DrawTileMask(ImDrawList* _DrawList, const ImVec2& _Min, const ImVec2& _Max, TILETYPE _Type, bool _DrawOutline) const;

    void LoadDefaultMap();
    bool SavePreset();
    bool LoadPreset();

    ImU32 GetCellColor(int _TypeValue) const;

public:
    virtual void Tick_UI() override;

public:
    TileScriptEditor();
    virtual ~TileScriptEditor();
};
