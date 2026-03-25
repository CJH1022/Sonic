#include "pch.h"
#include "TileMapUI.h"

#include "ATileMap.h"


TileMapUI::TileMapUI()
	: AssetUI(ASSET_TYPE::TILEMAP)
{
}

TileMapUI::~TileMapUI()
{
}

void TileMapUI::Tick_UI()
{
	OutputTitle();

	Ptr<ATileMap> pTileMap = (ATileMap*)GetTargetAsset().Get();
	if (nullptr == pTileMap)
		return;

	ImGui::Text("Key");
	ImGui::SameLine(120);
	string key = string(pTileMap->GetKey().begin(), pTileMap->GetKey().end());
	ImGui::TextWrapped("%s", key.c_str());

	ImGui::Text("Row / Col");
	ImGui::SameLine(120);
	ImGui::Text("%u / %u", pTileMap->GetRow(), pTileMap->GetCol());

	Vec2 tileSize = pTileMap->GetTileSize();
	ImGui::Text("Tile Size");
	ImGui::SameLine(120);
	ImGui::Text("%.2f, %.2f", tileSize.x, tileSize.y);

	ImGui::Spacing();
	ImGui::TextWrapped("Detailed paint tools live on the scene object that owns CTileRender.");
	ImGui::TextWrapped("Select TileMapRender in Outliner to open the full TileRenderUI editor.");
}
