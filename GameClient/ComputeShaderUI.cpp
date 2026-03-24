#include "pch.h"
#include "ComputeShaderUI.h"


ComputeShaderUI::ComputeShaderUI()
	: AssetUI(ASSET_TYPE::COMPUTESHADER)
{
}

ComputeShaderUI::~ComputeShaderUI()
{
}

void ComputeShaderUI::Tick_UI()
{
	OutputTitle();
}