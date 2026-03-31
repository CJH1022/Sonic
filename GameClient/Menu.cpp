#include "pch.h"
#include "Menu.h"

#include "AssetMgr.h"
#include "EditorMgr.h"
#include "ContentUI.h"
#include "LevelMgr.h"

#include "Inspector.h"
#include "GameObject.h"

#include "Source/ScriptMgr.h"
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
		if (ImGui::BeginMenu("Add Script"))
		{
			vector<wstring> vecScriptName;
			ScriptMgr::GetScriptInfo(vecScriptName);

			for (const auto& ScriptName : vecScriptName)
			{				
				if (ImGui::MenuItem(string(ScriptName.begin(), ScriptName.end()).c_str()))
				{					
					Ptr<Inspector> pInspector = (Inspector*)EditorMgr::GetInst()->FindUI("Inspector").Get();
					Ptr<GameObject> pObject = pInspector->GetTargetObject();

					if (nullptr != pObject)
					{
						CScript* pNewScript = ScriptMgr::GetScript(ScriptName);
						pObject->AddComponent(pNewScript);
					}					
				}				
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenu();
	}
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
