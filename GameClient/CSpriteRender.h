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

    virtual void SaveToLevelFile(FILE* _File) override;
    virtual void LoadFromLevelFile(FILE* _File) override;

    CLONE(CSpriteRender);
public:
    CSpriteRender();
    virtual ~CSpriteRender();
};

