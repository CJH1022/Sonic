#include "pch.h"
#include "TileRenderUI.h"

#include "AssetMgr.h"
#include "EditorMgr.h"
#include "ListUI.h"

#include "assets.h"

TileRenderUI::TileRenderUI()
	: ComponentUI(COMPONENT_TYPE::TILE_RENDER, "TileRenderUI")
{
}

TileRenderUI::~TileRenderUI()
{
}


void TileRenderUI::Tick_UI()
{
	OutputTitle("TileRender");

	Ptr<CTileRender> pTileRender = GetTarget()->TileRender();

	ImGui::Text("TileMap");
	ImGui::SameLine(120);

	Ptr<ATileMap> pTileMap = pTileRender->GetTileMap();
	string TileMapKey = "None";
	if (nullptr != pTileMap)
		TileMapKey = string(pTileMap->GetKey().begin(), pTileMap->GetKey().end());

	ImGui::InputText("##TileMapName", TileMapKey.data(), TileMapKey.length() + 1, ImGuiInputTextFlags_ReadOnly);

	if (ImGui::BeginDragDropTarget())
	{
		const ImGuiPayload* PayLoad = ImGui::AcceptDragDropPayload("Content");
		if (PayLoad)
		{
			DWORD_PTR data = *((DWORD_PTR*)PayLoad->Data);
			Ptr<Asset> pAsset = (Asset*)data;

			if (ASSET_TYPE::TILEMAP == pAsset->GetType())
			{
				pTileRender->SetTileMap((ATileMap*)pAsset.Get());
			}
		}

		ImGui::EndDragDropTarget();
	}

	ImGui::SameLine();
	if (ImGui::Button("##TileMapBtn", Vec2(20.f, 20.f)))
	{
		Ptr<ListUI> pUI = dynamic_cast<ListUI*>(EditorMgr::GetInst()->FindUI("ListUI").Get());
		assert(pUI.Get());

		pUI->SetUIName("TileMap List");

		vector<wstring> vecTileMapNames;
		AssetMgr::GetInst()->GetAssetNames(ASSET_TYPE::TILEMAP, vecTileMapNames);
		pUI->AddString(vecTileMapNames);
		pUI->AddDelegate(this, (DELEGATE_1)&TileRenderUI::SelectTileMap);
		pUI->SetActive(true);
	}

	float Opacity = pTileRender->GetOpacity();
	ImGui::Text("Opacity");
	ImGui::SameLine(120);
	if (ImGui::DragFloat("##TileOpacity", &Opacity, 0.01f, 0.f, 1.f))
	{
		pTileRender->SetOpacity(Opacity);
	}

	if (nullptr != pTileMap)
	{
		UINT Row = pTileMap->GetRow();
		UINT Col = pTileMap->GetCol();
		Vec2 TileSize = pTileMap->GetTileSize();

		ImGui::Text("Row / Col");
		ImGui::SameLine(120);
		ImGui::Text("%u / %u", Row, Col);

		ImGui::Text("Tile Size");
		ImGui::SameLine(120);
		ImGui::Text("%.2f, %.2f", TileSize.x, TileSize.y);
	}
}

void TileRenderUI::SelectTileMap(DWORD_PTR _ListUI)
{
	Ptr<ListUI> pListUI = ((ListUI*)_ListUI);

	wstring key = wstring(pListUI->GetSelectedString().begin(), pListUI->GetSelectedString().end());

	Ptr<ATileMap> pTileMap = FIND(ATileMap, key);

	GetTarget()->TileRender()->SetTileMap(pTileMap);
}
