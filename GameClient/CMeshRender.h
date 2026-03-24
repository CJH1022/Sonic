#pragma once
#include "CRenderComponent.h"

#include "assets.h"

class CMeshRender :
    public CRenderComponent
{
private:

public:
    virtual void FinalTick() override;
    virtual void Render() override;
    virtual void CreateMaterial() override {}

    CLONE(CMeshRender);
public:
    CMeshRender();
    virtual ~CMeshRender();
};

