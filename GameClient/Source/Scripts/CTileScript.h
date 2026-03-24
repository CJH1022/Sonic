#pragma once
#include "CScript.h"
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
};

enum class TILESTATE
{
    NONE,
    LINE_BLOCK,
    CIRCLE_BLOCK,
};

class CTileScript : public CScript
{
private:
    inline static bool m_bMapCreated = false;
    inline static bool s_bHalfChecker = false;

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

public:
    Vec2 GetCurNormal() { return vCurNormal; }
    TILETYPE GetTileType() const { return m_eType; }

public:
    // 전역 함수 형태 (어디서든 Lerp(v1, v2, t)로 사용 가능)
    inline Vec2 Lerp(const Vec2& _v1, const Vec2& _v2, float _t)
    {
        // 공식: 시작점 + (끝점 - 시작점) * 비율
        return _v1 + (_v2 - _v1) * _t;
    }
    float Cross2D(Vec2 _vOld, Vec2 _vNew)
    {
        // (x1 * y2) - (y1 * x2)
        return (_vOld.x * _vNew.y) - (_vOld.y * _vNew.x);
    }

    TILESTATE GetTileState()
    {
        if (m_eType == TILETYPE::LINE_BLOCK_1 ||
            m_eType == TILETYPE::LINE_BLOCK_2 ||
            m_eType == TILETYPE::LINE_BLOCK_3 ||
            m_eType == TILETYPE::LINE_BLOCK_4 ||
            m_eType == TILETYPE::LINE_BLOCK_5 ||
            m_eType == TILETYPE::LINE_BLOCK_6)
        {
            return m_TileState = TILESTATE::LINE_BLOCK;
        }
        else if (m_eType == TILETYPE::CIRCLE_BLOCK_1 ||
            m_eType == TILETYPE::CIRCLE_BLOCK_2 ||
            m_eType == TILETYPE::CIRCLE_BLOCK_3 ||
            m_eType == TILETYPE::CIRCLE_BLOCK_4)
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
    float GetFvalue(Vec2 _pos);
    void GetNormal(Vec2 _pos, Vec2& _normal, float& _mag);
    CLONE(CTileScript);
    CTileScript();
    virtual ~CTileScript();
};
