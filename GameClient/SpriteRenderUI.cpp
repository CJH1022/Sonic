#include "pch.h"
#include "SpriteRenderUI.h"

#include "AssetMgr.h"
#include "EditorMgr.h"
#include "ListUI.h"

#include "assets.h"
void SpriteRenderUI::Tick_UI()
{
	OutputTitle("SpriteRender");

	Ptr<CSpriteRender> pSpriteRender = GetTarget()->SpriteRender();

	ImGui::Text("Sprite");
	ImGui::SameLine(120);

	Ptr<ASprite> pSprite = pSpriteRender->GetSprite();
	string SprtieKey = string(pSprite->GetKey().begin(), pSprite->GetKey().end());
	ImGui::InputText("##SpriteName", SprtieKey.data(), SprtieKey.length() + 1, ImGuiInputTextFlags_ReadOnly);

	// 특정 위젯에서 드래그가 발생했고, 해당 위젯 위에 마우스가 호버링 중인지
	if (ImGui::BeginDragDropTarget())
	{
		const ImGuiPayload* PayLoad = ImGui::AcceptDragDropPayload("Content");
		if (PayLoad)
		{
			DWORD_PTR data = *((DWORD_PTR*)PayLoad->Data);
			Ptr<Asset> pAsset = (Asset*)data;

			if (ASSET_TYPE::SPRITE == pAsset->GetType())
			{
				pSpriteRender->SetSprite((ASprite*)pAsset.Get());
			}
		}

		ImGui::EndDragDropTarget();
	}

	ImGui::SameLine();
	if (ImGui::Button("##SpriteBtn", Vec2(20.f, 20.f)))
	{
		// 버튼이 눌리면, 리스트UI 를 찾아서 활성화 시키고, 출력시키고 싶은 문자열을 ListUI 에 등록시킨다.
		Ptr<ListUI> pUI = dynamic_cast<ListUI*>(EditorMgr::GetInst()->FindUI("ListUI").Get());
		assert(pUI.Get());

		pUI->SetUIName("Sprite List");

		vector<wstring> vecSpriteNames;
		AssetMgr::GetInst()->GetAssetNames(ASSET_TYPE::SPRITE, vecSpriteNames);
		pUI->AddString(vecSpriteNames);
		pUI->AddDelegate(this, (DELEGATE_1)&SpriteRenderUI::SelectSprite);
		pUI->SetActive(true);
	}
}

SpriteRenderUI::SpriteRenderUI()
	: ComponentUI(COMPONENT_TYPE::SPRITE_RENDER, "SpriteRenderUI")
{
}

SpriteRenderUI::~SpriteRenderUI()
{
}

void SpriteRenderUI::SelectSprite(DWORD_PTR _ListUI)
{
	Ptr<ListUI> pListUI = ((ListUI*)_ListUI);

	wstring key = wstring(pListUI->GetSelectedString().begin(), pListUI->GetSelectedString().end());

	Ptr<ASprite> pSprite = FIND(ASprite, key);

	GetTarget()->SpriteRender()->SetSprite(pSprite);
}