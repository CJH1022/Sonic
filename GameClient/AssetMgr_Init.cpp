#include "pch.h"
#include "AssetMgr.h"

#include "PathMgr.h"
#include "Source/Scripts/CMissileScript.h"
#include <filesystem>

namespace
{
	void LoadTileMapAssetsFromContent()
	{
		const std::filesystem::path tileMapDir = std::filesystem::path(CONTENT_PATH) / L"TileMap";

		if (!std::filesystem::exists(tileMapDir))
		{
			std::filesystem::create_directories(tileMapDir);
			return;
		}

		for (const auto& entry : std::filesystem::directory_iterator(tileMapDir))
		{
			if (!entry.is_regular_file())
				continue;

			const std::filesystem::path path = entry.path();
			if (path.extension() != L".tile")
				continue;

			const wstring key = path.stem().wstring();
			const wstring relativePath = L"TileMap\\" + path.filename().wstring();
			AssetMgr::GetInst()->Load<ATileMap>(key, relativePath);
		}
	}
}

void AssetMgr::Init()
{
	CreateEngineMesh();

	CreateEngineShader();

	CreateEngineTexture();

	CreateEngineMaterial();

	CreateEngineSprite();
	LoadTileMapAssetsFromContent();

	CreateEnginePrefab();
}

void AssetMgr::CreateEngineMesh()
{
	Ptr<AMesh> pMesh = nullptr;

	// ========
	// RectMesh
	// ========
	Vtx arrVtx[4] = {};

	arrVtx[0].vPos = Vec3(-0.5f, 0.5f, 0.f);
	arrVtx[0].vUV = Vec2(0.f, 0.f);
	arrVtx[0].vColor = Vec4(1.f, 0.f, 0.f, 0.f);

	arrVtx[1].vPos = Vec3(0.5f, 0.5f, 0.f);
	arrVtx[1].vUV = Vec2(1.f, 0.f);
	arrVtx[1].vColor = Vec4(0.f, 0.f, 1.f, 0.f);

	arrVtx[2].vPos = Vec3(0.5f, -0.5f, 0.f);
	arrVtx[2].vUV = Vec2(1.f, 1.f);
	arrVtx[2].vColor = Vec4(0.f, 1.f, 0.f, 0.f);

	arrVtx[3].vPos = Vec3(-0.5f, -0.5f, 0.f);
	arrVtx[3].vUV = Vec2(0.f, 1.f);
	arrVtx[3].vColor = Vec4(1.f, 0.f, 0.f, 0.f);

	UINT arrIdx[6] = { 0, 2, 3, 0, 1, 2 };

	// 사각형 메쉬 생성
	pMesh = new AMesh;
	pMesh->Create(arrVtx, 4, arrIdx, 6);	
	AddAsset(L"RectMesh", pMesh.Get());


	// ==================
	// RectMesh_LineStrip
	// ==================
	arrIdx[0] = 0; 	arrIdx[1] = 1;	arrIdx[2] = 2;	arrIdx[3] = 3; arrIdx[4] = 0;
	pMesh = new AMesh;
	pMesh->Create(arrVtx, 4, arrIdx, 5);
	AddAsset(L"RectMesh_LineStrip", pMesh.Get());


	// ==========
	// 삼각형 메쉬
	// ==========
	Vtx arr[3] = {};
	arr[0].vPos = Vec3(0.f, 1.f, 0.f);
	arr[0].vColor = Vec4(1.f, 1.f, 1.f, 1.f);

	arr[1].vPos = Vec3(1.f, -1.f, 0.f);
	arr[1].vColor = Vec4(1.f, 1.f, 1.f, 1.f);

	arr[2].vPos = Vec3(-1.f, -1.f, 0.f);
	arr[2].vColor = Vec4(1.f, 1.f, 1.f, 1.f);

	UINT idx[3] = { 0 , 1 , 2 };

	pMesh = new AMesh;
	pMesh->Create(arr, 3, idx, 3);
	AddAsset(L"TriMesh", pMesh.Get());


	// ===============
	// 원 (CircleMesh)
	// ===============
	vector<Vtx>	vecVtx;
	vector<UINT> vecIdx;

	// 중점
	Vtx v;
	v.vPos = Vec3(0.f, 0.f, 0.f);
	v.vUV = Vec2(0.5f, 0.5f);
	v.vColor = Vec4(1.f, 1.f, 1.f, 1.f);
	vecVtx.push_back(v);

	float Theta = 0.f;
	float Radius = 0.5f;
	float Slice = 40.f;

	// 원의 테두리 정점 추가
	for (int i = 0; i < (int)Slice + 1; ++i)
	{
		v.vPos = Vec3(Radius * cosf(Theta), Radius * sinf(Theta), 0.f);
		//v.vUV = Vec2(0.5f, 0.5f);
		v.vColor = Vec4(1.f, 1.f, 1.f, 1.f);
		vecVtx.push_back(v);

		Theta += XM_2PI / Slice;
	}	

	// 인덱스
	for (int i = 0; i < (int)Slice; ++i)
	{
		vecIdx.push_back(0);
		vecIdx.push_back(i + 2);
		vecIdx.push_back(i + 1);
	}

	pMesh = new AMesh;
	pMesh->Create(vecVtx.data(), vecVtx.size(), vecIdx.data(), vecIdx.size());
	AddAsset(L"CircleMesh", pMesh.Get());


	// ====================
	// CircleMesh_LineStrip
	// ====================
	vecIdx.clear();
	for (int i = 0; i < (int)Slice + 1; ++i)
	{
		vecIdx.push_back(i + 1);
	}

	pMesh = new AMesh;
	pMesh->Create(vecVtx.data(), vecVtx.size(), vecIdx.data(), vecIdx.size());
	AddAsset(L"CircleMesh_LineStrip", pMesh.Get());
}


