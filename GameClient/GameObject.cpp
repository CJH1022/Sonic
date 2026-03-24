#include "pch.h"
#include "GameObject.h"

#include "LevelMgr.h"
#include "TaskMgr.h"
#include "Source/ScriptMgr.h"


GameObject::GameObject()
	: m_Com{}
	, m_Parent(nullptr)
	, m_LayerIdx(-1)
	, m_Dead(false)	
{
}

GameObject::GameObject(const GameObject& _Origin)
	: Entity(_Origin)
	, m_Com{}
	, m_Parent(nullptr)
    , m_LayerIdx(-1)
    , m_Dead(false)
{
	// 원본 오브젝트와 동일한 세팅의 컴포넌트를 복사해서 나한테 넣어준다.
	for (UINT i = 0; i < (UINT)COMPONENT_TYPE::END; ++i)
	{
		if (nullptr == _Origin.m_Com[i])
			continue;

		AddComponent(_Origin.m_Com[i]->Clone());
	}

	// 원본 오브젝트와 동일한 스크립트를 복사해서 나한테 넣어준다.
	for (const auto& Script : _Origin.m_vecScripts)
	{
		AddComponent(Script->Clone());
	}

	// 원본 오브젝트가 보유한 자식 오브젝트를 복사해서 나한테 넣어준다.
	for (const auto& Child : _Origin.m_vecChild)
	{
		AddChild(Child->Clone());
	}
}

GameObject::~GameObject()
{
}

void GameObject::Begin()
{
	for (size_t i = 0; i < m_vecScripts.size(); ++i)
	{
		m_vecScripts[i]->Begin();
	}

	for (UINT i = 0; i < (UINT)COMPONENT_TYPE::END; ++i)
	{
		if (nullptr != m_Com[i])
			m_Com[i]->Begin();
	}

	for (size_t i = 0; i < m_vecChild.size(); ++i)
	{
		m_vecChild[i]->Begin();
	}
}

void GameObject::Tick()
{
	for (size_t i = 0; i < m_vecScripts.size(); ++i)
	{
		m_vecScripts[i]->Tick();
	}	
	
	for (size_t i = 0; i < m_vecChild.size(); ++i)
	{
		m_vecChild[i]->Tick();
	}
}

void GameObject::FinalTick()
{
	for (UINT i = 0; i < (UINT)COMPONENT_TYPE::END; ++i)
	{
		if(nullptr != m_Com[i])
			m_Com[i]->FinalTick();
	}

	// 자신이 소속된 Layer 에 자기자신을 알림(등록)
	RegisterLayer();

	// 자식 오브젝트 FinalTick 호출
	// 만약 Dead 상태인 자식 오브젝트가 있으면, Vector 에서 제거한다.
	vector<Ptr<GameObject>>::iterator iter = m_vecChild.begin();
	for (; iter != m_vecChild.end(); )
	{
		(*iter)->FinalTick();

		if ((*iter)->IsDead())
			iter = m_vecChild.erase(iter);
		else
			++iter;
	}
}

void GameObject::FinalTick_Editor()
{
	for (UINT i = 0; i < (UINT)COMPONENT_TYPE::END; ++i)
	{
		if (nullptr != m_Com[i])
			m_Com[i]->FinalTick();
	}

	// 자식 오브젝트 FinalTick 호출
	// 만약 Dead 상태인 자식 오브젝트가 있으면, Vector 에서 제거한다.
	vector<Ptr<GameObject>>::iterator iter = m_vecChild.begin();
	for (; iter != m_vecChild.end(); )
	{
		(*iter)->FinalTick();

		if ((*iter)->IsDead())
			iter = m_vecChild.erase(iter);
		else
			++iter;
	}
}

void GameObject::RegisterLayer()
{
	if (-1 == m_LayerIdx)
		return;

	Ptr<ALevel> pCurLevel = LevelMgr::GetInst()->GetCurLevel();
	if (nullptr == pCurLevel)
		return;

	Layer* pLayer = pCurLevel->GetLayer(m_LayerIdx);

	pLayer->RegisterObject(this);
}


