#include "pch.h"

#include "RenderMgr.h"
#include "TaskMgr.h"
#include "LevelMgr.h"
#include "Source/Scripts/COpeningScript.h"

void CreateObject(GameObject* _Object, int LayerIdx)
{
    TaskInfo info = {};

    info.Type = TASK_TYPE::CREATE_OBJECT;
    info.Param_0 = (DWORD_PTR)_Object;
    info.Param_1 = LayerIdx;

    TaskMgr::GetInst()->AddTask(info);
}


wchar_t Buff[255] = {};
void ChangeLevel(const wstring& _NextLevelName)
{
	TaskInfo info = {};

	wcscpy_s(Buff, 255, _NextLevelName.c_str());
	
	info.Type = TASK_TYPE::CHANGE_LEVEL;
	info.Param_0 = (DWORD_PTR)Buff;	

	TaskMgr::GetInst()->AddTask(info);
}

void ChangeLevelState(LEVEL_STATE _NextState)
{
	TaskInfo info = {};

	info.Type = TASK_TYPE::CHANGE_LEVEL_STATE;
	info.Param_0 = (DWORD_PTR)_NextState;

	TaskMgr::GetInst()->AddTask(info);
}

void DrawDebugRect(Vec3 _Pos, Vec3 _Scale, Vec3 _Rot, Vec4 _Color, float _Duration, bool _DepthTest)
{
    DbgInfo info = {};

    info.Shape = DBG_SHAPE::RECT;

    info.Pos = _Pos;
    info.Scale = _Scale;
    info.Rotation = _Rot;

    info.matWorld = XMMatrixIdentity();

    info.Color = _Color;
    info.Age = 0.f;
    info.Life = _Duration;

    info.DepthTest = _DepthTest;
    
    RenderMgr::GetInst()->AddDebugInfo(info);
}

void DrawDebugRect(const Matrix& _matWorld, Vec4 _Color, float _Duration, bool _DepthTest)
{
    DbgInfo info = {};

    info.Shape = DBG_SHAPE::RECT;

    info.matWorld = _matWorld;

    info.Color = _Color;
    info.Age = 0.f;
    info.Life = _Duration;

    info.DepthTest = _DepthTest;

    RenderMgr::GetInst()->AddDebugInfo(info);
}

void DrawDebugCircle(Vec3 _Pos, float _Radius, Vec4 _Color, float _Duration, bool _DepthTest)
{
    DbgInfo info = {};

    info.Shape = DBG_SHAPE::CIRCLE;
    info.Pos = _Pos;
    info.Scale = Vec3(_Radius * 2.f, _Radius * 2.f, 0.f);
    info.Rotation = Vec3(0.f, 0.f, 0.f);
    info.Color = _Color;
    info.Age = 0.f;
    info.Life = _Duration;
    info.DepthTest = _DepthTest;

    RenderMgr::GetInst()->AddDebugInfo(info);
}

void SaveWString(FILE* _File, const wstring& _String)
{   
    int Len = _String.length();
    fwrite(&Len, sizeof(int), 1, _File);        
    fwrite(_String.data(), sizeof(wchar_t), Len, _File);
}

wstring LoadWString(FILE* _File)
{
    int Len = 0;
    fread(&Len, sizeof(int), 1, _File);

    wchar_t buff[255] = {};
    fread(buff, sizeof(wchar_t), Len, _File);

    return buff;
}

void SaveAssetRef(FILE* _File, Asset* _Asset)
{
    // Asset 이 Null 인지 아닌지 저장
    bool IsNull = _Asset;
    fwrite(&IsNull, sizeof(bool), 1, _File);
     
    // Asset 의 Key, RelativePath 저장
    if (nullptr != _Asset)
    {
        SaveWString(_File, _Asset->GetKey());
        SaveWString(_File, _Asset->GetRelativePath());
    }
}





bool IsValid(Ptr<GameObject>& _Object)
{
    if (nullptr == _Object || _Object->IsDead())
    {
        _Object = nullptr;
        return false;
    }     
        
    return true;
}

float Saturate(float _Data)
{
    if (1.f < _Data)
        return 1.f;
    else if (_Data < 0.f)
        return 0.f;
    else
        return _Data;
}

#include "ALevel.h"
#include "GameObject.h"

#include "AssetMgr.h"
#include "Device.h"
#include "CollisionMgr.h"
#include <cmath>

#include "Source/Scripts/CPlayerScript.h"
#include "Source/Scripts/CCamMoveScript.h"
#include "Source/Scripts/CMonsterScript.h"
#include "Source/Scripts/CBossScript.h"
#include "Source/Scripts/CSpringScript.h"
#include "Source/Scripts/CBlockScript.h"
#include "Source/Scripts/CBlockMovingScript.h"
#include "Source/Scripts/CSurfaceScript.h"
#include "Source/Scripts/CTileScript.h"
#include <Source/Scripts/CBlockPushingScript.h>
#include <Source/Scripts/CDestroyBlockScript.h>
#include <Source/Scripts/CBackgroundScript.h>

