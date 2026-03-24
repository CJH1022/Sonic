#pragma once
#include "AssetUI.h"
class LevelUI :
    public AssetUI
{
public:
    virtual void Tick_UI() override;

public:
    LevelUI();
    virtual ~LevelUI();
};

