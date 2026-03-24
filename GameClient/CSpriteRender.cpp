#include "pch.h"
#include "CSpriteRender.h"

#include "AssetMgr.h"

CSpriteRender::CSpriteRender()
	: CRenderComponent(COMPONENT_TYPE::SPRITE_RENDER)
{
}

CSpriteRender::~CSpriteRender()
{
}

void CSpriteRender::FinalTick()
{

}

void CSpriteRender::Render()
{	
	GetMaterial()->SetTexture(TEX_0, m_Sprite->GetAtlas());
	GetMaterial()->SetScalar(VEC2_0, m_Sprite->GetLeftTopUV());
	GetMaterial()->SetScalar(VEC2_1, m_Sprite->GetSliceUV());
	GetMaterial()->SetScalar(VEC2_2, m_Sprite->GetBackgroundUV());
	GetMaterial()->SetScalar(VEC2_3, m_Sprite->GetOffsetUV());

	GetMaterial()->Binding();
	GetMesh()->Render();
}

void CSpriteRender::CreateMaterial()
{
	wstring MeshName = L"RectMesh";
	wstring MtrlName = L"SpriteMtrl";
	wstring ShaderName = L"SpriteShader";
	wstring FilePath = L"Shader\\sprite.fx";
	string VS = "VS_Sprite";	
	string PS = "PS_Sprite";

	// RectMesh 설정
	SetMesh(AssetMgr::GetInst()->Find<AMesh>(MeshName));

	// 재질 생성
	Ptr<AMaterial> pMtrl = AssetMgr::GetInst()->Find<AMaterial>(MtrlName);

	// 찾는 재질이 없으면 생성한다.
	if (nullptr == pMtrl)
	{
		pMtrl = new AMaterial;
		pMtrl->SetName(MtrlName);

		// 쉐이더를 찾아서 재질에 세팅해준다.
		Ptr<AGraphicShader> pShader = AssetMgr::GetInst()->Find<AGraphicShader>(ShaderName);
		
		// 찾은 or 생성한 쉐이더를 재질에 설정해주고, 재질도 에셋매니저에 등록한다.
		pMtrl->SetShader(pShader);
		pMtrl->SetDomain(RENDER_DOMAIN::DOMAIN_MASKED);
		AssetMgr::GetInst()->AddAsset(pMtrl->GetName(), pMtrl.Get());
	}

	SetMaterial(pMtrl);
}

void CSpriteRender::SaveToLevelFile(FILE* _File)
{
	CRenderComponent::SaveToLevelFile(_File);
	SaveAssetRef(_File, m_Sprite.Get());
}

void CSpriteRender::LoadFromLevelFile(FILE* _File)
{
	CRenderComponent::LoadFromLevelFile(_File);
	m_Sprite = LoadAssetRef<ASprite>(_File);
}
