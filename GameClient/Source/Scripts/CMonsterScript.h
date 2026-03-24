#pragma once
#include "CScript.h"

enum class MON_STATE
{
    RUN,  
    TURN, 
};

class CMonsterScript :
    public CScript
{
private:
    MON_STATE   m_State;   
    int         m_iDir;    
    float       m_fSpeed;  
	float       m_fMaxSpeed;
	float       m_fAccel;

public:

    virtual void Begin();
    virtual void Tick() override;

    void BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider);
    
public:
    CLONE(CMonsterScript);
    CMonsterScript();
    virtual ~CMonsterScript();
};



