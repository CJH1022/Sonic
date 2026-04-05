#pragma once

#include "CScript.h"

#include <vector>

class GameObject;

class CSurfaceScript
    : public CScript
{
public:
    enum class SURFACE_GEOMETRY
    {
        LINE = 0,
        ARC,
        CIRCLE,
        FULL_CIRCLE,
    };

    enum class SURFACE_ROLE
    {
        SURFACE = 0,
        CORRECTION,
        WALL,
        VERTICAL_ENTRY,
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
    std::vector<long long> m_SpatialCellKeys;
    Vec2                m_LastSpatialWorldPos;
    bool                m_bSpatialRegistered;

public:
    struct CONTACT_PROBE
    {
        Vec2 Normal = Vec2(0.f, 1.f);
        float SignedDistance = 0.f;
        float CandidateScore = 999999.f;
        bool TransitionSurface = false;
        bool Attachable = false;
        bool WallLike = false;
        bool Circle = false;
        bool InwardCircle = false;
        float SeamBlendT = 0.f;
        bool SeamBlendHasValue = false;
        Vec2 SeamStart = Vec2(0.f, 0.f);
        Vec2 SeamEnd = Vec2(0.f, 0.f);
        Vec2 ContactPoint = Vec2(0.f, 0.f);
    };

    virtual void Begin() override;
    virtual void Tick() override;

    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    bool ProbePlayerContact(CCollider2D* _OtherCollider, bool _WasGround, CONTACT_PROBE& _OutProbe);

    void ConfigureLine(const Vec2& _LocalStart, const Vec2& _LocalEnd, SURFACE_ROLE _Role, bool _FillAbove, bool _Attachable);
    void ConfigureArc(const Vec2& _LocalStart, const Vec2& _LocalEnd, SURFACE_ROLE _Role, ARC_CORNER _Corner, bool _FillInside, bool _Attachable);
    void ConfigureCircle(const Vec2& _LocalStart, const Vec2& _LocalEnd, SURFACE_ROLE _Role, ARC_CORNER _Corner, bool _FillInside, bool _Attachable);
    void ConfigureFullCircle(float _Radius, SURFACE_ROLE _Role, bool _FillInside, bool _Attachable);

    SURFACE_GEOMETRY GetGeometry() const { return m_Geometry; }
    SURFACE_ROLE GetRole() const { return m_Role; }
    ARC_CORNER GetArcCorner() const { return m_ArcCorner; }
    bool IsAttachable() const { return m_Attachable && m_Role != SURFACE_ROLE::WALL; }
    bool GetFillAbove() const { return m_FillAbove; }
    bool GetFillInside() const { return m_FillInside; }
    bool IntersectsWorldBounds(const Vec2& _Min, const Vec2& _Max, float _Margin = 0.f);
    static void ResetSpatialIndex();
    static void QueryNearbySurfaceObjects(const Vec2& _Min, const Vec2& _Max,
                                          std::vector<GameObject*>& _OutObjects,
                                          float _Margin = 0.f, bool _IncludeWalls = false);

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
    void RefreshSpatialRegistration();
    void UnregisterSpatialRegistration();
    bool EvaluateWallProbe(CCollider2D* _OtherCollider, Vec2& _OutNormal, float& _OutSignedDistance);
    bool EvaluateLineProbe(CCollider2D* _OtherCollider, const Vec2& _FootPos, Vec2& _OutNormal, float& _OutSignedDistance, bool& _OutTransitionSurface, float& _OutSeamBlendT, bool& _OutSeamBlendHasValue, Vec2& _OutSeamStart, Vec2& _OutSeamEnd, Vec2& _OutContactPoint, bool _WasGround);
    bool EvaluateArcProbe(CCollider2D* _OtherCollider, const Vec2& _FootPos, Vec2& _OutNormal, float& _OutSignedDistance, bool& _OutTransitionSurface, Vec2& _OutContactPoint, bool _WasGround);
    bool EvaluateCircleProbe(CCollider2D* _OtherCollider, const Vec2& _FootPos, Vec2& _OutNormal, float& _OutSignedDistance, bool& _OutTransitionSurface, Vec2& _OutContactPoint, bool _WasGround);
    bool GetPlayerSupportPoint(CCollider2D* _OtherCollider, const Vec2& _SurfaceNormal, Vec2& _OutSupportPoint);
};
