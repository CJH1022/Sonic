#pragma once
#include "AssetUI.h"
class SoundUI :
    public AssetUI
{
public:
    virtual void Tick_UI() override;

public:
    SoundUI();
    virtual ~SoundUI();
};

