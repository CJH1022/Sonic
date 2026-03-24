#include "pch.h"
#include "ComponentUI.h"



ComponentUI::ComponentUI(COMPONENT_TYPE _Type, const string& _Name)
	: EditorUI(_Name)
	, m_ComType(_Type)
{
}

ComponentUI::~ComponentUI()
{
}

void ComponentUI::SetTarget(Ptr<GameObject> _Object)
{
	m_Target = _Object;

	if (nullptr == m_Target || nullptr == m_Target->GetComponent(m_ComType))
		SetActive(false);
	else
		SetActive(true);
}

void ComponentUI::OutputTitle(const string& _Title)
{
	ImGui::PushID(0);
	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.f, 0.6f, 0.6f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.f, 0.6f, 0.6f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.f, 0.6f, 0.6f));
	ImGui::Button(_Title.c_str());
	ImGui::PopStyleColor(3);
	ImGui::PopID();

	ImGui::Spacing();
	ImGui::Spacing();
}