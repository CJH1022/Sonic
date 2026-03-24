#pragma once
#include "CScript.h"

class CCamMoveScript :
    public CScript
{
private:
    Ptr<GameObject> m_Target;
    wstring m_TargetName;

public:
    virtual void Begin() override;
    virtual void Tick() override;

    void SetTarget(Ptr<GameObject> _Target) { m_Target = _Target; }
    void SetTargetName(const wstring& _Name) { m_TargetName = _Name; }

private:
    Ptr<GameObject> FindTarget();
    void MovePerspective();
    void MoveOrthographic();

    CLONE(CCamMoveScript);
public:
    CCamMoveScript();
    virtual ~CCamMoveScript();
};

