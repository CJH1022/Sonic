#pragma once
#include "ComponentUI.h"
class SpriteRenderUI :
    public ComponentUI
{
public:
    virtual void Tick_UI() override;

public:
    SpriteRenderUI();
    virtual ~SpriteRenderUI();
};

