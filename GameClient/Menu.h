#pragma once
#include "EditorUI.h"

class Menu :
    public EditorUI
{
private:
    void File();
    void Level();
    void View();
    void GameObjectMenu();
    void Asset();

public:
    virtual void Tick() override;
    virtual void Tick_UI() override;

private:
    wstring GetAssetName(ASSET_TYPE _Type, const wstring& _Name);

public:
    Menu();
    virtual ~Menu();
};

