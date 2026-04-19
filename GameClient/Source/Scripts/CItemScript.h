#pragma once
#include "CScript.h"

class CCollider2D;
class CPlayerScript;

class CItemScript :
    public CScript
{
public:
    enum ITEMBOX
    {
        NONE = 0,
        UPLIFEBOX,
        ELECTIRCBOX,
        FIREBOX,
        WATERBOX,
        STARBOX,
        COINBOX,
    };

private:
    ITEMBOX m_Curstate;

public:
    virtual void Begin() override;
    virtual void Tick() override;

    virtual void SaveToLevelFile(FILE* _File) override;
    virtual void LoadFromLevelFile(FILE* _File) override;

    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);

    ITEMBOX GetBoxType() const { return m_Curstate; }
    void SetBoxType(ITEMBOX _Type);

    bool ApplyEditorBoxSetup();
    void ApplyItemEffect(CPlayerScript* _PlayerScript);
    void CreateItemBox(ITEMBOX _Type);
    void Dead();

public:
    CLONE(CItemScript);
    CItemScript();
    virtual ~CItemScript();
};
