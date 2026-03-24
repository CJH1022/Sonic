#pragma once
#include "AssetUI.h"
class MeshUI :
    public AssetUI
{
public:
    virtual void Tick_UI() override;

public:
    MeshUI();
    virtual ~MeshUI();
};