void CreateOpenLevel()
{
	// Level 생성
	Ptr<ALevel> pLevel = new ALevel;
	pLevel->SetName(L"Current Level");

	pLevel->GetLayer(0)->SetName(L"Default");
	pLevel->GetLayer(1)->SetName(L"Background");
	pLevel->GetLayer(2)->SetName(L"Effect");
	pLevel->GetLayer(3)->SetName(L"UI");

	Ptr<GameObject> pObject = nullptr;
	Ptr<GameObject> pCam = nullptr;

	// 카메라
	pCam = new GameObject;
	pCam->SetName(L"MainCamera");
	pCam->AddComponent(new CTransform);
	pCam->AddComponent(new CCamera);

	pCam->Camera()->LayerCheckAll();
	pCam->Camera()->SetProjType(PROJ_TYPE::ORTHOGRAPHIC);
	pCam->Camera()->SetFar(10000.f);
	pCam->Camera()->SetFOV(90.f);
	pCam->Camera()->SetOrthoScale(1.f);

	Vec2 vResolution = Device::GetInst()->GetRenderResolution();
	pCam->Camera()->SetAspectRatio(vResolution.x / vResolution.y);
	pCam->Camera()->SetWidth(vResolution.x);

	pLevel->AddObject(0, pCam);

	// 오프닝 플립북이 검게 보이지 않도록 기본 광원 추가
	pObject = new GameObject;
	pObject->SetName(L"OpenLight");
	pObject->AddComponent(new CTransform);
	pObject->AddComponent(new CLight2D);

	pObject->Light2D()->SetLightType(LIGHT_TYPE::DIRECTIONAL);
	pObject->Light2D()->SetLightColor(Vec3(1.f, 1.f, 1.f));
	pObject->Transform()->SetRelativePos(Vec3(-0.f, 0.f, 0.f));
	pLevel->AddObject(0, pObject);

	// 오프닝 배경
	pObject = new GameObject;
	pObject->SetName(L"Opening");
	pObject->AddComponent(new CFlipbookRender);
	pObject->AddComponent(new CTransform);
	pObject->AddComponent(new COpeningScript);

	pObject->Transform()->SetRelativePos(Vec3(0.f, 0.f, 5.f));
	pObject->Transform()->SetRelativeScale(Vec3(vResolution.x, vResolution.y, 1.f));

	Ptr<AFlipbook> pOpeningFlip = LOAD(AFlipbook, L"Flipbook\\Opening.flip");
	if (pOpeningFlip != nullptr)
	{
		pObject->FlipbookRender()->SetFlipbook(0, pOpeningFlip);
		pObject->FlipbookRender()->Play(0, 0, 8.f, 0); // 0이면 1회만 재생
	}

	pLevel->AddObject(1, pObject);




	AssetMgr::GetInst()->AddAsset(L"OpenLevel", pLevel.Get());
	ChangeLevel(L"OpenLevel");
	ChangeLevelState(LEVEL_STATE::PLAY);
}


