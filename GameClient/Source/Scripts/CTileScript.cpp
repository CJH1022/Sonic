#include "pch.h"
#include "CTileScript.h"

#include "KeyMgr.h"
#include "TimeMgr.h"
#include "RenderMgr.h"
#include "CTransform.h"
#include "LevelMgr.h"
#include "GameObject.h"
#include "CCollider2D.h"
#include "TaskMgr.h"
#include "AssetMgr.h"
#include "CTileRender.h"
#include "CPlayerScript.h"

namespace
{
	TileShapeDesc MakeLineShape(float _A, float _B, float _C)
	{
		TileShapeDesc shape = {};
		shape.Mode = TILE_DRAW_MODE::LINE;
		shape.a = _A;
		shape.b = _B;
		shape.c = _C;
		return shape;
	}

	TileShapeDesc MakeCircleShape(float _Radius, float _CenterX, float _CenterY)
	{
		TileShapeDesc shape = {};
		shape.Mode = TILE_DRAW_MODE::CIRCLE;
		shape.r = _Radius;
		shape.centerX = _CenterX;
		shape.centerY = _CenterY;
		return shape;
	}

	TileShapeDesc MakeVerticalShape(float _X, float _C)
	{
		TileShapeDesc shape = {};
		shape.Mode = TILE_DRAW_MODE::VERTICAL;
		shape.a = _X;
		shape.c = _C;
		return shape;
	}

	Vec2 GetColliderHorizontalEdgeWorldPos(CCollider2D* _Collider, float _NormalX)
	{
		if (_Collider == nullptr)
			return Vec2(0.f, 0.f);

		const Matrix& matWorld = _Collider->GetWorldMat();
		const Vec3 vLeftEdge = XMVector3TransformCoord(Vec3(-0.5f, 0.f, 0.f), matWorld);
		const Vec3 vRightEdge = XMVector3TransformCoord(Vec3(0.5f, 0.f, 0.f), matWorld);

		// 수직벽은 발바닥이 아니라 벽을 향하고 있는 몸통의 좌/우 끝점으로 샘플해야
		// 실제 보이는 충돌면과 판정이 일치한다.
		if (_NormalX < -0.001f)
			return Vec2(vRightEdge.x, vRightEdge.y);

		if (_NormalX > 0.001f)
			return Vec2(vLeftEdge.x, vLeftEdge.y);

		const Vec3 vCenter = XMVector3TransformCoord(Vec3(0.f, 0.f, 0.f), matWorld);
		return Vec2(vCenter.x, vCenter.y);
	}
}

CTileScript::CTileScript()
	: CScript(SCRIPT_TYPE::TILESCRIPT)
{
	TileMapSetting(TILETYPE::EMPTY_BLOCK);
}

CTileScript::~CTileScript()
{
}

