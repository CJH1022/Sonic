#include "pch.h"
#include "CollisionMgr.h"

#include "AssetMgr.h"
#include "Source/Scripts/CSurfaceScript.h"

namespace
{
	ULONGLONG MakeCollisionKey(UINT _LeftID, UINT _RightID)
	{
		COL_ID colid = {};
		colid.LeftID = (_LeftID < _RightID) ? _LeftID : _RightID;
		colid.RightID = (_LeftID < _RightID) ? _RightID : _LeftID;
		return colid.ID;
	}

	bool IsPlayerLayer(Layer* _Layer)
	{
		return (_Layer != nullptr) && (_Layer->GetName() == wstring(L"Player"));
	}

	void CollectLayerCollidersForPair(Layer* _Layer, bool _PairedWithPlayerLayer, vector<Ptr<CCollider2D>>& _Out)
	{
		if (_Layer == nullptr)
			return;

		const vector<Ptr<GameObject>>& vecObjects = _Layer->GetAllObjects();
		_Out.reserve(vecObjects.size());

		for (size_t i = 0; i < vecObjects.size(); ++i)
		{
			if (vecObjects[i] == nullptr || vecObjects[i]->IsDead())
				continue;

			Ptr<CCollider2D> pCollider = vecObjects[i]->Collider2D();
			if (pCollider == nullptr)
				continue;

			if (_PairedWithPlayerLayer)
			{
				auto pSurface = vecObjects[i]->GetScript<CSurfaceScript>();
				if (pSurface != nullptr && pSurface->GetRole() != CSurfaceScript::SURFACE_ROLE::WALL)
					continue;
			}

			_Out.push_back(pCollider);
		}
	}
}

CollisionMgr::CollisionMgr()
{
}

CollisionMgr::~CollisionMgr()
{
}

void CollisionMgr::Progress(Ptr<ALevel> _Level)
{
	UINT* pMatrix = _Level->GetCollisionMatrix();

	for (UINT Row = 0; Row < MAX_LAYER; ++Row)
	{
		for (UINT Col = Row; Col < MAX_LAYER; ++Col)
		{
			if (false == (pMatrix[Row] & (1 << Col)))
				continue;

			CollisionBtwLayer(_Level->GetLayer(Row), _Level->GetLayer(Col));
		}
	}
}

void CollisionMgr::CollisionBtwLayer(Layer* _Left, Layer* _Right)
{
	const bool bSameLayer = (_Left == _Right);
	const bool leftPlayerLayer = IsPlayerLayer(_Left);
	const bool rightPlayerLayer = IsPlayerLayer(_Right);

	vector<Ptr<CCollider2D>> vecLeft;
	vector<Ptr<CCollider2D>> vecRight;
	CollectLayerCollidersForPair(_Left, rightPlayerLayer, vecLeft);
	if (bSameLayer)
		vecRight = vecLeft;
	else
		CollectLayerCollidersForPair(_Right, leftPlayerLayer, vecRight);

	for (size_t i = 0; i < vecLeft.size(); ++i)
	{
		if (vecLeft[i] == nullptr || vecLeft[i]->GetOwner() == nullptr)
			continue;

		const size_t startIdx = bSameLayer ? (i + 1) : 0;
		for (size_t j = startIdx; j < vecRight.size(); ++j)
		{
			if (vecRight[j] == nullptr || vecRight[j]->GetOwner() == nullptr)
				continue;

			const ULONGLONG collisionKey =
				MakeCollisionKey(vecLeft[i]->GetID(), vecRight[j]->GetID());

			map<ULONGLONG, bool>::iterator iter = m_mapColID.find(collisionKey);
			if (iter == m_mapColID.end())
			{
				m_mapColID.insert(make_pair(collisionKey, false));
				iter = m_mapColID.find(collisionKey);
			}

			const bool isDead = vecLeft[i]->GetOwner()->IsDead() || vecRight[j]->GetOwner()->IsDead();
			if (IsCollision(vecLeft[i], vecRight[j]))
			{
				if (isDead)
				{
					vecLeft[i]->EndOverlap(vecRight[j]);
					vecRight[j]->EndOverlap(vecLeft[i]);
				}
				else if (iter->second)
				{
					vecLeft[i]->Overlap(vecRight[j]);
					vecRight[j]->Overlap(vecLeft[i]);
				}
				else
				{
					vecLeft[i]->BeginOverlap(vecRight[j]);
					vecRight[j]->BeginOverlap(vecLeft[i]);
				}

				iter->second = true;
			}
			else
			{
				if (iter->second)
				{
					vecLeft[i]->EndOverlap(vecRight[j]);
					vecRight[j]->EndOverlap(vecLeft[i]);
				}

				iter->second = false;
			}
		}
	}
}

bool CollisionMgr::IsCollision(Ptr<CCollider2D> _LeftCol, Ptr<CCollider2D> _RightCol)
{
	if (_LeftCol->m_WorldAABBMax.x < _RightCol->m_WorldAABBMin.x ||
		_RightCol->m_WorldAABBMax.x < _LeftCol->m_WorldAABBMin.x ||
		_LeftCol->m_WorldAABBMax.y < _RightCol->m_WorldAABBMin.y ||
		_RightCol->m_WorldAABBMax.y < _LeftCol->m_WorldAABBMin.y)
	{
		return false;
	}

	Ptr<AMesh> pRectMesh = FIND(AMesh, L"RectMesh");

	const Vtx* pVtx = pRectMesh->GetVtxSysMem();

	const Matrix& matWorldLeft = _LeftCol->GetWorldMat();
	const Matrix& matWorldRight = _RightCol->GetWorldMat();

	Vec3 Axis[4] = {};
	Axis[0] = XMVector3TransformCoord(pVtx[1].vPos, matWorldLeft) - XMVector3TransformCoord(pVtx[0].vPos, matWorldLeft);
	Axis[1] = XMVector3TransformCoord(pVtx[3].vPos, matWorldLeft) - XMVector3TransformCoord(pVtx[0].vPos, matWorldLeft);
	Axis[2] = XMVector3TransformCoord(pVtx[1].vPos, matWorldRight) - XMVector3TransformCoord(pVtx[0].vPos, matWorldRight);
	Axis[3] = XMVector3TransformCoord(pVtx[3].vPos, matWorldRight) - XMVector3TransformCoord(pVtx[0].vPos, matWorldRight);

	Vec3 vCenter = XMVector3TransformCoord(Vec3(0.f, 0.f, 0.f), matWorldRight) - XMVector3TransformCoord(Vec3(0.f, 0.f, 0.f), matWorldLeft);

	for (int i = 0; i < 4; ++i)
	{
		Vec3 vProjAxis = Axis[i];
		vProjAxis.Normalize();

		float Dot = 0.f;
		for (int j = 0; j < 4; ++j)
		{
			Dot += fabs(vProjAxis.Dot(Axis[j]));
		}
		Dot /= 2.f;

		float fCenter = fabs(vCenter.Dot(vProjAxis));
		if (fCenter > Dot)
			return false;
	}

	return true;
}