void AssetMgr::CreateEngineShader()
{	
	Ptr<AGraphicShader> pShader = nullptr;

	// ===========
	// Std2DShader
	// ===========
	pShader = new AGraphicShader;
	pShader->CreateVertexShader(L"Shader\\std2d.fx", "VS_Std2D");
	pShader->CreatePixelShader(L"Shader\\std2d.fx", "PS_Std2D");
	pShader->SetRSType(RS_TYPE::CULL_NONE);

	pShader->AddShaderParam(SHADER_PARAM::VEC4, 0, L"TintColor");
	pShader->AddShaderParam(SHADER_PARAM::TEX, 0, L"OutColor");

	AddAsset(L"Std2DShader", pShader.Get());

	
	// ===============
	// BillboardShader
	// ===============
	pShader = new AGraphicShader;
	pShader->SetName(L"BillboardShader");
	pShader->CreateVertexShader(L"Shader\\billboard.fx", "VS_Billboard");
	pShader->CreatePixelShader(L"Shader\\billboard.fx", "PS_Billboard");
	pShader->SetBSType(BS_TYPE::DEFAULT);
	pShader->SetRSType(RS_TYPE::CULL_NONE);
	AssetMgr::GetInst()->AddAsset(pShader->GetName(), pShader.Get());


	// ============
	// SpriteShader
	// ============
	pShader = new AGraphicShader;
	pShader->SetName(L"SpriteShader");
	pShader->CreateVertexShader(L"Shader\\sprite.fx", "VS_Sprite");
	pShader->CreatePixelShader(L"Shader\\sprite.fx", "PS_Sprite");
	pShader->SetBSType(BS_TYPE::DEFAULT);
	pShader->SetRSType(RS_TYPE::CULL_NONE);

	AssetMgr::GetInst()->AddAsset(pShader->GetName(), pShader.Get());

	// ==============
	// FlipbookShader
	// ==============
	pShader = new AGraphicShader;
	pShader->SetName(L"FlipbookShader");
	pShader->CreateVertexShader(L"Shader\\flipbook.fx","VS_Flipbook");
	pShader->CreatePixelShader(L"Shader\\flipbook.fx", "PS_Flipbook");
	pShader->SetBSType(BS_TYPE::DEFAULT);
	pShader->SetRSType(RS_TYPE::CULL_NONE);
	AssetMgr::GetInst()->AddAsset(pShader->GetName(), pShader.Get());

	// =============
	// TileMapShader
	// =============
	// 찾는 쉐이더가 없으면 만들어서 에셋매니저에 등록해둔다

	pShader = new AGraphicShader;
	pShader->SetName(L"TileShader");
	pShader->CreateVertexShader(L"Shader\\tile.fx", "VS_Tile");
	pShader->CreatePixelShader(L"Shader\\tile.fx",  "PS_Tile");
	pShader->SetBSType(BS_TYPE::ALPHABLEND);
	pShader->SetRSType(RS_TYPE::CULL_NONE);
	AssetMgr::GetInst()->AddAsset(pShader->GetName(), pShader.Get());



	// ===============
	// DbgRenderShader
	// ===============
	pShader = new AGraphicShader;
	pShader->CreateVertexShader(L"Shader\\dbg.fx", "VS_Debug");
	pShader->CreatePixelShader(L"Shader\\dbg.fx", "PS_Debug");
	pShader->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	pShader->SetRSType(RS_TYPE::CULL_NONE);
	pShader->SetDSType(DS_TYPE::NO_TEST_NO_WRITE);
	pShader->SetBSType(BS_TYPE::DEFAULT);

	AddAsset(L"DbgShader", pShader.Get());
}

