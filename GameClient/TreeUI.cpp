#include "pch.h"
#include "TreeUI.h"

// ========
// TreeNode
// ========
TreeNode::TreeNode()
	: Parent(nullptr)
	, m_Owner(nullptr)
	, Framed(false)	
{
	// 각 노드가 표기하려는 이름이 같을 수가 있기 때문에, 보여주려는 이름 뒤에 붙을 고유 문자열을 미리 생성해둔다.
	char szKey[50] = {};
	sprintf_s(szKey, 50, "##TreeNode%d", GetID());	
	Key = szKey;
}

TreeNode::~TreeNode()
{
}


void TreeNode::Tick()
{
	// TreeNode Flag 설정
	UINT Flags = ImGuiTreeNodeFlags_SpanFullWidth		// 클릭 판정범위 확장
			   | ImGuiTreeNodeFlags_OpenOnDoubleClick	// 더블 클릭으로만 열리기
			   | ImGuiTreeNodeFlags_OpenOnArrow;		// 화살표 누르면 열리기			   
			  

	// 노드가 자식노드를 보유하고 있지 않으면 Leaf 플래그 추가
	if (vecChildNode.empty())
		Flags |= ImGuiTreeNodeFlags_Leaf;

	if(m_Owner->GetSelected() == this)
		Flags |= ImGuiTreeNodeFlags_Selected;

	if (Framed)
		Flags |= ImGuiTreeNodeFlags_Framed;

	string NodeName = Str + Key;

	if (Framed && vecChildNode.empty())
		NodeName = "   " + NodeName;

	// 트리노드에 등록한 문자열을 Key 로 해서 출력
	if (ImGui::TreeNodeEx(NodeName.c_str(), Flags))
	{
		ClickCheck();

		DragCheck();

		DropCheck();

		for (size_t i = 0; i < vecChildNode.size(); ++i)
		{
			vecChildNode[i]->Tick();
		}

		ImGui::TreePop();
	}
	else
	{
		ClickCheck();

		DragCheck();

		DropCheck();
	}
}

void TreeNode::ClickCheck()
{
	if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		m_Owner->RegisterSelected(this);
	}
}

void TreeNode::DragCheck()
{
	// 드래그
	if (ImGui::BeginDragDropSource())
	{		
		// Drag 사이에 Text 를 넣어주면, 드래그중인 마우스 위치에 Text 가 따라다니면서 렌더링된다.
		ImGui::Text(Str.c_str());

		if (0 != Data)
		{
			ImGui::SetDragDropPayload(m_Owner->GetParentUI()->GetUIName().c_str()
									, &Data, sizeof(DWORD_PTR));
		}

		m_Owner->RegisterDragged(this);

		ImGui::EndDragDropSource();
	}
}

void TreeNode::DropCheck()
{
	if (ImGui::BeginDragDropTarget())
	{
		const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(m_Owner->GetDropKey().c_str());

		if (Payload)
		{
			// Drag Drop 성공
			m_Owner->RegisterDropped(this);
		}

		ImGui::EndDragDropTarget();
	}
}



// ======
// TreeUI
// ======
TreeUI::TreeUI()
	: EditorUI("TreeUI")
	, m_Inst(nullptr)
	, m_MemFunc(nullptr)
	, m_DDInst(nullptr)
	, m_DDMemFunc(nullptr)
{
}

TreeUI::~TreeUI()
{
}

void TreeUI::Tick_UI()
{
	for (size_t i = 0; i < m_vecNode.size(); ++i)
	{
		m_vecNode[i]->Tick();
	}

	// Drag 하던 노드를 특정 노드에 Drop 함
	if (   (m_DragNode.Get() && m_DropNode.Get()) 
		|| (m_DragNode.Get() && ImGui::IsMouseReleased(ImGuiMouseButton_Left)))
	{
		if (m_DDInst && m_DDMemFunc)
		{
			(m_DDInst->*m_DDMemFunc)((DWORD_PTR)m_DragNode.Get(), (DWORD_PTR)m_DropNode.Get());
		}

		m_DragNode = nullptr;
		m_DropNode = nullptr;
	}
}

Ptr<TreeNode> TreeUI::AddItem(Ptr<TreeNode> _ParentNode, string _String, DWORD_PTR _Data)
{
	Ptr<TreeNode> pNewNode = new TreeNode;
	pNewNode->Str = _String;
	pNewNode->m_Owner = this;
	pNewNode->Data = _Data;

	// 최상위 부모노드로 추가
	if (nullptr == _ParentNode)
	{
		m_vecNode.push_back(pNewNode);
	}

	// 특정 노드 및에 자식으로 추가
	else
	{
		_ParentNode->AddChildNode(pNewNode);
	}

	return pNewNode;
}

void TreeUI::RegisterSelected(Ptr<TreeNode> _Node)
{
	m_Selected = _Node;
	
	if (m_Inst && m_MemFunc)
	{
		(m_Inst->*m_MemFunc)(_Node->Data);
	}	
}
