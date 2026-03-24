#include "pch.h"
#include "Menu.h"

#include "AssetMgr.h"
#include "EditorMgr.h"
#include "ContentUI.h"
#include "LevelMgr.h"

#include "Inspector.h"
#include "GameObject.h"

#include "Source/ScriptMgr.h"

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
		if (ImGui::MenuItem("Level Save"))
		{
		}

		if (ImGui::MenuItem("Level Load"))
		{
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
		if (ImGui::BeginMenu("Create Asset"))
		{
			if (ImGui::MenuItem("Create Material"))
			{
				Ptr<AMaterial> pMtrl = new AMaterial;
				wstring Key = GetAssetName(ASSET_TYPE::MATERIAL, L"Material\\Default Material");
				AssetMgr::GetInst()->AddAsset(Key, pMtrl.Get());				
			}

			if (ImGui::MenuItem("Create Sprite"))
			{

			}

			if (ImGui::MenuItem("Create Flipbook"))
			{

			}

			if (ImGui::MenuItem("Create TileMap"))
			{

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