void AssetMgr::CreateEngineTexture()
{
	Load<ATexture>(L"SonicMap", L"Texture\\SonicMap.png");

	Load<ATexture>(L"PlayerImage", L"Texture\\Character.png");

	Load<ATexture>(L"Fighter", L"Texture\\Fighter.bmp");

	Load<ATexture>(L"Missile", L"Texture\\missile.png");

	Load<ATexture>(L"Link", L"Texture\\link.png");

	Load<ATexture>(L"TileAtlas", L"Texture\\TILE.bmp");

	Load<ATexture>(L"Sonic_TileAtlas", L"Texture\\Sonic_MapTile.png");

	Load<ATexture>(L"Sonic", L"Texture\\Sonic.png");

	Load<ATexture>(L"Item", L"Texture\\Item.png");

	Load<ATexture>(L"Enimy", L"Texture\\Enimy.png");

	Load<ATexture>(L"MapTest", L"Texture\\MapTest.png");
}

void AssetMgr::CreateEngineMaterial()
{
	Ptr<AMaterial> pMtrl = nullptr;

	// =========
	// BackGroundMtrl
	// =========
	pMtrl = new AMaterial;
	pMtrl->SetName(L"BackGroundMtrl");
	pMtrl->SetShader(Find<AGraphicShader>(L"Std2DShader"));

	// Parameter
	pMtrl->SetScalar(VEC4_0, Vec4(1.f, 1.f, 1.f, 1.f));
	pMtrl->SetTexture(TEX_0, Find<ATexture>(L"SonicMap"));

	pMtrl->SetDomain(RENDER_DOMAIN::DOMAIN_MASKED);
	AddAsset(pMtrl->GetName(), pMtrl.Get());

	// =========
	// Std2DMtrl
	// =========
	pMtrl = new AMaterial;
	pMtrl->SetName(L"Std2DMtrl");
	pMtrl->SetShader(Find<AGraphicShader>(L"Std2DShader"));

	// Parameter
	pMtrl->SetScalar(VEC4_0, Vec4(1.f, 1.f, 1.f, 1.f));
	pMtrl->SetTexture(TEX_0, Find<ATexture>(L"Fighter"));

	pMtrl->SetDomain(RENDER_DOMAIN::DOMAIN_MASKED);
	AddAsset(pMtrl->GetName(), pMtrl.Get());

	// ===========
	// MonsterMtrl
	// ===========
	pMtrl = new AMaterial;
	pMtrl->SetName(L"MonsterMtrl");
	pMtrl->SetShader(Find<AGraphicShader>(L"Std2DShader"));

	// Parameter
	pMtrl->SetScalar(VEC4_0, Vec4(1.f, 1.f, 1.f, 1.f));
	pMtrl->SetTexture(TEX_0, Find<ATexture>(L"PlayerImage"));

	pMtrl->SetDomain(RENDER_DOMAIN::DOMAIN_MASKED);
	AddAsset(pMtrl->GetName(), pMtrl.Get());


	// =======
	// DbgMtrl 
	// =======
	pMtrl = new AMaterial;
	pMtrl->SetName(L"DbgMtrl");
	pMtrl->SetShader(Find<AGraphicShader>(L"DbgShader"));
	pMtrl->SetDomain(RENDER_DOMAIN::DOMAIN_DEBUG);
	AddAsset(pMtrl->GetName(), pMtrl.Get());

	Load<AMaterial>(L"Material\\Default Material_0.mtrl", L"Material\\Default Material_0.mtrl");
}

