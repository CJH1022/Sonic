#pragma once
#include "CRenderComponent.h"

#include "ATileMap.h"
#include "StructuredBuffer.h"


struct TileInfo
{
    Vec4 FuncParam0; // a, b, c, r
    Vec4 FuncParam1; // center_x, center_y, mode, tileType
    Vec4 FuncParam2; // customNormal.x, customNormal.y, flags, reserved
    Vec4 FuncParam3; // scale.x, scale.y, reserved, reserved
};

class CTileRender :
    public CRenderComponent
{
private:
    Ptr<ATileMap>           m_TileMap;    
    float                   m_fOpacity;
    vector<TileInfo>        m_vecTileInfo;
    Ptr<StructuredBuffer>   m_Buffer;

private:
    void UpdateTileInfoBuffer();
    void RenderPlacementTiles();

public:
    Ptr<ATileMap> GetTileMap() const { return m_TileMap; }
    void SetTileMap(Ptr<ATileMap> _TileMap);
    void SetOpacity(float _Opacity)
    {
        if (_Opacity < 0.f)
            m_fOpacity = 0.f;
        else if (_Opacity > 1.f)
            m_fOpacity = 1.f;
        else
            m_fOpacity = _Opacity;
    }
    float GetOpacity() const { return m_fOpacity; }

public:
    virtual void Init() override;
    virtual void FinalTick() override;
    virtual void Render() override;
    virtual void CreateMaterial() override;

    virtual void SaveToLevelFile(FILE* _File) override;
    virtual void LoadFromLevelFile(FILE* _File) override;

    CLONE(CTileRender);
public:
    CTileRender();
    CTileRender(const CTileRender& _Origin);
    virtual ~CTileRender();
};

