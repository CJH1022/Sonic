#pragma once
#include "ComponentUI.h"

class AFlipbook;

class FlipbookRenderUI :
    public ComponentUI
{
public:
    virtual void Tick_UI() override;

private:
    void SelectFlipbook(DWORD_PTR _ListUI);
    void SetCurrentFlipbook(Ptr<AFlipbook> _Flipbook);

public:
    FlipbookRenderUI();
    virtual ~FlipbookRenderUI();
};

