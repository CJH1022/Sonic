#include "pch.h"
#include "CTileRender.h"

#include "AssetMgr.h"
#include "CTransform.h"
#include "Device.h"
#include "RenderMgr.h"
#include "Source/Scripts/CTileScript.h"

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

	// 가로 = Col, 세로 = Row
	Vec3 vScale = Vec3(TileSize.x * (float)Col, TileSize.y * (float)Row, 1.f);
	GetOwner()->Transform()->SetRelativeScale(vScale);

	const vector<UINT>& vecTileTypes = m_TileMap->GetTileTypes();

	for (size_t i = 0; i < vecTileTypes.size(); ++i)
	{
		TileInfo info = {};
		info.FuncParam1.w = (float)vecTileTypes[i];
		info.FuncParam2 = Vec4(0.f, 1.f, 0.f, 0.f);

		m_vecTileInfo.push_back(info);
	}

	// 구조화 버퍼 크기 부족 시 재생성
	if (m_Buffer->GetBufferSize() < sizeof(TileInfo) * m_vecTileInfo.size())
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

	for (size_t i = 0; i < m_vecTileInfo.size(); ++i)
	{
		TileInfo& info = m_vecTileInfo[i];
		info.FuncParam0 = Vec4(0.f, 0.f, 0.f, 0.f);
		info.FuncParam1.x = 0.f;
		info.FuncParam1.y = 0.f;
		info.FuncParam1.z = 0.f;
		info.FuncParam1.w = 0.f;
		info.FuncParam2 = Vec4(0.f, 1.f, 0.f, 0.f);

		if (vecTileTypes.size() <= i)
			continue;

		const UINT tileValue = vecTileTypes[i];
		info.FuncParam1.w = (float)tileValue;

		if (vecTileDefs.size() <= tileValue)
			continue;

		TileDrawInfo drawInfo = {};
		if (!CTileScript::ResolveTileTypeDesc(vecTileDefs[tileValue], halfChecker, drawInfo))
			continue;

		info.FuncParam0 = Vec4(drawInfo.a, drawInfo.b, drawInfo.c, drawInfo.r);
		info.FuncParam1.x = drawInfo.center_x;
		info.FuncParam1.y = drawInfo.center_y;
		info.FuncParam1.z = (float)drawInfo.mode;
		info.FuncParam2.x = drawInfo.customNormal.x;
		info.FuncParam2.y = drawInfo.customNormal.y;
		info.FuncParam2.z = (float)drawInfo.flags;
	}

	m_Buffer->SetData(m_vecTileInfo.data(), sizeof(TileInfo) * (UINT)m_vecTileInfo.size());
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
