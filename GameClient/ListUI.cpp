#include "pch.h"
#include "ListUI.h"

ListUI::ListUI()
	: EditorUI("ListUI")
	, m_SelectedIdx(-1)
	, m_Inst(nullptr)
	, m_MemFunc(nullptr)
{
	SetUIKey("##ListUI");
}

ListUI::~ListUI()
{
}

void ListUI::Tick_UI()
{
	// ListUI 에 등록된 문자열들을 TreeNode 위젯으로 출력
	for (size_t i = 0; i < m_vecList.size(); ++i)
	{
		// TreeNode Flag 설정
		UINT Flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth;

		// 선택된 문자열은 Selected 플래그 추가
		if (i == m_SelectedIdx)
			Flags |= ImGuiTreeNodeFlags_Selected;

		// 트리노드에 등록한 문자열을 Key 로 해서 출력
		if (ImGui::TreeNodeEx(m_vecList[i].c_str(), Flags))
		{
			// 해당 위젯이 클릭된 적이 있으면
			if (ImGui::IsItemClicked())
			{
				// 선택된 상태로 인덱스를 기억
				m_SelectedIdx = i;
				m_SelectedString = m_vecList[i];
			}

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				SetActive(false);

				if (m_Inst && m_MemFunc)
					(m_Inst->*m_MemFunc)((DWORD_PTR)this);
			}

			ImGui::TreePop();
		}
	}
}

void ListUI::Activate()
{
	m_SelectedString.clear();
}

void ListUI::Deactivate()
{
	m_vecList.clear();
}