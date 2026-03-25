#pragma once
#include "CScript.h"
#include "ATileMap.h"
#include <functional>

enum class TILETYPE
{
    FIRST,
    EMPTY_BLOCK, // 1
    LINE_BLOCK_1, // 2 높은 내리막
    LINE_BLOCK_2, // 3 낮은 내리막
    LINE_BLOCK_3, // 4 평지 하단
    LINE_BLOCK_4, // 5 낮은 오르막
    LINE_BLOCK_5, // 6 높은 오르막
    LINE_BLOCK_6, // 7 평지 상단
    CIRCLE_BLOCK_1, // 8 원 왼쪽 탑
    CIRCLE_BLOCK_2, // 9 원 우측 탑
    CIRCLE_BLOCK_3, // 10 원 왼쪽 하단
    CIRCLE_BLOCK_4, // 11 원 우측 상단
    LINE_BLOCK_VERTICAL, // 세로 벽
};

enum class TILESTATE
{
    NONE,
    LINE_BLOCK,
    CIRCLE_BLOCK,
};

struct TileDrawInfo
{
    float a = 0.f;
    float b = 0.f;
    float c = 0.f;
    float r = 0.f;
    float center_x = 0.5f;
    float center_y = 0.5f;
    int   mode = 0;
    Vec2  customNormal = Vec2(0.f, 1.f);
    UINT  flags = TILE_FLAG_NONE;
};

class CTileScript : public CScript
{
private:
    inline static bool m_bMapCreated = false;
    inline static bool s_bHalfChecker = false;

    // ResolveTileTypeDesc 결과를 멤버로 복사해 둔다.
    // 충돌 계산은 매 프레임 자주 일어나므로, 현재 타일이 쓰는 수식을
    // 람다 형태로 캐시해두는 편이 훨씬 단순하다.
    float a = 0.f, b = 0.f, c = 0.f;
    float r = 0.f, center_x = 0.f, center_y = 0.f;

    TILETYPE        m_eType = TILETYPE::EMPTY_BLOCK;
    TILESTATE       m_TileState = TILESTATE::NONE;
    UINT            m_TileTypeIdx = (UINT)TILETYPE::EMPTY_BLOCK;
    TileTypeDesc    m_TileDesc;
    TileDrawInfo    m_ResolvedInfo;

    Vec2 vCurNormal = Vec2(0.f, 1.f);

    std::function<float(float, float)> f;
    std::function<float(float, float)> dfdx;
    std::function<float(float, float)> dfdy;

private:
    static void RefreshHalfCheckerTiles();
    void ApplyResolvedInfo();

public:
    Vec2 GetCurNormal() { return vCurNormal; }
    TILETYPE GetTileType() const { return m_eType; }
    UINT GetTileTypeIndex() const { return m_TileTypeIdx; }
    const TileTypeDesc& GetTileDesc() const { return m_TileDesc; }
    bool IsSolid() const { return (m_ResolvedInfo.flags & TILE_FLAG_SOLID) != 0; }
    static bool GetHalfCheckerState() { return s_bHalfChecker; }

    static void BuildPresetTileTypeDesc(TILETYPE _Type, TileTypeDesc& _OutDesc);
    static bool ResolveTileTypeDesc(const TileTypeDesc& _Desc, bool _HalfChecker, TileDrawInfo& _OutInfo);
    static bool GetTileDrawInfo(TILETYPE _Type, bool _HalfChecker, TileDrawInfo& _OutInfo);

public:
    void SetTileDesc(const TileTypeDesc& _Desc, UINT _TypeIdx = (UINT)TILETYPE::EMPTY_BLOCK);

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
        switch (m_ResolvedInfo.mode)
        {
        case (int)TILE_DRAW_MODE::LINE:
            return m_TileState = TILESTATE::LINE_BLOCK;
        case (int)TILE_DRAW_MODE::CIRCLE:
            return m_TileState = TILESTATE::CIRCLE_BLOCK;
        default:
            return m_TileState = TILESTATE::NONE;
        }
    }

    virtual void Begin() override;
    virtual void Tick() override;
    void Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void TileMapSetting(TILETYPE i);
    float GetFvalue(Vec2 _pos);
    void GetNormal(Vec2 _pos, Vec2& _normal, float& _mag);
    CLONE(CTileScript);
    CTileScript();
    virtual ~CTileScript();
};
