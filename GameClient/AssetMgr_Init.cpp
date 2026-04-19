#include "pch.h"
#include "AssetMgr.h"

#include "PathMgr.h"
#include "Source/Scripts/CMissileScript.h"
#include "Source/Scripts/CBlockScript.h"
#include "Source/Scripts/CBlockMovingScript.h"
#include "Source/Scripts/CBlockPushingScript.h"
#include "Source/Scripts/CBackScript.h"
#include "Source/Scripts/CDestroyBlockScript.h"
#include "Source/Scripts/CKnockbackScript.h"
#include "Source/Scripts/CSpikeScript.h"
#include "Source/Scripts/CCoinScript.h"
#include "Source/Scripts/CItemScript.h"

void AssetMgr::Init()
{
	CreateEngineMesh();

	CreateEngineShader();

	CreateEngineTexture();

	CreateEngineMaterial();

	CreateEngineSprite();

	CreateEnginePrefab();
}

void AssetMgr::CreateEngineMesh()
{
	Ptr<AMesh> pMesh = nullptr;

	// =========
	// PointMesh
	// =========
	{
		Vtx pointVtx = {};
		pointVtx.vPos = Vec3(0.f, 0.f, 0.f);
		pointVtx.vUV = Vec2(0.f, 0.f);
		pointVtx.vColor = Vec4(1.f, 1.f, 1.f, 1.f);
		UINT pointIdx = 0;

		pMesh = new AMesh;
		pMesh->Create(&pointVtx, 1, &pointIdx, 1);
		AddAsset(L"PointMesh", pMesh.Get());
	}

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

	// Shader param metadata is editor-only. Skip registration here to avoid
	// startup instability from dynamic param list construction.

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

	// ==============
	// HitFlashShader
	// ==============
	pShader = new AGraphicShader;
	pShader->SetName(L"HitFlashShader");
	pShader->CreateVertexShader(L"Shader\\hitflash.fx", "VS_HitFlash");
	pShader->CreatePixelShader(L"Shader\\hitflash.fx", "PS_HitFlash");
	pShader->SetBSType(BS_TYPE::ALPHABLEND);
	pShader->SetRSType(RS_TYPE::CULL_NONE);
	pShader->SetDSType(DS_TYPE::NO_TEST_NO_WRITE);
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

	Load<ATexture>(L"SonicMapBillboard", L"Texture\\Sonic_Background_Billboard.png");

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

	Load<ATexture>(L"Object", L"Texture\\Sonic_Object.png");

	Load<ATexture>(L"Object2", L"Texture\\Sonic_Object2.png");

	Load<ATexture>(L"BOSS", L"Texture\\Boss.png");

	Load<ATexture>(L"Opening", L"Texture\\Opening.png");

	Load<ATexture>(L"Coin", L"Texture\\Sonic_Ring.png");
	Load<ATexture>(L"Monitor", L"Texture\\Monitor.png");
	Load<ATexture>(L"Number", L"Texture\\Number.png");
	Load<ATexture>(L"UI_Start", L"Texture\\UI_Start.png");
	Load<ATexture>(L"UI_General", L"Texture\\UI_General.png");
	Load<ATexture>(L"UI_Ending", L"Texture\\UI_ENDING.png");
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
	// BackGroundMtrl
	// =========
	pMtrl = new AMaterial;
	pMtrl->SetName(L"BackGroundMtrl_Billboard");
	pMtrl->SetShader(Find<AGraphicShader>(L"Std2DShader"));

	// Parameter

	pMtrl->SetScalar(VEC4_0, Vec4(1.f, 1.f, 1.f, 1.f));
	pMtrl->SetTexture(TEX_0, Find<ATexture>(L"SonicMapBillboard"));

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

	// ================
	// BossHitFlashMtrl
	// ================
	pMtrl = new AMaterial;
	pMtrl->SetName(L"BossHitFlashMtrl");
	pMtrl->SetShader(Find<AGraphicShader>(L"HitFlashShader"));
	pMtrl->SetScalar(VEC4_0, Vec4(1.f, 1.f, 1.f, 0.f));
	pMtrl->SetDomain(RENDER_DOMAIN::DOMAIN_TRANSPARENT);
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
		// The engine used to bootstrap a hard-coded tempMap here. That made the
		// runtime always start from a baked sample map even when the editor has
		// moved to surface-based authoring. Keep only an empty TileMap asset so
		// legacy code paths can still resolve TestTileMap without forcing sample
		// content into the level.
		Ptr<ATileMap> pTileMap = new ATileMap;
		pTileMap->SetName(L"TestTileMap");
		pTileMap->SetRowCol(6, 25);
		pTileMap->SetTileSize(Vec2(378.f, 378.f));
		pTileMap->SetAtlas(FIND(ATexture, L"MapTest"));

		Ptr<ASprite> pEmptySprite = AssetMgr::GetInst()->Load<ASprite>(L"MapTest_0", L"Sprite\\MapTest_0.sprite");
		if (nullptr != pEmptySprite)
		{
			for (int i = 0; i < 6; ++i)
			{
				for (int j = 0; j < 25; ++j)
				{
					pTileMap->SetSprite(i, j, pEmptySprite);
				}
			}
		}

		AddAsset(pTileMap->GetName(), pTileMap.Get());
		// pTileMap->Save(CONTENT_PATH + pTileMap->GetKey());

		Ptr<ATexture> pMonitorAtlas = FIND(ATexture, L"Monitor");
		if (nullptr != pMonitorAtlas)
		{
			const float monitorWidth = pMonitorAtlas->GetWidth();
			const float monitorHeight = pMonitorAtlas->GetHeight();

			auto SaveMonitorSprite = [this, pMonitorAtlas, monitorWidth, monitorHeight](const wchar_t* _Name, const Vec2& _LeftTopPx, const Vec2& _SizePx)
			{
				Ptr<ASprite> pMonitorSprite = new ASprite;
				pMonitorSprite->SetName(_Name);
				pMonitorSprite->SetAtlas(pMonitorAtlas);
				pMonitorSprite->SetLeftTopUV(Vec2(_LeftTopPx.x / monitorWidth, _LeftTopPx.y / monitorHeight));
				pMonitorSprite->SetSliceUV(Vec2(_SizePx.x / monitorWidth, _SizePx.y / monitorHeight));
				AddAsset(pMonitorSprite->GetName(), pMonitorSprite.Get());
				pMonitorSprite->Save(CONTENT_PATH + pMonitorSprite->GetKey());
			};

			SaveMonitorSprite(L"Sprite\\ItemBox_Base.sprite", Vec2(26.f, 747.f), Vec2(29.f, 38.f));
			SaveMonitorSprite(L"Sprite\\ItemBox_Lid.sprite", Vec2(26.f, 699.f), Vec2(29.f, 14.f));
			SaveMonitorSprite(L"Sprite\\ItemBox_Dead.sprite", Vec2(56.f, 747.f), Vec2(29.f, 38.f));
			SaveMonitorSprite(L"Sprite\\ItemBox_0.sprite", Vec2(104.f, 698.f), Vec2(16.f, 16.f));
			SaveMonitorSprite(L"Sprite\\ItemBox_1.sprite", Vec2(128.f, 698.f), Vec2(16.f, 16.f));
			SaveMonitorSprite(L"Sprite\\ItemBox_2.sprite", Vec2(152.f, 698.f), Vec2(16.f, 16.f));
			SaveMonitorSprite(L"Sprite\\ItemBox_3.sprite", Vec2(176.f, 698.f), Vec2(16.f, 16.f));
			SaveMonitorSprite(L"Sprite\\ItemBox_4.sprite", Vec2(200.f, 698.f), Vec2(16.f, 16.f));
			SaveMonitorSprite(L"Sprite\\ItemBox_5.sprite", Vec2(224.f, 698.f), Vec2(16.f, 16.f));
		}

		auto SaveFullTextureSprite = [this](const wchar_t* _SpritePath, const wchar_t* _TextureKey)
		{
			Ptr<ATexture> pTexture = FIND(ATexture, _TextureKey);
			if (nullptr == pTexture)
				return;

			Ptr<ASprite> pSprite = new ASprite;
			pSprite->SetName(_SpritePath);
			pSprite->SetAtlas(pTexture);
			pSprite->SetLeftTopUV(Vec2(0.f, 0.f));
			pSprite->SetSliceUV(Vec2(1.f, 1.f));
			AddAsset(pSprite->GetName(), pSprite.Get());
			pSprite->Save(CONTENT_PATH + pSprite->GetKey());
		};

		auto SaveFlipbookFromSprites = [this](const wchar_t* _FlipbookPath, std::initializer_list<const wchar_t*> _SpritePaths)
		{
			Ptr<AFlipbook> pFlipbook = new AFlipbook;
			pFlipbook->SetName(_FlipbookPath);

			for (const wchar_t* pSpritePath : _SpritePaths)
			{
				Ptr<ASprite> pSprite = LOAD(ASprite, pSpritePath);
				if (nullptr != pSprite)
					pFlipbook->AddSprite(pSprite);
			}

			if (0 == pFlipbook->GetSpriteCount())
				return;

			AddAsset(pFlipbook->GetName(), pFlipbook.Get());
			pFlipbook->Save(CONTENT_PATH + pFlipbook->GetKey());
		};

        auto SaveFlipbookFromSpriteKeys = [this](const wchar_t* _FlipbookPath, const vector<wstring>& _SpritePaths)
        {
            Ptr<AFlipbook> pFlipbook = new AFlipbook;
            pFlipbook->SetName(_FlipbookPath);

            for (size_t i = 0; i < _SpritePaths.size(); ++i)
            {
                Ptr<ASprite> pSprite = LOAD(ASprite, _SpritePaths[i]);
                if (nullptr != pSprite)
                    pFlipbook->AddSprite(pSprite);
            }

            if (0 == pFlipbook->GetSpriteCount())
                return;

            AddAsset(pFlipbook->GetName(), pFlipbook.Get());
            pFlipbook->Save(CONTENT_PATH + pFlipbook->GetKey());
        };

        auto SaveAtlasSprite = [this](Ptr<ATexture> _Atlas
            , const wchar_t* _SpritePath
            , const Vec2& _LeftTopPx
            , const Vec2& _SizePx)
        {
            if (nullptr == _Atlas)
                return;

            const float atlasWidth = max(1.f, _Atlas->GetWidth());
            const float atlasHeight = max(1.f, _Atlas->GetHeight());

            Ptr<ASprite> pSprite = new ASprite;
            pSprite->SetName(_SpritePath);
            pSprite->SetAtlas(_Atlas);
            pSprite->SetLeftTopUV(Vec2(_LeftTopPx.x / atlasWidth, _LeftTopPx.y / atlasHeight));
            pSprite->SetSliceUV(Vec2(_SizePx.x / atlasWidth, _SizePx.y / atlasHeight));
            pSprite->SetBackgroundUV(Vec2(_SizePx.x / atlasWidth, _SizePx.y / atlasHeight));
            pSprite->SetOffsetUV(Vec2(0.f, 0.f));
            AddAsset(pSprite->GetName(), pSprite.Get());
            pSprite->Save(CONTENT_PATH + pSprite->GetKey());
        };

        auto SaveAtlasSpriteSequence = [&](const wchar_t* _SpritePrefix
            , const wchar_t* _FlipbookPath
            , Ptr<ATexture> _Atlas
            , const Vec2& _StartPx
            , const Vec2& _FrameSizePx
            , const Vec2& _StepPx
            , int _FrameCount)
        {
            vector<wstring> spritePaths;
            spritePaths.reserve(_FrameCount);

            for (int i = 0; i < _FrameCount; ++i)
            {
                const Vec2 leftTopPx = Vec2(_StartPx.x + (_StepPx.x * (float)i)
                    , _StartPx.y + (_StepPx.y * (float)i));

                wchar_t spritePath[128] = {};
                swprintf_s(spritePath, L"Sprite\\%ls_frame_%d.sprite", _SpritePrefix, i);
                SaveAtlasSprite(_Atlas, spritePath, leftTopPx, _FrameSizePx);
                spritePaths.push_back(spritePath);
            }

            SaveFlipbookFromSpriteKeys(_FlipbookPath, spritePaths);
        };

        Ptr<ATexture> pItemAtlas = FIND(ATexture, L"Item");
        if (nullptr != pItemAtlas)
        {
            const Vec2 shieldFrameSize = Vec2(47.f, 47.f);
            SaveAtlasSpriteSequence(L"FireShield"
                , L"Flipbook\\FireShield.flip"
                , pItemAtlas
                , Vec2(512.f, 605.f)
                , shieldFrameSize
                , Vec2(47.f, 0.f)
                , 8);
            SaveAtlasSpriteSequence(L"ElectricShield"
                , L"Flipbook\\ElectricShield.flip"
                , pItemAtlas
                , Vec2(807.f, 813.f)
                , shieldFrameSize
                , Vec2(47.f, 0.f)
                , 8);
            SaveAtlasSpriteSequence(L"WaterShield"
                , L"Flipbook\\WaterShield.flip"
                , pItemAtlas
                , Vec2(512.f, 939.f)
                , shieldFrameSize
                , Vec2(47.f, 0.f)
                , 8);

            // Attack strips are rebuilt as separate assets so later runtime hooks
            // can swap in the matching motion without relying on the broken restore set.
            SaveAtlasSpriteSequence(L"FireShieldAttack"
                , L"Flipbook\\FireShieldAttack.flip"
                , pItemAtlas
                , Vec2(20.f, 705.f)
                , shieldFrameSize
                , Vec2(47.f, 0.f)
                , 8);
            SaveAtlasSpriteSequence(L"ElectricShieldAttack"
                , L"Flipbook\\ElectricShieldAttack.flip"
                , pItemAtlas
                , Vec2(40.f, 913.f)
                , shieldFrameSize
                , Vec2(36.f, 0.f)
                , 25);
            SaveAtlasSpriteSequence(L"WaterShieldAttack"
                , L"Flipbook\\WaterShieldAttack.flip"
                , pItemAtlas
                , Vec2(225.f, 1039.f)
                , shieldFrameSize
                , Vec2(47.f, 0.f)
                , 8);
        }

		SaveFullTextureSprite(L"Sprite\\Stage_Start_0.sprite", L"UI_Start");
		SaveFullTextureSprite(L"Sprite\\Stage_General.sprite", L"UI_General");
		SaveFullTextureSprite(L"Sprite\\Stage_Start_1.sprite", L"UI_General");
		SaveFullTextureSprite(L"Sprite\\Stage_End_0.sprite", L"UI_Ending");
		SaveFlipbookFromSprites(L"Flipbook\\Dead.flip",
		{
			L"Sprite\\Sonic_Hurt.sprite",
			L"Sprite\\Sonic_Hurt_1.sprite",
		});
	}

