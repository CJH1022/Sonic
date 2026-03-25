#include "pch.h"
#include "Inspector.h"

#include "LevelMgr.h"
#include "GameObject.h"



Inspector::Inspector()
	: EditorUI("Inspector")
{
	CreateChildUI();

	SetTargetObject(nullptr);
}

Inspector::~Inspector()
{
}

void Inspector::SetTargetObject(Ptr<GameObject> _Object)
{
	// 입력된 게임오브젝트의 정보를 보여줄 ComponentUI 들을 활성화 시킨다.
	m_TargetObject = _Object;

	for (UINT i = 0; i < (UINT)COMPONENT_TYPE::END; ++i)
	{
		if (nullptr == m_arrComUI[i])
			continue;

		m_arrComUI[i]->SetTarget(m_TargetObject);
	}

	if (nullptr != m_TargetObject)
	{
		const vector<Ptr<CScript>>& vecScripts = m_TargetObject->GetScripts();

		if (m_vecScriptUI.size() < vecScripts.size())
		{
			int AddCount = (int)vecScripts.size() - (int)m_vecScriptUI.size();

			for (int i = 0; i < AddCount; ++i)
			{
				ScriptUI* pScriptUI = new ScriptUI;
				pScriptUI->SetSizeAsChild(Vec2(0.f, 150.f));
				AddChildUI(pScriptUI);

				m_vecScriptUI.push_back(pScriptUI);
			}
		}

		for (size_t i = 0; i < m_vecScriptUI.size(); ++i)
		{
			if (vecScripts.size() <= i)
				m_vecScriptUI[i]->SetScript(nullptr);
			else
				m_vecScriptUI[i]->SetScript(vecScripts[i].Get());
		}
	}
	else
	{
		for (size_t i = 0; i < m_vecScriptUI.size(); ++i)
		{
			m_vecScriptUI[i]->SetScript(nullptr);
		}
	}
		
	// AssetUI 를 비활성화한다.
	m_TargetAsset = nullptr;
	for (UINT i = 0; i < (UINT)ASSET_TYPE::END; ++i)
	{
		if(nullptr != m_arrAssetUI[i])
			m_arrAssetUI[i]->SetActive(false);
	}
}

void Inspector::SetTargetAsset(Ptr<Asset> _Asset)
{
	// ComponentUI 들을 비활성화 시킨다.
	SetTargetObject(nullptr);


	// 입력된 에셋 담당 UI 를 활성화시킨다.
	m_TargetAsset = _Asset;
	if (nullptr == m_TargetAsset)
	{
		for (UINT i = 0; i < (UINT)ASSET_TYPE::END; ++i)
		{
			m_arrAssetUI[i]->SetActive(false);
		}
	}

	else
	{
		ASSET_TYPE Type = m_TargetAsset->GetType();
		m_arrAssetUI[(UINT)Type]->SetActive(true);
		m_arrAssetUI[(UINT)Type]->SetTargetAsset(m_TargetAsset);
	}
}

void Inspector::Tick_UI()
{
	if (nullptr == m_TargetObject)
	{
		if (nullptr == m_TargetAsset)
		{
			ImGui::TextWrapped("Select an object in Outliner or an asset in Content.");
			ImGui::Spacing();
			ImGui::TextWrapped("Tile editing tools live on the scene object that owns CTileRender, usually named TileMapRender.");
		}

		return;
	}

	wstring Name = m_TargetObject->GetName();
	string strName = string(Name.begin(), Name.end());

	if (strName.empty())
		strName = "No Name";

	ImGui::Button(strName.c_str());

	ImGui::Separator();
}

