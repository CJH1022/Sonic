#include "pch.h"
#include "CTileRender.h"

#include "AssetMgr.h"
#include "CTransform.h"

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
	, m_vecSpriteInfo(_Origin.m_vecSpriteInfo)
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
        return;

	m_Buffer->Binding(20);

	GetMaterial()->SetTexture(TEX_0, m_TileMap->GetAtlas());
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
	m_vecSpriteInfo.clear();

	// 크기 조정
	UINT Row = m_TileMap->GetRow();
	UINT Col = m_TileMap->GetCol();
	Vec2 TileSize = m_TileMap->GetTileSize();

    if (m_TileMap->GetLayoutMode() == TILEMAP_LAYOUT_MODE::PLACEMENT)
        return;

	// 가로 = Col, 세로 = Row
	Vec3 vScale = Vec3(TileSize.x * (float)Col, TileSize.y * (float)Row, 1.f);
	GetOwner()->Transform()->SetRelativeScale(vScale);

	// TileMap 의 Sprite UV 정보 받아오기
	vector<Ptr<ASprite>> vecSprites = m_TileMap->GetSprites();

	for (int i = 0; i < (int)vecSprites.size(); ++i)
	{
		SpriteInfo info = {};

		if (nullptr != vecSprites[i])
		{
			info.LeftTop = vecSprites[i]->GetLeftTopUV();
			info.Slice = vecSprites[i]->GetSliceUV();
		}

		m_vecSpriteInfo.push_back(info);
	}

	// 구조화 버퍼 크기 부족 시 재생성
	if (m_Buffer->GetBufferSize() < sizeof(SpriteInfo) * m_vecSpriteInfo.size())
	{
		m_Buffer->Create(sizeof(SpriteInfo), (UINT)m_vecSpriteInfo.size(), SB_TYPE::SRV_ONLY, true);
	}

	// 데이터 업로드
	if (!m_vecSpriteInfo.empty())
	{
		m_Buffer->SetData(m_vecSpriteInfo.data(), sizeof(SpriteInfo) * m_vecSpriteInfo.size());
	}
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