void AssetMgr::CreateEnginePrefab()
{
	CreateDirectoryW((wstring(CONTENT_PATH) + L"Prefab").c_str(), nullptr);

	auto SaveMeshPrefabAsset = [&](const wchar_t* _AssetKey, const wchar_t* _ObjectName, const Vec3& _Scale, CScript* _Script)
	{
		Ptr<GameObject> pPrefabObject = new GameObject;
		pPrefabObject->SetName(_ObjectName);

		pPrefabObject->AddComponent(new CTransform);
		pPrefabObject->AddComponent(new CMeshRender);
		pPrefabObject->AddComponent(new CCollider2D);
		if (nullptr != _Script)
			pPrefabObject->AddComponent(_Script);

		pPrefabObject->Transform()->SetRelativeScale(_Scale);
		pPrefabObject->MeshRender()->SetMesh(FIND(AMesh, L"RectMesh"));
		pPrefabObject->MeshRender()->SetMaterial(FIND(AMaterial, L"Std2DMtrl"));

		Ptr<APrefab> pPrefab = new APrefab;
		pPrefab->SetObject(pPrefabObject);
		AddAsset(_AssetKey, pPrefab.Get());
		pPrefab->SetRelativePath(_AssetKey);
		pPrefab->Save(wstring(CONTENT_PATH) + _AssetKey);
	};

	auto SaveSpritePrefabAsset = [&](const wchar_t* _AssetKey, const wchar_t* _ObjectName, const wchar_t* _SpritePath, const Vec3& _Scale, CScript* _Script)
	{
		Ptr<GameObject> pPrefabObject = new GameObject;
		pPrefabObject->SetName(_ObjectName);

		pPrefabObject->AddComponent(new CTransform);
		pPrefabObject->AddComponent(new CSpriteRender);
		pPrefabObject->AddComponent(new CCollider2D);
		if (nullptr != _Script)
			pPrefabObject->AddComponent(_Script);

		pPrefabObject->Transform()->SetRelativeScale(_Scale);
		pPrefabObject->SpriteRender()->SetSprite(LOAD(ASprite, _SpritePath));

		Ptr<APrefab> pPrefab = new APrefab;
		pPrefab->SetObject(pPrefabObject);
		AddAsset(_AssetKey, pPrefab.Get());
		pPrefab->SetRelativePath(_AssetKey);
		pPrefab->Save(wstring(CONTENT_PATH) + _AssetKey);
	};

	auto SaveFlipbookPrefabAsset = [&](const wchar_t* _AssetKey, const wchar_t* _ObjectName, const wchar_t* _FlipbookPath, const Vec3& _Scale, CScript* _Script)
	{
		Ptr<GameObject> pPrefabObject = new GameObject;
		pPrefabObject->SetName(_ObjectName);

		pPrefabObject->AddComponent(new CTransform);
		pPrefabObject->AddComponent(new CFlipbookRender);
		pPrefabObject->AddComponent(new CCollider2D);

		if (nullptr != _Script)
			pPrefabObject->AddComponent(_Script);

		pPrefabObject->Transform()->SetRelativeScale(_Scale);
		pPrefabObject->FlipbookRender()->AddFlipbook(LOAD(AFlipbook, _FlipbookPath));

		Ptr<APrefab> pPrefab = new APrefab;
		pPrefab->SetObject(pPrefabObject);
		AddAsset(_AssetKey, pPrefab.Get());
		pPrefab->SetRelativePath(_AssetKey);
		pPrefab->Save(wstring(CONTENT_PATH) + _AssetKey);
	};

	auto SaveItemBoxPrefabAsset = [&](const wchar_t* _AssetKey, const wchar_t* _ObjectName, CItemScript::ITEMBOX _Type)
	{
		Ptr<GameObject> pPrefabObject = new GameObject;
		pPrefabObject->SetName(_ObjectName);

		pPrefabObject->AddComponent(new CTransform);
		pPrefabObject->AddComponent(new CSpriteRender);
		pPrefabObject->AddComponent(new CCollider2D);
		pPrefabObject->AddComponent(new CBackScript);

		Ptr<CItemScript> pItemScript = new CItemScript;
		pPrefabObject->AddComponent(pItemScript.Get());
		pPrefabObject->Transform()->SetRelativeScale(Vec3(58.f, 76.f, 1.f));
		pItemScript->SetBoxType(_Type);
		pItemScript->ApplyEditorBoxSetup();

		Ptr<APrefab> pPrefab = new APrefab;
		pPrefab->SetObject(pPrefabObject);
		AddAsset(_AssetKey, pPrefab.Get());
		pPrefab->SetRelativePath(_AssetKey);
		pPrefab->Save(wstring(CONTENT_PATH) + _AssetKey);
	};

	auto BinaryContainsWideText = [&](const wstring& _FilePath, const wchar_t* _Text)
	{
		FILE* pFile = nullptr;
		_wfopen_s(&pFile, _FilePath.c_str(), L"rb");
		if (nullptr == pFile)
			return false;

		fseek(pFile, 0, SEEK_END);
		const long fileSize = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);

		if (fileSize <= 0)
		{
			fclose(pFile);
			return false;
		}

		vector<unsigned char> fileData((size_t)fileSize);
		fread(fileData.data(), 1, (size_t)fileSize, pFile);
		fclose(pFile);

		const unsigned char* textBytes = reinterpret_cast<const unsigned char*>(_Text);
		const size_t textByteCount = wcslen(_Text) * sizeof(wchar_t);
		if (0 == textByteCount || fileData.size() < textByteCount)
			return false;

		for (size_t i = 0; i + textByteCount <= fileData.size(); ++i)
		{
			if (0 == memcmp(fileData.data() + i, textBytes, textByteCount))
				return true;
		}

		return false;
	};

	SaveMeshPrefabAsset(L"Prefab\\Missile.pref", L"Missile", Vec3(10.f, 30.f, 1.f), new CMissileScript);
	SaveSpritePrefabAsset(L"Prefab\\Block_Mid.pref", L"Block_Mid", L"Sprite\\Block_Mid.sprite", Vec3(100.f, 100.f, 1.f), new CBlockPushingScript);
	SaveSpritePrefabAsset(L"Prefab\\Block_Tall.pref", L"Block_Tall", L"Sprite\\Block_Tall.sprite", Vec3(100.f, 150, 1.f), new CBlockScript);
	SaveSpritePrefabAsset(L"Prefab\\Block_Small.pref", L"Block_Small", L"Sprite\\Block_Small.sprite", Vec3(100, 70, 1.f), new CBlockScript);
	SaveSpritePrefabAsset(L"Prefab\\Block_Move.pref", L"Block_Move", L"Sprite\\Block_Move.sprite", Vec3(150.f, 100.f, 1.f), new CBlockMovingScript);
	SaveSpritePrefabAsset(L"Prefab\\Spike.pref", L"Spike", L"Sprite\\Spike.sprite", Vec3(150.f, 150.f, 1.f), new CSpikeScript);
	SaveItemBoxPrefabAsset(L"Prefab\\ITEMBOX_LIFE.pref", L"ITEMBOX_LIFE", CItemScript::ITEMBOX::UPLIFEBOX);
	SaveItemBoxPrefabAsset(L"Prefab\\ITEMBOX_ELECTRIC.pref", L"ITEMBOX_ELECTRIC", CItemScript::ITEMBOX::ELECTIRCBOX);
	SaveItemBoxPrefabAsset(L"Prefab\\ITEMBOX_FIRE.pref", L"ITEMBOX_FIRE", CItemScript::ITEMBOX::FIREBOX);
	SaveItemBoxPrefabAsset(L"Prefab\\ITEMBOX_FRIE.pref", L"ITEMBOX_FRIE", CItemScript::ITEMBOX::FIREBOX);
	SaveItemBoxPrefabAsset(L"Prefab\\ITEMBOX_WATER.pref", L"ITEMBOX_WATER", CItemScript::ITEMBOX::WATERBOX);
	SaveItemBoxPrefabAsset(L"Prefab\\ITEMBOX_STAR.pref", L"ITEMBOX_STAR", CItemScript::ITEMBOX::STARBOX);
	SaveItemBoxPrefabAsset(L"Prefab\\ITEMBOX_COIN.pref", L"ITEMBOX_COIN", CItemScript::ITEMBOX::COINBOX);
	SaveSpritePrefabAsset(L"Prefab\\ITEMBOX_DEAD.pref", L"ITEMBOX_DEAD", L"Sprite\\ItemBox_Dead.sprite", Vec3(58.f, 76.f, 1.f), nullptr);

	const wstring coinPrefabPath = wstring(CONTENT_PATH) + L"Prefab\\Coin.pref";
	const bool coinPrefabMissing = (GetFileAttributesW(coinPrefabPath.c_str()) == INVALID_FILE_ATTRIBUTES);
	const bool coinPrefabSavedWithBrokenSpritePath =
		!coinPrefabMissing
		&& BinaryContainsWideText(coinPrefabPath, L"SpriteMtrl")
		&& BinaryContainsWideText(coinPrefabPath, L"Flipbook\\CoinTurn.flip");

	if (coinPrefabMissing || coinPrefabSavedWithBrokenSpritePath)
	{
		SaveFlipbookPrefabAsset(L"Prefab\\Coin.pref", L"Coin", L"Flipbook\\CoinTurn.flip", Vec3(100.f, 100.f, 1.f), new CCoinScript);
	}
}