void GameObject::Render()
{
	// 렌더링 관련 기능을 보유한 컴포넌트가 없으면 GameObejct 는 Rendering 될 수 없다.
	if (nullptr != m_RenderCom)
	{
		Transform()->Binding();
		m_RenderCom->Render();
	}

	for (size_t i = 0; i < m_vecChild.size(); ++i)
	{
		m_vecChild[i]->Render();
	}	
}

void GameObject::AddComponent(Ptr<Component> _Com)
{
	// 렌더링 기능 컴포넌트는 하나만 가질 수 있음
	if (dynamic_cast<CRenderComponent*>(_Com.Get()))
	{
		assert(!m_RenderCom.Get());

		m_RenderCom = (CRenderComponent*)_Com.Get();
	}
	
	// 입력으로 들어온 컴포넌트가 스크립트면, vector 로 관리
	if (_Com->GetType() == COMPONENT_TYPE::SCRIPT)
	{
		m_vecScripts.push_back((CScript*)_Com.Get());
	}

	// 입력으로 들어온 컴포넌트가 스크립트가 아니면, 알맞은 배열 포인터로 가리킴
	else
	{
		// 해당 컴포넌트를 이미 가지고 있지 않아야 한다.
		assert(nullptr == m_Com[(UINT)_Com->GetType()]);
		m_Com[(UINT)_Com->GetType()] = _Com;
	}
		
	_Com->m_Owner = this;

	_Com->Init();
}

void GameObject::AddChild(Ptr<GameObject> _Child)
{
	// 부모 오브젝트가 있는지 확인
	if (_Child->GetParent().Get())
	{
		// 기존 부모 오브젝트와 관계를 해제한다.
		_Child->DisconnectWithParent();
	}

	// 최상위 부모 오브젝트 였다면
	else
	{
		// 레벨 내부에 있던 오브젝트 라면
		if (_Child->m_LayerIdx != -1)
		{
			// Layer 에서 최상위 부모로 가리키던 포인터를 제거
			_Child->DeregisterAsParent();
		}		
	}

	m_vecChild.push_back(_Child); 
	_Child->m_Parent = this;

	// 자식으로 들어오려는 오브젝트가, 원래 이 레벨 소속이 아니라, 외부에서 들어온 경우
	if (_Child->m_LayerIdx == -1)
	{
		_Child->m_LayerIdx = m_LayerIdx;

		// 부모가 될 오브젝트는 레벨 내부 소속인 경우 + 레벨이 Play 모드
		if (m_LayerIdx != -1 && LEVEL_STATE::PLAY == LevelMgr::GetInst()->GetLevelState())
		{
			// Play 중인 레벨 안에 있는 어떤 오브젝트의 자식으로서 레벨에 합류했기 때문에, 
			// Begin 호출
			_Child->Begin();
		}
	}

	// 부모 오브젝트가 레벨 소속이면
	if (m_LayerIdx != -1)
	{
		Ptr<ALevel> pCurLevel = LevelMgr::GetInst()->GetCurLevel();
		if (nullptr != pCurLevel)
		{
			// 현재 레벨에 변경이 발생했음을 알림
			pCurLevel->SetChanged();
		}
	}
}

void GameObject::DisconnectWithParent()
{
	if (nullptr == m_Parent)
		return;

	if (m_LayerIdx != -1)
	{
		Ptr<ALevel> pCurLevel = LevelMgr::GetInst()->GetCurLevel();
		if (nullptr != pCurLevel)
		{
			pCurLevel->SetChanged();
		}
	}

	vector<Ptr<GameObject>>::iterator iter = m_Parent->m_vecChild.begin();

	for (; iter != m_Parent->m_vecChild.end(); ++iter)
	{
		if (*iter == this)
		{
			m_Parent->m_vecChild.erase(iter);
			m_Parent = nullptr;
			return;
		}		
	}

	assert(nullptr);
}

void GameObject::RegisterAsParent()
{
	if (m_LayerIdx == -1)
		return;

	LevelMgr::GetInst()->GetCurLevel()->GetLayer(m_LayerIdx)->AddObject(this);
}

