#pragma once
#include "ComponentUI.h"


class TransformUI :
    public ComponentUI
{
public:
    virtual void Tick_UI() override;

public:
    TransformUI();
    virtual ~TransformUI();
};