void CreateTestLevel()
{
	// Level 생성
	Ptr<ALevel> pLevel = new ALevel;
	pLevel->SetName(L"Current Level");

	pLevel->GetLayer(0)->SetName(L"Default");
	pLevel->GetLayer(1)->SetName(L"Background");
	pLevel->GetLayer(2)->SetName(L"Tile");
	pLevel->GetLayer(3)->SetName(L"Player");
	pLevel->GetLayer(4)->SetName(L"PlayerProjectile");
	pLevel->GetLayer(5)->SetName(L"Enermy");
	pLevel->GetLayer(6)->SetName(L"EnermyProjectile");

	Ptr<GameObject> pObject = nullptr;
	Ptr<GameObject> pCam = nullptr;
	// 카메라 역할 오브젝트 
	pCam = new GameObject;
	pCam->SetName(L"MainCamera");

	pCam->AddComponent(new CTransform);
	pCam->AddComponent(new CCamera);
	CCamMoveScript* pCamScript = new CCamMoveScript;
	pCamScript->SetTargetName(L"Player");
	pCam->AddComponent(pCamScript);

	pCam->Camera()->LayerCheckAll();
	//pObject->Camera()->LayerCheck(0); 
	//pObject->Camera()->LayerCheck(1); 
	//pObject->Camera()->LayerCheck(2);

	pCam->Camera()->SetProjType(PROJ_TYPE::ORTHOGRAPHIC);
	pCam->Camera()->SetFar(10000.f);
	pCam->Camera()->SetFOV(90.f);
	pCam->Camera()->SetOrthoScale(1.f);
	Vec2 vResolution = Device::GetInst()->GetRenderResolution();
	pCam->Camera()->SetAspectRatio(vResolution.x / vResolution.y); // 종횡비(AspectRatio)
	pCam->Camera()->SetWidth(vResolution.x);
	pLevel->AddObject(0, pCam);

	// 배경 추가
	pObject = new GameObject;
	pObject->SetName(L"BackGround_1");
	pObject->AddComponent(new CMeshRender);
	pObject->AddComponent(new CTransform);

	pObject->Transform()->SetRelativePos(Vec3(0.f, 0.f, 5000.f));
	pObject->Transform()->SetRelativeScale(Vec3(20736.f, 3424.f, 0.f));

	pObject->MeshRender()->SetMesh(FIND(AMesh, L"RectMesh"));
	pObject->MeshRender()->SetMaterial(FIND(AMaterial, L"BackGroundMtrl"));

	pLevel->AddObject(1, pObject);

	// 배경 추가
	pObject = new GameObject;
	pObject->SetName(L"BackGround_Billboard");
	pObject->AddComponent(new CMeshRender);
	pObject->AddComponent(new CTransform);
	pObject->AddComponent(new CBackgroundScript);
	pObject->Transform()->SetRelativePos(Vec3(1120.f, 110.f, 5000.f));
	pObject->Transform()->SetRelativeScale(Vec3(15000.f, 2500.f, 0.f));

	pObject->MeshRender()->SetMesh(FIND(AMesh, L"RectMesh"));
	pObject->MeshRender()->SetMaterial(FIND(AMaterial, L"BackGroundMtrl_Billboard"));

	pLevel->AddObject(1, pObject);

	// 광원 추가
	pObject = new GameObject;
	pObject->SetName(L"Light 1");
	pObject->AddComponent(new CTransform);
	pObject->AddComponent(new CLight2D);

	pObject->Light2D()->SetLightType(LIGHT_TYPE::DIRECTIONAL);
	pObject->Light2D()->SetLightColor(Vec3(1.5f, 1.5f, 1.5f));
	pObject->Light2D()->SetRadius(400.f);
	pObject->Light2D()->SetAngle(XM_PI / 2.f);

	pObject->Transform()->SetRelativePos(Vec3(-250.f, 0.f, 0.f));
	pObject->Transform()->SetRelativeRot(Vec3(0.f, 0.f, XM_PI / 4.f));

	pLevel->AddObject(0, pObject);

	// 몬스터 생성
	//for (int i = 0; i < 5; ++i)
	//{
	//	Ptr<GameObject> pMonster = new GameObject;
	//	pMonster->SetName(L"Monster");

	//	pMonster->AddComponent(new CTransform);
	//	pMonster->AddComponent(new CMeshRender);
	//	pMonster->AddComponent(new CCollider2D);
	//	pMonster->AddComponent(new CMonsterScript);

	//	pMonster->Transform()->SetRelativePos(Vec3(300.f * (float)i, 0.f, 100.f));
	//	pMonster->Transform()->SetRelativeScale(Vec3(200.f, 200.f, 0.f));

	//	pMonster->MeshRender()->SetMesh(FIND(AMesh, L"RectMesh"));
	//	pMonster->MeshRender()->SetMaterial(FIND(AMaterial, L"MonsterMtrl"));

	//	pLevel->AddObject(5, pMonster);
	//}

	//// 몬스터 생성
	//Ptr<GameObject> pMonster = new GameObject;
	//pMonster->SetName(L"Monster");

	//pMonster->AddComponent(new CTransform);
	//pMonster->AddComponent(new CSpriteRender);
	//pMonster->AddComponent(new CCollider2D);

	//pMonster->Transform()->SetRelativePos(Vec3(300.f, 100.f, 100.f));
	//pMonster->Transform()->SetRelativeScale(Vec3(50.f, 50.f, 0.f));
	//pMonster->SpriteRender()->SetSprite(FIND(ASprite, L"TileSprite_47"));

	//pLevel->AddObject(5, pMonster);

	// Player Object 추가
	pObject = new GameObject;
	pObject->SetName(L"Player");
	pObject->AddComponent(new CTransform);
	pObject->AddComponent(new CFlipbookRender);
	pObject->AddComponent(new CCollider2D);

	Ptr<CPlayerScript> pPlayerScript = new CPlayerScript;
	// pPlayerScript->SetTarget(pMonster);

	pObject->AddComponent(pPlayerScript.Get());

	pObject->Transform()->SetRelativePos(Vec3(0.f, 0.f, 1.f));
	pObject->Transform()->SetRelativeScale(Vec3(100.f, 100.f, 1.f));

	pObject->Collider2D()->SetOffset(Vec2(0.f, -0.1f));
	pObject->Collider2D()->SetScale(Vec2(0.4f, 0.6f));

	pObject->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"SonicRun_Basic"));
	pObject->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_IDLE"));
	pObject->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_IDLE_Long"));
	pObject->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_MaxSpeed")); // 3
	pObject->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_Jump"));
	pObject->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_Up"));
	pObject->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_Down"));
	pObject->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_Skill"));//7
	pObject->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_Break"));//8
	pObject->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_Turn")); //9
	pObject->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_Spring_Jump")); //10
	pObject->FlipbookRender()->Play(1, 10.f, -1);

	Ptr<GameObject> pChild = new GameObject;
	pChild->SetName(L"Child");
	pChild->AddComponent(new CTransform);
	pChild->AddComponent(new CMeshRender);
	pChild->AddComponent(new CCollider2D);

	pChild->Transform()->SetRelativePos(Vec3(-200.f, 0.f, 0.f));
	pChild->Transform()->SetRelativeScale(Vec3(50.f, 50.f, 1.f));
	pChild->Transform()->SetIndependentScale(true);

	pChild->MeshRender()->SetMesh(AssetMgr::GetInst()->Find<AMesh>(L"RectMesh"));
	pChild->MeshRender()->SetMaterial(AssetMgr::GetInst()->Find<AMaterial>(L"Std2DMtrl"));

	// Player 와 Child 부모자식 연결
	pObject->AddChild(pChild);
	// 광원 추가
	//Ptr<GameObject> pChild1 = new GameObject;
	//pChild1 = new GameObject;
	//pChild1->SetName(L"Light 1");
	//pChild1->AddComponent(new CTransform);
	//pChild1->AddComponent(new CLight2D);
	//// pChild1->Transform()->SetIndependentScale(true);

	//pChild1->Light2D()->SetLightType(LIGHT_TYPE::DIRECTIONAL);
	//pChild1->Light2D()->SetLightColor(Vec3(1.f, 1.f, 1.0f));
	//pChild1->Light2D()->SetRadius(300.f);
	//pChild1->Light2D()->SetAngle(XM_PI / 2.f);

	//pChild1->Transform()->SetRelativePos(Vec3(-1.f, 0.f, 0.f));

	//pObject->AddChild(pChild1);

	//Player(부모 오브젝트) 를 레벨에 추가
	pLevel->AddObject(3, pObject);

    Ptr<GameObject> pBoss = new GameObject;
    pBoss->SetName(L"Boss");
    pBoss->AddComponent(new CTransform);
    pBoss->AddComponent(new CFlipbookRender);
    pBoss->AddComponent(new CCollider2D);

    Ptr<CBossScript> pBossScript = new CBossScript;
    pBossScript->SetState(BOSS_STATE::IDLE);
    pBoss->AddComponent(pBossScript.Get());

    pBoss->Transform()->SetRelativePos(Vec3(320.f, 0.f, 1.f));
    pBoss->Transform()->SetRelativeScale(Vec3(220.f, 220.f, 1.f));
    pBoss->Collider2D()->SetScale(Vec2(0.5f, 0.7f));

    Ptr<AFlipbook> pBossMoveFlipbook = LOAD(AFlipbook, L"Flipbook\\Boss_Move.flip");
    if (pBossMoveFlipbook != nullptr)
    {
        pBoss->FlipbookRender()->AddFlipbook(pBossMoveFlipbook);
        pBoss->FlipbookRender()->Play(0, 10.f, -1);
    }

    pLevel->AddObject(5, pBoss);


	//// Tile Object
	//Ptr<ATileMap> pTileMapAsset = FIND(ATileMap, L"TestTileMap");

	//if (pTileMapAsset != nullptr)
	//{
	//	Ptr<GameObject> pTileMapObj = new GameObject;
	//	pTileMapObj->SetName(L"TileMapRender");

	//	pTileMapObj->AddComponent(new CTransform);
	//	pTileMapObj->AddComponent(new CTileRender);

	//	UINT Col = pTileMapAsset->GetCol();
	//	UINT Row = pTileMapAsset->GetRow();
	//	vector<int> tileTypeValues;

	//	const wstring presetPath = wstring(CONTENT_PATH) + L"TileMap\\TileScriptPreset.txt";
	//	if (!CTileScript::LoadTileScriptPreset(presetPath, Row, Col, tileTypeValues, true))
	//	{
	//		CTileScript::GetDefaultTileMap(Row, Col, tileTypeValues);
	//	}

	//	if (tileTypeValues.size() != (size_t)Row * (size_t)Col)
	//	{
	//		CTileScript::GetDefaultTileMap(Row, Col, tileTypeValues);
	//	}

	//	if (pTileMapAsset->GetRow() != Row || pTileMapAsset->GetCol() != Col)
	//	{
	//		pTileMapAsset->SetRowCol(Row, Col);
	//	}

	//	for (UINT row = 0; row < Row; ++row)
	//	{
	//		for (UINT col = 0; col < Col; ++col)
	//		{
	//			int tileTypeValue = tileTypeValues[(size_t)row * Col + col];
	//			if (!CTileScript::IsValidTileTypeValue(tileTypeValue))
	//				tileTypeValue = (int)TILETYPE::EMPTY_BLOCK;

	//			int tileIdx = tileTypeValue - 1;
	//			if (tileIdx < 0)
	//				tileIdx = 0;

	//			wchar_t szKey[50] = {};
	//			wchar_t szRelativePath[100] = {};
	//			swprintf_s(szKey, L"MapTest_%d", tileIdx);
	//			swprintf_s(szRelativePath, L"Sprite\\MapTest_%d.sprite", tileIdx);

	//			Ptr<ASprite> pSprite = AssetMgr::GetInst()->Load<ASprite>(szKey, szRelativePath);
	//			if (nullptr == pSprite && tileTypeValue != (int)TILETYPE::EMPTY_BLOCK)
	//			{
	//				if (CTileScript::IsLineTileTypeValue(tileTypeValue))
	//					tileIdx = (int)TILETYPE::LINE_BLOCK_3 - 1;
	//				else if (CTileScript::IsCircleTileTypeValue(tileTypeValue))
	//					tileIdx = (int)TILETYPE::CIRCLE_BLOCK_1 - 1;
	//				else
	//					tileIdx = 0;

	//				swprintf_s(szKey, L"MapTest_%d", tileIdx);
	//				swprintf_s(szRelativePath, L"Sprite\\MapTest_%d.sprite", tileIdx);
	//				pSprite = AssetMgr::GetInst()->Load<ASprite>(szKey, szRelativePath);
	//			}

	//			if (nullptr != pSprite)
	//			{
	//				pTileMapAsset->SetSprite(row, col, pSprite);
	//			}
	//		}
	//	}

	//	Vec2 TileSize = pTileMapAsset->GetTileSize();

	//	// 가로 = Col, 세로 = Row

	//	pTileMapObj->Transform()->SetRelativeScale(Vec3(TileSize.x * (float)Col, TileSize.y * (float)Row, 1.f));

	//	CTileScript::TILE_MAP_PLACEMENT placement = {};
	//	CTileScript::GetTileMapPlacement(placement);
	//	pTileMapObj->Transform()->SetRelativePos(Vec3(placement.map_pos_x, placement.map_pos_y, placement.map_pos_z));

	//	pTileMapObj->TileRender()->SetTileMap(pTileMapAsset);
	//	pTileMapObj->TileRender()->SetOpacity(0.5f);

	//	pLevel->AddObject(2, pTileMapObj);

	//	Vec2 tileSize = pTileMapAsset->GetTileSize();
	//	const float tileW = tileSize.x;
	//	const float tileH = tileSize.y;

	//	const float startLocalX = tileW * 0.5f + placement.collision_offset_x;
	//	const float startLocalY = -tileH * 0.5f + placement.collision_offset_y;
	//	const float localZ = 10.f - placement.map_pos_z;

	//	for (UINT row = 0; row < Row; ++row)
	//	{
	//		for (UINT col = 0; col < Col; ++col)
	//		{
	//			const int tileTypeValue = tileTypeValues[(size_t)row * Col + col];
	//			if (!CTileScript::IsValidTileTypeValue(tileTypeValue))
	//				continue;
	//			if (tileTypeValue == (int)TILETYPE::EMPTY_BLOCK)
	//				continue;

	//			GameObject* pTileObj = new GameObject;
	//			pTileObj->SetName(L"Tile");

	//			pTileObj->AddComponent(new CTransform);
	//			pTileObj->AddComponent(new CCollider2D);
	//			pTileObj->Transform()->SetIndependentScale(true);

	//			float x = startLocalX + col * tileW;
	//			float y = startLocalY - row * tileH;

	//			pTileObj->Transform()->SetRelativePos(Vec3(x, y, localZ));
	//			pTileObj->Transform()->SetRelativeScale(Vec3(tileW, tileH, 1.f));

	//			pTileObj->Collider2D()->SetOffset(Vec2(0.f, 0.f));
	//			pTileObj->Collider2D()->SetScale(Vec2(1.f, 1.f));

	//			CTileScript* pTileScript = new CTileScript;
	//			pTileObj->AddComponent(pTileScript);
	//			pTileScript->TileMapSetting(tileTypeValue);

	//			pTileMapObj->AddChild(pTileObj);
	//		}
	//	}
	//}

	//// 스프링
	//pObject = new GameObject;
	//pObject->SetName(L"Spring");

	//pObject->AddComponent(new CTransform);
	//pObject->AddComponent(new CCollider2D);
	//pObject->AddComponent(new CFlipbookRender);
	//pObject->AddComponent(new CSpringScript);

	//pObject->Collider2D()->SetOffset(Vec2(-0.3f, 0.0f));
	//pObject->Collider2D()->SetScale(Vec2(0.5f, 1.f));

	//pObject->Transform()->SetRelativePos(Vec3(1200.f, 320.f, 9.f));
	//pObject->Transform()->SetRelativeScale(Vec3(80.f, 80.f, 0.f));
	//pObject->Transform()->SetRelativeRot(Vec3(0.f, 0.f, XM_PIDIV2));
	//pObject->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Spring"));

	//pLevel->AddObject(5, pObject);

	//// 스프링2
	//pObject = new GameObject;
	//pObject->SetName(L"Spring");

	//pObject->AddComponent(new CTransform);
	//pObject->AddComponent(new CCollider2D);
	//pObject->AddComponent(new CFlipbookRender);
	//pObject->AddComponent(new CSpringScript);

	//pObject->Collider2D()->SetOffset(Vec2(-0.3f, 0.0f));
	//pObject->Collider2D()->SetScale(Vec2(0.5f, 1.f));

	//pObject->Transform()->SetRelativePos(Vec3(-50.f, 320.f, 9.f));
	//pObject->Transform()->SetRelativeScale(Vec3(80.f, 80.f, 0.f));
	//pObject->Transform()->SetRelativeRot(Vec3(0.f, 0.f, 0.f));
	//pObject->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Spring"));

	// =========================
	// 바닥 타일 부근 테스트용 스프링 / 공중 블록
	// =========================
	auto SpawnSpring = [&](const Vec3& _Pos, float _RotZ)
	{
		Ptr<GameObject> pSpring = new GameObject;
		pSpring->SetName(L"Spring");

		pSpring->AddComponent(new CTransform);
		pSpring->AddComponent(new CCollider2D);
		pSpring->AddComponent(new CFlipbookRender);
		pSpring->AddComponent(new CSpringScript);

		pSpring->Collider2D()->SetOffset(Vec2(-0.3f, 0.0f));
		pSpring->Collider2D()->SetScale(Vec2(0.5f, 1.f));

		pSpring->Transform()->SetRelativePos(_Pos);
		pSpring->Transform()->SetRelativeScale(Vec3(80.f, 80.f, 0.f));
		pSpring->Transform()->SetRelativeRot(Vec3(0.f, 0.f, _RotZ));
		pSpring->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Spring"));

		pLevel->AddObject(5, pSpring);
	};

	auto SpawnAirBlock = [&](const Vec3& _StartPos, const Vec3& _EndPos, const Vec3& _Scale)
	{
		Ptr<GameObject> pBlock = new GameObject;
		pBlock->SetName(L"AirBlock");

		pBlock->AddComponent(new CTransform);
		pBlock->AddComponent(new CMeshRender);
		pBlock->AddComponent(new CCollider2D);
		// pBlock->AddComponent(new CBlockScript);
		// CBlockMovingScript* pMoveScript = new CBlockMovingScript;
		//CBlockPushingScript* pMoveScript = new CBlockPushingScript;
		CDestroyBlockScript* pMoveScript = new CDestroyBlockScript;
		pBlock->AddComponent(pMoveScript);

		Vec3 blockScale = _Scale;
		if (fabsf(blockScale.z) <= 0.0001f)
			blockScale.z = 1.f;

		pBlock->Transform()->SetRelativePos(_StartPos);
		pBlock->Transform()->SetRelativeScale(blockScale);

		pBlock->Collider2D()->SetOffset(Vec2(0.f, 0.f));
		pBlock->Collider2D()->SetScale(Vec2(1.f, 1.f));

		pBlock->MeshRender()->SetMesh(FIND(AMesh, L"RectMesh"));
		pBlock->MeshRender()->SetMaterial(FIND(AMaterial, L"Std2DMtrl"));

		// pMoveScript->SetStartPos(_StartPos);
		// pMoveScript->SetEndPos(_EndPos);
		// pMoveScript->SetVelocity(Vec2(120.f, 120.f));

		pLevel->AddObject(5, pBlock);
	};


	auto SpawnSpike = [&](const wchar_t* _PrefabKey, const wchar_t* _Name, const Vec3& _Pos)
		{
			Ptr<APrefab> pPrefab = FIND(APrefab, _PrefabKey);
			if (nullptr == pPrefab)
				return;

			Ptr<GameObject> pBlock = pPrefab->Instantiate();
			if (nullptr == pBlock)
				return;

			pBlock->SetName(_Name);
			pBlock->Transform()->SetRelativePos(_Pos);
			pLevel->AddObject(5, pBlock);
		};

	auto SpawnBlockPrefab = [&](const wchar_t* _PrefabKey, const wchar_t* _Name, const Vec3& _Pos)
	{
		Ptr<APrefab> pPrefab = FIND(APrefab, _PrefabKey);
		if (nullptr == pPrefab)
			return;

		Ptr<GameObject> pBlock = pPrefab->Instantiate();
		if (nullptr == pBlock)
			return;

		pBlock->SetName(_Name);
		pBlock->Transform()->SetRelativePos(_Pos);
		pLevel->AddObject(5, pBlock);
	};

	auto SpawnMovingBlockPrefab = [&](const wchar_t* _PrefabKey, const wchar_t* _Name, const Vec3& _StartPos, const Vec3& _EndPos, const Vec2& _Velocity)
	{
		Ptr<APrefab> pPrefab = FIND(APrefab, _PrefabKey);
		if (nullptr == pPrefab)
			return;

		Ptr<GameObject> pBlock = pPrefab->Instantiate();
		if (nullptr == pBlock)
			return;

		pBlock->SetName(_Name);
		pBlock->Transform()->SetRelativePos(_StartPos);

		Ptr<CBlockMovingScript> pMoveScript = pBlock->GetScript<CBlockMovingScript>();
		if (nullptr != pMoveScript)
		{
			pMoveScript->SetStartPos(_StartPos);
			pMoveScript->SetEndPos(_EndPos);
			pMoveScript->SetVelocity(_Velocity);
		}

		pLevel->AddObject(5, pBlock);
	};

	// 스프링: 시작 지점 근처에서 바로 보이도록 배치
	SpawnSpring(Vec3(120.f, -260.f, 9.f), 0.f);
	SpawnSpring(Vec3(520.f, -220.f, 9.f), 0.f);

	// 프리팹 블록: 테스트 맵 곳곳에 섞어서 배치
	SpawnBlockPrefab(L"Prefab\\Block_Mid.pref", L"Block_Mid_A", Vec3(-420.f, -260.f, 9.f));
	SpawnBlockPrefab(L"Prefab\\Block_Mid.pref", L"Block_Mid_B", Vec3(340.f, -280.f, 9.f));
	SpawnBlockPrefab(L"Prefab\\Block_Mid.pref", L"Block_Mid_C", Vec3(980.f, -220.f, 9.f));
	SpawnBlockPrefab(L"Prefab\\Block_Mid.pref", L"Block_Mid_D", Vec3(1700.f, -260.f, 9.f));

	SpawnBlockPrefab(L"Prefab\\Block_Tall.pref", L"Block_Tall_A", Vec3(-760.f, -120.f, 9.f));
	SpawnBlockPrefab(L"Prefab\\Block_Tall.pref", L"Block_Tall_C", Vec3(2220.f, -120.f, 9.f));

	SpawnBlockPrefab(L"Prefab\\Block_Small.pref", L"Block_Small_A", Vec3(-120.f, -520.f, 9.f));
	SpawnBlockPrefab(L"Prefab\\Block_Small.pref", L"Block_Small_B", Vec3(1380.f, -420.f, 9.f));
	SpawnBlockPrefab(L"Prefab\\Block_Small.pref", L"Block_Small_C", Vec3(2520.f, -320.f, 9.f));

	SpawnMovingBlockPrefab(L"Prefab\\Block_Move.pref", L"Block_Move_A", Vec3(80.f, -760.f, 9.f), Vec3(520.f, -760.f, 9.f), Vec2(180.f, 0.f));
	SpawnMovingBlockPrefab(L"Prefab\\Block_Move.pref", L"Block_Move_B", Vec3(1160.f, -980.f, 9.f), Vec3(1160.f, -540.f, 9.f), Vec2(0.f, 160.f));
	SpawnMovingBlockPrefab(L"Prefab\\Block_Move.pref", L"Block_Move_C", Vec3(1940.f, -760.f, 9.f), Vec3(2360.f, -560.f, 9.f), Vec2(150.f, 90.f));

	SpawnSpike(L"Prefab\\Spike.pref", L"Spike", Vec3(760.f, -80.f, 9.f));

	// 공중 블록: 플레이어 초기 동선 근처에서 보이도록 배치
	SpawnAirBlock(Vec3(180.f, -1600.f, 9.f), Vec3(200.f, -1900.f, 9.f), Vec3(200.f, 200.f, 0.f));
	SpawnAirBlock(Vec3(360.f, -1400.f, 9.f), Vec3(400.f, -1400.f, 9.f), Vec3(400.f, 40.f, 0.f));
	SpawnAirBlock(Vec3(560.f, -1400.f, 9.f), Vec3(760.f, -1300.f, 9.f), Vec3(120.f, 40.f, 0.f));
	SpawnAirBlock(Vec3(760.f, -1500.f, 9.f), Vec3(500.f, -1200.f, 9.f), Vec3(140.f, 40.f, 0.f));

	// 레벨 충돌 설정
	pLevel->CheckCollisionLayer(2, 3);
	pLevel->CheckCollisionLayer(2, 5);
	pLevel->CheckCollisionLayer(3, 5);
	pLevel->CheckCollisionLayer(4, 5);
	pLevel->CheckCollisionLayer(3, 6);

	// 레벨 변경점 체크
	pLevel->SetChanged();

	// 레벨을 AssetMgr 에 등록
	AssetMgr::GetInst()->AddAsset(L"TestLevel", pLevel.Get());

	ChangeLevel(L"TestLevel");
}