void AssetMgr::CreateEngineSprite()
{			
	// =======
	// TileMap
	// =======
	//Ptr<ATileMap> pTileMap = nullptr;

	//pTileMap = new ATileMap;
	//pTileMap->SetName(L"TileMap\\TestTileMap.tile");
	//pTileMap->SetRowCol(20, 20);
	//pTileMap->SetTileSize(Vec2(64.f, 64.f));
	//pTileMap->SetAtlas(FIND(ATexture, L"TileAtlas"));

	//for (int i = 0; i < 20; ++i)
	//{
	//	for (int j = 0; j < 20; ++j)
	//	{
	//		pTileMap->SetSprite(i, j, LOAD(ASprite, L"Sprite\\TileSprite_1.sprite"));
	//	}		
	//}		

	//AddAsset(pTileMap->GetName(), pTileMap.Get());
	//pTileMap->Save(CONTENT_PATH + pTileMap->GetKey());


		Ptr<ATexture> pAtlas = FIND(ATexture, L"Sonic");
		float Width = pAtlas->GetWidth();
		float Height = pAtlas->GetHeight();
		Vec2 SlicePixel = Vec2(47.f, 47.f);

		Ptr<ASprite> pSprite = nullptr;
		Ptr<AFlipbook> pFlipbook = nullptr;

		// =========================
		// SonicRun_Basic
		// =========================
		pFlipbook = new AFlipbook;
		pFlipbook->SetName(L"SonicRun_Basic");

		for (int i = 0; i < 8; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"SonicRun_Basic_%d", i);

			pSprite = new ASprite;
			pSprite->SetName(Buff);
			pSprite->SetAtlas(pAtlas);
			pSprite->SetLeftTopUV(Vec2((664.f + (float)i * 52.f) / Width, 329.f / Height));
			pSprite->SetSliceUV(SlicePixel / Vec2(Width, Height));
			AddAsset(pSprite->GetName(), pSprite.Get());
			pSprite->Save(CONTENT_PATH + pSprite->GetKey());
		}

		for (int i = 0; i < 8; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"SonicRun_Basic_%d", i);
			pFlipbook->AddSprite(FIND(ASprite, Buff));
		}
		AddAsset(pFlipbook->GetName(), pFlipbook.Get());
		pFlipbook->Save(CONTENT_PATH + pFlipbook->GetKey());

		// =========================
		// Sonic_IDLE
		// =========================
		pFlipbook = new AFlipbook;
		pFlipbook->SetName(L"Sonic_IDLE");

		for (int i = 0; i < 1; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_IDLE_%d", i);

			pSprite = new ASprite;
			pSprite->SetName(Buff);
			pSprite->SetAtlas(pAtlas);
			pSprite->SetLeftTopUV(Vec2((24.f + (float)i * 52.f) / Width, 256.f / Height));
			pSprite->SetSliceUV(SlicePixel / Vec2(Width, Height));
			AddAsset(pSprite->GetName(), pSprite.Get());
			pSprite->Save(CONTENT_PATH + pSprite->GetKey());
		}

		for (int i = 0; i < 1; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_IDLE_%d", i);
			pFlipbook->AddSprite(FIND(ASprite, Buff));
		}
		AddAsset(pFlipbook->GetName(), pFlipbook.Get());
		pFlipbook->Save(CONTENT_PATH + pFlipbook->GetKey());

		// =========================
		// Sonic_IDLE_Long
		// =========================
		pFlipbook = new AFlipbook;
		pFlipbook->SetName(L"Sonic_IDLE_Long");

		for (int i = 0; i < 9; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_IDLE_Long_%d", i);

			pSprite = new ASprite;
			pSprite->SetName(Buff);
			pSprite->SetAtlas(pAtlas);
			pSprite->SetLeftTopUV(Vec2((24.f + (float)i * 52.f) / Width, 256.f / Height));
			pSprite->SetSliceUV(SlicePixel / Vec2(Width, Height));
			AddAsset(pSprite->GetName(), pSprite.Get());
			pSprite->Save(CONTENT_PATH + pSprite->GetKey());
		}

		for (int i = 0; i < 9; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_IDLE_Long_%d", i);
			pFlipbook->AddSprite(FIND(ASprite, Buff));
		}
		AddAsset(pFlipbook->GetName(), pFlipbook.Get());
		pFlipbook->Save(CONTENT_PATH + pFlipbook->GetKey());

		// =========================
		// Sonic_MaxSpeed
		// =========================
		pFlipbook = new AFlipbook;
		pFlipbook->SetName(L"Sonic_MaxSpeed");

		for (int i = 0; i < 4; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_MaxSpeed_%d", i);

			pSprite = new ASprite;
			pSprite->SetName(Buff);
			pSprite->SetAtlas(pAtlas);
			pSprite->SetLeftTopUV(Vec2((24.f + (float)i * 52.f) / Width, 402.f / Height));
			pSprite->SetSliceUV(SlicePixel / Vec2(Width, Height));
			AddAsset(pSprite->GetName(), pSprite.Get());
			pSprite->Save(CONTENT_PATH + pSprite->GetKey());
		}

		for (int i = 0; i < 4; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_MaxSpeed_%d", i);
			pFlipbook->AddSprite(FIND(ASprite, Buff));
		}
		AddAsset(pFlipbook->GetName(), pFlipbook.Get());
		pFlipbook->Save(CONTENT_PATH + pFlipbook->GetKey());

		// =========================
		// Sonic_Jump
		// =========================
		pFlipbook = new AFlipbook;
		pFlipbook->SetName(L"Sonic_Jump");

		for (int i = 0; i < 4; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_Jump_%d", i);

			pSprite = new ASprite;
			pSprite->SetName(Buff);
			pSprite->SetAtlas(pAtlas);
			pSprite->SetLeftTopUV(Vec2((248.f + (float)i * 52.f) / Width, 402.f / Height));
			pSprite->SetSliceUV(SlicePixel / Vec2(Width, Height));
			AddAsset(pSprite->GetName(), pSprite.Get());
			pSprite->Save(CONTENT_PATH + pSprite->GetKey());
		}

		for (int i = 0; i < 4; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_Jump_%d", i);
			pFlipbook->AddSprite(FIND(ASprite, Buff));
		}
		AddAsset(pFlipbook->GetName(), pFlipbook.Get());
		pFlipbook->Save(CONTENT_PATH + pFlipbook->GetKey());

		// =========================
		// Sonic_Up
		// =========================
		pFlipbook = new AFlipbook;
		pFlipbook->SetName(L"Sonic_Up");

		for (int i = 0; i < 2; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_Up_%d", i);

			pSprite = new ASprite;
			pSprite->SetName(Buff);
			pSprite->SetAtlas(pAtlas);
			pSprite->SetLeftTopUV(Vec2((504.f + (float)i * 52.f) / Width, 256.f / Height));
			pSprite->SetSliceUV(SlicePixel / Vec2(Width, Height));
			AddAsset(pSprite->GetName(), pSprite.Get());
			pSprite->Save(CONTENT_PATH + pSprite->GetKey());
		}

		for (int i = 0; i < 2; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_Up_%d", i);
			pFlipbook->AddSprite(FIND(ASprite, Buff));
		}
		AddAsset(pFlipbook->GetName(), pFlipbook.Get());
		pFlipbook->Save(CONTENT_PATH + pFlipbook->GetKey());

		// =========================
		// Sonic_Down
		// =========================
		pFlipbook = new AFlipbook;
		pFlipbook->SetName(L"Sonic_Down");

		for (int i = 0; i < 2; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_Down_%d", i);

			pSprite = new ASprite;
			pSprite->SetName(Buff);
			pSprite->SetAtlas(pAtlas);
			pSprite->SetLeftTopUV(Vec2((622.f + (float)i * 52.f) / Width, 256.f / Height));
			pSprite->SetSliceUV(SlicePixel / Vec2(Width, Height));
			AddAsset(pSprite->GetName(), pSprite.Get());
			pSprite->Save(CONTENT_PATH + pSprite->GetKey());
		}

		for (int i = 0; i < 2; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_Down_%d", i);
			pFlipbook->AddSprite(FIND(ASprite, Buff));
		}
		AddAsset(pFlipbook->GetName(), pFlipbook.Get());
		pFlipbook->Save(CONTENT_PATH + pFlipbook->GetKey());
		// =========================
		// Sonic_Break
		// =========================
		pFlipbook = new AFlipbook;
		pFlipbook->SetName(L"Sonic_Break");

		for (int i = 0; i < 4; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_Break_%d", i);

			pSprite = new ASprite;
			pSprite->SetName(Buff);
			pSprite->SetAtlas(pAtlas);
			pSprite->SetLeftTopUV(Vec2((412.f + (float)i * 52.f) / Width, 483.f / Height));
			pSprite->SetSliceUV(SlicePixel / Vec2(Width, Height));
			AddAsset(pSprite->GetName(), pSprite.Get());
			pSprite->Save(CONTENT_PATH + pSprite->GetKey());
		}

		for (int i = 0; i < 3; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_Break_%d", i);
			pFlipbook->AddSprite(FIND(ASprite, Buff));
		}
		AddAsset(pFlipbook->GetName(), pFlipbook.Get());
		pFlipbook->Save(CONTENT_PATH + pFlipbook->GetKey());

		// =========================
		// Sonic_Turn
		// =========================
		pFlipbook = new AFlipbook;
		pFlipbook->SetName(L"Sonic_Turn");

		for (int i = 0; i < 1; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_Turn_%d", i);

			pSprite = new ASprite;
			pSprite->SetName(Buff);
			pSprite->SetAtlas(pAtlas);
			pSprite->SetLeftTopUV(Vec2(568.f / Width, 483.f / Height));
			pSprite->SetSliceUV(SlicePixel / Vec2(Width, Height));
			AddAsset(pSprite->GetName(), pSprite.Get());
			pSprite->Save(CONTENT_PATH + pSprite->GetKey());
		}

		for (int i = 0; i < 1; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_Turn_%d", i);
			pFlipbook->AddSprite(FIND(ASprite, Buff));
		}
		AddAsset(pFlipbook->GetName(), pFlipbook.Get());
		pFlipbook->Save(CONTENT_PATH + pFlipbook->GetKey());

		// =========================
		// Sonic_Skill
		// =========================
		pFlipbook = new AFlipbook;
		pFlipbook->SetName(L"Sonic_Skill");

		for (int i = 0; i < 6; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_Skill_%d", i);

			pSprite = new ASprite;
			pSprite->SetName(Buff);
			pSprite->SetAtlas(pAtlas);
			pSprite->SetLeftTopUV(Vec2((24.f + (float)i * 52.f) / Width, 487.f / Height));
			pSprite->SetSliceUV(SlicePixel / Vec2(Width, Height));
			AddAsset(pSprite->GetName(), pSprite.Get());
			pSprite->Save(CONTENT_PATH + pSprite->GetKey());
		}

		for (int i = 0; i < 6; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_Skill_%d", i);
			pFlipbook->AddSprite(FIND(ASprite, Buff));
		}
		AddAsset(pFlipbook->GetName(), pFlipbook.Get());
		pFlipbook->Save(CONTENT_PATH + pFlipbook->GetKey());
		// =========================
		// Sonic_Spring_Jump
		// =========================
		pFlipbook = new AFlipbook;
		pFlipbook->SetName(L"Sonic_Spring_Jump");

		for (int i = 0; i < 1; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_Spring_Jump_%d", i);

			pSprite = new ASprite;
			pSprite->SetName(Buff);
			pSprite->SetAtlas(pAtlas);
			pSprite->SetLeftTopUV(Vec2(636.f / Width, 483.f / Height));
			pSprite->SetSliceUV(Vec2(47.f, 51.f) / Vec2(Width, Height));
			AddAsset(pSprite->GetName(), pSprite.Get());
			pSprite->Save(CONTENT_PATH + pSprite->GetKey());
		}

		for (int i = 0; i < 1; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Sonic_Spring_Jump_%d", i);
			pFlipbook->AddSprite(FIND(ASprite, Buff));
		}
		AddAsset(pFlipbook->GetName(), pFlipbook.Get());
		pFlipbook->Save(CONTENT_PATH + pFlipbook->GetKey());

		// =========================
		// Spring
		// =========================
		Ptr<ATexture> pAtlas_Spring = FIND(ATexture, L"Item");
		Width = pAtlas_Spring->GetWidth();
		Height = pAtlas_Spring->GetHeight();
		SlicePixel = Vec2(31.f, 31.f);

		pFlipbook = new AFlipbook;
		pFlipbook->SetName(L"Spring");

		for (int i = 0; i < 5; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Spring_%d", i);

			pSprite = new ASprite;
			pSprite->SetName(Buff);
			pSprite->SetAtlas(pAtlas_Spring);
			pSprite->SetLeftTopUV(Vec2((246.f + (float)i * 33.f) / Width, 352.f / Height));
			pSprite->SetSliceUV(SlicePixel / Vec2(Width, Height));
			AddAsset(pSprite->GetName(), pSprite.Get());
			pSprite->Save(CONTENT_PATH + pSprite->GetKey());
		}

		for (int i = 0; i < 5; ++i)
		{
			wchar_t Buff[50] = {};
			swprintf_s(Buff, L"Spring_%d", i);
			pFlipbook->AddSprite(FIND(ASprite, Buff));
		}
		AddAsset(pFlipbook->GetName(), pFlipbook.Get());
		pFlipbook->Save(CONTENT_PATH + pFlipbook->GetKey());

		// =========================
		// Enimy
		// =========================
		Ptr<ATexture> pAtlas1 = FIND(ATexture, L"Enimy");
		Width = pAtlas1->GetWidth();
		Height = pAtlas1->GetHeight();

		struct tFrameInfo
		{
			Vec2 Pos;
			Vec2 Size;
			Vec2 Offset;
		};

		vector<tFrameInfo> vecFrameInfo;

		// Enimy_Turn
		vecFrameInfo.push_back({ Vec2(228.f, 343.f), Vec2(47.f, 31.f), Vec2(0.f, 0.f) });
		vecFrameInfo.push_back({ Vec2(280.f, 343.f), Vec2(39.f, 31.f), Vec2(0.f, 0.f) });
		vecFrameInfo.push_back({ Vec2(324.f, 343.f), Vec2(24.f, 31.f), Vec2(0.f, 0.f) });
		vecFrameInfo.push_back({ Vec2(353.f, 343.f), Vec2(39.f, 31.f), Vec2(0.f, 0.f) });
		vecFrameInfo.push_back({ Vec2(397.f, 343.f), Vec2(31.f, 31.f), Vec2(0.f, 0.f) });
		vecFrameInfo.push_back({ Vec2(433.f, 343.f), Vec2(39.f, 31.f), Vec2(0.f, 0.f) });
		vecFrameInfo.push_back({ Vec2(477.f, 343.f), Vec2(39.f, 31.f), Vec2(0.f, 0.f) });
		vecFrameInfo.push_back({ Vec2(521.f, 343.f), Vec2(39.f, 31.f), Vec2(0.f, 0.f) });

		pFlipbook = new AFlipbook;
		pFlipbook->SetName(L"Enimy_Turn");

		for (size_t i = 0; i < vecFrameInfo.size(); ++i)
		{
			wchar_t szName[50] = {};
			swprintf_s(szName, L"Enimy_Turn_%d", (int)i);

			Ptr<ASprite> pEnemySprite = new ASprite;
			pEnemySprite->SetName(szName);
			pEnemySprite->SetAtlas(pAtlas1);

			float leftX = vecFrameInfo[i].Pos.x / Width;
			float topY = vecFrameInfo[i].Pos.y / Height;
			float sizeX = vecFrameInfo[i].Size.x / Width;
			float sizeY = vecFrameInfo[i].Size.y / Height;

			pEnemySprite->SetLeftTopUV(Vec2(leftX, topY));
			pEnemySprite->SetSliceUV(Vec2(sizeX, sizeY));

			AddAsset(pEnemySprite->GetName(), pEnemySprite.Get());
			pEnemySprite->Save(CONTENT_PATH + pEnemySprite->GetKey());
			pFlipbook->AddSprite(pEnemySprite);

		}
		AddAsset(pFlipbook->GetName(), pFlipbook.Get());
		pFlipbook->Save(CONTENT_PATH + pFlipbook->GetKey());

		// Enimy_Run
		vecFrameInfo.clear();
		vecFrameInfo.push_back({ Vec2(228.f, 379.f), Vec2(55.f, 31.f), Vec2(0.f, 0.f) });
		vecFrameInfo.push_back({ Vec2(288.f, 379.f), Vec2(47.f, 31.f), Vec2(0.f, 0.f) });
		vecFrameInfo.push_back({ Vec2(340.f, 379.f), Vec2(55.f, 31.f), Vec2(0.f, 0.f) });
		vecFrameInfo.push_back({ Vec2(400.f, 379.f), Vec2(55.f, 31.f), Vec2(0.f, 0.f) });

		pFlipbook = new AFlipbook;
		pFlipbook->SetName(L"Enimy_Run");

		for (size_t i = 0; i < vecFrameInfo.size(); ++i)
		{
			wchar_t szName[50] = {};
			swprintf_s(szName, L"Enimy_Run_%d", (int)i);

			Ptr<ASprite> pEnemySprite = new ASprite;
			pEnemySprite->SetName(szName);
			pEnemySprite->SetAtlas(pAtlas1);

			float leftX = vecFrameInfo[i].Pos.x / Width;
			float topY = vecFrameInfo[i].Pos.y / Height;
			float sizeX = vecFrameInfo[i].Size.x / Width;
			float sizeY = vecFrameInfo[i].Size.y / Height;

			pEnemySprite->SetLeftTopUV(Vec2(leftX, topY));
			pEnemySprite->SetSliceUV(Vec2(sizeX, sizeY));

			AddAsset(pEnemySprite->GetName(), pEnemySprite.Get());
			pEnemySprite->Save(CONTENT_PATH + pEnemySprite->GetKey());
			pFlipbook->AddSprite(pEnemySprite);
		}
		AddAsset(pFlipbook->GetName(), pFlipbook.Get());
		pFlipbook->Save(CONTENT_PATH + pFlipbook->GetKey());

		// =========================
		// Tile Sprite
		// =========================
		pAtlas = FIND(ATexture, L"TileAtlas");
		Width = pAtlas->GetWidth();
		Height = pAtlas->GetHeight();
		SlicePixel = Vec2(64.f, 64.f);

		int Count = 0;
		for (int i = 0; i < 6; ++i)
		{
			for (int j = 0; j < 8; ++j, ++Count)
			{
				wchar_t Buff[50] = {};
				swprintf_s(Buff, L"TileSprite_%d", Count);

				pSprite = new ASprite;
				pSprite->SetName(Buff);
				pSprite->SetAtlas(pAtlas);
				pSprite->SetLeftTopUV(Vec2((SlicePixel.x / Width) * (float)j, (SlicePixel.y / Height) * i));
				pSprite->SetSliceUV(SlicePixel / Vec2(Width, Height));
				AddAsset(pSprite->GetName(), pSprite.Get());
				pSprite->Save(CONTENT_PATH + pSprite->GetKey());
			}
		}

		// =========================
		// TileMap
		// =========================

		int tempMap[6][25] = {
			{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
			{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
			{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
			{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 8, 9, 1, 1, 1, 8, 9},
			{7, 7, 7, 7, 2, 3, 4, 4, 4, 4, 5, 6, 7, 7, 2, 3, 4, 4, 10, 11, 4, 4, 4, 10, 11},
			{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		};

		Ptr<ATileMap> pTileMap = Find<ATileMap>(L"TestTileMap");
		if (nullptr == pTileMap)
		{
			pTileMap = new ATileMap;
			pTileMap->SetName(L"TestTileMap");

			pTileMap->SetRowCol(6, 25);
			pTileMap->SetTileSize(Vec2(378.f, 378.f));

			for (int i = 0; i < 6; ++i)
			{
				for (int j = 0; j < 25; ++j)
				{
					pTileMap->SetTileType(i, j, (UINT)tempMap[i][j]);
				}
			}

			AddAsset(pTileMap->GetName(), pTileMap.Get());
			// pTileMap->Save(CONTENT_PATH + pTileMap->GetKey());
		}
}

void AssetMgr::CreateEnginePrefab()
{
	CreateDirectoryW((wstring(CONTENT_PATH) + L"Prefab").c_str(), nullptr);

	Ptr<GameObject> pObject = new GameObject;
	pObject->SetName(L"Missile");

	pObject->AddComponent(new CTransform);
	pObject->AddComponent(new CMeshRender);
	pObject->AddComponent(new CCollider2D);
	pObject->AddComponent(new CMissileScript);

	pObject->Transform()->SetRelativeScale(Vec3(10.f, 30.f, 1.f));
	pObject->MeshRender()->SetMesh(FIND(AMesh, L"RectMesh"));
	pObject->MeshRender()->SetMaterial(FIND(AMaterial, L"Std2DMtrl"));

	Ptr<APrefab> pMissilePrefab = new APrefab;
	pMissilePrefab->SetObject(pObject);
	AddAsset(L"Prefab\\Missile.pref", pMissilePrefab.Get());

	wstring FilePath = wstring(CONTENT_PATH) + L"Prefab\\Missile.pref";
	pMissilePrefab->Save(FilePath);

	LOAD(APrefab, L"Prefab\\Missile.pref");
}
