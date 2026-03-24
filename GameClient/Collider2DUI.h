#pragma once
#include "ComponentUI.h"

class Collider2DUI :
    public ComponentUI
{
public:
    virtual void Tick_UI() override;

public:
    Collider2DUI();
    virtual ~Collider2DUI();
};

