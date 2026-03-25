#pragma once
#include "ComponentUI.h"
class TileRenderUI :
    public ComponentUI
{
public:
    virtual void Tick_UI() override;

private:
    void SelectTileMap(DWORD_PTR _ListUI);

public:
    TileRenderUI();
    virtual ~TileRenderUI();
};

