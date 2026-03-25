#pragma once
#include "Asset.h"

#include "ASprite.h"

// Tile cell이 참조하는 실제 타일 정의는 이 구조체에 담긴다.
// 셀 데이터는 "타일 정의 인덱스"만 저장하고, 렌더/충돌은 인덱스를 따라
// 아래 정의를 읽어와 처리한다.
enum class TILE_DRAW_MODE
{
    EMPTY = 0,
    LINE = 1,
    CIRCLE = 2,
    VERTICAL = 3,
};

enum class TILEMAP_LAYOUT_MODE
{
    GRID = 0,
    PLACEMENT = 1,
};

// 일부 원형 타일은 플레이어의 진행 방향에 따라 "보정용 기울기"를 써야 한다.
// 기존 half-checker 동작을 일반화한 값이다.
enum class TILE_CORRECTION_TRIGGER
{
    NONE = 0,
    HALF_CHECKER_FALSE = 1,
    HALF_CHECKER_TRUE = 2,
};

enum TILE_TYPE_FLAGS
{
    TILE_FLAG_NONE = 0,
    TILE_FLAG_SOLID = 1 << 0,
    TILE_FLAG_ATTACHABLE = 1 << 1,
    TILE_FLAG_USE_CUSTOM_NORMAL = 1 << 2,
};

struct TileShapeDesc
{
    TILE_DRAW_MODE Mode = TILE_DRAW_MODE::EMPTY;

    float a = 0.f;
    float b = 0.f;
    float c = -1.f;

    float r = 0.f;
    float centerX = 0.5f;
    float centerY = 0.5f;
};

struct TileTypeDesc
{
    wstring Name;

    // 기존 enum 기반 코드와 호환하기 위한 값이다.
    // 커스텀 타일은 EMPTY_BLOCK 계열로 남겨도 된다.
    UINT LegacyType = 0;
    UINT Flags = TILE_FLAG_NONE;

    TileShapeDesc MainShape;
    TileShapeDesc CorrectionShape;
    TILE_CORRECTION_TRIGGER CorrectionTrigger = TILE_CORRECTION_TRIGGER::NONE;

    // 수직벽처럼 기울기만으로 노말을 정하기 애매한 타일은
    // 이 값을 직접 사용하도록 설정할 수 있다.
    Vec2 CustomNormal = Vec2(0.f, 1.f);
};

struct TilePlacement
{
    UINT TypeIdx = 1;
    Vec2 LocalPos = Vec2(0.f, 0.f);
    Vec2 Size = Vec2(96.f, 96.f);
    float Rotation = 0.f;
    float LocalZ = 0.f;
};

class ATileMap :
    public Asset
{
private:
    TILEMAP_LAYOUT_MODE     m_LayoutMode;
    UINT                    m_Row;              // 타일맵의 행 개수
    UINT                    m_Col;              // 타일맵의 열 개수
    Vec2                    m_TileSize;         // 타일맵을 구성하는 타일 1개의 크기
    vector<UINT>            m_vecTileType;      // 각 셀이 참조하는 타일 정의 인덱스
    vector<Vec2>            m_vecTileScale;     // 각 셀의 개별 크기 배율(셀 중심 기준)
    vector<TileTypeDesc>    m_vecTileDefs;      // TileRenderUI에서 편집하는 실제 타일 정의 목록
    vector<TilePlacement>   m_vecPlacements;    // 배치형 타일 에디터용 개별 타일 목록

private:
    void ResetToDefaultTileDefs();

public:
    void SetRowCol(UINT _Row, UINT _Col);
    void Resize(UINT _Row, UINT _Col);
    void SetTileType(UINT _Row, UINT _Col, UINT _Type);
    UINT GetTileType(UINT _Row, UINT _Col) const;
    void SetTileScale(UINT _Row, UINT _Col, const Vec2& _Scale);
    Vec2 GetTileScale(UINT _Row, UINT _Col) const;
    void SetSprite(UINT _Row, UINT _Col, Ptr<ASprite> _Sprite);

    int AddTileType(const TileTypeDesc& _Desc);
    int DuplicateTileType(UINT _Idx);
    void RemoveTileType(UINT _Idx);

    int AddPlacement(const TilePlacement& _Placement);
    int DuplicatePlacement(UINT _Idx);
    void RemovePlacement(UINT _Idx);
    TilePlacement* GetPlacement(UINT _Idx);
    const TilePlacement* GetPlacement(UINT _Idx) const;
    const vector<TilePlacement>& GetPlacements() const { return m_vecPlacements; }
    void ConvertGridToPlacements();

    TileTypeDesc* GetTileTypeDesc(UINT _Idx);
    const TileTypeDesc* GetTileTypeDesc(UINT _Idx) const;
    const vector<TileTypeDesc>& GetTileTypeDescs() const { return m_vecTileDefs; }

    UINT GetRow() { return m_Row; }
    UINT GetCol() { return m_Col; }

    const vector<UINT>& GetTileTypes() const { return m_vecTileType; }
    const vector<Vec2>& GetTileScales() const { return m_vecTileScale; }

    GET_SET(Vec2, TileSize);
    GET_SET(TILEMAP_LAYOUT_MODE, LayoutMode);

    virtual int Load(const wstring& _FilePath) override;
    virtual int Save(const wstring& _FilePath) override;

public:
    ATileMap();
    virtual ~ATileMap();
};
