#pragma once

#include "CScript.h"

class CSurfaceScript
    : public CScript
{
public:
    enum class SURFACE_GEOMETRY
    {
        LINE = 0,
        ARC,
        CIRCLE,
    };

    enum class SURFACE_ROLE
    {
        SURFACE = 0,
        CORRECTION,
        WALL,
    };

    enum class ARC_CORNER
    {
        TOP_LEFT = 0,
        TOP_RIGHT,
        BOTTOM_LEFT,
        BOTTOM_RIGHT,
    };

private:
    SURFACE_GEOMETRY    m_Geometry;
    SURFACE_ROLE        m_Role;
    ARC_CORNER          m_ArcCorner;

    Vec2                m_LocalStart;
    Vec2                m_LocalEnd;

    bool                m_FillAbove;
    bool                m_FillInside;
    bool                m_Attachable;

public:
    virtual void Begin() override;
    virtual void Tick() override;

    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);

    void ConfigureLine(const Vec2& _LocalStart, const Vec2& _LocalEnd, SURFACE_ROLE _Role, bool _FillAbove, bool _Attachable);
    void ConfigureArc(const Vec2& _LocalStart, const Vec2& _LocalEnd, SURFACE_ROLE _Role, ARC_CORNER _Corner, bool _FillInside, bool _Attachable);
    void ConfigureCircle(const Vec2& _LocalStart, const Vec2& _LocalEnd, SURFACE_ROLE _Role, ARC_CORNER _Corner, bool _FillInside, bool _Attachable);

    SURFACE_GEOMETRY GetGeometry() const { return m_Geometry; }
    SURFACE_ROLE GetRole() const { return m_Role; }
    ARC_CORNER GetArcCorner() const { return m_ArcCorner; }
    bool IsAttachable() const { return m_Attachable; }
    bool GetFillAbove() const { return m_FillAbove; }
    bool GetFillInside() const { return m_FillInside; }

    void GetWorldEndpoints(Vec2& _OutStart, Vec2& _OutEnd);
    void GetArcWorldData(Vec2& _OutCenter, float& _OutRadius, Vec2& _OutBoxMin, Vec2& _OutBoxMax);
    Vec4 GetEditorColor() const;

    virtual void SaveToLevelFile(FILE* _File) override;
    virtual void LoadFromLevelFile(FILE* _File) override;

    CLONE(CSurfaceScript);

public:
    CSurfaceScript();
    CSurfaceScript(const CSurfaceScript& _Origin);
    virtual ~CSurfaceScript();

private:
    void UpdateBounds();
    void GetWorldBounds(Vec2& _OutMin, Vec2& _OutMax);
    bool EvaluateWallProbe(CCollider2D* _OtherCollider, Vec2& _OutNormal, float& _OutSignedDistance);
    bool EvaluateLineProbe(CCollider2D* _OtherCollider, const Vec2& _FootPos, Vec2& _OutNormal, float& _OutSignedDistance, bool& _OutTransitionSurface, float& _OutSeamBlendT, bool& _OutSeamBlendHasValue, Vec2& _OutSeamStart, Vec2& _OutSeamEnd, Vec2& _OutContactPoint, bool _WasGround);
    bool EvaluateArcProbe(CCollider2D* _OtherCollider, const Vec2& _FootPos, Vec2& _OutNormal, float& _OutSignedDistance, bool& _OutTransitionSurface, bool _WasGround);
    bool EvaluateCircleProbe(CCollider2D* _OtherCollider, const Vec2& _FootPos, Vec2& _OutNormal, float& _OutSignedDistance, bool& _OutTransitionSurface, bool _WasGround);
    bool GetPlayerSupportPoint(CCollider2D* _OtherCollider, const Vec2& _SurfaceNormal, Vec2& _OutSupportPoint);
};
