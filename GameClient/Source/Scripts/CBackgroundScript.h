#pragma once
#include "CScript.h"
class CBackgroundScript :
    public CScript
{
private:
    Vec3 m_vInitialPos;
    float m_fParallaxRatio;

public:
    virtual void Begin() override;
    virtual void Tick() override;

public:
    CLONE(CBackgroundScript);
    CBackgroundScript();
    virtual ~CBackgroundScript();
};

