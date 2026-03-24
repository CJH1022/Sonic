#pragma once
#include "EditorUI.h"
#include "ASprite.h"
#include "AFlipbook.h"

enum class IMAGE_STATE
{
    ATLAS,
    SPRITE,
};

class ImageEditor :
    public EditorUI
{
private:
	bool m_bIsDragging;
	int m_iRows;
	int m_iCols;
	int m_iCurFrame;
	float m_fAccTime;

    Vec2    m_vSliceSize;

    Vec2   m_vRegionStart;
    Vec2   m_vRegionEnd;
    Vec2   m_vBackgroundStart;
    Vec2   m_vBackgroundSize;
    Vec2   m_vCellOffset;
    Vec2   m_vCellSpriteSize;

    bool  m_bPreviewPlaying;
    bool  m_bPreviewLoop;
    float m_fPreviewFPS;
    bool  m_bAutoTrimBlackBG;
    int   m_iBlackThreshold;
    int   m_iAlphaThreshold;

	Vec2 m_vStartPos;
	vector<int> m_vecSelectedIndex;

	ASSET_TYPE m_AssetType;
	IMAGE_STATE m_curState;

	Ptr<ASprite> m_pTargetSprite;
	Ptr<ATexture> m_pTargetAtlas;

	AFlipbook* m_pTargetFlipbook;   // 작업용
	Ptr<AFlipbook> m_pLoadedFlipbook; // 로드용

private:
    void LoadAllSprites();
    void LoadAllFlipbooks();
    void OutputTitle(ASSET_TYPE _AssetType)
    {
        ImGui::PushID(0);
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.f, 0.6f, 0.6f));
        ImGui::Button(ToString(m_AssetType));
        ImGui::PopStyleColor(3);
        ImGui::PopID();

        ImGui::Spacing();
        ImGui::Spacing();
    }

    wstring ToWString(const string& _str) 
    {
        return wstring(_str.begin(), _str.end());
    }

public:
    virtual void Tick_UI() override;

public:
    ImageEditor();
    ~ImageEditor();
};
