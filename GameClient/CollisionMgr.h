#pragma once

#include "ALevel.h"
#include "Layer.h"
#include "CCollider2D.h"

union COL_ID
{
	struct
	{
		UINT	LeftID;
		UINT	RightID;
	};
	ULONGLONG	ID;
};

class CollisionMgr
	: public singleton<CollisionMgr>
{
	SINGLE(CollisionMgr);
private:
	map<ULONGLONG, bool>	m_mapColID;

public:
	void Progress(Ptr<ALevel> _Level);

private:
	void CollisionBtwLayer(Layer* _Left, Layer* _Right);
	bool IsCollision(Ptr<CCollider2D> _LeftCol, Ptr<CCollider2D> _RightCol);
};

