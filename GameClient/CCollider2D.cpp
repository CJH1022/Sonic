#include "pch.h"
#include "CCollider2D.h"

#include "RenderMgr.h"
#include "CScript.h"

CCollider2D::CCollider2D()
	: Component(COMPONENT_TYPE::COLLIDER2D)
	, m_Scale(Vec2(1.f, 1.f))
	, m_OverlapCount(0)
{
}

CCollider2D::CCollider2D(const CCollider2D& _Origin)
	: Component(_Origin)
	, m_Offset(_Origin.m_Offset)
	, m_Scale(_Origin.m_Scale)
	, m_OverlapCount(0)	
{
}

CCollider2D::~CCollider2D()
{
}

void CCollider2D::FinalTick()
{
	Matrix matTran = XMMatrixTranslation(m_Offset.x, m_Offset.y, 0.f);
	Matrix matScale = XMMatrixScaling(m_Scale.x, m_Scale.y, 0.f);

	m_matWorld = matScale * matTran;
	m_matWorld *= Transform()->GetWorldMat();

	const Vec3 center = XMVector3TransformCoord(Vec3(0.f, 0.f, 0.f), m_matWorld);
	const Vec3 halfRight = XMVector3TransformNormal(Vec3(0.5f, 0.f, 0.f), m_matWorld);
	const Vec3 halfUp = XMVector3TransformNormal(Vec3(0.f, 0.5f, 0.f), m_matWorld);
	const Vec2 aabbHalfExtent = Vec2(fabsf(halfRight.x) + fabsf(halfUp.x),
		fabsf(halfRight.y) + fabsf(halfUp.y));
	m_WorldAABBMin = Vec2(center.x, center.y) - aabbHalfExtent;
	m_WorldAABBMax = Vec2(center.x, center.y) + aabbHalfExtent;

	if (RenderMgr::GetInst()->IsDebugRender())
	{
		if (0 < m_OverlapCount)
			DrawDebugRect(m_matWorld, Vec4(1.f, 0.f, 0.f, 1.f), 0.f);
		else if (m_OverlapCount == 0)
			DrawDebugRect(m_matWorld, Vec4(0.f, 1.f, 0.f, 1.f), 0.f);
		else
			assert(nullptr);
	}
}


void CCollider2D::BeginOverlap(Ptr<CCollider2D> _Other)
{	
	++m_OverlapCount;

	for (size_t i = 0; i < m_vecBeginDel.size(); ++i)
	{
		(m_vecBeginDel[i].Inst->*m_vecBeginDel[i].MemFunc)(this, _Other.Get());
	}

}

void CCollider2D::Overlap(Ptr<CCollider2D> _Other)
{
	for (size_t i = 0; i < m_vecOverDel.size(); ++i)
	{
		(m_vecOverDel[i].Inst->*m_vecOverDel[i].MemFunc)(this, _Other.Get());
	}
}

void CCollider2D::EndOverlap(Ptr<CCollider2D> _Other)
{
	--m_OverlapCount;

	for (size_t i = 0; i < m_vecEndDel.size(); ++i)
	{
		(m_vecEndDel[i].Inst->*m_vecEndDel[i].MemFunc)(this, _Other.Get());
	}

}


void CCollider2D::AddDynamicBeginOverlap(CScript* _Inst, COLLISION_EVENT _MemFunc)
{
	m_vecBeginDel.push_back(COLLISION_DELEGATE{ _Inst , _MemFunc });
}

void CCollider2D::AddDynamicOverlap(CScript* _Inst, COLLISION_EVENT _MemFunc)
{
	m_vecOverDel.push_back(COLLISION_DELEGATE{ _Inst , _MemFunc });

}

void CCollider2D::AddDynamicEndOverlap(CScript* _Inst, COLLISION_EVENT _MemFunc)
{
	m_vecEndDel.push_back(COLLISION_DELEGATE{ _Inst , _MemFunc });
}

void CCollider2D::RemoveDynamicDelegates(CScript* _Inst)
{
	if (nullptr == _Inst)
		return;

	for (vector<COLLISION_DELEGATE>::iterator iter = m_vecBeginDel.begin(); iter != m_vecBeginDel.end(); )
	{
		if (iter->Inst == _Inst)
			iter = m_vecBeginDel.erase(iter);
		else
			++iter;
	}

	for (vector<COLLISION_DELEGATE>::iterator iter = m_vecOverDel.begin(); iter != m_vecOverDel.end(); )
	{
		if (iter->Inst == _Inst)
			iter = m_vecOverDel.erase(iter);
		else
			++iter;
	}

	for (vector<COLLISION_DELEGATE>::iterator iter = m_vecEndDel.begin(); iter != m_vecEndDel.end(); )
	{
		if (iter->Inst == _Inst)
			iter = m_vecEndDel.erase(iter);
		else
			++iter;
	}
}

void CCollider2D::SaveToLevelFile(FILE* _File)
{
	fwrite(&m_Offset, sizeof(Vec2), 1, _File);
	fwrite(&m_Scale, sizeof(Vec2), 1, _File);
}

void CCollider2D::LoadFromLevelFile(FILE* _File)
{
	fread(&m_Offset, sizeof(Vec2), 1, _File);
	fread(&m_Scale, sizeof(Vec2), 1, _File);
}
