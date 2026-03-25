#include "pch.h"
#include "CTileRender.h"

#include "AssetMgr.h"
#include "CTransform.h"
#include "Device.h"
#include "RenderMgr.h"
#include "Source/Scripts/CTileScript.h"

namespace
{
	bool FillTileInfoFromDesc(const TileTypeDesc& _Desc, bool _HalfChecker, TileInfo& _OutInfo)
	{
		TileDrawInfo drawInfo = {};
		if (!CTileScript::ResolveTileTypeDesc(_Desc, _HalfChecker, drawInfo))
			return false;

		_OutInfo.FuncParam0 = Vec4(drawInfo.a, drawInfo.b, drawInfo.c, drawInfo.r);
		_OutInfo.FuncParam1.x = drawInfo.center_x;
		_OutInfo.FuncParam1.y = drawInfo.center_y;
		_OutInfo.FuncParam1.z = (float)drawInfo.mode;
		_OutInfo.FuncParam2.x = drawInfo.customNormal.x;
		_OutInfo.FuncParam2.y = drawInfo.customNormal.y;
		_OutInfo.FuncParam2.z = (float)drawInfo.flags;
		return true;
	}
}

CTileRender::CTileRender()
	: CRenderComponent(COMPONENT_TYPE::TILE_RENDER)
	, m_fOpacity(1.f)
{
	m_Buffer = new StructuredBuffer;
}

CTileRender::CTileRender(const CTileRender& _Origin)
	: CRenderComponent(_Origin)
	, m_TileMap(_Origin.m_TileMap)
	, m_fOpacity(_Origin.m_fOpacity)
	, m_vecTileInfo(_Origin.m_vecTileInfo)
	, m_Buffer(nullptr)
{
	m_Buffer = new StructuredBuffer;	
}

CTileRender::~CTileRender()
{
}

void CTileRender::Init()
{
	CRenderComponent::Init();
	
	SetTileMap(m_TileMap);
}

void CTileRender::FinalTick()
{
	
}

void CTileRender::Render()
{
	if (nullptr == m_TileMap || nullptr == m_Buffer)
		return;

	if (m_TileMap->GetLayoutMode() == TILEMAP_LAYOUT_MODE::PLACEMENT)
	{
		RenderPlacementTiles();
		return;
	}

	UpdateTileInfoBuffer();
	m_Buffer->Binding(20);

	FunctionConst func = {};
	func.contourColor = Vec4(1.f, 0.85f, 0.15f, 1.f);
	func.normalColor = Vec4(0.2f, 1.f, 0.35f, 1.f);
	func.lineThickness = 0.025f;
	func.normalLength = 0.18f;
	func.normalThickness = 0.01f;
	func.showOverlay = RenderMgr::GetInst()->IsDebugRender() ? 1.f : 0.f;

	Device::GetInst()->GetCB(CB_TYPE::FUNCTION)->SetData(&func);
	Device::GetInst()->GetCB(CB_TYPE::FUNCTION)->Binding();

	GetMaterial()->SetScalar(INT_0, m_TileMap->GetRow());
	GetMaterial()->SetScalar(INT_1, m_TileMap->GetCol());
	GetMaterial()->SetScalar(FLOAT_0, m_fOpacity);
	GetMaterial()->Binding();

	GetMesh()->Render();

	m_Buffer->Clear();
}

void CTileRender::SetTileMap(Ptr<ATileMap> _TileMap)
{
	m_TileMap = _TileMap;

	if (nullptr == m_TileMap)
		return;

	if (nullptr == m_Buffer)
		return; // 또는 여기서 생성

	// 이전 정보 리셋
	m_vecTileInfo.clear();

	// 크기 조정
	UINT Row = m_TileMap->GetRow();
	UINT Col = m_TileMap->GetCol();
	Vec2 TileSize = m_TileMap->GetTileSize();

	if (m_TileMap->GetLayoutMode() == TILEMAP_LAYOUT_MODE::GRID)
	{
		// 가로 = Col, 세로 = Row
		Vec3 vScale = Vec3(TileSize.x * (float)Col, TileSize.y * (float)Row, 1.f);
		GetOwner()->Transform()->SetRelativeScale(vScale);
	}
	else
	{
		// Placement 모드는 각 타일이 자기 크기를 직접 가지므로,
		// TileMapRender 오브젝트의 스케일은 1로 유지하는 편이 덜 헷갈린다.
		GetOwner()->Transform()->SetRelativeScale(Vec3(1.f, 1.f, 1.f));
	}

	const vector<UINT>& vecTileTypes = m_TileMap->GetTileTypes();
	const size_t infoCount = (m_TileMap->GetLayoutMode() == TILEMAP_LAYOUT_MODE::PLACEMENT)
		? 1
		: vecTileTypes.size();

	for (size_t i = 0; i < infoCount; ++i)
	{
		TileInfo info = {};
		info.FuncParam1.w = (m_TileMap->GetLayoutMode() == TILEMAP_LAYOUT_MODE::PLACEMENT) ? 0.f : (float)vecTileTypes[i];
		info.FuncParam2 = Vec4(0.f, 1.f, 0.f, 0.f);
		info.FuncParam3 = Vec4(1.f, 1.f, 0.f, 0.f);

		m_vecTileInfo.push_back(info);
	}

	// 구조화 버퍼 크기 부족 시 재생성
	if (!m_vecTileInfo.empty() && m_Buffer->GetBufferSize() < sizeof(TileInfo) * m_vecTileInfo.size())
	{
		m_Buffer->Create(sizeof(TileInfo), (UINT)m_vecTileInfo.size(), SB_TYPE::SRV_ONLY, true);
	}

	UpdateTileInfoBuffer();
}