void CTileScript::BuildPresetTileTypeDesc(TILETYPE _Type, TileTypeDesc& _OutDesc)
{
	_OutDesc = {};
	_OutDesc.LegacyType = (UINT)_Type;

	switch (_Type)
	{
	case TILETYPE::EMPTY_BLOCK:
		_OutDesc.Name = L"Empty";
		_OutDesc.Flags = TILE_FLAG_NONE;
		_OutDesc.MainShape.Mode = TILE_DRAW_MODE::EMPTY;
		break;
	case TILETYPE::LINE_BLOCK_1:
		_OutDesc.Name = L"Line Block 1";
		_OutDesc.Flags = TILE_FLAG_SOLID | TILE_FLAG_ATTACHABLE;
		_OutDesc.MainShape = MakeLineShape(0.1f, 0.4f, -1.f);
		break;
	case TILETYPE::LINE_BLOCK_2:
		_OutDesc.Name = L"Line Block 2";
		_OutDesc.Flags = TILE_FLAG_SOLID | TILE_FLAG_ATTACHABLE;
		_OutDesc.MainShape = MakeLineShape(0.4f, 0.8f, -1.f);
		break;
	case TILETYPE::LINE_BLOCK_3:
		_OutDesc.Name = L"Line Block 3";
		_OutDesc.Flags = TILE_FLAG_SOLID | TILE_FLAG_ATTACHABLE;
		_OutDesc.MainShape = MakeLineShape(0.8f, 0.8f, -1.f);
		break;
	case TILETYPE::LINE_BLOCK_4:
		_OutDesc.Name = L"Line Block 4";
		_OutDesc.Flags = TILE_FLAG_SOLID | TILE_FLAG_ATTACHABLE;
		_OutDesc.MainShape = MakeLineShape(0.8f, 0.4f, -1.f);
		break;
	case TILETYPE::LINE_BLOCK_5:
		_OutDesc.Name = L"Line Block 5";
		_OutDesc.Flags = TILE_FLAG_SOLID | TILE_FLAG_ATTACHABLE;
		_OutDesc.MainShape = MakeLineShape(0.4f, 0.1f, -1.f);
		break;
	case TILETYPE::LINE_BLOCK_6:
		_OutDesc.Name = L"Line Block 6";
		_OutDesc.Flags = TILE_FLAG_SOLID | TILE_FLAG_ATTACHABLE;
		_OutDesc.MainShape = MakeLineShape(0.1f, 0.1f, -1.f);
		break;
	case TILETYPE::CIRCLE_BLOCK_1:
		_OutDesc.Name = L"Circle Block 1";
		_OutDesc.Flags = TILE_FLAG_SOLID | TILE_FLAG_ATTACHABLE;
		_OutDesc.MainShape = MakeCircleShape(0.8f, 1.f, 1.f);
		break;
	case TILETYPE::CIRCLE_BLOCK_2:
		_OutDesc.Name = L"Circle Block 2";
		_OutDesc.Flags = TILE_FLAG_SOLID | TILE_FLAG_ATTACHABLE;
		_OutDesc.MainShape = MakeCircleShape(0.8f, 0.f, 1.f);
		break;
	case TILETYPE::CIRCLE_BLOCK_3:
		_OutDesc.Name = L"Circle Block 3";
		_OutDesc.Flags = TILE_FLAG_SOLID | TILE_FLAG_ATTACHABLE;
		_OutDesc.MainShape = MakeCircleShape(0.8f, 1.f, 0.f);
		_OutDesc.CorrectionShape = MakeLineShape(0.8f, 0.8f, -1.f);
		_OutDesc.CorrectionTrigger = TILE_CORRECTION_TRIGGER::HALF_CHECKER_FALSE;
		break;
	case TILETYPE::CIRCLE_BLOCK_4:
		_OutDesc.Name = L"Circle Block 4";
		_OutDesc.Flags = TILE_FLAG_SOLID | TILE_FLAG_ATTACHABLE;
		_OutDesc.MainShape = MakeCircleShape(0.8f, 0.f, 0.f);
		_OutDesc.CorrectionShape = MakeLineShape(0.8f, 0.8f, -1.f);
		_OutDesc.CorrectionTrigger = TILE_CORRECTION_TRIGGER::HALF_CHECKER_TRUE;
		break;
	case TILETYPE::LINE_BLOCK_VERTICAL:
		_OutDesc.Name = L"Vertical Wall";
		_OutDesc.Flags = TILE_FLAG_SOLID | TILE_FLAG_USE_CUSTOM_NORMAL;
		_OutDesc.MainShape = MakeVerticalShape(0.5f, 1.f);
		_OutDesc.CustomNormal = Vec2(-1.f, 0.f);
		break;
	default:
		BuildPresetTileTypeDesc(TILETYPE::EMPTY_BLOCK, _OutDesc);
		break;
	}
}

