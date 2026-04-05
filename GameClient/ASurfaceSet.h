#pragma once

#include "Asset.h"

enum class SURFACE_SET_GEOMETRY : UINT
{
    LINE = 0,
    ARC,
    CIRCLE,
    FULL_CIRCLE,
};

enum class SURFACE_SET_ROLE : UINT
{
    SURFACE = 0,
    CORRECTION,
    WALL,
    VERTICAL_ENTRY,
};

enum class SURFACE_SET_ARC_CORNER : UINT
{
    TOP_LEFT = 0,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
};

class ASurfaceSet
    : public Asset
{
public:
    struct SURFACE_DESC
    {
        wstring                 Name;
        SURFACE_SET_GEOMETRY    Geometry = SURFACE_SET_GEOMETRY::LINE;
        SURFACE_SET_ROLE        Role = SURFACE_SET_ROLE::SURFACE;
        SURFACE_SET_ARC_CORNER  ArcCorner = SURFACE_SET_ARC_CORNER::BOTTOM_LEFT;
        Vec3                    LocalPosition = Vec3(0.f, 0.f, 0.f);
        Vec2                    LocalStart = Vec2(-100.f, 0.f);
        Vec2                    LocalEnd = Vec2(100.f, 0.f);
        bool                    FillAbove = false;
        bool                    FillInside = false;
        bool                    Attachable = true;
        bool                    HasCircleGuide = false;
        bool                    CircleGuideEnabled = false;
        float                   GuideEntryAngleDeg = -35.f;
        float                   GuideHalfCheckAngleDeg = 90.f;
        float                   GuideExitAngleDeg = -145.f;
        Vec2                    GuideCorrectionLineStartLocal = Vec2(0.f, 0.f);
        Vec2                    GuideCorrectionLineEndLocal = Vec2(0.f, 0.f);
        wstring                 LinkedCorrectionLineName;
    };

private:
    vector<SURFACE_DESC> m_vecSurfaceDescs;

public:
    void Clear() { m_vecSurfaceDescs.clear(); }
    void Reserve(UINT _Capacity) { m_vecSurfaceDescs.reserve(_Capacity); }
    UINT AddSurface(const SURFACE_DESC& _Desc);
    bool RemoveSurface(UINT _Idx);

    UINT GetSurfaceCount() const { return (UINT)m_vecSurfaceDescs.size(); }
    const vector<SURFACE_DESC>& GetSurfaces() const { return m_vecSurfaceDescs; }
    vector<SURFACE_DESC>& GetSurfaces() { return m_vecSurfaceDescs; }

    virtual int Load(const wstring& _FilePath) override;
    virtual int Save(const wstring& _FilePath) override;

public:
    ASurfaceSet();
    virtual ~ASurfaceSet();
};
