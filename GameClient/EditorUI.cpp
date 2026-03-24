#include "pch.h"
#include "EditorUI.h"

#include "ImGui/imgui.h"
#include "EditorMgr.h"


EditorUI::EditorUI(const string& _Name)
	: m_UIName(_Name)
	, m_IsModal(false)
	, m_Separator(true)
	, m_Active(true)
	, m_Parent(nullptr)
{
}

EditorUI::~EditorUI()
{
}

void EditorUI::Tick()
{
	if (m_IsModal)
	{
		string StrKey = m_UIName + m_UIKey;
		ImGui::OpenPopup(StrKey.c_str());

		bool Active = m_Active;
		
		if (ImGui::BeginPopupModal(StrKey.c_str(), &Active, ImGuiWindowFlags_AlwaysAutoResize))
		{
			CheckFocus();

			Tick_UI();

			for (size_t i = 0; i < m_ChildUI.size(); ++i)
			{
				if (m_ChildUI[i]->IsActive())
				{
					m_ChildUI[i]->Tick();
					ImGui::Separator();
				}
			}

			ImGui::EndPopup();
		}
		else
		{			
			SetActive(Active);
		}
	}

	else if (nullptr == m_Parent)
	{
		bool Active = m_Active;

		string StrKey = m_UIName + m_UIKey;

		ImGui::Begin(StrKey.c_str(), &Active);

		if (m_Active != Active)
		{
			SetActive(Active);
		}

		CheckFocus();

		Tick_UI();

		for (size_t i = 0; i < m_ChildUI.size(); ++i)
		{
			if (m_ChildUI[i]->IsActive())
			{
				m_ChildUI[i]->Tick();				
			}			
		}

		ImGui::End();

		
	}

	else
	{
		ImGui::BeginChild(m_UIName.c_str(), m_SizeAsChild);

		CheckFocus();

		Tick_UI();

		for (size_t i = 0; i < m_ChildUI.size(); ++i)
		{
			if (m_ChildUI[i]->IsActive())
			{
				m_ChildUI[i]->Tick();
				ImGui::Separator();
			}				
		}

		ImGui::EndChild();

		if (m_Separator)
		{
			ImGui::Separator();
		}		
	}	
}

void EditorUI::CheckFocus()
{
	if (ImGui::IsWindowFocused())
		EditorMgr::GetInst()->RegisterFocusedUI(this);
}

Vec2::operator ImVec2() const
{
	return ImVec2(x, y);
}

Vec4::operator ImVec4() const
{
	return ImVec4(x, y, z, w);
}
