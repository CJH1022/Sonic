#pragma once
#include "AssetUI.h"
class ComputeShaderUI :
    public AssetUI
{
public:
    virtual void Tick_UI() override;

public:
    ComputeShaderUI();
    virtual ~ComputeShaderUI();
};

