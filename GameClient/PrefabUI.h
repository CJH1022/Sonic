#pragma once
#include "AssetUI.h"
class PrefabUI :
    public AssetUI
{
public:
    virtual void Tick_UI() override;

public:
    PrefabUI();
    virtual ~PrefabUI();
};

