#include "pch.h"
#include "Outliner.h"

#include "TreeUI.h"
#include "LevelMgr.h"
#include "ALevel.h"

#include "EditorMgr.h"
#include "Inspector.h"


Outliner::Outliner()
	: EditorUI("Outliner")
{
	m_Tree = new TreeUI;
	m_Tree->SetSaperator(false);
	m_Tree->AddDynamicSelect(this, (DELEGATE_1)&Outliner::SelectGameObject);

	m_Tree->SetDropKey("Outliner"); // Self DragDrop 사용
	m_Tree->AddDynamicDragDrop(this, (DELEGATE_2)&Outliner::AddChild);

	AddChildUI(m_Tree.Get());	
}

Outliner::~Outliner()
{
}

void Outliner::Tick_UI()
{
	Ptr<ALevel> pCurLevel = LevelMgr::GetInst()->GetCurLevel();
	if (nullptr != pCurLevel)
	{
		if (pCurLevel->IsChanged())
		{
			Ptr<Inspector> pInspector = (Inspector*)EditorMgr::GetInst()->FindUI("Inspector").Get();
			Ptr<GameObject> pPrevTarget = nullptr;
			if (nullptr != pInspector)
			{
				pPrevTarget = pInspector->GetTargetObject();
			}

			Renew();

			// 타일 편집처럼 레벨이 자주 dirty 되는 작업에서도 Inspector 선택이
			// 매번 날아가면 기능이 없는 것처럼 보인다.
			// 아직 살아 있는 오브젝트면 선택을 유지하고, 정말 사라졌을 때만 해제한다.
			if (nullptr != pInspector)
			{
				if (nullptr != pPrevTarget && !pPrevTarget->IsDead())
					pInspector->SetTargetObject(pPrevTarget);
				else
					pInspector->SetTargetObject(nullptr);
			}
		}
	}
}

void Outliner::Renew()
{
	// 트리에 표기된 오브젝트 정보를 전부 삭제
	m_Tree->Clear();
	m_vecAddedObject.clear();

	// 현재 레벨을 가져옴
	Ptr<ALevel> pLevel = LevelMgr::GetInst()->GetCurLevel();
	if (nullptr == pLevel)
		return;	

	// 32개의 레이어를 순회
	for (UINT i = 0; i < MAX_LAYER; ++i)
	{
		// 각 레이어에 등록된 최상위 부모 오브젝트를 가져옴
		const vector<Ptr<GameObject>>& vecParents = pLevel->GetLayer(i)->GetParentObjects();

		// 최상위 부모 오브젝트들을 트리에 추가한다.
		for (const auto& Object : vecParents)
		{
			if (nullptr == Object || Object->IsDead())
				continue;

			// Layer 쪽 루트 목록이 잠깐 꼬여도, 실제 부모가 있는 오브젝트는
			// 루트로 다시 그리지 않는다.
			if (nullptr != Object->GetParent())
				continue;

			AddGameObject(nullptr, Object);
		}
	}	
}

bool Outliner::HasAddedObject(GameObject* _Object) const
{
	if (nullptr == _Object)
		return false;

	for (size_t i = 0; i < m_vecAddedObject.size(); ++i)
	{
		if (m_vecAddedObject[i] == _Object)
			return true;
	}

	return false;
}

void Outliner::AddGameObject(Ptr<TreeNode> _ParentNode, Ptr<GameObject> _Object)
{
	if (nullptr == _Object || _Object->IsDead())
		return;

	// 동일한 GameObject 포인터는 트리에 한 번만 추가한다.
	// 부모/레이어 등록이 꼬여도 Outliner 에서는 중복 노드가 생기지 않게 막는다.
	if (HasAddedObject(_Object.Get()))
		return;

	m_vecAddedObject.push_back(_Object.Get());

	string ObjectName = string(_Object->GetName().begin(), _Object->GetName().end());

	if (ObjectName.empty())
		ObjectName = "No Name";

	Ptr<TreeNode> pNewNode = m_Tree->AddItem(_ParentNode, ObjectName.c_str(), (DWORD_PTR)_Object.Get());

	for (size_t i = 0; i < _Object->GetChild().size(); ++i)
	{
		AddGameObject(pNewNode, _Object->GetChild()[i]);
	}
}

void Outliner::SelectGameObject(DWORD_PTR _Object)
{
	Ptr<GameObject> pSelectedObject = (GameObject*)_Object;

	Ptr<Inspector> pInspector = (Inspector*)EditorMgr::GetInst()->FindUI("Inspector").Get();
	assert(pInspector.Get());

	pInspector->SetTargetObject(pSelectedObject);
}

void Outliner::AddChild(DWORD_PTR _Src, DWORD_PTR _Dest)
{
	Ptr<TreeNode> pDragNode = (TreeNode*)_Src;
	Ptr<TreeNode> pDropNode = (TreeNode*)_Dest;

	if (nullptr == pDragNode)
		return;

	Ptr<GameObject> SrcObj = (GameObject*)pDragNode->Data;
	Ptr<GameObject> DestObj = nullptr;

	if (nullptr != pDropNode)
	{
		DestObj = (GameObject*)pDropNode->Data;
	}

	// 목적지가 없고, 
	if (nullptr == DestObj)
	{
		// 자식타입 오브젝트인 경우
		if (nullptr != SrcObj->GetParent())
		{
			// 자식오브젝트를 최상위 부모 타입으로 뺀다
			SrcObj->DisconnectWithParent();
			SrcObj->RegisterAsParent();
		}		
	}
	else
	{
		// SrcObj 가 DestObj 의 Ancetor 이면 안된다.
		if (SrcObj == DestObj)
			return;

		if (DestObj->IsDescendantOf(SrcObj))
			return;

		DestObj->AddChild(SrcObj);
	}	
}
