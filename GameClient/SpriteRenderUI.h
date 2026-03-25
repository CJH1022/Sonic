#pragma once
#include "ComponentUI.h"
class SpriteRenderUI :
    public ComponentUI
{
public:
    virtual void Tick_UI() override;

private:
    void SelectSprite(DWORD_PTR _ListUI);

public:
    SpriteRenderUI();
    virtual ~SpriteRenderUI();
};

