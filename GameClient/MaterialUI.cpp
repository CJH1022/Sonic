#include "pch.h"
#include "MaterialUI.h"

#include "PathMgr.h"
#include "AMaterial.h"
#include "EditorMgr.h"
#include "ListUI.h"
#include "AssetMgr.h"

MaterialUI::MaterialUI()
	: AssetUI(ASSET_TYPE::MATERIAL)
{
}

MaterialUI::~MaterialUI()
{
}

void MaterialUI::Tick_UI()
{
	OutputTitle();

	Ptr<AMaterial> pMtrl = (AMaterial*)GetTargetAsset().Get();

	string Key = string(pMtrl->GetKey().begin(), pMtrl->GetKey().end());

	ImGui::Text("Name");
	ImGui::SameLine(120);
	ImGui::InputText("##MtrlName", Key.data(), Key.length() + 1, ImGuiInputTextFlags_ReadOnly);

	// ======
	// Shader
	// ======
	ImGui::Text("Shader");
	ImGui::SameLine(120);

	Ptr<AGraphicShader> pShader = pMtrl->GetShader();

	wstring ShaderKey = L"None";
	if (nullptr != pShader)
	{
		ShaderKey = pShader->GetKey();
	}
	
	ImGui::InputText("##ShaderName", string(ShaderKey.begin(), ShaderKey.end()).data(), ShaderKey.length() + 1, ImGuiInputTextFlags_ReadOnly);

	// 특정 위젯에서 드래그가 발생했고, 해당 위젯 위에 마우스가 호버링 중인지
	if (ImGui::BeginDragDropTarget())
	{
		const ImGuiPayload* PayLoad = ImGui::AcceptDragDropPayload("Content");
		if (PayLoad)
		{
			DWORD_PTR data = *((DWORD_PTR*)PayLoad->Data);
			Ptr<Asset> pAsset = (Asset*)data;

			if (ASSET_TYPE::GRAPHICSHADER == pAsset->GetType())
			{
				pMtrl->SetShader((AGraphicShader*)pAsset.Get());
			}
		}

		ImGui::EndDragDropTarget();
	}

	ImGui::SameLine();
	if (ImGui::Button("##ShaderBtn", Vec2(20.f, 20.f)))
	{
		// 버튼이 눌리면, 리스트UI 를 찾아서 활성화 시키고, 출력시키고 싶은 문자열을 ListUI 에 등록시킨다.
		Ptr<ListUI> pUI = dynamic_cast<ListUI*>(EditorMgr::GetInst()->FindUI("ListUI").Get());
		assert(pUI.Get());

		pUI->SetUIName("Shader List");

		vector<wstring> vecShaderNames;
		AssetMgr::GetInst()->GetAssetNames(ASSET_TYPE::GRAPHICSHADER, vecShaderNames);
		pUI->AddString(vecShaderNames);
		pUI->AddDelegate(this, (DELEGATE_1)&MaterialUI::SelectShader);
		pUI->SetActive(true);
	}

	// Render Domain



	// Shader Param
	ShaderParameter();

	// Save Button
	ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
	if (ImGui::Button("Save##MtrlSaveBtn"))
	{
		wstring FilePath = CONTENT_PATH + pMtrl->GetKey();
		pMtrl->Save(FilePath);
	}
}

void MaterialUI::ShaderParameter()
{
	Ptr<AMaterial> pMtrl = (AMaterial*)GetTargetAsset().Get();
	if (nullptr == pMtrl->GetShader())
		return;

	ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
	ImGui::Text("Shader Parameter");
	ImGui::Separator();

	const vector<ShaderParam>& vecParam = pMtrl->GetShader()->GetShaderParam();

	for (size_t i = 0; i < vecParam.size(); ++i)
	{
		switch (vecParam[i].Type)
		{
		case SHADER_PARAM::INT:
			break;
		case SHADER_PARAM::FLOAT:
			break;
		case SHADER_PARAM::VEC2:
			break;
		case SHADER_PARAM::VEC4:
		{
			ImGui::Text(string(vecParam[i].Desc.begin(), vecParam[i].Desc.end()).c_str());
			ImGui::SameLine();

			SCALAR_PARAM Param = (SCALAR_PARAM)((UINT)SCALAR_PARAM::VEC4_0 + vecParam[i].Index);
			Vec4& Data = pMtrl->GetScalar<Vec4>(Param);
			ImGui::InputFloat4("##InputFloat4", Data);
		}
			break;
		case SHADER_PARAM::MAT:
			break;
		case SHADER_PARAM::TEX:
		{
			ImGui::Text(string(vecParam[i].Desc.begin(), vecParam[i].Desc.end()).c_str());
					

			Ptr<ATexture> pTex = pMtrl->GetTexture((TEX_PARAM)vecParam[i].Index);
			ImTextureRef SRV = nullptr;

			if (nullptr != pTex)
			{
				SRV = pTex->GetSRV().Get();
			}

			// 이미지 샘플
			ImGui::ImageWithBg(SRV , ImVec2(100, 100)
							, Vec2(0.f, 0.f), Vec2(1.f, 1.f)
							, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

			// 특정 위젯에서 드래그가 발생했고, 해당 위젯 위에 마우스가 호버링 중인지
			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* PayLoad = ImGui::AcceptDragDropPayload("Content");
				if (PayLoad)
				{
					DWORD_PTR data = *((DWORD_PTR*)PayLoad->Data);
					Ptr<Asset> pAsset = (Asset*)data;

					if (ASSET_TYPE::TEXTURE == pAsset->GetType())
					{
						pMtrl->SetTexture((TEX_PARAM)vecParam[i].Index, (ATexture*)pAsset.Get());
					}
				}

				ImGui::EndDragDropTarget();
			}
		}
			break;	
		}		
	}	
}

void MaterialUI::SelectShader(DWORD_PTR _ListUI)
{
	Ptr<ListUI> pListUI = ((ListUI*)_ListUI);

	wstring key = wstring(pListUI->GetSelectedString().begin(), pListUI->GetSelectedString().end());
		
	Ptr<AGraphicShader> pShader = FIND(AGraphicShader, key);

	((AMaterial*)GetTargetAsset().Get())->SetShader(pShader);
}
