#pragma once

#include "AssetUI.h"

class GraphicShaderUI
    : public AssetUI
{
public:
    virtual void Tick_UI() override;

public:
    GraphicShaderUI();
    virtual ~GraphicShaderUI();
};

