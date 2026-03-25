#pragma once
#include "ComponentUI.h"
class TileRenderUI :
    public ComponentUI
{
private:
    // Inspector 안에서 선택 중인 타일 정의와 현재 브러시를 분리해둔다.
    // 덕분에 "정의 편집"과 "맵에 칠하기"를 같은 창 안에서 동시에 다룰 수 있다.
    int     m_SelectedTypeIdx;
    int     m_BrushTypeIdx;
    int     m_EditRow;
    int     m_EditCol;
    Vec2    m_EditTileSize;

    char    m_TileTypeName[128];

    class ATileMap* m_LastTileMap;
    int             m_LastSyncedTypeIdx;
    bool            m_RequestCollisionRebuild;

public:
    virtual void Tick_UI() override;

private:
    void SelectTileMap(DWORD_PTR _ListUI);
    void SyncUIState(class ATileMap* _TileMap);
    void RequestTileMapRefresh(bool _NeedRenderReset, bool _NeedCollisionRebuild);
    bool DrawShapeEditor(const char* _Label, struct TileShapeDesc& _Shape);
    bool DrawTileTypeEditor(class ATileMap* _TileMap);
    void DrawTilePaintCanvas(class ATileMap* _TileMap);
    void SaveTileMapAsset(class ATileMap* _TileMap);
    Ptr<class ATileMap> CreateTileMapAsset();

public:
    TileRenderUI();
    virtual ~TileRenderUI();
};

