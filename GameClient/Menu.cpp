#include "pch.h"
#include "Menu.h"

#include "AssetMgr.h"
#include "EditorMgr.h"
#include "ContentUI.h"
#include "LevelMgr.h"

#include "CCollider2D.h"
#include "CFlipbookRender.h"
#include "CSpriteRender.h"
#include "CTransform.h"
#include "Inspector.h"
#include "GameObject.h"

#include "Source/ScriptMgr.h"
#include "Source/Scripts/CBlockMovingScript.h"
#include "Source/Scripts/CBlockPushingScript.h"
#include "Source/Scripts/CBlockScript.h"
#include "Source/Scripts/CCylinderScript.h"
#include "Source/Scripts/CSpikeScript.h"
#include "Source/Scripts/CSpringScript.h"
#include "func.h"

namespace
{
	bool FileExists(const wstring& _Path)
	{
		DWORD Attr = GetFileAttributesW(_Path.c_str());
		return Attr != INVALID_FILE_ATTRIBUTES && !(Attr & FILE_ATTRIBUTE_DIRECTORY);
	}

	bool HasExtension(const wstring& _Path, const wstring& _Ext)
	{
		if (_Path.length() < _Ext.length())
			return false;

		return _Path.substr(_Path.length() - _Ext.length()) == _Ext;
	}

	wstring BuildLevelRelativePath(const wstring& _Key)
	{
		if (_Key.empty())
			return L"";

		wstring RelativePath = _Key;
		if (RelativePath.find(L'\\') == wstring::npos && RelativePath.find(L'/') == wstring::npos)
		{
			RelativePath = L"Level\\" + RelativePath;
		}

		if (!HasExtension(RelativePath, L".lv"))
		{
			RelativePath += L".lv";
		}

		return RelativePath;
	}

	bool CanEditSceneObjects()
	{
		return (nullptr != LevelMgr::GetInst()->GetCurLevel())
			&& (LevelMgr::GetInst()->GetLevelState() == LEVEL_STATE::STOP);
	}

	bool ObjectHasScriptNamed(Ptr<GameObject> _Object, const wstring& _ScriptName)
	{
		if (nullptr == _Object)
			return false;

		const vector<Ptr<CScript>>& vecScripts = _Object->GetScripts();
		for (size_t i = 0; i < vecScripts.size(); ++i)
		{
			if (_ScriptName == ScriptMgr::GetScriptName(vecScripts[i].Get()))
				return true;
		}

		return false;
	}
}

Menu::Menu()
	: EditorUI("Menu")
{
}

Menu::~Menu()
{
}

void Menu::Tick_UI()
{
}


void Menu::Tick()
{
	if (ImGui::BeginMainMenuBar())
	{
		File();

		Level();
		
		View();

		GameObjectMenu();

		Asset();		

		ImGui::EndMainMenuBar();
	}
}




void Menu::File()
{
	if (ImGui::BeginMenu("File"))
	{
		Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();
		const bool HasLevel = (nullptr != pLevel);

		if (ImGui::MenuItem("Level Save", nullptr, nullptr, HasLevel))
		{
			wstring LevelKey = pLevel->GetKey();
			if (LevelKey.empty())
			{
				if (!pLevel->GetRelativePath().empty())
					LevelKey = pLevel->GetRelativePath();
				else
					LevelKey = GetAssetName(ASSET_TYPE::LEVEL, L"Level\\Level");
			}

			wstring RelativePath = pLevel->GetRelativePath();
			if (RelativePath.empty())
			{
				RelativePath = BuildLevelRelativePath(LevelKey);
			}

			CreateDirectoryW((wstring(CONTENT_PATH) + L"Level").c_str(), nullptr);

			wstring FilePath = wstring(CONTENT_PATH) + RelativePath;
			if (SUCCEEDED(pLevel->Save(FilePath)))
			{
				AssetMgr::GetInst()->Load<ALevel>(LevelKey, RelativePath);
				ChangeLevel(LevelKey);
			}
		}

		if (ImGui::MenuItem("Level Load", nullptr, nullptr, HasLevel))
		{
			wstring LevelKey = pLevel->GetKey();
			wstring RelativePath = pLevel->GetRelativePath();

			if (RelativePath.empty() && !LevelKey.empty())
			{
				RelativePath = BuildLevelRelativePath(LevelKey);
			}

			if (LevelKey.empty())
			{
				LevelKey = RelativePath;
			}

			if (!RelativePath.empty())
			{
				wstring FilePath = wstring(CONTENT_PATH) + RelativePath;
				if (FileExists(FilePath))
				{
					AssetMgr::GetInst()->Load<ALevel>(LevelKey, RelativePath);
					ChangeLevel(LevelKey);
				}
			}
		}

		ImGui::EndMenu();
	}
}

