#pragma once
#include "ComponentUI.h"
class MeshRenderUI :
    public ComponentUI
{
public:
    virtual void Tick_UI() override;


private:
    void SelectMesh(DWORD_PTR _ListUI);
    void SelectMtrl(DWORD_PTR _ListUI);

public:
    MeshRenderUI();
    virtual ~MeshRenderUI();
};

