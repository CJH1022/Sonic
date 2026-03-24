#include "pch.h"
#include "GraphicShaderUI.h"


GraphicShaderUI::GraphicShaderUI()
	: AssetUI(ASSET_TYPE::GRAPHICSHADER)
{
}

GraphicShaderUI::~GraphicShaderUI()
{
}

void GraphicShaderUI::Tick_UI()
{
	OutputTitle();
}