void CTileRender::UpdateTileInfoBuffer()
{
	if (nullptr == m_TileMap || nullptr == m_Buffer || m_vecTileInfo.empty())
		return;

	const bool halfChecker = CTileScript::GetHalfCheckerState();
	const vector<UINT>& vecTileTypes = m_TileMap->GetTileTypes();
	const vector<TileTypeDesc>& vecTileDefs = m_TileMap->GetTileTypeDescs();
	const vector<Vec2>& vecTileScales = m_TileMap->GetTileScales();

	for (size_t i = 0; i < m_vecTileInfo.size(); ++i)
	{
		TileInfo& info = m_vecTileInfo[i];
		info.FuncParam0 = Vec4(0.f, 0.f, 0.f, 0.f);
		info.FuncParam1.x = 0.f;
		info.FuncParam1.y = 0.f;
		info.FuncParam1.z = 0.f;
		info.FuncParam1.w = 0.f;
		info.FuncParam2 = Vec4(0.f, 1.f, 0.f, 0.f);
		info.FuncParam3 = Vec4(1.f, 1.f, 0.f, 0.f);

		if (vecTileTypes.size() <= i)
			continue;

		const UINT tileValue = vecTileTypes[i];
		info.FuncParam1.w = (float)tileValue;

		if (vecTileDefs.size() <= tileValue)
			continue;

		if (!FillTileInfoFromDesc(vecTileDefs[tileValue], halfChecker, info))
			continue;

		if (i < vecTileScales.size())
		{
			info.FuncParam3.x = vecTileScales[i].x;
			info.FuncParam3.y = vecTileScales[i].y;
		}
	}

	m_Buffer->SetData(m_vecTileInfo.data(), sizeof(TileInfo) * (UINT)m_vecTileInfo.size());
}

