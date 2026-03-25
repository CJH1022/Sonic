#include "pch.h"

#include "RenderMgr.h"
#include "TaskMgr.h"
#include "LevelMgr.h"

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
#include "Source/Scripts/CSpringScript.h"
#include "Source/Scripts/CBlockScript.h"
#include "Source/Scripts/CBlockMovingScript.h"
#include "Source/Scripts/CTileScript.h"
#include <Source/Scripts/CBlockPushingScript.h>

void RebuildTileCollision(GameObject* _TileMapObject)
{
	if (nullptr == _TileMapObject || nullptr == _TileMapObject->TileRender())
		return;

	Ptr<ATileMap> pTileMapAsset = _TileMapObject->TileRender()->GetTileMap();
	if (nullptr == pTileMapAsset)
		return;

	// 기존 자식 중 Tile collision 오브젝트만 제거한다.
	// TileRender 밑에 다른 편집용 자식이 생겨도 건드리지 않기 위해
	// CTileScript가 붙은 오브젝트만 골라낸다.
	const vector<Ptr<GameObject>>& vecChild = _TileMapObject->GetChild();
	for (size_t i = 0; i < vecChild.size(); ++i)
	{
		if (vecChild[i] == nullptr)
			continue;

		if (vecChild[i]->GetScript<CTileScript>() != nullptr)
		{
			vecChild[i]->Destroy();
		}
	}

	UINT Col = pTileMapAsset->GetCol();
	UINT Row = pTileMapAsset->GetRow();
	Vec2 tileSize = pTileMapAsset->GetTileSize();
	const float tileW = tileSize.x;
	const float tileH = tileSize.y;
	const vector<UINT>& tileTypes = pTileMapAsset->GetTileTypes();
	const vector<Vec2>& tileScales = pTileMapAsset->GetTileScales();

	Vec3 mapPos = _TileMapObject->Transform()->GetRelativePos();
	const float collisionLocalZ = 10.f - mapPos.z;

	for (UINT row = 0; row < Row; ++row)
	{
		for (UINT col = 0; col < Col; ++col)
		{
			const UINT idx = row * Col + col;
			if (idx >= tileTypes.size())
				continue;

			const UINT typeIdx = tileTypes[idx];
			const TileTypeDesc* pDesc = pTileMapAsset->GetTileTypeDesc(typeIdx);
			if (nullptr == pDesc)
				continue;

			if ((pDesc->Flags & TILE_FLAG_SOLID) == 0)
				continue;

			GameObject* pTileObj = new GameObject;
			pTileObj->SetName(L"Tile");

			pTileObj->AddComponent(new CTransform);
			pTileObj->AddComponent(new CCollider2D);
			pTileObj->Transform()->SetIndependentScale(true);

			float x = ((float)col + 0.5f) * tileW;
			float y = -((float)row + 0.5f) * tileH;
			Vec2 cellScale = Vec2(1.f, 1.f);
			if (idx < tileScales.size())
			{
				cellScale = tileScales[idx];
			}

			pTileObj->Transform()->SetRelativePos(Vec3(x, y, collisionLocalZ));
			pTileObj->Transform()->SetRelativeScale(Vec3(tileW * cellScale.x, tileH * cellScale.y, 1.f));

			pTileObj->Collider2D()->SetOffset(Vec2(0.f, 0.f));
			pTileObj->Collider2D()->SetScale(Vec2(1.f, 1.f));

			CTileScript* pTileScript = new CTileScript;
			pTileObj->AddComponent(pTileScript);
			pTileScript->SetTileDesc(*pDesc, typeIdx);

			_TileMapObject->AddChild(pTileObj);
		}
	}
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

	// 광원 추가
	pObject = new GameObject;
	pObject->SetName(L"Light 1");
	pObject->AddComponent(new CTransform);
	pObject->AddComponent(new CLight2D);

	pObject->Light2D()->SetLightType(LIGHT_TYPE::DIRECTIONAL);
	pObject->Light2D()->SetLightColor(Vec3(1.f, 1.f, 1.f));
	pObject->Light2D()->SetRadius(400.f);
	pObject->Light2D()->SetAngle(XM_PI / 2.f);

	pObject->Transform()->SetRelativePos(Vec3(-250.f, 0.f, 0.f));
	pObject->Transform()->SetRelativeRot(Vec3(0.f, 0.f, XM_PI / 4.f));

	pLevel->AddObject(0, pObject);

	// 몬스터 생성
	for (int i = 0; i < 5; ++i)
	{
		Ptr<GameObject> pMonster = new GameObject;
		pMonster->SetName(L"Monster");

		pMonster->AddComponent(new CTransform);
		pMonster->AddComponent(new CMeshRender);
		pMonster->AddComponent(new CCollider2D);
		pMonster->AddComponent(new CMonsterScript);

		pMonster->Transform()->SetRelativePos(Vec3(300.f * (float)i, 0.f, 100.f));
		pMonster->Transform()->SetRelativeScale(Vec3(200.f, 200.f, 0.f));

		pMonster->MeshRender()->SetMesh(FIND(AMesh, L"RectMesh"));
		pMonster->MeshRender()->SetMaterial(FIND(AMaterial, L"MonsterMtrl"));

		pLevel->AddObject(5, pMonster);
	}

	// 몬스터 생성
	Ptr<GameObject> pMonster = new GameObject;
	pMonster->SetName(L"Monster");

	pMonster->AddComponent(new CTransform);
	pMonster->AddComponent(new CSpriteRender);
	pMonster->AddComponent(new CCollider2D);

	pMonster->Transform()->SetRelativePos(Vec3(300.f, 100.f, 100.f));
	pMonster->Transform()->SetRelativeScale(Vec3(50.f, 50.f, 0.f));
	pMonster->SpriteRender()->SetSprite(FIND(ASprite, L"TileSprite_47"));

	pLevel->AddObject(5, pMonster);

	// Player Object 추가
	pObject = new GameObject;
	pObject->SetName(L"Player");
	pObject->AddComponent(new CTransform);
	pObject->AddComponent(new CFlipbookRender);
	pObject->AddComponent(new CCollider2D);

	Ptr<CPlayerScript> pPlayerScript = new CPlayerScript;
	pPlayerScript->SetTarget(pMonster);

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


	// Tile Object
	Ptr<ATileMap> pTileMapAsset = FIND(ATileMap, L"TestTileMap");

	if (pTileMapAsset != nullptr)
	{
		Ptr<GameObject> pTileMapObj = new GameObject;
		pTileMapObj->SetName(L"TileMapRender");

		pTileMapObj->AddComponent(new CTransform);
		pTileMapObj->AddComponent(new CTileRender);

		UINT Col = pTileMapAsset->GetCol();
		UINT Row = pTileMapAsset->GetRow();
		Vec2 TileSize = pTileMapAsset->GetTileSize();

		// 가로 = Col, 세로 = Row

		pTileMapObj->Transform()->SetRelativeScale(Vec3(TileSize.x * (float)Col, TileSize.y * (float)Row, 1.f));

		// 원하는 맵 위치

		pTileMapObj->Transform()->SetRelativePos(Vec3(0.f, 0.f, 500.f));

		pTileMapObj->TileRender()->SetTileMap(pTileMapAsset);
		pTileMapObj->TileRender()->SetOpacity(0.9f);

		pLevel->AddObject(2, pTileMapObj);
		RebuildTileCollision(pTileMapObj.Get());
	}

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

	////// 스프링2
	//pObject = new GameObject;
	//pObject->SetName(L"Spring3");

	//pObject->AddComponent(new CTransform);
	//pObject->AddComponent(new CCollider2D);
	//pObject->AddComponent(new CFlipbookRender);
	//pObject->AddComponent(new CSpringScript);

	////pObject->Collider2D()->SetOffset(Vec2(-0.3f, 0.0f));
	////pObject->Collider2D()->SetScale(Vec2(0.5f, 1.f));

	//// pObject->Transform()->SetRelativePos(Vec3(-50.f, 320.f, 9.f));
	////pObject->Transform()->SetRelativeScale(Vec3(80.f, 80.f, 0.f));
	////pObject->Transform()->SetRelativeRot(Vec3(0.f, 0.f, 0.f));
	//// pObject->FlipbookRender()->AddFlipbook(FIND(AFlipbook, L"Spring"));

	//pLevel->AddObject(5, pObject);

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
		CBlockPushingScript* pMoveScript = new CBlockPushingScript;
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

	// 스프링: 시작 지점 근처에서 바로 보이도록 배치
	SpawnSpring(Vec3(120.f, -260.f, 9.f), 0.f);
	SpawnSpring(Vec3(520.f, -220.f, 9.f), 0.f);

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
		
	// 레벨을 변경
	ChangeLevel(L"TestLevel");
}
