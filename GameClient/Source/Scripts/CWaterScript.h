#pragma once

#include "CScript.h"

class CCollider2D;

class CWaterScript
    : public CScript
{
private:
    float m_WaterAccelScale;
    float m_WaveSpeed;
    float m_WaveTiling;
    float m_ShimmerStrength;
    float m_OverlayAlpha;
    Vec4 m_TintColor;

    Ptr<GameObject> m_OverlayObject;

public:
    virtual void Begin() override;
    virtual void Tick() override;
    virtual void SaveToLevelFile(FILE* _File) override;
    virtual void LoadFromLevelFile(FILE* _File) override;

    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    void EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);

private:
    void EnsureOverlay();
    void UpdateOverlay();

public:
    CLONE(CWaterScript);
    CWaterScript();
    CWaterScript(const CWaterScript& _Origin);
    virtual ~CWaterScript();
};
