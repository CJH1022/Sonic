#include "pch.h"
#include "TextureUI.h"

#include "ATexture.h"

TextureUI::TextureUI()
	: AssetUI(ASSET_TYPE::TEXTURE)
{
}

TextureUI::~TextureUI()
{
}

void TextureUI::Tick_UI()
{
	OutputTitle();

	Ptr<ATexture> pTexture = (ATexture*)GetTargetAsset().Get();

	string Key = string(pTexture->GetKey().begin(), pTexture->GetKey().end());

	ImGui::Text("Name");
	ImGui::SameLine(120);
	ImGui::InputText("##TextureName", Key.data(), Key.length() + 1, ImGuiInputTextFlags_ReadOnly);


	// 이미지 샘플		
	ImGui::ImageWithBg((ImTextureRef)pTexture->GetSRV().Get()
					  , ImVec2(200, 200)
					  , Vec2(0.f, 0.f), Vec2(1.f, 1.f)
					  , ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

	// 해상도
	int Width = pTexture->GetWidth();
	int Height = pTexture->GetHeight();

	ImGui::Text("Resolution");
	ImGui::SameLine(150);
	ImGui::SetNextItemWidth(100);
	ImGui::InputInt("##Width", &Width, 0, 0, ImGuiInputTextFlags_ReadOnly);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	ImGui::InputInt("##Height", &Height, 0, 0, ImGuiInputTextFlags_ReadOnly);
}