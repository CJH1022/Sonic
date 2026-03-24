#pragma once
#include "AssetUI.h"
class SpriteUI :
    public AssetUI
{
public:
    virtual void Tick_UI() override;

public:
    SpriteUI();
    virtual ~SpriteUI();
};

