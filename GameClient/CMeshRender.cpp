#include "pch.h"
#include "CMeshRender.h"


CMeshRender::CMeshRender()
	: CRenderComponent(COMPONENT_TYPE::MESHRENDER)
{
}

CMeshRender::~CMeshRender()
{
}

void CMeshRender::FinalTick()
{

}

void CMeshRender::Render()
{
	// Mesh or Shader 가 미설정 상태
	if (nullptr == GetMesh() || GetMaterial() == nullptr)
		return;

	GetMaterial()->Binding();

	GetMesh()->Render();

	GetMaterial()->Clear();
}