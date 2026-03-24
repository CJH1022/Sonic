#pragma once
#include "CRenderComponent.h"

#include "AFlipbook.h"

class CFlipbookRender :
    public CRenderComponent
{
private:
    vector<Ptr<AFlipbook>>  m_vecFlipbook;

    int                     m_CurFlipbook;
    int                     m_CurSprite;

    int                     m_RepeatCount;  // -1 : 반복재생, 1 이상이면 재생 횟수
    bool                    m_Finish;
    float                   m_FPS;
    float                   m_AccTime;

public:

    bool IsFinish() const { return m_Finish; }

    void SetFlipbook(int _Idx, Ptr<AFlipbook> _Flipbook)
    {
        if (m_vecFlipbook.size() <= _Idx)
            m_vecFlipbook.resize(_Idx + 1);
        m_vecFlipbook[_Idx] = _Flipbook;
    }

    void AddFlipbook(Ptr<AFlipbook> _Flipbook) { m_vecFlipbook.push_back(_Flipbook); }

    void Play(int _FlipbookIdx, float _FPS, int _RepeatCount)
    {
        if (m_CurFlipbook == _FlipbookIdx && m_RepeatCount == _RepeatCount && !m_Finish)
            return;
        m_CurFlipbook = _FlipbookIdx;
        m_RepeatCount = _RepeatCount;
        m_FPS = _FPS;
        m_AccTime = 0.f;
        m_CurSprite = 0;
        m_Finish = false;
    }

    void Play(int _FlipbookIdx, int _CurSprite, float _FPS, int _RepeatCount)
    {
        //if (m_CurFlipbook == _FlipbookIdx && m_RepeatCount == _RepeatCount && !m_Finish)
        //    return;
        m_CurFlipbook = _FlipbookIdx;
        m_RepeatCount = _RepeatCount;
        m_FPS = _FPS;
        m_AccTime = 0.f;
        m_CurSprite = _CurSprite;
        m_Finish = false;
    }

private:
    bool CheckFinish();

public:
    virtual void FinalTick() override;
    virtual void Render() override;
    virtual void CreateMaterial() override;

    CLONE(CFlipbookRender);
public:
    CFlipbookRender();
    virtual ~CFlipbookRender();
};

