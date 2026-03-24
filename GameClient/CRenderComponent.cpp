#include "pch.h"
#include "CRenderComponent.h"
#include "LevelMgr.h"

CRenderComponent::CRenderComponent(COMPONENT_TYPE _Type)
	: Component(_Type)
{
}

CRenderComponent::CRenderComponent(const CRenderComponent& _Origin)
	: Component(_Origin)
	, m_Mesh(_Origin.m_Mesh)
	, m_SharedMtrl(_Origin.m_SharedMtrl)
{
	// 원본 렌더컴포넌트가 공유재질(에셋 매니저로부터 관리되는) 을 사용하고 있다면
	if (_Origin.m_Mtrl == _Origin.m_SharedMtrl)
	{
		// 나도 공유재질 사용
		m_Mtrl = m_SharedMtrl;
	}

	// 원본 렌더컴포넌트의 동적재질이 존재하고, 현재 사용중인 재질이 동적 재질인 경우
	else if (nullptr != _Origin.m_DynamicMtrl && _Origin.m_Mtrl == _Origin.m_DynamicMtrl)
	{
		CreateDynamicMaterial();
	}
}

CRenderComponent::~CRenderComponent()
{
}

void CRenderComponent::Init()
{
	CreateMaterial();
}

void CRenderComponent::SetMaterial(Ptr<AMaterial> _Mtrl)
{
	m_Mtrl = m_SharedMtrl = _Mtrl;
}

Ptr<AMaterial> CRenderComponent::GetSharedMaterial()
{
	m_Mtrl = m_SharedMtrl;
	return m_SharedMtrl;
}

Ptr<AMaterial> CRenderComponent::CreateDynamicMaterial()
{
	// 동적 재질 생성은 반드시 레벨이 Play 모드일 경우에만 사용 가능한 기능
	assert(LEVEL_STATE::PLAY == LevelMgr::GetInst()->GetLevelState());

	if (nullptr != m_DynamicMtrl)
	{
		m_Mtrl = m_DynamicMtrl;
		return m_DynamicMtrl;
	}
	else
	{
		m_Mtrl = m_DynamicMtrl = m_SharedMtrl->Clone();
		return m_DynamicMtrl;
	}	
}