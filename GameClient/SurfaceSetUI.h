#pragma once

#include "AssetUI.h"

#include <string>

class ASurfaceSet;
class GameObject;

class SurfaceSetUI
    : public AssetUI
{
private:
    std::string m_StatusText;

public:
    virtual void Tick_UI() override;
    static Ptr<ASurfaceSet> EnsureCurrentLevelSurfaceSetAsset(std::string* _OutStatusText = nullptr);
    static bool ReplaceSceneSurfacesWithActorInLevel(ASurfaceSet* _SurfaceSet, UINT& _OutCount, std::string* _OutStatusText = nullptr);

public:
    SurfaceSetUI();
    virtual ~SurfaceSetUI();

private:
    bool BakeSceneSurfaces(ASurfaceSet* _SurfaceSet, UINT& _OutCount);
    bool SaveSurfaceSetAsset(ASurfaceSet* _SurfaceSet);
    Ptr<GameObject> CreateSurfaceSetActor(ASurfaceSet* _SurfaceSet);
    bool ReplaceSceneSurfacesWithActor(ASurfaceSet* _SurfaceSet, UINT& _OutCount);
};
