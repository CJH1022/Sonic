#pragma once

#include "CRenderComponent.h"
#include "ASprite.h"

class CSpriteRender :
    public CRenderComponent
{
private:
    Ptr<ASprite>        m_Sprite;

public:
    void SetSprite(Ptr<ASprite> _Sprite) { m_Sprite = _Sprite; }
    Ptr<ASprite> GetSprite() { return m_Sprite; }

public:
    virtual void FinalTick() override;
    virtual void Render() override;
    virtual void CreateMaterial() override;

    CLONE(CSpriteRender);
public:
    CSpriteRender();
    virtual ~CSpriteRender();
};