void Menu::Level()
{
	if (ImGui::BeginMenu("Level"))
	{
		bool HasLevel = LevelMgr::GetInst()->GetCurLevel().Get();
		bool IsPlay = false, IsPause = false, IsStop = false;
		if (HasLevel)
		{
			LEVEL_STATE CurState = LevelMgr::GetInst()->GetLevelState();
			if (LEVEL_STATE::PLAY == CurState)
				IsPlay = true;
			else if (LEVEL_STATE::PAUSE == CurState)
				IsPause = true;
			else if (LEVEL_STATE::STOP == CurState)
				IsStop = true;
		}

		if (ImGui::MenuItem("Play", nullptr, nullptr, HasLevel && !IsPlay))
		{
			ChangeLevelState(LEVEL_STATE::PLAY);
		}

		if (ImGui::MenuItem("Pause", nullptr, nullptr, HasLevel && IsPlay))
		{
			ChangeLevelState(LEVEL_STATE::PAUSE);
		}

		if (ImGui::MenuItem("Stop", nullptr, nullptr, HasLevel && !IsStop))
		{
			ChangeLevelState(LEVEL_STATE::STOP);
		}

		ImGui::EndMenu();
	}
}

void Menu::View()
{
	if (ImGui::BeginMenu("View"))
	{
		bool ShowDemo = EditorMgr::GetInst()->IsShowDemo();
		if (ImGui::MenuItem("Demo", nullptr, &ShowDemo, true))
		{
			EditorMgr::GetInst()->ShowDemo(ShowDemo);
		}

		Ptr<EditorUI> pInspector = EditorMgr::GetInst()->FindUI("Inspector");
		bool InspectorActive = pInspector->IsActive();
		if (ImGui::MenuItem("Inspector", nullptr, &InspectorActive))
		{
			pInspector->SetActive(InspectorActive);
		}

		Ptr<EditorUI> pOutliner = EditorMgr::GetInst()->FindUI("Outliner");
		bool OutlinerActive = pOutliner->IsActive();
		if (ImGui::MenuItem("Outliner", nullptr, &OutlinerActive))
		{
			pOutliner->SetActive(OutlinerActive);
		}

		Ptr<EditorUI> pImageEditor = EditorMgr::GetInst()->FindUI("IMAGE_EDITOR");
		if (pImageEditor != nullptr)
		{
			bool ImageEditorActive = pImageEditor->IsActive();
			if (ImGui::MenuItem("Image Editor", nullptr, &ImageEditorActive))
			{
				pImageEditor->SetActive(ImageEditorActive);
			}
		}

		Ptr<EditorUI> pMapEditor = EditorMgr::GetInst()->FindUI("MapEditor");
		if (pMapEditor != nullptr)
		{
			bool MapEditorActive = pMapEditor->IsActive();
			if (ImGui::MenuItem("Map Editor", "F10", &MapEditorActive))
			{
				pMapEditor->SetActive(MapEditorActive);
			}
		}

		ImGui::EndMenu();
	}
}

