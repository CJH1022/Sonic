#pragma once
#include "EditorUI.h"
#include "Asset.h"

class AssetUI :
    public EditorUI
{
private:
    const ASSET_TYPE  m_AssetType;
    Ptr<Asset>        m_TargetAsset;

public:
    void SetTargetAsset(Ptr<Asset> _Asset) { m_TargetAsset = _Asset; }
    Ptr<Asset> GetTargetAsset() { return  m_TargetAsset; }

protected:
    void OutputTitle();

public:
    AssetUI(ASSET_TYPE _Type);
    virtual ~AssetUI();
};