void CreateSurfaceAutoplayLevel()
{
    Ptr<ALevel> pLevel = new ALevel;
    pLevel->SetName(L"Surface Autoplay Level");

    pLevel->GetLayer(0)->SetName(L"Default");
    pLevel->GetLayer(1)->SetName(L"Background");
    pLevel->GetLayer(2)->SetName(L"Tile");
    pLevel->GetLayer(3)->SetName(L"Player");
    pLevel->GetLayer(4)->SetName(L"PlayerProjectile");
    pLevel->GetLayer(5)->SetName(L"Enermy");
    pLevel->GetLayer(6)->SetName(L"EnermyProjectile");

    Ptr<GameObject> pObject = new GameObject;
    pObject->SetName(L"MainCamera");
    pObject->AddComponent(new CTransform);
    pObject->AddComponent(new CCamera);
    CCamMoveScript* pCamScript = new CCamMoveScript;
    pCamScript->SetTargetName(L"Player");
    pObject->AddComponent(pCamScript);
    pObject->Camera()->LayerCheckAll();
    pObject->Camera()->SetProjType(PROJ_TYPE::ORTHOGRAPHIC);
    pObject->Camera()->SetFar(10000.f);
    pObject->Camera()->SetFOV(90.f);
    pObject->Camera()->SetOrthoScale(1.f);
    Vec2 vResolution = Device::GetInst()->GetRenderResolution();
    pObject->Camera()->SetAspectRatio(vResolution.x / vResolution.y);
    pObject->Camera()->SetWidth(vResolution.x);
    pLevel->AddObject(0, pObject);

    pObject = new GameObject;
    pObject->SetName(L"BackGround_Billboard");
    pObject->AddComponent(new CMeshRender);
    pObject->AddComponent(new CTransform);
    pObject->AddComponent(new CBackgroundScript);
    pObject->Transform()->SetRelativePos(Vec3(0.f, 0.f, 5000.f));
    pObject->Transform()->SetRelativeScale(Vec3(10000.f, 3000.f, 0.f));
    pObject->MeshRender()->SetMesh(FIND(AMesh, L"RectMesh"));
    pObject->MeshRender()->SetMaterial(FIND(AMaterial, L"BackGroundMtrl_Billboard"));
    pLevel->AddObject(1, pObject);

    pObject = new GameObject;
    pObject->SetName(L"Light 1");
    pObject->AddComponent(new CTransform);
    pObject->AddComponent(new CLight2D);
    pObject->Light2D()->SetLightType(LIGHT_TYPE::DIRECTIONAL);
    pObject->Light2D()->SetLightColor(Vec3(1.5f, 1.5f, 1.5f));
    pObject->Light2D()->SetRadius(400.f);
    pObject->Light2D()->SetAngle(XM_PI / 2.f);
    pObject->Transform()->SetRelativePos(Vec3(-250.f, 0.f, 0.f));
    pObject->Transform()->SetRelativeRot(Vec3(0.f, 0.f, XM_PI / 4.f));
    pLevel->AddObject(0, pObject);

    Ptr<GameObject> pPlayer = new GameObject;
    pPlayer->SetName(L"Player");
    pPlayer->AddComponent(new CTransform);
    pPlayer->AddComponent(new CFlipbookRender);
    pPlayer->AddComponent(new CCollider2D);

    Ptr<CPlayerScript> pPlayerScript = new CPlayerScript;
    pPlayer->AddComponent(pPlayerScript.Get());

    const bool surfaceJumpScenario = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_jump"));
    const bool surfaceTopJumpScenario = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_top_jump"));
    const bool surfaceSlowFallScenario = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_slow_fall"));
    const bool surfaceLineUnderScenario = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_line_under"));
    const bool surfaceChordScenario = (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_chord"));
    const bool surfaceLeftScenario =
        (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_left")) ||
        (nullptr != wcsstr(GetCommandLineW(), L"-autoplay_surface_chord_left"));
    const Vec3 rightSecantStartPos = Vec3(-80.f, -50.f, 1.f);
    const Vec3 leftSecantStartPos = Vec3(360.f, -50.f, 1.f);
    const Vec2 rightSecantStartVel = Vec2(260.f, 0.f);
    const Vec2 leftSecantStartVel = Vec2(-260.f, 0.f);
    if (surfaceTopJumpScenario)
        pPlayer->Transform()->SetRelativePos(Vec3(128.f, -95.f, 1.f));
    else if (surfaceSlowFallScenario)
        pPlayer->Transform()->SetRelativePos(Vec3(128.f, -95.f, 1.f));
    else if (surfaceLineUnderScenario)
        pPlayer->Transform()->SetRelativePos(Vec3(-40.f, 60.f, 1.f));
    else if (surfaceJumpScenario)
        pPlayer->Transform()->SetRelativePos(rightSecantStartPos);
    else if (surfaceChordScenario && surfaceLeftScenario)
        pPlayer->Transform()->SetRelativePos(leftSecantStartPos);
    else if (surfaceChordScenario)
        pPlayer->Transform()->SetRelativePos(rightSecantStartPos);
    else if (surfaceLeftScenario)
        pPlayer->Transform()->SetRelativePos(leftSecantStartPos);
    else
        pPlayer->Transform()->SetRelativePos(rightSecantStartPos);
    pPlayer->Transform()->SetRelativeScale(Vec3(100.f, 100.f, 1.f));
    if (surfaceTopJumpScenario)
        pPlayerScript->SetVelocity(Vec2(220.f, -35.f));
    else if (surfaceSlowFallScenario)
        pPlayerScript->SetVelocity(Vec2(55.f, -8.f));
    else if (surfaceLineUnderScenario)
        pPlayerScript->SetVelocity(Vec2(0.f, -240.f));
    else if (surfaceJumpScenario)
        pPlayerScript->SetVelocity(rightSecantStartVel);
    else if (surfaceChordScenario && surfaceLeftScenario)
        pPlayerScript->SetVelocity(leftSecantStartVel);
    else if (surfaceChordScenario)
        pPlayerScript->SetVelocity(rightSecantStartVel);
    else if (surfaceLeftScenario)
        pPlayerScript->SetVelocity(leftSecantStartVel);
    else
        pPlayerScript->SetVelocity(rightSecantStartVel);

    pPlayer->Collider2D()->SetOffset(Vec2(0.f, -0.1f));
    pPlayer->Collider2D()->SetScale(Vec2(0.4f, 0.6f));

    pPlayer->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"SonicRun_Basic"));
    pPlayer->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_IDLE"));
    pPlayer->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_IDLE_Long"));
    pPlayer->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_MaxSpeed"));
    pPlayer->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_Jump"));
    pPlayer->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_Up"));
    pPlayer->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_Down"));
    pPlayer->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_Skill"));
    pPlayer->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_Break"));
    pPlayer->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_Turn"));
    pPlayer->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Sonic_Spring_Jump"));
    pPlayer->FlipbookRender()->Play(1, 10.f, -1);
    pLevel->AddObject(3, pPlayer);

    Ptr<GameObject> pSurfaceRoot = new GameObject;
    pSurfaceRoot->SetName(L"SurfaceRoot");
    pSurfaceRoot->AddComponent(new CTransform);
    pLevel->AddObject(2, pSurfaceRoot);

    auto SpawnLineSurface = [&](const wchar_t* _Name, const Vec2& _StartWorld, const Vec2& _EndWorld)
    {
        Ptr<GameObject> pSurface = new GameObject;
        pSurface->SetName(_Name);
        pSurface->AddComponent(new CTransform);
        pSurface->AddComponent(new CCollider2D);

        CSurfaceScript* pScript = new CSurfaceScript;
        pSurface->AddComponent(pScript);

        Vec2 center = (_StartWorld + _EndWorld) * 0.5f;
        pSurface->Transform()->SetRelativePos(Vec3(center.x, center.y, 10.f));
        pScript->ConfigureLine(_StartWorld - center, _EndWorld - center,
                               CSurfaceScript::SURFACE_ROLE::SURFACE, false, true);
        pSurfaceRoot->AddChild(pSurface);
    };

    auto SpawnCircleSurface = [&](const wchar_t* _Name, const Vec2& _Center, float _Radius,
                                  CSurfaceScript::ARC_CORNER _Corner, bool _FillInside)
    {
        Ptr<GameObject> pSurface = new GameObject;
        pSurface->SetName(_Name);
        pSurface->AddComponent(new CTransform);
        pSurface->AddComponent(new CCollider2D);

        CSurfaceScript* pScript = new CSurfaceScript;
        pSurface->AddComponent(pScript);

        Vec2 minBox = {};
        Vec2 maxBox = {};
        switch (_Corner)
        {
        case CSurfaceScript::ARC_CORNER::TOP_LEFT:
            minBox = Vec2(_Center.x, _Center.y - _Radius);
            maxBox = Vec2(_Center.x + _Radius, _Center.y);
            break;
        case CSurfaceScript::ARC_CORNER::TOP_RIGHT:
            minBox = Vec2(_Center.x - _Radius, _Center.y - _Radius);
            maxBox = Vec2(_Center.x, _Center.y);
            break;
        case CSurfaceScript::ARC_CORNER::BOTTOM_LEFT:
            minBox = Vec2(_Center.x, _Center.y);
            maxBox = Vec2(_Center.x + _Radius, _Center.y + _Radius);
            break;
        case CSurfaceScript::ARC_CORNER::BOTTOM_RIGHT:
        default:
            minBox = Vec2(_Center.x - _Radius, _Center.y);
            maxBox = Vec2(_Center.x, _Center.y + _Radius);
            break;
        }

        pSurface->Transform()->SetRelativePos(Vec3(_Center.x, _Center.y, 10.f));
        pScript->ConfigureCircle(minBox - _Center, maxBox - _Center,
                                 CSurfaceScript::SURFACE_ROLE::SURFACE, _Corner, _FillInside, true);
        pSurfaceRoot->AddChild(pSurface);
    };

    constexpr float kAutoplayCircleCenterX = 140.f;
    constexpr float kAutoplayCircleCenterY = 0.f;
    constexpr float kAutoplayCircleRadius = 140.f;
    constexpr float kAutoplayChordY = -80.f;
    const float chordOffsetY = fabsf(kAutoplayChordY - kAutoplayCircleCenterY);
    const float chordHalfSpan =
        sqrtf(max(0.f, kAutoplayCircleRadius * kAutoplayCircleRadius - chordOffsetY * chordOffsetY));
    const float chordStartX = kAutoplayCircleCenterX - chordHalfSpan;
    const float chordEndX = kAutoplayCircleCenterX + chordHalfSpan;

    SpawnCircleSurface(L"SurfaceCircleBottomLeft", Vec2(kAutoplayCircleCenterX, kAutoplayCircleCenterY), kAutoplayCircleRadius, CSurfaceScript::ARC_CORNER::BOTTOM_LEFT, true);
    SpawnCircleSurface(L"SurfaceCircleBottomRight", Vec2(kAutoplayCircleCenterX, kAutoplayCircleCenterY), kAutoplayCircleRadius, CSurfaceScript::ARC_CORNER::BOTTOM_RIGHT, true);
    SpawnCircleSurface(L"SurfaceCircleTopRight", Vec2(kAutoplayCircleCenterX, kAutoplayCircleCenterY), kAutoplayCircleRadius, CSurfaceScript::ARC_CORNER::TOP_RIGHT, true);
    SpawnCircleSurface(L"SurfaceCircleTopLeft", Vec2(kAutoplayCircleCenterX, kAutoplayCircleCenterY), kAutoplayCircleRadius, CSurfaceScript::ARC_CORNER::TOP_LEFT, true);
    SpawnLineSurface(L"SurfaceLineApproach", Vec2(-120.f, kAutoplayChordY), Vec2(chordStartX, kAutoplayChordY));
    SpawnLineSurface(L"SurfaceLineChord", Vec2(chordStartX, kAutoplayChordY), Vec2(chordEndX, kAutoplayChordY));
    SpawnLineSurface(L"SurfaceLineExit", Vec2(chordEndX, kAutoplayChordY), Vec2(400.f, kAutoplayChordY));
    if (!surfaceChordScenario)
    {
        SpawnLineSurface(L"SurfaceLineSlope", Vec2(-360.f, 20.f), Vec2(-120.f, 140.f));
        SpawnLineSurface(L"SurfaceLineFlat", Vec2(-120.f, 140.f), Vec2(0.f, 140.f));
    }

    pLevel->CheckCollisionLayer(2, 3);
    pLevel->SetChanged();
    pLevel->FinalTick();

    AssetMgr::GetInst()->AddAsset(L"SurfaceAutoplayLevel", pLevel.Get());
    ChangeLevel(L"SurfaceAutoplayLevel");
}
