#include "pch.h"
#include "CameraUI.h"

#include "LevelMgr.h"

CameraUI::CameraUI()
	: ComponentUI(COMPONENT_TYPE::CAMERA, "CameraUI")
{
}

CameraUI::~CameraUI()
{
}

void CameraUI::Tick_UI()
{
	OutputTitle("Camera");

    int SameLine = 180;

    // ==========
    // LayerCheck
    // ==========
    if (ImGui::TreeNode("LayerCheck"))
    {
        UINT LayerCheck = GetTarget()->Camera()->GetLayerCheck();
        Ptr<ALevel> pCurLevel = LevelMgr::GetInst()->GetCurLevel();

        string strLayerName[MAX_LAYER] = {};
        bool selected[MAX_LAYER] = {};

        for (UINT i = 0; i < MAX_LAYER; ++i)
        {
            wstring Name = pCurLevel->GetLayer(i)->GetName();
            strLayerName[i] = string(Name.begin(), Name.end());

            if (strLayerName[i].empty())
            {
                char buff[255] = {};
                sprintf_s(buff, "None##%d", i);
                strLayerName[i] = buff;
            }
             
            selected[i] = LayerCheck & (1 << i);
        }
        
        if (ImGui::BeginTable("##LayerCheck", 1, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders))
        {
            for (int i = 0; i < 32; i++)
            {  
                ImGui::TableNextColumn();

                if (ImGui::Selectable(strLayerName[i].c_str(), &selected[i]))
                {
                    GetTarget()->Camera()->LayerCheck(i);
                }
            }
            ImGui::EndTable();
        }

        SetSizeAsChild(Vec2(0.f, 1100.f));

        ImGui::TreePop();
    }
    else
    {
        SetSizeAsChild(Vec2(0.f, 300.f));
    }

    ImGui::Spacing();
    ImGui::Spacing();

    // ==============
    // ProjectionType
    // ==============
    ImGui::Text("ProjectionType");
    ImGui::SameLine(SameLine);

    PROJ_TYPE ProjType = GetTarget()->Camera()->GetProjType();
    const char* items[] = { "ORTHOGRAPHIC", "PERSPECTIVE"};  
    ImGui::SetNextItemWidth(200);
    if (ImGui::Combo("##ProjType", (int*)&ProjType, items, 2))
    {
        GetTarget()->Camera()->SetProjType(ProjType);
    }

    // ===
    // FOV
    // ===
    ImGui::BeginDisabled(ProjType == PROJ_TYPE::ORTHOGRAPHIC);

    ImGui::Text("FOV");
    ImGui::SameLine(SameLine);
    float  FOV = GetTarget()->Camera()->GetFOV();
    ImGui::SetNextItemWidth(200);
    if (ImGui::DragFloat("##FOV", &FOV, 0.1f))
    {
        GetTarget()->Camera()->SetFOV(FOV);
    }    

    ImGui::EndDisabled();

    
    float       m_Far;          // 카메라 시야 최대거리
    float       m_Width;        // 직교투영 가로길이
    float       m_AspectRatio;  // 종횡비 (가로 / 세로), 세로대비 가로의 길이 비율   
    float       m_OrthoScale;   // 직교투영 배율
}