void CTileRender::RenderPlacementTiles()
{
	const vector<TilePlacement>& vecPlacements = m_TileMap->GetPlacements();
	const vector<TileTypeDesc>& vecTileDefs = m_TileMap->GetTileTypeDescs();
	if (vecPlacements.empty() || vecTileDefs.empty())
		return;

	if (m_vecTileInfo.empty())
	{
		m_vecTileInfo.push_back(TileInfo{});
	}

	if (m_Buffer->GetBufferSize() < sizeof(TileInfo))
	{
		m_Buffer->Create(sizeof(TileInfo), 1, SB_TYPE::SRV_ONLY, true);
	}

	FunctionConst func = {};
	func.contourColor = Vec4(1.f, 0.85f, 0.15f, 1.f);
	func.normalColor = Vec4(0.2f, 1.f, 0.35f, 1.f);
	func.lineThickness = 0.025f;
	func.normalLength = 0.18f;
	func.normalThickness = 0.01f;
	func.showOverlay = RenderMgr::GetInst()->IsDebugRender() ? 1.f : 0.f;

	Device::GetInst()->GetCB(CB_TYPE::FUNCTION)->SetData(&func);
	Device::GetInst()->GetCB(CB_TYPE::FUNCTION)->Binding();

	GetMaterial()->SetScalar(INT_0, 1);
	GetMaterial()->SetScalar(INT_1, 1);
	GetMaterial()->SetScalar(FLOAT_0, m_fOpacity);
	GetMaterial()->Binding();

	const bool halfChecker = CTileScript::GetHalfCheckerState();
	TransformMatrix prevTrans = g_Trans;
	const Matrix ownerWorld = GetOwner()->Transform()->GetWorldMat();

	Vec3 ownerScale = GetOwner()->Transform()->GetWorldScale();
	if (fabsf(ownerScale.x) <= 0.0001f) ownerScale.x = 1.f;
	if (fabsf(ownerScale.y) <= 0.0001f) ownerScale.y = 1.f;
	if (fabsf(ownerScale.z) <= 0.0001f) ownerScale.z = 1.f;

	const Matrix ownerScaleInv = XMMatrixScaling(1.f / ownerScale.x, 1.f / ownerScale.y, 1.f / ownerScale.z);

	for (size_t i = 0; i < vecPlacements.size(); ++i)
	{
		const TilePlacement& placement = vecPlacements[i];
		if (placement.TypeIdx >= vecTileDefs.size())
			continue;

		TileInfo info = {};
		info.FuncParam1.w = (float)placement.TypeIdx;
		info.FuncParam2 = Vec4(0.f, 1.f, 0.f, 0.f);
		info.FuncParam3 = Vec4(1.f, 1.f, 0.f, 0.f);

		if (!FillTileInfoFromDesc(vecTileDefs[placement.TypeIdx], halfChecker, info))
			continue;

		m_vecTileInfo[0] = info;
		m_Buffer->SetData(m_vecTileInfo.data(), sizeof(TileInfo));
		m_Buffer->Binding(20);

		Matrix matScale = XMMatrixScaling(placement.Size.x, placement.Size.y, 1.f);
		Matrix matRot = XMMatrixRotationZ(placement.Rotation);

		// tile.fx 는 정사각형을 "좌상단 원점" 기준으로 그리지만,
		// Placement 데이터와 충돌 오브젝트는 "중심 좌표"를 들고 있다.
		// 따라서 렌더 시에만 중심 -> 좌상단 보정을 넣어야 그림과 물리가 같은 자리에 놓인다.
		Vec3 centerOffset = Vec3(placement.Size.x * 0.5f, -placement.Size.y * 0.5f, 0.f);
		Vec3 rotatedCenterOffset = XMVector3TransformNormal(centerOffset, matRot);
		Vec3 renderOrigin = Vec3(
			placement.LocalPos.x - rotatedCenterOffset.x,
			placement.LocalPos.y - rotatedCenterOffset.y,
			placement.LocalZ
		);

		Matrix matTrans = XMMatrixTranslation(renderOrigin.x, renderOrigin.y, renderOrigin.z);
		g_Trans.matWorld = matScale * matRot * matTrans;
		g_Trans.matWorld = g_Trans.matWorld * ownerScaleInv * ownerWorld;
		Device::GetInst()->GetCB(CB_TYPE::TRANSFORM)->SetData(&g_Trans);
		Device::GetInst()->GetCB(CB_TYPE::TRANSFORM)->Binding();

		GetMesh()->Render();
	}

	g_Trans = prevTrans;
	Device::GetInst()->GetCB(CB_TYPE::TRANSFORM)->SetData(&g_Trans);
	Device::GetInst()->GetCB(CB_TYPE::TRANSFORM)->Binding();
	m_Buffer->Clear();
}

void CTileRender::CreateMaterial()
{
	wstring MeshName = L"RectMesh";
	wstring MtrlName = L"TileMtrl";
	wstring ShaderName = L"TileShader";
	wstring FilePath = L"Shader\\tile.fx";
	string VS = "VS_Tile";
	string PS = "PS_Tile";

	// RectMesh 설정
	SetMesh(AssetMgr::GetInst()->Find<AMesh>(MeshName));

	// 재질 생성
	Ptr<AMaterial> pMtrl = AssetMgr::GetInst()->Find<AMaterial>(MtrlName);

	// 찾는 재질이 없으면 생성한다.
	if (nullptr == pMtrl)
	{
		pMtrl = new AMaterial;
		pMtrl->SetName(MtrlName);

		// 쉐이더를 찾아서 재질에 세팅해준다.
		Ptr<AGraphicShader> pShader = AssetMgr::GetInst()->Find<AGraphicShader>(ShaderName);
		

		// 찾은 or 생성한 쉐이더를 재질에 설정해주고, 재질도 에셋매니저에 등록한다.
		pMtrl->SetShader(pShader);
		pMtrl->SetDomain(RENDER_DOMAIN::DOMAIN_TRANSPARENT);
		pMtrl->SetScalar(FLOAT_0, 1.f);
		AssetMgr::GetInst()->AddAsset(pMtrl->GetName(), pMtrl.Get());
	}

	SetMaterial(pMtrl);
}

void CTileRender::SaveToLevelFile(FILE* _File)
{
	CRenderComponent::SaveToLevelFile(_File);
	SaveAssetRef(_File, m_TileMap.Get());
	fwrite(&m_fOpacity, sizeof(float), 1, _File);
}

void CTileRender::LoadFromLevelFile(FILE* _File)
{
	CRenderComponent::LoadFromLevelFile(_File);
	m_TileMap = LoadAssetRef<ATileMap>(_File);
	fread(&m_fOpacity, sizeof(float), 1, _File);
	SetTileMap(m_TileMap);
}
