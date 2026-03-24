#pragma once
#include "CScript.h"
#include <functional>
#include <vector>

enum class TILETYPE
{
    FIRST = 0,

    EMPTY_BLOCK = 1,

    // Built-in line tiles
    LINE_BLOCK_1 = 2,
    LINE_BLOCK_2 = 3,
    LINE_BLOCK_3 = 4,
    LINE_BLOCK_4 = 5,
    LINE_BLOCK_5 = 6,
    LINE_BLOCK_6 = 7,

    // Built-in circle tiles
    CIRCLE_BLOCK_1 = 8,
    CIRCLE_BLOCK_2 = 9,
    CIRCLE_BLOCK_3 = 10,
    CIRCLE_BLOCK_4 = 11,

    // Extra line slots (unlock by editor button)
    LINE_BLOCK_7 = 12,
    LINE_BLOCK_8,
    LINE_BLOCK_9,
    LINE_BLOCK_10,
    LINE_BLOCK_11,
    LINE_BLOCK_12,
    LINE_BLOCK_13,
    LINE_BLOCK_14,
    LINE_BLOCK_15,
    LINE_BLOCK_16,
    LINE_BLOCK_17,
    LINE_BLOCK_18,
    LINE_BLOCK_19,
    LINE_BLOCK_20,

    // Extra circle slots (unlock by editor button)
    CIRCLE_BLOCK_5,
    CIRCLE_BLOCK_6,
    CIRCLE_BLOCK_7,
    CIRCLE_BLOCK_8,
    CIRCLE_BLOCK_9,
    CIRCLE_BLOCK_10,
    CIRCLE_BLOCK_11,
    CIRCLE_BLOCK_12,
    CIRCLE_BLOCK_13,
    CIRCLE_BLOCK_14,
    CIRCLE_BLOCK_15,
    CIRCLE_BLOCK_16,
    CIRCLE_BLOCK_17,
    CIRCLE_BLOCK_18,
    CIRCLE_BLOCK_19,
    CIRCLE_BLOCK_20,

    END,
};

enum class TILESTATE
{
    NONE,
    LINE_BLOCK,
    CIRCLE_BLOCK,
};

class CTileScript : public CScript
{
public:
    struct TILE_FORMULA_CONFIG
    {
        float a;
        float b;
        float c;

        float r;
        float center_x;
        float center_y;
    };

    struct TILE_MAP_PLACEMENT
    {
        float map_pos_x;
        float map_pos_y;
        float map_pos_z;

        float collision_offset_x;
        float collision_offset_y;
    };

private:
    inline static bool m_bMapCreated = false;
    inline static bool s_bHalfChecker = false;
    inline static bool s_bFormulaInitialized = false;
    inline static bool s_bTileTypeUnlockInitialized = false;
    inline static TILE_FORMULA_CONFIG s_FormulaConfig[(int)TILETYPE::END] = {};
    inline static bool s_TileTypeUnlocked[(int)TILETYPE::END] = {};
    inline static TILE_MAP_PLACEMENT s_MapPlacement = { 0.f, 0.f, 500.f, 0.f, 0.f };

    float a = 0.f, b = 0.f, c = 0.f;
    float r = 0.f, center_x = 0.f, center_y = 0.f;
    float fFinalRot = 0.f;

    int halfcount;

    TILETYPE m_eType = TILETYPE::EMPTY_BLOCK;
    TILESTATE m_TileState = TILESTATE::NONE;
    Vec2 vCurNormal = Vec2(0.f, 1.f);
    int   m_tileMap[6][25];

    std::function<float(float, float)> f;
    std::function<float(float, float)> dfdx;
    std::function<float(float, float)> dfdy;

    static void RefreshHalfCheckerTiles();
    static void InitializeTileTypeUnlockState();

public:
    Vec2 GetCurNormal() { return vCurNormal; }
    TILETYPE GetTileType() const { return m_eType; }

    static bool IsValidTileTypeValue(int _TypeValue);
    static TILETYPE ToTileType(int _TypeValue);
    static bool IsLineTileType(TILETYPE _Type);
    static bool IsCircleTileType(TILETYPE _Type);
    static bool IsLineTileTypeValue(int _TypeValue);
    static bool IsCircleTileTypeValue(int _TypeValue);
    static bool IsUnlockedTileTypeValue(int _TypeValue);

    static void GetEditableTileTypeValues(vector<int>& _OutTypeValues, bool _IncludeEmpty = true, bool _IncludeLocked = false);
    static bool CreateCustomTileType(bool _Circle, TILETYPE _CopyFrom, TILETYPE& _OutNewType);
    static bool DeleteCustomTileType(TILETYPE _Type);
    static bool DeleteCustomTileTypeByValue(int _TypeValue);

    static bool IsSpecialTileType(TILETYPE _Type);
    static Vec4 GetDebugColorByType(TILETYPE _Type);
    static Vec4 GetDebugColorByTypeValue(int _TypeValue);
    static const char* GetTileTypeName(TILETYPE _Type);
    static const char* GetTileTypeNameByValue(int _TypeValue);

    static void ResetTileFormulaConfigToDefault();
    static bool SetTileFormulaConfig(TILETYPE _Type, const TILE_FORMULA_CONFIG& _Config);
    static bool GetTileFormulaConfig(TILETYPE _Type, TILE_FORMULA_CONFIG& _OutConfig);
    static bool SetTileFormulaConfigByValue(int _TypeValue, const TILE_FORMULA_CONFIG& _Config);
    static bool GetTileFormulaConfigByValue(int _TypeValue, TILE_FORMULA_CONFIG& _OutConfig);
    static void SetTileMapPlacement(const TILE_MAP_PLACEMENT& _Placement);
    static void GetTileMapPlacement(TILE_MAP_PLACEMENT& _OutPlacement);

    static void GetDefaultTileMap(UINT& _OutRow, UINT& _OutCol, vector<int>& _OutTileValues);
    static bool SaveTileScriptPreset(const wstring& _FilePath, UINT _Row, UINT _Col, const vector<int>& _TileValues);
    static bool LoadTileScriptPreset(const wstring& _FilePath, UINT& _OutRow, UINT& _OutCol, vector<int>& _OutTileValues, bool _ApplyFormula = true);

    // 전역 함수 형태 (어디서든 Lerp(v1, v2, t)로 사용 가능)
    inline Vec2 Lerp(const Vec2& _v1, const Vec2& _v2, float _t)
    {
        return _v1 + (_v2 - _v1) * _t;
    }
    float Cross2D(Vec2 _vOld, Vec2 _vNew)
    {
        return (_vOld.x * _vNew.y) - (_vOld.y * _vNew.x);
    }

    TILESTATE GetTileState()
    {
        if (IsLineTileType(m_eType))
        {
            return m_TileState = TILESTATE::LINE_BLOCK;
        }
        else if (IsCircleTileType(m_eType))
        {
            return m_TileState = TILESTATE::CIRCLE_BLOCK;
        }
        else
        {
            return m_TileState = TILESTATE::NONE;
        }
    }

    virtual void Begin() override;
    virtual void Tick() override;
    void Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void TileMapSetting(TILETYPE i);
    void TileMapSetting(int _TypeValue);
    float GetFvalue(Vec2 _pos);
    void GetNormal(Vec2 _pos, Vec2& _normal, float& _mag);
    CLONE(CTileScript);
    CTileScript();
    virtual ~CTileScript();
};