void Menu::GameObjectMenu()
{
	if (ImGui::BeginMenu("GameObject"))
	{
		const bool canEditObjects = CanEditSceneObjects();
		Ptr<Inspector> pInspector = (Inspector*)EditorMgr::GetInst()->FindUI("Inspector").Get();
		Ptr<GameObject> pObject = (nullptr != pInspector) ? pInspector->GetTargetObject() : nullptr;

		if (ImGui::MenuItem("Create Block", nullptr, nullptr, canEditObjects))
		{
			CreateBlockObject();
		}

		if (ImGui::MenuItem("Create Moving Block", nullptr, nullptr, canEditObjects))
		{
			CreateMovingBlockObject();
		}

		if (ImGui::MenuItem("Create Pushing Block", nullptr, nullptr, canEditObjects))
		{
			CreatePushingBlockObject();
		}

		if (ImGui::BeginMenu("Create Spike"))
		{
			if (ImGui::MenuItem("Up", nullptr, nullptr, canEditObjects))
			{
				CreateSpikeObject(0.f, L"SpikeUp_");
			}

			if (ImGui::MenuItem("Left", nullptr, nullptr, canEditObjects))
			{
				CreateSpikeObject(XM_PIDIV2, L"SpikeLeft_");
			}

			if (ImGui::MenuItem("Right", nullptr, nullptr, canEditObjects))
			{
				CreateSpikeObject(-XM_PIDIV2, L"SpikeRight_");
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Create Spring"))
		{
			if (ImGui::MenuItem("Up", nullptr, nullptr, canEditObjects))
			{
				CreateSpringObject(XM_PIDIV2, L"SpringUp_");
			}

			if (ImGui::MenuItem("Left", nullptr, nullptr, canEditObjects))
			{
				CreateSpringObject(XM_PI, L"SpringLeft_");
			}

			if (ImGui::MenuItem("Right", nullptr, nullptr, canEditObjects))
			{
				CreateSpringObject(0.f, L"SpringRight_");
			}

			ImGui::EndMenu();
		}

		if (ImGui::MenuItem("Create Cylinder", nullptr, nullptr, canEditObjects))
		{
			CreateCylinderObject();
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Delete Selected Object", "Del", nullptr, canEditObjects && nullptr != pObject))
		{
			DeleteSelectedObject();
		}

		if (ImGui::BeginMenu("Add Script"))
		{
			vector<wstring> vecScriptName;
			ScriptMgr::GetScriptInfo(vecScriptName);

			for (const auto& ScriptName : vecScriptName)
			{
				const bool canAddScript = canEditObjects && nullptr != pObject && !ObjectHasScriptNamed(pObject, ScriptName);
				if (ImGui::MenuItem(string(ScriptName.begin(), ScriptName.end()).c_str(), nullptr, nullptr, canAddScript))
				{
					CScript* pNewScript = ScriptMgr::GetScript(ScriptName);
					if (nullptr != pNewScript)
					{
						pObject->AddComponent(pNewScript);
						pObject->RegisterAsParent();
						LevelMgr::GetInst()->GetCurLevel()->SetChanged();
						if (nullptr != pInspector)
							pInspector->SetTargetObject(pObject);
					}
				}
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenu();
	}
}

void Menu::CreateCylinderObject()
{
	Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();
	if (nullptr == pLevel || LevelMgr::GetInst()->GetLevelState() != LEVEL_STATE::STOP)
		return;

	Ptr<GameObject> pCylinder = new GameObject;
	pCylinder->SetName(L"Cylinder_" + std::to_wstring(pCylinder->GetID()));
	pCylinder->AddComponent(new CTransform);
	pCylinder->AddComponent(new CCollider2D);
	pCylinder->AddComponent(new CSpriteRender);
	pCylinder->AddComponent(new CCylinderScript);

	pCylinder->Transform()->SetRelativePos(Vec3(0.f, 0.f, 9.f));
	pCylinder->Transform()->SetRelativeScale(Vec3(260.f, 420.f, 1.f));
	pCylinder->Collider2D()->SetOffset(Vec2(0.f, 0.f));
	pCylinder->Collider2D()->SetScale(Vec2(1.f, 1.f));

	Ptr<ASprite> pCylinderSprite = LOAD(ASprite, L"Sprite\\Sonic_CylinderTree.sprite");
	if (pCylinderSprite != nullptr)
	{
		pCylinder->SpriteRender()->SetSprite(pCylinderSprite);
	}

	pLevel->AddObject(5, pCylinder);
	pLevel->SetChanged();

	Ptr<Inspector> pInspector = (Inspector*)EditorMgr::GetInst()->FindUI("Inspector").Get();
	if (pInspector != nullptr)
		pInspector->SetTargetObject(pCylinder);
}

void Menu::CreateBlockObject()
{
	Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();
	if (nullptr == pLevel || LevelMgr::GetInst()->GetLevelState() != LEVEL_STATE::STOP)
		return;

	Ptr<GameObject> pBlock = new GameObject;
	pBlock->SetName(L"Block_" + std::to_wstring(pBlock->GetID()));
	pBlock->AddComponent(new CTransform);
	pBlock->AddComponent(new CCollider2D);
	pBlock->AddComponent(new CSpriteRender);
	pBlock->AddComponent(new CBlockScript);

	pBlock->Transform()->SetRelativePos(Vec3(0.f, 0.f, 9.f));
	pBlock->Transform()->SetRelativeScale(Vec3(128.f, 128.f, 1.f));
	pBlock->Collider2D()->SetOffset(Vec2(0.f, 0.f));
	pBlock->Collider2D()->SetScale(Vec2(1.f, 1.f));

	Ptr<ASprite> pSprite = LOAD(ASprite, L"Sprite\\Block_Mid.sprite");
	if (nullptr != pSprite)
	{
		pBlock->SpriteRender()->SetSprite(pSprite);
	}

	pLevel->AddObject(5, pBlock);
	pLevel->SetChanged();

	Ptr<Inspector> pInspector = (Inspector*)EditorMgr::GetInst()->FindUI("Inspector").Get();
	if (nullptr != pInspector)
		pInspector->SetTargetObject(pBlock);
}

void Menu::CreateMovingBlockObject()
{
	Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();
	if (nullptr == pLevel || LevelMgr::GetInst()->GetLevelState() != LEVEL_STATE::STOP)
		return;

	Ptr<GameObject> pBlock = new GameObject;
	pBlock->SetName(L"MovingBlock_" + std::to_wstring(pBlock->GetID()));
	pBlock->AddComponent(new CTransform);
	pBlock->AddComponent(new CCollider2D);
	pBlock->AddComponent(new CSpriteRender);

	CBlockMovingScript* pMovingScript = new CBlockMovingScript;
	pMovingScript->SetStartPos(Vec3(0.f, 0.f, 0.f));
	pMovingScript->SetEndPos(Vec3(250.f, 0.f, 0.f));
	pMovingScript->SetVelocity(Vec2(120.f, 0.f));
	pBlock->AddComponent(pMovingScript);

	pBlock->Transform()->SetRelativePos(Vec3(0.f, 0.f, 9.f));
	pBlock->Transform()->SetRelativeScale(Vec3(160.f, 64.f, 1.f));
	pBlock->Collider2D()->SetOffset(Vec2(0.f, 0.f));
	pBlock->Collider2D()->SetScale(Vec2(1.f, 1.f));

	Ptr<ASprite> pSprite = LOAD(ASprite, L"Sprite\\Block_Move.sprite");
	if (nullptr != pSprite)
	{
		pBlock->SpriteRender()->SetSprite(pSprite);
	}

	pLevel->AddObject(5, pBlock);
	pLevel->SetChanged();

	Ptr<Inspector> pInspector = (Inspector*)EditorMgr::GetInst()->FindUI("Inspector").Get();
	if (nullptr != pInspector)
		pInspector->SetTargetObject(pBlock);
}

void Menu::CreatePushingBlockObject()
{
	Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();
	if (nullptr == pLevel || LevelMgr::GetInst()->GetLevelState() != LEVEL_STATE::STOP)
		return;

	Ptr<GameObject> pBlock = new GameObject;
	pBlock->SetName(L"PushingBlock_" + std::to_wstring(pBlock->GetID()));
	pBlock->AddComponent(new CTransform);
	pBlock->AddComponent(new CCollider2D);
	pBlock->AddComponent(new CSpriteRender);
	pBlock->AddComponent(new CBlockPushingScript);

	pBlock->Transform()->SetRelativePos(Vec3(0.f, 0.f, 9.f));
	pBlock->Transform()->SetRelativeScale(Vec3(128.f, 128.f, 1.f));
	pBlock->Collider2D()->SetOffset(Vec2(0.f, 0.f));
	pBlock->Collider2D()->SetScale(Vec2(1.f, 1.f));

	Ptr<ASprite> pSprite = LOAD(ASprite, L"Sprite\\Block_Tall.sprite");
	if (nullptr != pSprite)
	{
		pBlock->SpriteRender()->SetSprite(pSprite);
	}

	pLevel->AddObject(5, pBlock);
	pLevel->SetChanged();

	Ptr<Inspector> pInspector = (Inspector*)EditorMgr::GetInst()->FindUI("Inspector").Get();
	if (nullptr != pInspector)
		pInspector->SetTargetObject(pBlock);
}

void Menu::CreateSpikeObject(float _RotationZ, const wchar_t* _NamePrefix)
{
	Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();
	if (nullptr == pLevel || LevelMgr::GetInst()->GetLevelState() != LEVEL_STATE::STOP)
		return;

	Ptr<GameObject> pSpike = new GameObject;
	pSpike->SetName(wstring(_NamePrefix) + std::to_wstring(pSpike->GetID()));
	pSpike->AddComponent(new CTransform);
	pSpike->AddComponent(new CCollider2D);
	pSpike->AddComponent(new CSpriteRender);
	pSpike->AddComponent(new CSpikeScript);

	pSpike->Transform()->SetRelativePos(Vec3(0.f, 0.f, 9.f));
	pSpike->Transform()->SetRelativeScale(Vec3(150.f, 150.f, 1.f));
	pSpike->Transform()->SetRelativeRot(Vec3(0.f, 0.f, _RotationZ));
	pSpike->Collider2D()->SetOffset(Vec2(0.f, 0.f));
	pSpike->Collider2D()->SetScale(Vec2(1.f, 1.f));

	Ptr<ASprite> pSprite = LOAD(ASprite, L"Sprite\\Spike.sprite");
	if (nullptr != pSprite)
	{
		pSpike->SpriteRender()->SetSprite(pSprite);
	}

	pLevel->AddObject(5, pSpike);
	pLevel->SetChanged();

	Ptr<Inspector> pInspector = (Inspector*)EditorMgr::GetInst()->FindUI("Inspector").Get();
	if (nullptr != pInspector)
		pInspector->SetTargetObject(pSpike);
}

void Menu::CreateSpringObject(float _RotationZ, const wchar_t* _NamePrefix)
{
	Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();
	if (nullptr == pLevel || LevelMgr::GetInst()->GetLevelState() != LEVEL_STATE::STOP)
		return;

	Ptr<GameObject> pSpring = new GameObject;
	pSpring->SetName(wstring(_NamePrefix) + std::to_wstring(pSpring->GetID()));
	pSpring->AddComponent(new CTransform);
	pSpring->AddComponent(new CCollider2D);
	pSpring->AddComponent(new CFlipbookRender);
	pSpring->AddComponent(new CSpringScript);

	pSpring->Transform()->SetRelativePos(Vec3(0.f, 0.f, 9.f));
	pSpring->Transform()->SetRelativeScale(Vec3(96.f, 96.f, 1.f));
	pSpring->Transform()->SetRelativeRot(Vec3(0.f, 0.f, _RotationZ));
	pSpring->Collider2D()->SetOffset(Vec2(0.f, 0.f));
	pSpring->Collider2D()->SetScale(Vec2(0.9f, 0.9f));

	Ptr<AFlipbook> pFlipbook = LOAD(AFlipbook, L"Flipbook\\Spring.flip");
	if (nullptr == pFlipbook)
	{
		pFlipbook = LOAD(AFlipbook, L"Flipbook\\Default Flipbook_0.flip");
	}

	if (nullptr != pFlipbook)
	{
		pSpring->FlipbookRender()->SetFlipbook(0, pFlipbook);
	}

	pLevel->AddObject(5, pSpring);
	pLevel->SetChanged();

	Ptr<Inspector> pInspector = (Inspector*)EditorMgr::GetInst()->FindUI("Inspector").Get();
	if (nullptr != pInspector)
		pInspector->SetTargetObject(pSpring);
}

void Menu::DeleteSelectedObject()
{
	if (!CanEditSceneObjects())
		return;

	Ptr<Inspector> pInspector = (Inspector*)EditorMgr::GetInst()->FindUI("Inspector").Get();
	if (nullptr == pInspector)
		return;

	Ptr<GameObject> pTarget = pInspector->GetTargetObject();
	if (nullptr == pTarget)
		return;

	pTarget->Destroy();
	pInspector->SetTargetObject(nullptr);
}

void Menu::Asset()
{
	if (ImGui::BeginMenu("Asset"))
	{
		Ptr<Inspector> pInspector = (Inspector*)EditorMgr::GetInst()->FindUI("Inspector").Get();
		Ptr<GameObject> pTargetObject = nullptr;
		if (nullptr != pInspector)
		{
			pTargetObject = pInspector->GetTargetObject();
		}

		if (ImGui::BeginMenu("Create Asset"))
		{
			if (ImGui::MenuItem("Create Material"))
			{
				CreateDirectoryW((wstring(CONTENT_PATH) + L"Material").c_str(), nullptr);
				Ptr<AMaterial> pMtrl = new AMaterial;
				wstring Key = GetAssetName(ASSET_TYPE::MATERIAL, L"Material\\Default Material");
				AssetMgr::GetInst()->AddAsset(Key, pMtrl.Get());

				// [추가] 파일 저장 및 인스펙터 포커스
				pMtrl->Save(wstring(CONTENT_PATH) + Key);
				if (pInspector != nullptr) pInspector->SetTargetAsset(pMtrl.Get());
			}

			if (ImGui::MenuItem("Create Sprite"))
			{
				CreateDirectoryW((wstring(CONTENT_PATH) + L"Sprite").c_str(), nullptr);
				Ptr<ASprite> pSprite = new ASprite;
				wstring Key = GetAssetName(ASSET_TYPE::SPRITE, L"Sprite\\Default Sprite");
				AssetMgr::GetInst()->AddAsset(Key, pSprite.Get());

				// [추가] 파일 저장 및 인스펙터 포커스
				pSprite->Save(wstring(CONTENT_PATH) + Key);
				if (pInspector != nullptr) pInspector->SetTargetAsset(pSprite.Get());
			}

			if (ImGui::MenuItem("Create Flipbook"))
			{
				CreateDirectoryW((wstring(CONTENT_PATH) + L"Flipbook").c_str(), nullptr);
				Ptr<AFlipbook> pFlipbook = new AFlipbook;
				wstring Key = GetAssetName(ASSET_TYPE::FLIPBOOK, L"Flipbook\\Default Flipbook");
				AssetMgr::GetInst()->AddAsset(Key, pFlipbook.Get());

				// [추가] 파일 저장 및 인스펙터 포커스
				pFlipbook->Save(wstring(CONTENT_PATH) + Key);
				if (pInspector != nullptr) pInspector->SetTargetAsset(pFlipbook.Get());
			}

			if (ImGui::MenuItem("Create TileMap"))
			{
				CreateDirectoryW((wstring(CONTENT_PATH) + L"TileMap").c_str(), nullptr);
				Ptr<ATileMap> pTileMap = new ATileMap;
				// [수정] 오타(pTileMap) 수정
				wstring Key = GetAssetName(ASSET_TYPE::TILEMAP, L"TileMap\\Default TileMap");
				AssetMgr::GetInst()->AddAsset(Key, pTileMap.Get());

				// [추가] 파일 저장 및 인스펙터 포커스
				pTileMap->Save(wstring(CONTENT_PATH) + Key);
				if (pInspector != nullptr) pInspector->SetTargetAsset(pTileMap.Get());
			}

			if (ImGui::MenuItem("Create Surface Set"))
			{
				CreateDirectoryW((wstring(CONTENT_PATH) + L"SurfaceSet").c_str(), nullptr);
				Ptr<ASurfaceSet> pSurfaceSet = new ASurfaceSet;
				wstring Key = GetAssetName(ASSET_TYPE::SURFACESET, L"SurfaceSet\\Default Surface Set");
				AssetMgr::GetInst()->AddAsset(Key, pSurfaceSet.Get());

				pSurfaceSet->Save(wstring(CONTENT_PATH) + Key);
				if (pInspector != nullptr) pInspector->SetTargetAsset(pSurfaceSet.Get());
			}

			if (ImGui::MenuItem("Create Prefab", nullptr, nullptr, nullptr != pTargetObject))
			{
				CreateDirectoryW((wstring(CONTENT_PATH) + L"Prefab").c_str(), nullptr);

				Ptr<APrefab> pPrefab = new APrefab;
				pPrefab->SetObject(pTargetObject->Clone());

				wstring Key = GetAssetName(ASSET_TYPE::PREFAB, L"Prefab\\Default Prefab");
				AssetMgr::GetInst()->AddAsset(Key, pPrefab.Get());

				wstring FilePath = wstring(CONTENT_PATH) + Key;
				if (SUCCEEDED(pPrefab->Save(FilePath)))
				{
					// [수정] 이미 저장 성공했으면 메모리(AssetMgr)에 있으므로, 쓸데없이 Load하지 않고 바로 세팅합니다.
					if (pInspector != nullptr) pInspector->SetTargetAsset(pPrefab.Get());
				}
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenu();
	}
}


wstring Menu::GetAssetName(ASSET_TYPE _Type, const wstring& _Name)
{
	wstring Ext;

	switch (_Type)
	{
	case ASSET_TYPE::MESH:
		Ext = L".mesh";
		break;
	case ASSET_TYPE::MATERIAL:
		Ext = L".mtrl";
		break;
	case ASSET_TYPE::TEXTURE:
	case ASSET_TYPE::SOUND:
	case ASSET_TYPE::GRAPHICSHADER:
	case ASSET_TYPE::COMPUTESHADER:
		assert(nullptr);
		break;
	case ASSET_TYPE::SPRITE:
		Ext = L".sprite";
		break;
	case ASSET_TYPE::FLIPBOOK:
		Ext = L".flip";
		break;
	case ASSET_TYPE::PREFAB:
		Ext = L".pref";
		break;
	case ASSET_TYPE::SURFACESET:
		Ext = L".sset";
		break;
	case ASSET_TYPE::LEVEL:
		Ext = L".lv";
		break;
	case ASSET_TYPE::TILEMAP:
		Ext = L".tile"; // 엔진에서 사용하는 타일맵 확장자로 맞춰주세요
		break;
	}

	int i = 0;
	while (true)
	{
		wchar_t Num[50] = {};
		swprintf_s(Num, 50, L"_%d", i);

		wstring AssetName = wstring(_Name + Num + Ext);
		if (nullptr == AssetMgr::GetInst()->FindAsset(_Type, AssetName))
		{
			return AssetName;
		}

		i++;
	}
}
