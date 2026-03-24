#include "pch.h"
#include "FlipbookUI.h"


FlipbookUI::FlipbookUI()
	: AssetUI(ASSET_TYPE::FLIPBOOK)
{
}

FlipbookUI::~FlipbookUI()
{
}

void FlipbookUI::Tick_UI()
{
	OutputTitle();
}