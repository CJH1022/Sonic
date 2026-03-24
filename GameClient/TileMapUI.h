#pragma once
#include "AssetUI.h"
class TileMapUI :
    public AssetUI
{
public:
    virtual void Tick_UI() override;

public:
    TileMapUI();
    virtual ~TileMapUI();
};

