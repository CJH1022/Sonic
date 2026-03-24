#include "pch.h"
#include "SoundUI.h"


SoundUI::SoundUI()
	: AssetUI(ASSET_TYPE::SOUND)
{
}

SoundUI::~SoundUI()
{
}

void SoundUI::Tick_UI()
{
	OutputTitle();
}