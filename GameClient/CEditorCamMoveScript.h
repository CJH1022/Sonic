#pragma once
#include "CScript.h"
class CEditorCamMoveScript :
    public CScript
{
private:
    float   m_Speed;

public:
    virtual void Tick() override;

private:
    void MovePerspective();
    void MoveOrthographic();


    CLONE(CEditorCamMoveScript);
public:
    CEditorCamMoveScript();
    virtual ~CEditorCamMoveScript();
};

