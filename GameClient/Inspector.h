#pragma once
#include "EditorUI.h"

#include "GameObject.h"
#include "ComponentUI.h"
#include "ScriptUI.h"
#include "AssetUI.h"

class Inspector :
    public EditorUI
{
private:
    Ptr<GameObject>         m_TargetObject;
    Ptr<ComponentUI>        m_arrComUI[(UINT)COMPONENT_TYPE::END];
    vector<Ptr<ScriptUI>>   m_vecScriptUI;

    Ptr<Asset>              m_TargetAsset;
    Ptr<AssetUI>            m_arrAssetUI[(UINT)ASSET_TYPE::END];

public:
    virtual void Tick_UI() override;

public:
    Ptr<GameObject> GetTargetObject() { return m_TargetObject; }
    void SetTargetObject(Ptr<GameObject> _Object);

    Ptr<Asset> GetTargetAsset() { return m_TargetAsset; }
    void SetTargetAsset(Ptr<Asset> _Asset);

private:
    void CreateChildUI();

public:
    Inspector();
    virtual ~Inspector();
};


