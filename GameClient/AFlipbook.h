#pragma once
#include "Asset.h"

#include "ASprite.h"

class AFlipbook :
    public Asset
{
private:
    vector<Ptr<ASprite>>    m_vecSprite;

public:
    void AddSprite(Ptr<ASprite> _Sprite) { m_vecSprite.push_back(_Sprite); }

    void SetSprite(int _Idx, Ptr<ASprite> _Sprite)
    {
        if (m_vecSprite.size() <= _Idx)
            m_vecSprite.resize(_Idx + 1);

        m_vecSprite[_Idx] = _Sprite;
    }

    Ptr<ASprite> GetSprite(int _Idx) { return m_vecSprite[_Idx]; }

    UINT GetSpriteCount() { return m_vecSprite.size(); }

    virtual int Load(const wstring& _FilePath) override;
    virtual int Save(const wstring& _FilePath) override;

public:
    AFlipbook();
    virtual ~AFlipbook();
};

