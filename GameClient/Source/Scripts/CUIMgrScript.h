#pragma once

#include "CScript.h"

#include <vector>

class GameObject;
class CCollider2D;

class CUIMgrScript : public CScript
{
public:
    enum class STAGESTATE
    {
        START,
        GENERAL,
        END,
    };

private:
    STAGESTATE m_curState = STAGESTATE::GENERAL;
    int m_StageStateIndex = (int)STAGESTATE::GENERAL;
    int m_UsePlayerTrigger = 0;
    int m_AutoSyncCoin = 1;
    int m_AutoSyncLife = 1;
    int m_AutoAdvanceToPlay = 1;
    int m_AutoChangeLevelOnEnd = 0;
    float m_Time = 0.f;
    int m_curScore = 0;
    int m_curCoin = 0;
    int m_curLife = 0;
    bool m_bInitializedStageUI = false;
    bool m_bEndLevelChangeQueued = false;
    float m_StageStartAnimTime = 0.f;
    GameObject* m_pStageStartVisual = nullptr;
    GameObject* m_pStageGeneralVisual = nullptr;
    GameObject* m_pStageEndVisual = nullptr;
    std::vector<GameObject*> m_vecStageGeneralScoreDigits;
    std::vector<GameObject*> m_vecStageGeneralTimeDigits;
    std::vector<GameObject*> m_vecStageGeneralCoinDigits;
    std::vector<GameObject*> m_vecStageGeneralLifeDigits;
    int m_LastSyncedCoinForScore = 0;
    bool m_bCoinScoreInitialized = false;
    std::wstring m_NextLevelName;

public:
    virtual void Begin() override;
    virtual void Tick() override;
    virtual void SaveToLevelFile(FILE* _File) override;
    virtual void LoadFromLevelFile(FILE* _File) override;

    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);

    void SetStageState(STAGESTATE _State);
    void SetStageStateByIndex(int _StateIndex);
    void PrepareForLevelStart();
    STAGESTATE GetStageState() const { return m_curState; }
    int GetStageStateIndex() const { return m_StageStateIndex; }
    float GetStageTime() const { return m_Time; }
    int GetScore() const { return m_curScore; }
    int GetCoin() const { return m_curCoin; }
    int GetLife() const { return m_curLife; }
    void SetScore(int _Score) { m_curScore = _Score; }
    void SetCoin(int _Coin) { m_curCoin = _Coin; }
    void SetLife(int _Life) { m_curLife = _Life; }
    void SetNextLevelName(const std::wstring& _LevelName) { m_NextLevelName = _LevelName; }
    const std::wstring& GetNextLevelName() const { return m_NextLevelName; }

private:
    void RegisterScriptParams();
    void SyncStageStateFromInspector();
    Vec3 GetStageUICameraAnchorPos() const;
    int ResolveUIRenderLayer();
    void SyncUIRootToCamera();
    void RefreshRuntimeStats();
    void ApplyStageUI();
    void AdvanceToNextStage();
    void UpdateStageStartUI();
    void UpdateStageGeneralUI();
    GameObject* CreateFullscreenSpriteChild(const wchar_t* _Name, const wchar_t* _SpritePath, float _LocalZ);
    GameObject* CreateGeneralDigitChild(GameObject* _Parent, const wchar_t* _Name, const Vec2& _PixelCenter, const Vec2& _PixelSize, float _LocalZ);
    void DestroyUIChild(GameObject*& _Child);
    void DestroyUIChildren(std::vector<GameObject*>& _Children);
    void RefreshGeneralUIDigits();
    void RefreshGeneralDigitTransforms(std::vector<GameObject*>& _Digits, const Vec2* _PixelCenters, int _DigitCount, const Vec2& _PixelSize, float _LocalZ);
    void SetGeneralDigitSprite(GameObject* _DigitObj, int _Digit);
    void SyncScoreFromCoinDelta(int _CoinValue);
    void HandleTimeOver();
    void CreateStageStartUI();
    void CreateStageGeneralUI();
    void CreateStageEndUI();

public:
    CLONE(CUIMgrScript);
    CUIMgrScript();
    CUIMgrScript(const CUIMgrScript& _Origin);
    ~CUIMgrScript();
};
