#include "pch.h"
#include "CBillboardRender.h"

#include "AssetMgr.h"

CBillboardRender::CBillboardRender()
	: CRenderComponent(COMPONENT_TYPE::BILLBOARD_RENDER)
{
}

CBillboardRender::~CBillboardRender()
{
}

void CBillboardRender::FinalTick()
{
}

void CBillboardRender::Render()
{
	if (nullptr == GetMesh() || nullptr == GetMaterial())
		return;


	GetMaterial()->SetScalar(VEC2_0, m_BillboardScale);
	GetMaterial()->Binding();

	GetMesh()->Render();

	GetMaterial()->Clear();
}

void CBillboardRender::CreateMaterial()
{
	// RectMesh 설정
	SetMesh(AssetMgr::GetInst()->Find<AMesh>(L"RectMesh"));

	// 재질 생성
	Ptr<AMaterial> pMtrl = AssetMgr::GetInst()->Find<AMaterial>(L"BillboardMtrl");

	// 찾는 재질이 없으면 생성한다.
	if (nullptr == pMtrl)
	{
		pMtrl = new AMaterial;
		pMtrl->SetName(L"BillboardMtrl");	

		// 쉐이더를 찾아서 재질에 세팅해준다.
		Ptr<AGraphicShader> pShader = AssetMgr::GetInst()->Find<AGraphicShader>(L"BillboardShader");
			

		// 찾은 or 생성한 쉐이더를 재질에 설정해주고, 재질도 에셋매니저에 등록한다.
		pMtrl->SetShader(pShader);
		pMtrl->SetDomain(RENDER_DOMAIN::DOMAIN_OPAQUE);
		AssetMgr::GetInst()->AddAsset(pMtrl->GetName(), pMtrl.Get());
	}

	SetMaterial(pMtrl);	
}
