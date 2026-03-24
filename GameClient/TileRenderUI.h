#pragma once
#include "ComponentUI.h"
class TileRenderUI :
    public ComponentUI
{
public:
    virtual void Tick_UI() override;

public:
    TileRenderUI();
    virtual ~TileRenderUI();
};

