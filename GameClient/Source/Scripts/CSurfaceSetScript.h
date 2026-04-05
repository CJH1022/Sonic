#pragma once

#include "CScript.h"

#include <vector>

class ASurfaceSet;
class GameObject;

class CSurfaceSetScript
    : public CScript
{
private:
    Ptr<ASurfaceSet>        m_SurfaceSet;
    vector<Ptr<GameObject>> m_vecRuntimeSurfaceObjects;
    bool                    m_bRuntimeBuilt;
    bool                    m_bBuiltForPlayState;
    UINT                    m_BuiltSurfaceCount;

public:
    virtual void Begin() override;
    virtual void Tick() override;

    void SetSurfaceSet(Ptr<ASurfaceSet> _SurfaceSet);
    Ptr<ASurfaceSet> GetSurfaceSet() const { return m_SurfaceSet; }
    UINT GetBuiltSurfaceCount() const { return m_BuiltSurfaceCount; }

    virtual void SaveToLevelFile(FILE* _File) override;
    virtual void LoadFromLevelFile(FILE* _File) override;

    CLONE(CSurfaceSetScript);

public:
    CSurfaceSetScript();
    CSurfaceSetScript(const CSurfaceSetScript& _Origin);
    virtual ~CSurfaceSetScript();

private:
    void BuildRuntimeSurfaces();
    void ClearRuntimeSurfaces();
    bool ShouldBuildForPlayState() const;
};
