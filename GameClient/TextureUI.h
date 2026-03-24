#pragma once
#include "AssetUI.h"
class TextureUI :
    public AssetUI
{
public:
    virtual void Tick_UI() override;

public:
    TextureUI();
    virtual ~TextureUI();
};

