#include "pch.h"
#include "SpriteUI.h"

#include "ASprite.h"

SpriteUI::SpriteUI()
	: AssetUI(ASSET_TYPE::SPRITE)
{
}

SpriteUI::~SpriteUI()
{
}

void SpriteUI::Tick_UI()
{
	OutputTitle();

	Ptr<ASprite> pSprite = (ASprite*)GetTargetAsset().Get();

	string Key = string(pSprite->GetKey().begin(), pSprite->GetKey().end());

	ImGui::Text("Name");
	ImGui::SameLine(120);
	ImGui::InputText("##SpriteName", Key.data(), Key.length() + 1, ImGuiInputTextFlags_ReadOnly);

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();

	// Sprite 에 Atlas 텍스쳐가 세팅되어있는지 확인
	Ptr<ATexture> pAtlas = pSprite->GetAtlas();
	bool IsAtlas = pAtlas.Get();

	// Atlas 이름
	string AtlasName = "None";
	if(IsAtlas)
		AtlasName = string(pAtlas->GetKey().begin(), pAtlas->GetKey().end());

	ImGui::Text("Atlas Name");
	ImGui::SameLine(120);
	ImGui::InputText("##AtlasName", AtlasName.data(), AtlasName.length() + 1, ImGuiInputTextFlags_ReadOnly);


	// Sprite UV 정보
	Vec2 LeftTopUV = pSprite->GetLeftTopUV();
	Vec2 SliceUV = pSprite->GetSliceUV();
	Vec2 BackgroundUV = pSprite->GetBackgroundUV();
	Vec2 OffsetUV = pSprite->GetOffsetUV();
	
	ImGui::Text("LeftTop");
	ImGui::BeginDisabled(!IsAtlas);
	{		
		if (IsAtlas)
		{
			int pixel[2] = { LeftTopUV.x * (float)pAtlas->GetWidth()
						  ,  LeftTopUV.y * (float)pAtlas->GetHeight() };

			if (ImGui::InputInt2("##LeftTop", pixel))
			{
				pSprite->SetLeftTopUV(Vec2((float)pixel[0] / (float)pAtlas->GetWidth() 
										 , (float)pixel[1] / (float)pAtlas->GetHeight()));
			}
		}
		else
		{
			int pixel[2] = { 0, 0 };
			ImGui::InputInt2("##LeftTop", pixel);
		}		
	}	
	ImGui::EndDisabled();

	ImGui::Text("Slice");	
	ImGui::BeginDisabled(!IsAtlas);
	{		
		if (IsAtlas)
		{
			int pixel[2] = { SliceUV.x * (float)pAtlas->GetWidth()
						  ,  SliceUV.y * (float)pAtlas->GetHeight() };

			if (ImGui::InputInt2("##Slice", pixel))
			{
				pSprite->SetSliceUV(Vec2((float)pixel[0] / (float)pAtlas->GetWidth() 
									   , (float)pixel[1] / (float)pAtlas->GetHeight()));
			}
		}
		else
		{
			int pixel[2] = { 0, 0 };
			ImGui::InputInt2("##Slice", pixel);
		}		
	}	
	ImGui::EndDisabled();


	ImGui::Text("Background");	
	ImGui::BeginDisabled(!IsAtlas);
	{		
		if (IsAtlas)
		{
			int pixel[2] = { BackgroundUV.x * (float)pAtlas->GetWidth()
						  ,  BackgroundUV.y * (float)pAtlas->GetHeight() };

			if (ImGui::InputInt2("##Background", pixel))
			{
				pSprite->SetBackgroundUV(Vec2((float)pixel[0] / (float)pAtlas->GetWidth() 
										    , (float)pixel[1] / (float)pAtlas->GetHeight()));
			}
		}
		else
		{
			int pixel[2] = { 0, 0 };
			ImGui::InputInt2("##Background", pixel);
		}		
	}	
	ImGui::EndDisabled();

	ImGui::Text("Offset");	
	ImGui::BeginDisabled(!IsAtlas);
	{		
		if (IsAtlas)
		{
			int pixel[2] = { OffsetUV.x * (float)pAtlas->GetWidth()
						  ,  OffsetUV.y * (float)pAtlas->GetHeight() };

			if (ImGui::InputInt2("##Offset", pixel))
			{
				pSprite->SetOffsetUV(Vec2((float)pixel[0] / (float)pAtlas->GetWidth() 
										, (float)pixel[1] / (float)pAtlas->GetHeight()));
			}
		}
		else
		{
			int pixel[2] = { 0, 0 };
			ImGui::InputInt2("##Offset", pixel);
		}		
	}	
	ImGui::EndDisabled();
}
