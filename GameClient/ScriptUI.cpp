#include "pch.h"
#include "ScriptUI.h"

#include "assets.h"
#include "Source/ScriptMgr.h"

ScriptUI::ScriptUI()
	: ComponentUI(COMPONENT_TYPE::SCRIPT, "ScriptUI")
	, m_ItemHeight(0)
{
	int idx = GetID();
	char szNum[50] = {};
	_itoa_s(idx, szNum, 50, 10);

	SetUIKey(szNum);
}

ScriptUI::~ScriptUI()
{
}

void ScriptUI::SetScript(CScript* _Script)
{
	m_TargetScript = _Script;

	if (nullptr == m_TargetScript)
		SetActive(false);
	else
		SetActive(true);
}

void ScriptUI::Tick_UI()
{
	m_ItemHeight = 0;

	ImGui::PushID(0);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.3f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.1f, 0.3f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.3f, 1.f));

	wstring WScriptName = ScriptMgr::GetScriptName(m_TargetScript.Get());
	string ScriptName = string(WScriptName.begin(), WScriptName.end());

	ImGui::Button(ScriptName.c_str());
	AddItemHeight();

	ImGui::PopStyleColor(3);
	ImGui::PopID();

	const vector<tScriptParam>& vecParam = m_TargetScript->GetScriptParam();

	for (size_t i = 0; i < vecParam.size(); ++i)
	{
		char ID[255] = {};
		sprintf_s(ID, 255, "%d", (int)i);

		switch (vecParam[i].Param)
		{
		case SCRIPT_PARAM::FLOAT:
		{
			ImGui::Text(string(vecParam[i].Desc.begin(), vecParam[i].Desc.end()).c_str());
			ImGui::SameLine(120);

			string Key = "##Float";
			Key += ID;

			if (vecParam[i].IsInput)
				ImGui::InputFloat(Key.c_str(), (float*)vecParam[i].Data, vecParam[i].Step);
			else
				ImGui::DragFloat(Key.c_str(), (float*)vecParam[i].Data, vecParam[i].Step);

			AddItemHeight();
		}
			break;
		case SCRIPT_PARAM::TEXTURE:
		{
			ImGui::Text(string(vecParam[i].Desc.begin(), vecParam[i].Desc.end()).c_str());
			AddItemHeight();

			Ptr<ATexture> pTex = *((Ptr<ATexture>*)vecParam[i].Data);

			ImTextureRef TexID = nullptr;
			if (nullptr != pTex)
			{
				TexID = (ImTextureRef)pTex->GetSRV().Get();
			}

			ImGui::ImageWithBg(TexID, ImVec2(200, 200)
							 , Vec2(0.f, 0.f), Vec2(1.f, 1.f)
							 , ImVec4(0.f, 0.f, 0.f, 1.f));

			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("Content");
				if (Payload)
				{
					DWORD_PTR data = *((DWORD_PTR*)Payload->Data);
					Ptr<Asset> pAsset = (Asset*)data;

					if (ASSET_TYPE::TEXTURE == pAsset->GetType())
					{
						*((Ptr<ATexture>*)vecParam[i].Data) = ((ATexture*)pAsset.Get());
					}
				}

				ImGui::EndDragDropTarget();
			}

			AddItemHeight();
		}
			break;
		case SCRIPT_PARAM::PREFAB:
		{
			ImGui::Text(string(vecParam[i].Desc.begin(), vecParam[i].Desc.end()).c_str());
			ImGui::SameLine(120);

			string Key = "##Prefab";
			Key += ID;

			Ptr<APrefab> pPrefab = *((Ptr<APrefab>*)vecParam[i].Data);
			string PrefabName = "None";
			if (nullptr != pPrefab)
			{
				PrefabName = string(pPrefab->GetKey().begin(), pPrefab->GetKey().end());
			}

			char Buffer[256] = {};
			strcpy_s(Buffer, PrefabName.c_str());
			ImGui::InputText(Key.c_str(), Buffer, sizeof(Buffer), ImGuiInputTextFlags_ReadOnly);
			AddItemHeight();

			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("Content");
				if (Payload)
				{
					DWORD_PTR data = *((DWORD_PTR*)Payload->Data);
					Ptr<Asset> pAsset = (Asset*)data;

					if (ASSET_TYPE::PREFAB == pAsset->GetType())
					{
						*((Ptr<APrefab>*)vecParam[i].Data) = ((APrefab*)pAsset.Get());
					}
				}

				ImGui::EndDragDropTarget();
			}

			AddItemHeight();
		}
			break;
		default:
			break;
		}
	}

	SetSizeAsChild(Vec2(0.f, (float)m_ItemHeight));
}

void ScriptUI::AddItemHeight()
{
	ImVec2 vSize = ImGui::GetItemRectSize();
	m_ItemHeight += (UINT)(vSize.y + 5.f);
}
