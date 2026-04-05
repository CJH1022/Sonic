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
    void CreateCylinderObject();
    void CreateBlockObject();
    void CreateMovingBlockObject();
    void CreatePushingBlockObject();
    void CreateSpikeObject(float _RotationZ, const wchar_t* _NamePrefix);
    void CreateSpringObject(float _RotationZ, const wchar_t* _NamePrefix);
    void DeleteSelectedObject();

public:
    virtual void Tick() override;
    virtual void Tick_UI() override;

private:
    wstring GetAssetName(ASSET_TYPE _Type, const wstring& _Name);

public:
    Menu();
    virtual ~Menu();
};