bool CTileScript::ResolveTileTypeDesc(const TileTypeDesc& _Desc, bool _HalfChecker, TileDrawInfo& _OutInfo)
{
	_OutInfo = {};

	const TileShapeDesc* pShape = &_Desc.MainShape;

	if (_Desc.CorrectionTrigger == TILE_CORRECTION_TRIGGER::HALF_CHECKER_FALSE && !_HalfChecker)
	{
		pShape = &_Desc.CorrectionShape;
	}
	else if (_Desc.CorrectionTrigger == TILE_CORRECTION_TRIGGER::HALF_CHECKER_TRUE && _HalfChecker)
	{
		pShape = &_Desc.CorrectionShape;
	}

	_OutInfo.a = pShape->a;
	_OutInfo.b = pShape->b;
	_OutInfo.c = pShape->c;
	_OutInfo.r = pShape->r;
	_OutInfo.center_x = pShape->centerX;
	_OutInfo.center_y = pShape->centerY;
	_OutInfo.mode = (int)pShape->Mode;
	_OutInfo.customNormal = _Desc.CustomNormal;
	_OutInfo.flags = _Desc.Flags;

	return true;
}

bool CTileScript::GetTileDrawInfo(TILETYPE _Type, bool _HalfChecker, TileDrawInfo& _OutInfo)
{
	TileTypeDesc desc = {};
	BuildPresetTileTypeDesc(_Type, desc);
	return ResolveTileTypeDesc(desc, _HalfChecker, _OutInfo);
}

void CTileScript::ApplyResolvedInfo()
{
	ResolveTileTypeDesc(m_TileDesc, s_bHalfChecker, m_ResolvedInfo);

	m_eType = (TILETYPE)m_TileDesc.LegacyType;

	a = m_ResolvedInfo.a;
	b = m_ResolvedInfo.b;
	c = m_ResolvedInfo.c;
	r = m_ResolvedInfo.r;
	center_x = m_ResolvedInfo.center_x;
	center_y = m_ResolvedInfo.center_y;

	switch ((TILE_DRAW_MODE)m_ResolvedInfo.mode)
	{
	case TILE_DRAW_MODE::LINE:
		f = [this](float x, float y) { return c * (y - (b - a) * x - a); };
		dfdx = [this](float x, float y) { return c * (a - b); };
		dfdy = [this](float x, float y) { return c; };
		break;
	case TILE_DRAW_MODE::VERTICAL:
		f = [this](float x, float y) { return c * (x - a); };
		dfdx = [this](float x, float y) { return c; };
		dfdy = [this](float x, float y) { return 0.f; };
		break;
	case TILE_DRAW_MODE::CIRCLE:
		f = [this](float x, float y)
			{
				return r * r - (x - center_x) * (x - center_x) - (y - center_y) * (y - center_y);
			};
		dfdx = [this](float x, float y) { return -2.f * (x - center_x); };
		dfdy = [this](float x, float y) { return -2.f * (y - center_y); };
		break;
	case TILE_DRAW_MODE::EMPTY:
	default:
		f = [](float x, float y) { return 1.f; };
		dfdx = [](float x, float y) { return 0.f; };
		dfdy = [](float x, float y) { return 0.f; };
		break;
	}
}

void CTileScript::SetTileDesc(const TileTypeDesc& _Desc, UINT _TypeIdx)
{
	m_TileDesc = _Desc;
	m_TileTypeIdx = _TypeIdx;
	ApplyResolvedInfo();
}

void CTileScript::Begin()
{
	if (Collider2D())
	{
		Collider2D()->AddDynamicBeginOverlap(this, (COLLISION_EVENT)&CTileScript::BeginOverlap);
		Collider2D()->AddDynamicEndOverlap(this, (COLLISION_EVENT)&CTileScript::EndOverlap);
		Collider2D()->AddDynamicOverlap(this, (COLLISION_EVENT)&CTileScript::Overlap);
	}
}

void CTileScript::Tick()
{
	if (!IsSolid())
		return;

	DbgInfo info = {};
	info.Color = Vec4(0.f, 1.f, 0.f, 1.f);
	info.DepthTest = false;
	info.Life = 0.f;

	Vec3 vWorldPos = Transform()->GetWorldPos();
	Vec3 vWorldScale = Transform()->GetRelativeScale();

	if (m_ResolvedInfo.mode == (int)TILE_DRAW_MODE::CIRCLE)
	{
		info.Shape = DBG_SHAPE::CIRCLE;
		info.Pos = Vec3(
			vWorldPos.x + (center_x - 0.5f) * vWorldScale.x,
			vWorldPos.y - (center_y - 0.5f) * vWorldScale.y,
			vWorldPos.z
		);
		info.Scale = Vec3(r * vWorldScale.x * 2.f, r * vWorldScale.y * 2.f, 1.f);
	}
	else
	{
		info.Shape = DBG_SHAPE::RECT;
		info.Pos = vWorldPos;
		info.Scale = vWorldScale;
	}

	RenderMgr::GetInst()->AddDebugInfo(info);
}

void CTileScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
	Overlap(_OwnCollider, _OtherCollider);

	auto pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
	if (pPlayer == nullptr)
		return;

	// 기존 half-checker 전환 로직은 원형 보정용 기본 타일에 그대로 유지한다.
	if (m_eType == TILETYPE::CIRCLE_BLOCK_3)
	{
		Vec3 vPlayerPos = _OtherCollider->GetOwner()->Transform()->GetWorldPos();
		Vec3 vTilePos = Transform()->GetWorldPos();
		Vec3 vTileScale = Transform()->GetRelativeScale();

		float LeftX = vTilePos.x - vTileScale.x * 0.5f;
		if (vPlayerPos.x < LeftX)
		{
			if (s_bHalfChecker == true)
			{
				s_bHalfChecker = false;
				RefreshHalfCheckerTiles();
			}
		}
	}

	if (m_eType == TILETYPE::CIRCLE_BLOCK_4)
	{
		Vec3 vPlayerPos = _OtherCollider->GetOwner()->Transform()->GetWorldPos();
		Vec3 vTilePos = Transform()->GetWorldPos();
		Vec3 vTileScale = Transform()->GetRelativeScale();

		float RightX = vTilePos.x + vTileScale.x * 0.5f;

		if (vPlayerPos.x > RightX)
		{
			if (s_bHalfChecker == false)
			{
				s_bHalfChecker = true;
				RefreshHalfCheckerTiles();
			}
		}
	}

	if (m_eType == TILETYPE::CIRCLE_BLOCK_1)
	{
		if (s_bHalfChecker == false)
		{
			s_bHalfChecker = true;
			RefreshHalfCheckerTiles();
		}
	}
	else if (m_eType == TILETYPE::CIRCLE_BLOCK_2)
	{
		if (s_bHalfChecker == true)
		{
			s_bHalfChecker = false;
			RefreshHalfCheckerTiles();
		}
	}
}

void CTileScript::EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
	auto pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
	if (pPlayer == nullptr)
		return;

	if (m_eType == TILETYPE::CIRCLE_BLOCK_2 && pPlayer->GetIsGround() == true)
	{
		Vec3 vPlayerPos = _OtherCollider->GetOwner()->Transform()->GetWorldPos();
		Vec3 vTilePos = Transform()->GetWorldPos();
		Vec3 vTileScale = Transform()->GetRelativeScale();

		float splitX = vTilePos.x - vTileScale.x * 0.5f;
		bool bExitToRight = (vPlayerPos.x >= splitX);

		bool bNextHalfChecker = bExitToRight ? false : true;
		if (s_bHalfChecker != bNextHalfChecker)
		{
			s_bHalfChecker = bNextHalfChecker;
			RefreshHalfCheckerTiles();
		}
	}
}

void CTileScript::RefreshHalfCheckerTiles()
{
	Ptr<ALevel> pCurLevel = LevelMgr::GetInst()->GetCurLevel();
	if (pCurLevel == nullptr)
		return;

	for (int layerIdx = 0; layerIdx < MAX_LAYER; ++layerIdx)
	{
		const vector<Ptr<GameObject>>& vecObjects = pCurLevel->GetLayer(layerIdx)->GetAllObjects();
		for (const Ptr<GameObject>& pObject : vecObjects)
		{
			if (pObject == nullptr)
				continue;

			auto pTileScript = pObject->GetScript<CTileScript>();
			if (pTileScript == nullptr)
				continue;

			// 동일한 타일 정의를 유지한 채, 현재 half-checker 상태에 맞는
			// 수식/노말만 다시 계산한다.
			pTileScript->ApplyResolvedInfo();
		}
	}
}

