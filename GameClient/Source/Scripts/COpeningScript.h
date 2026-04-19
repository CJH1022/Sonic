#pragma once
#include "CScript.h"

class ASprite;
class GameObject;

class COpeningScript
    : public CScript
{
private:
    bool m_bSpawnedSonicMain;
    bool m_bSpawnedEyeHand;

private:
    float           m_FrontOpeningZ;
    float           m_EyeHandLocalZ;
    float           m_SonicMainZ;
    float           m_Overlay20Z;
    float           m_StartButtonLocalZ;
    float           m_TransitionAccTime;
    float           m_PostTransitionAccTime;
    bool            m_bTransitionEnd;
    Ptr<GameObject> m_pSonicMain;
    Ptr<GameObject> m_pOverlay20;
    Ptr<GameObject> m_pStartButton;
    Ptr<ASprite>    m_pStartButtonOnSprite;
    Ptr<ASprite>    m_pStartButtonOffSprite;
    bool            m_bStartLevelRequested;

private:
    void RegisterScriptParams();

public:
    virtual void Begin() override;
    virtual void Tick() override;

public:
    CLONE(COpeningScript);
    COpeningScript();
    COpeningScript(const COpeningScript& _Origin);
    virtual ~COpeningScript();
};
