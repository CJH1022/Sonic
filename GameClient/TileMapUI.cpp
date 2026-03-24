#include "pch.h"
#include "TileMapUI.h"


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
}