void GameObject::DeregisterAsParent()
{
	Ptr<ALevel> pCurLevel = LevelMgr::GetInst()->GetCurLevel();

	Layer* pLayer = pCurLevel->GetLayer(m_LayerIdx);

	pLayer->DeregisterAsParent(this);		
}

void GameObject::Destroy()
{
	if (m_Dead) 
		return;

	TaskInfo info = {};

	info.Type = TASK_TYPE::DESTROY_OBJECT;
	info.Param_0 = (DWORD_PTR)this;

	TaskMgr::GetInst()->AddTask(info);
}

void GameObject::SaveToLevelFile(FILE* _File)
{
	SaveWString(_File, GetName());

	for (UINT i = 0; i < (UINT)COMPONENT_TYPE::END; ++i)
	{
		if (nullptr == m_Com[i])
			continue;

		fwrite(&i, sizeof(UINT), 1, _File);
		m_Com[i]->SaveToLevelFile(_File);
	}

	UINT ComEnd = (UINT)COMPONENT_TYPE::END;
	fwrite(&ComEnd, sizeof(UINT), 1, _File);

	size_t ScriptCount = m_vecScripts.size();
	fwrite(&ScriptCount, sizeof(size_t), 1, _File);

	for (const auto& Script : m_vecScripts)
	{
		wstring ScriptName = ScriptMgr::GetScriptName(Script.Get());
		SaveWString(_File, ScriptName);
		Script->SaveToLevelFile(_File);
	}

	size_t ChildCount = m_vecChild.size();
	fwrite(&ChildCount, sizeof(size_t), 1, _File);

	for (const auto& Child : m_vecChild)
	{
		Child->SaveToLevelFile(_File);
	}
}

void GameObject::LoadFromLevelFile(FILE* _File)
{
	SetName(LoadWString(_File));

	UINT ComType = 0;

	while (true)
	{
		fread(&ComType, sizeof(UINT), 1, _File);
		if (ComType == (UINT)COMPONENT_TYPE::END)
			break;

		Ptr<Component> pComponent = nullptr;

		switch ((COMPONENT_TYPE)ComType)
		{
		case COMPONENT_TYPE::TRANSFORM:
			pComponent = new CTransform;
			break;
		case COMPONENT_TYPE::CAMERA:
			pComponent = new CCamera;
			break;
		case COMPONENT_TYPE::COLLIDER2D:
			pComponent = new CCollider2D;
			break;
		case COMPONENT_TYPE::LIGHT2D:
			pComponent = new CLight2D;
			break;
		case COMPONENT_TYPE::MESHRENDER:
			pComponent = new CMeshRender;
			break;
		case COMPONENT_TYPE::BILLBOARD_RENDER:
			pComponent = new CBillboardRender;
			break;
		case COMPONENT_TYPE::SPRITE_RENDER:
			pComponent = new CSpriteRender;
			break;
		case COMPONENT_TYPE::FLIPBOOK_RENDER:
			pComponent = new CFlipbookRender;
			break;
		case COMPONENT_TYPE::TILE_RENDER:
			pComponent = new CTileRender;
			break;
		default:
			break;
		}

		assert(nullptr != pComponent);
		AddComponent(pComponent);
		pComponent->LoadFromLevelFile(_File);
	}

	size_t ScriptCount = 0;
	fread(&ScriptCount, sizeof(size_t), 1, _File);

	for (size_t i = 0; i < ScriptCount; ++i)
	{
		wstring ScriptName = LoadWString(_File);
		Ptr<CScript> pScript = ScriptMgr::GetScript(ScriptName);
		assert(nullptr != pScript);
		AddComponent(pScript.Get());
		pScript->LoadFromLevelFile(_File);
	}

	size_t ChildCount = 0;
	fread(&ChildCount, sizeof(size_t), 1, _File);

	for (size_t i = 0; i < ChildCount; ++i)
	{
		Ptr<GameObject> ChildObject = new GameObject;
		AddChild(ChildObject);
		ChildObject->LoadFromLevelFile(_File);
	}
}
