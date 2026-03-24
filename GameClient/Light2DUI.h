#pragma once
#include "ComponentUI.h"


class Light2DUI
    : public ComponentUI
{

public:
    virtual void Tick_UI() override;

public:
    Light2DUI();
    virtual ~Light2DUI();
};