void CTileScript::Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
	if (!IsSolid())
		return;

	if (_OtherCollider->GetOwner()->GetName() != L"Player")
		return;

	auto pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
	if (pPlayer == nullptr)
		return;

	const bool wasGround = pPlayer->GetIsGround();
	GetTileState();

	const bool bAttachable = (m_ResolvedInfo.flags & TILE_FLAG_ATTACHABLE) != 0;
	const bool bUseCustomNormal = (m_ResolvedInfo.flags & TILE_FLAG_USE_CUSTOM_NORMAL) != 0;

	Vec3 vPlayerPos = _OtherCollider->GetOwner()->Transform()->GetWorldPos();
	Vec3 vPlayerScale = _OtherCollider->GetOwner()->Transform()->GetRelativeScale();

	Vec3 vTilePos = Transform()->GetWorldPos();
	Vec3 vTileScale = Transform()->GetRelativeScale();

	Vec2 vFootPos = Vec2(vPlayerPos.x, vPlayerPos.y);
	const bool bVerticalTile = ((TILE_DRAW_MODE)m_ResolvedInfo.mode == TILE_DRAW_MODE::VERTICAL);

	if (bVerticalTile)
	{
		float sampleNormalX = m_ResolvedInfo.customNormal.x;
		if (fabsf(sampleNormalX) <= 0.001f)
			sampleNormalX = c;

		vFootPos = GetColliderHorizontalEdgeWorldPos(_OtherCollider, sampleNormalX);
	}
	else if (wasGround)
	{
		float fRotZ = _OtherCollider->GetOwner()->Transform()->GetRelativeRot().z;
		Vec2 vLocalDown = Vec2(sinf(fRotZ), -cosf(fRotZ));
		vFootPos += vLocalDown * (vPlayerScale.y * 0.5f);
	}
	else
	{
		float halfW = fabsf(vPlayerScale.x) * 0.5f;
		float halfH = fabsf(vPlayerScale.y) * 0.5f;
		float radius = (halfW < halfH) ? halfW : halfH;
		vFootPos.y -= radius;
	}

	float left = vTilePos.x - vTileScale.x * 0.5f;
	float top = vTilePos.y + vTileScale.y * 0.5f;

	Vec2 localPos = {};
	localPos.x = (vFootPos.x - left) / vTileScale.x;
	localPos.y = (top - vFootPos.y) / vTileScale.y;

	const float edgeEpsilon = 0.02f;
	if (localPos.x < -edgeEpsilon || localPos.x > 1.f + edgeEpsilon ||
		localPos.y < -edgeEpsilon || localPos.y > 1.f + edgeEpsilon)
	{
		return;
	}

	if (localPos.x < 0.f) localPos.x = 0.f;
	else if (localPos.x > 1.f) localPos.x = 1.f;
	if (localPos.y < 0.f) localPos.y = 0.f;
	else if (localPos.y > 1.f) localPos.y = 1.f;

	float fValue = GetFvalue(localPos);
	const float detachThreshold = wasGround ? 0.02f : 0.005f;
	const float transitionSnapThreshold = wasGround ? 0.08f : detachThreshold;
	if (fValue > transitionSnapThreshold)
	{
		return;
	}
	const bool bTransitionSurface = (fValue > detachThreshold);

	Vec2 normalLocal = {};
	float mag = 0.f;
	GetNormal(localPos, normalLocal, mag);

	if (mag <= 0.001f)
	{
		return;
	}

	Vec2 worldNormal = Vec2(
		normalLocal.x * vTileScale.x,
		-normalLocal.y * vTileScale.y
	);

	float worldNormalLen = sqrtf(worldNormal.x * worldNormal.x + worldNormal.y * worldNormal.y);
	if (worldNormalLen <= 0.001f)
		return;

	worldNormal.x /= worldNormalLen;
	worldNormal.y /= worldNormalLen;

	// 수직벽/특수 타일은 수학적 gradient 대신 사용자가 고른 노말을 강제로 쓰게 할 수 있다.
	if (bUseCustomNormal)
	{
		Vec2 customNormal = m_ResolvedInfo.customNormal;
		float customLen = sqrtf(customNormal.x * customNormal.x + customNormal.y * customNormal.y);
		if (customLen > 0.001f)
		{
			worldNormal.x = customNormal.x / customLen;
			worldNormal.y = customNormal.y / customLen;
		}
	}

	Vec2 vOldNormal = pPlayer->GetNormal();
	if (fabsf(vOldNormal.x) < 0.0001f && fabsf(vOldNormal.y) < 0.0001f)
		vOldNormal = worldNormal;
	else
		vOldNormal.Normalize();

	float normalDot = vOldNormal.x * worldNormal.x + vOldNormal.y * worldNormal.y;
	if (normalDot > 1.f) normalDot = 1.f;
	if (normalDot < -1.f) normalDot = -1.f;

	float normalBlend = wasGround ? 0.2f : 1.f;
	if (m_TileState == TILESTATE::CIRCLE_BLOCK && wasGround)
		normalBlend = 0.3f;
	if (wasGround)
	{
		if (normalDot < 0.75f)
			normalBlend = 1.f;
		else if (normalDot < 0.9f && normalBlend < 0.6f)
			normalBlend = 0.6f;
	}
	if (bTransitionSurface)
		normalBlend = 1.f;

	vCurNormal = Lerp(vOldNormal, worldNormal, normalBlend);
	if (fabsf(vCurNormal.x) < 0.0001f && fabsf(vCurNormal.y) < 0.0001f)
		vCurNormal = worldNormal;
	else
		vCurNormal.Normalize();

	pPlayer->SetNormal(vCurNormal);

	float worldPush = 0.001f;
	if (fValue < 0.f)
	{
		float push = (-fValue / mag);
		worldPush = push * ((vTileScale.x + vTileScale.y) * 0.5f) + 0.001f;
		const float maxPush = 12.f;
		if (worldPush > maxPush)
			worldPush = maxPush;

		Vec3 vPos = _OtherCollider->GetOwner()->Transform()->GetRelativePos();
		vPos.x += vCurNormal.x * worldPush;
		vPos.y += vCurNormal.y * worldPush;
		_OtherCollider->GetOwner()->Transform()->SetRelativePos(vPos);
	}

	Vec2 tangent = Vec2(vCurNormal.y, -vCurNormal.x);
	Vec2 v = pPlayer->GetVelocity();

	float rawVn = v.x * vCurNormal.x + v.y * vCurNormal.y;
	const float incomingVn = rawVn;
	float vt = v.x * tangent.x + v.y * tangent.y;

	const bool isCircleTile = (m_TileState == TILESTATE::CIRCLE_BLOCK);
	const float breakDropSpeedThreshold = 100.f;
	const bool blockedByWallLikeTile =
		pPlayer->IsBreakOrRollAction() &&
		(fabsf(vCurNormal.y) < 0.35f) &&
		(fabsf(incomingVn) > 20.f);

	const bool bBreakCircleDrop =
		isCircleTile &&
		pPlayer->GetAction() == ActionState::Break &&
		(fabsf(vCurNormal.y) < 0.35f) &&
		(pPlayer->GetBreakSpeed() < breakDropSpeedThreshold);

	if (bBreakCircleDrop)
	{
		Vec2 vDrop = pPlayer->GetVelocity();
		vDrop.x = 0.f;
		pPlayer->SetVelocity(vDrop);
		pPlayer->SetIsGround(false);
		pPlayer->StopBlockedAction();
		return;
	}

	if (blockedByWallLikeTile && pPlayer->GetAction() == ActionState::Break)
	{
		pPlayer->RequestBreakWallStop();
		pPlayer->SetVelocity(Vec2(0.f, 0.f));
		return;
	}

	if (rawVn < 0.f)
	{
		v.x -= vCurNormal.x * rawVn;
		v.y -= vCurNormal.y * rawVn;
		rawVn = 0.f;
	}

	// attachable 플래그가 꺼진 타일은 "벽처럼 막기만 하고"
	// 플레이어를 바닥 상태로 고정하지 않는다.
	if (!bAttachable)
	{
		pPlayer->SetIsGround(false);
		pPlayer->SetGroundTangent(tangent);
		pPlayer->SetVelocity(v);
		return;
	}

	const float stickMinSpeed = 500.f;
	const float normalEnterY = 0.65f;
	const float normalKeepY = 0.45f;
	const float vnEnterTolerance = 5.f;
	const float vnKeepTolerance = 80.f;
	const float normalThreshold = wasGround ? normalKeepY : normalEnterY;
	const float vnTolerance = wasGround ? vnKeepTolerance : vnEnterTolerance;

	bool bSpeedStick = (fabsf(vt) > stickMinSpeed) && (vCurNormal.y > 0.2f);
	bool bCanStick =
		(rawVn <= vnTolerance) &&
		(vCurNormal.y > normalThreshold || bSpeedStick);

	// 수직벽처럼 노말을 강제로 지정한 타일은 "붙는 벽" 플래그가 켜져 있으면
	// y 성분이 작아도 플레이어를 강제로 부착시킨다.
	const bool bForceAttach = bAttachable && (bUseCustomNormal || m_ResolvedInfo.mode == (int)TILE_DRAW_MODE::VERTICAL);
	if (bForceAttach)
		bCanStick = true;

	if (bTransitionSurface && vCurNormal.y < normalThreshold && !bForceAttach)
		bCanStick = false;

	if (blockedByWallLikeTile)
	{
		if (pPlayer->GetAction() == ActionState::Break)
			pPlayer->RequestBreakWallStop();
		else
			pPlayer->StopBlockedAction();

		if (pPlayer->GetAction() == ActionState::Break)
		{
			Vec2 vBlocked = pPlayer->GetVelocity();
			vBlocked.x = 0.f;
			pPlayer->SetVelocity(vBlocked);
			return;
		}
	}

	if (bCanStick)
	{
		pPlayer->SetIsGround(true);
		pPlayer->SetNormal(vCurNormal);
		pPlayer->SetGroundTangent(tangent);

		if (vCurNormal.y > 0.f)
		{
			float dt = DT;
			float friction = 100.f;

			if (vt > 0.f)
			{
				vt -= friction * dt;
				if (vt < 0.f) vt = 0.f;
			}
			else if (vt < 0.f)
			{
				vt += friction * dt;
				if (vt > 0.f) vt = 0.f;
			}
		}

		v = tangent * vt;
	}
	else
	{
		const bool definiteAirborne =
			(rawVn > vnKeepTolerance) ||
			(vCurNormal.y < 0.2f && fabsf(vt) < stickMinSpeed);

		if (definiteAirborne)
			pPlayer->SetIsGround(false);

		pPlayer->SetNormal(vCurNormal);
		pPlayer->SetGroundTangent(tangent);

		if (vCurNormal.y <= 0.65f && fabsf(vt) < stickMinSpeed)
		{
			if (v.y > 0.f)
			{
				v.y = 0.f;
			}
		}
	}

	pPlayer->SetVelocity(v);
}

void CTileScript::TileMapSetting(TILETYPE i)
{
	TileTypeDesc desc = {};
	BuildPresetTileTypeDesc(i, desc);
	SetTileDesc(desc, (UINT)i);
}

float CTileScript::GetFvalue(Vec2 _pos)
{
	if (!f)
		return -1.f;

	return f(_pos.x, _pos.y);
}

void CTileScript::GetNormal(Vec2 _pos, Vec2& _normal, float& _mag)
{
	if (!dfdx || !dfdy)
	{
		_normal = Vec2(0.f, 0.f);
		_mag = 0.f;
		return;
	}

	float dx = dfdx(_pos.x, _pos.y);
	float dy = dfdy(_pos.x, _pos.y);

	_mag = sqrtf(dx * dx + dy * dy);

	if (_mag > 0.f)
	{
		_normal.x = dx / _mag;
		_normal.y = dy / _mag;
	}
	else
	{
		_normal = Vec2(0.f, 0.f);
	}
}
