#include "pch.h"
#include "CMissileScript.h"

#include "GameObject.h"
#include "TimeMgr.h"
#include "CCollider2D.h"

CMissileScript::CMissileScript()
	: CScript(SCRIPT_TYPE::MISSILESCRIPT)
{
}

CMissileScript::~CMissileScript()
{
}

void CMissileScript::Begin()
{	
	ADD_DYNAMIC_BEGIN_OVERLAP(CMissileScript::BeginOverlap);
	ADD_DYNAMIC_OVERLAP(CMissileScript::Overlap);
	ADD_DYNAMIC_END_OVERLAP(CMissileScript::EndOverlap);
}

void CMissileScript::Tick()
{
	// 추적 대상이 있다면
	if (IsValid(m_Target))
	{		
		Vec3 vTargetPos = m_Target->Transform()->GetWorldPos();
		Vec3 vPos = Transform()->GetRelativePos();

		
		if (m_Dir == Vec3(0.f, 0.f, 0.f))
		{
			m_Dir = vTargetPos - vPos;
			m_Dir.Normalize();

			Vec3 vBase = Vec3(0.f, 1.f, 0.f);						
			float Dot = vBase.Dot(m_Dir);
			float Radian = acosf(Dot);

			if (vTargetPos.x < vPos.x)
				Transform()->SetRelativeRot(Vec3(0.f, 0.f, Radian));
			else
				Transform()->SetRelativeRot(Vec3(0.f, 0.f, -Radian));
		}	
		vPos += m_Dir * 200.f * DT;

		//Vec3 vDirToTarget = vTargetPos - vPos;
		//vDirToTarget.Normalize();
		//vPos += vDirToTarget * 200.f * DT;

		Transform()->SetRelativePos(vPos);
	}
}

void CMissileScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
}

void CMissileScript::Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
}

void CMissileScript::EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
}

void CMissileScript::SaveToLevelFile(FILE* _File)
{
	fwrite(&m_Dir, sizeof(Vec3), 1, _File);
}

void CMissileScript::LoadFromLevelFile(FILE* _File)
{
	fread(&m_Dir, sizeof(Vec3), 1, _File);
}
