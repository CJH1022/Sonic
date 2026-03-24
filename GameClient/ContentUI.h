#pragma once
#include "EditorUI.h"

#include "TreeUI.h"

class ContentUI :
    public EditorUI
{
private:
    Ptr<TreeUI> m_Tree;

public:
    virtual void Tick_UI() override;

public:
    void Renew();

private:
    void SelectAsset(DWORD_PTR _Asset);

public:
    ContentUI();
    virtual ~ContentUI();
};

