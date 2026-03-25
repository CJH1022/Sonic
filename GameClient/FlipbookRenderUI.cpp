#include "pch.h"
#include "FlipbookRenderUI.h"

#include "AssetMgr.h"
#include "EditorMgr.h"
#include "ListUI.h"

#include "assets.h"

void FlipbookRenderUI::Tick_UI()
{
	OutputTitle("FlipbookRender");

	Ptr<CFlipbookRender> pFlipbookRender = GetTarget()->FlipbookRender();

	ImGui::Text("Flipbook");
	ImGui::SameLine(120);

	Ptr<AFlipbook> pFlipbook = pFlipbookRender->GetCurFlipbook();
	string FlipbookKey = "None";
	if (nullptr != pFlipbook)
		FlipbookKey = string(pFlipbook->GetKey().begin(), pFlipbook->GetKey().end());

	ImGui::InputText("##FlipbookName", FlipbookKey.data(), FlipbookKey.length() + 1, ImGuiInputTextFlags_ReadOnly);

	if (ImGui::BeginDragDropTarget())
	{
		const ImGuiPayload* PayLoad = ImGui::AcceptDragDropPayload("Content");
		if (PayLoad)
		{
			DWORD_PTR data = *((DWORD_PTR*)PayLoad->Data);
			Ptr<Asset> pAsset = (Asset*)data;

			if (ASSET_TYPE::FLIPBOOK == pAsset->GetType())
			{
				SetCurrentFlipbook((AFlipbook*)pAsset.Get());
			}
		}

		ImGui::EndDragDropTarget();
	}

	ImGui::SameLine();
	if (ImGui::Button("##FlipbookBtn", Vec2(20.f, 20.f)))
	{
		Ptr<ListUI> pUI = dynamic_cast<ListUI*>(EditorMgr::GetInst()->FindUI("ListUI").Get());
		assert(pUI.Get());

		pUI->SetUIName("Flipbook List");

		vector<wstring> vecFlipbookNames;
		AssetMgr::GetInst()->GetAssetNames(ASSET_TYPE::FLIPBOOK, vecFlipbookNames);
		pUI->AddString(vecFlipbookNames);
		pUI->AddDelegate(this, (DELEGATE_1)&FlipbookRenderUI::SelectFlipbook);
		pUI->SetActive(true);
	}

	ImGui::Text("Current Slot");
	ImGui::SameLine(120);
	ImGui::Text("%d", pFlipbookRender->GetCurFlipbookIdx());

	ImGui::Text("Current Sprite");
	ImGui::SameLine(120);
	ImGui::Text("%d", pFlipbookRender->GetCurSpriteIdx());

	ImGui::Text("FPS");
	ImGui::SameLine(120);
	ImGui::Text("%.2f", pFlipbookRender->GetFPS());

	ImGui::Text("Repeat");
	ImGui::SameLine(120);
	ImGui::Text("%d", pFlipbookRender->GetRepeatCount());

	ImGui::Text("Finished");
	ImGui::SameLine(120);
	ImGui::Text("%s", pFlipbookRender->IsFinish() ? "True" : "False");
}

FlipbookRenderUI::FlipbookRenderUI()
	: ComponentUI(COMPONENT_TYPE::FLIPBOOK_RENDER, "FlipbookRenderUI")
{
}

FlipbookRenderUI::~FlipbookRenderUI()
{
}

void FlipbookRenderUI::SelectFlipbook(DWORD_PTR _ListUI)
{
	Ptr<ListUI> pListUI = ((ListUI*)_ListUI);

	wstring key = wstring(pListUI->GetSelectedString().begin(), pListUI->GetSelectedString().end());

	Ptr<AFlipbook> pFlipbook = FIND(AFlipbook, key);

	SetCurrentFlipbook(pFlipbook);
}

void FlipbookRenderUI::SetCurrentFlipbook(Ptr<AFlipbook> _Flipbook)
{
	if (nullptr == _Flipbook)
		return;

	Ptr<CFlipbookRender> pFlipbookRender = GetTarget()->FlipbookRender();
	int CurIdx = pFlipbookRender->GetCurFlipbookIdx();

	if (CurIdx < 0)
		CurIdx = 0;

	pFlipbookRender->SetFlipbook(CurIdx, _Flipbook);
}
