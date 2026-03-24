#include "pch.h"
#include "CMonsterScript.h"

#include "CFlipbookRender.h"
#include "CCollider2D.h"
#include "CTransform.h"
#include "TimeMgr.h"
#include "LevelMgr.h"
#include "TaskMgr.h"
#include "GameObject.h"

CMonsterScript::CMonsterScript()
    : CScript(SCRIPT_TYPE::MONSTERSCRIPT)
    , m_State(MON_STATE::RUN)
    , m_iDir(-1)
    , m_fSpeed(100.f)
	, m_fMaxSpeed(200.f)
	, m_fAccel(200.f)
{
}

CMonsterScript::~CMonsterScript()
{
}

void CMonsterScript::Begin()
{
    Collider2D()->AddDynamicBeginOverlap(this, (COLLISION_EVENT)&CMonsterScript::BeginOverlap);
}

void CMonsterScript::Tick()
{
    m_fSpeed += m_fAccel * DT;
    if (m_fSpeed > m_fMaxSpeed)
    {
        m_fSpeed = m_fMaxSpeed;
    }

    // 1. RUN
    if (m_State == MON_STATE::RUN)
    {

        Vec3 vPos = Transform()->GetRelativePos();

        vPos.x += (float)m_iDir * m_fSpeed * DT;

        Transform()->SetRelativePos(vPos);
    }

    // 2. TURN 
    else if (m_State == MON_STATE::TURN)
    {
        m_fSpeed = 0.f;

        if (GetOwner()->FlipbookRender()->IsFinish())
        {
            m_iDir *= -1;

            Vec3 vScale = Transform()->GetRelativeScale();
            vScale.x *= -1.f;
            Transform()->SetRelativeScale(vScale);

            m_State = MON_STATE::RUN;

            GetOwner()->FlipbookRender()->Play(0, 15.f, -1);
        }
    }
}

void CMonsterScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    // 벽(Wall)과 충돌 시
    if (_OtherCollider->GetOwner()->GetName() == L"Wall1" || _OtherCollider->GetOwner()->GetName() == L"Wall2")
    {
        if (m_State == MON_STATE::TURN)
            return;

        m_State = MON_STATE::TURN;
        GetOwner()->FlipbookRender()->Play(1, 10.f, 1);
    }
}