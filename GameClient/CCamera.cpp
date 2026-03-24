#include "pch.h"
#include "CCamera.h"

#include "LevelMgr.h"
#include "ALevel.h"
#include "Layer.h"

#include "Device.h"
#include "RenderMgr.h"

#include "GameObject.h"

CCamera::CCamera()
	: Component(COMPONENT_TYPE::CAMERA)
	, m_LayerCheck(0)
	, m_OrthoScale(1.f)
{

}

CCamera::~CCamera()
{
}

void CCamera::Begin()
{
	// 레벨이 시작될때 호출됨
	// RenderMgr 에 카메라(본인)를 등록
	RenderMgr::GetInst()->RegisterCamera(this);
}

void CCamera::FinalTick()
{
	// 뷰 행렬 계산
	// 카메라의 위치
	Vec3 vPos = Transform()->GetRelativePos();

	// 이동 ( 카메라 위치를 원점으로 되돌리는 만큼의 이동행렬)
	Matrix matTrans = XMMatrixIdentity();
	matTrans._41 = -vPos.x; 
	matTrans._42 = -vPos.y; 
	matTrans._43 = -vPos.z;

	// View 행렬 회전
	//  - 카메라가 바라보는 방향을 z 축이 되도록 회전하는 부분이 추가
	// 카메라의 Right, Up, Front 방향 벡터에 회전행렬을 곱하면 이게 다시 x축, y축, z 축이 되는 회전행을 구해야 함
	 
	// vR				  ( 1 0 0)
	// vU    x    R   =   ( 0 1 0)
	// vZ				  ( 0 0 1)
								 
	//			vR
	// R 행렬은  vU     의 역핼렬
	//			vZ	
	
	//	vR
	//  vU   행렬은 행 끼리의 관계가 직교상태이기 때문에, 전치(Transpose) 를 통해서 역행렬을 쉽게 구할 수 있음
	//	vZ

	// 전치한 행렬과 곱해보면, 자기자신과 내적을 한 경우 결과가 1, 다른 직교벡터랑 내적을 한 경우 0 이 나오기 때문
	// 내적은 각 성분끼리의 곱을 합친 결과, 
	// 내적 결과값의 의미는 (벡터의 길이) x (길이) x (두 벡터가 이루는 각도의 cos 값) 

	//		 ( vR.x   vU.x   vF.x    0 ) 
	// R ==  ( vR.y   vU.y   vF.y    0 )
	//		 ( vR.z   vU.z   vF.z    0 )
	//       (   0     0       0     1 )

	Vec3 vR = Transform()->GetDir(DIR::RIGHT);
	Vec3 vU = Transform()->GetDir(DIR::UP);
	Vec3 vF = Transform()->GetDir(DIR::FRONT);

	Matrix matRot = XMMatrixIdentity();
	matRot._11 = vR.x;  matRot._12 = vU.x;  matRot._13 = vF.x;
	matRot._21 = vR.y;	matRot._22 = vU.y;	matRot._23 = vF.y;
	matRot._31 = vR.z;	matRot._32 = vU.z;	matRot._33 = vF.z;

	// 카메라가 원점인 공간으로 이동, 카메라가 바라보는 방향을 z 축으로 회전하는 회전을 적용
	m_matView = matTrans * matRot;
	


	// 투영행렬
	if (PROJ_TYPE::ORTHOGRAPHIC == m_ProjType)
	{
		// 직교투영(Orthographic) 행렬 계산	
		m_matProj = XMMatrixOrthographicLH(m_Width * m_OrthoScale, (m_Width / m_AspectRatio) * m_OrthoScale, 1.f, m_Far);
	}
	else
	{
		// 원근투영(Perspective)
		m_matProj = XMMatrixPerspectiveFovLH(m_FOV, m_AspectRatio, 1.f, m_Far);
	}	
}

void CCamera::SortObject()
{
	// 렌더링 할 물체들을 정렬한다.
	m_vecOpaque.clear();
	m_vecMasked.clear();
	m_vecTrapsnarent.clear();
	m_vecPostProcess.clear();

	Ptr<ALevel> pCurLevel = LevelMgr::GetInst()->GetCurLevel();
	if (nullptr == pCurLevel)
		return;

	for (UINT i = 0; i < MAX_LAYER; ++i)
	{
		// 카메라가 레이어를 볼 수 있어야 함
		if (false == (m_LayerCheck & (1 << i)))
			continue;

		// 레이어에 소속된 모든 오브젝트를 가져온다
		Layer* pLayer = pCurLevel->GetLayer(i);
		const vector<Ptr<GameObject>>& vecObjects = pLayer->GetAllObjects();

		for (size_t j = 0; j < vecObjects.size(); ++j)
		{
			// 오브젝트가 렌더링을 할 수 있는 상태인지 확인
			if (nullptr == vecObjects[j]->GetRenderCom()
				|| nullptr == vecObjects[j]->GetRenderCom()->GetMesh()
				|| nullptr == vecObjects[j]->GetRenderCom()->GetMaterial())
			{
				continue;
			}

			RENDER_DOMAIN domain = vecObjects[j]->GetRenderCom()->GetMaterial()->GetDomain();

			switch (domain)
			{
			case RENDER_DOMAIN::DOMAIN_OPAQUE:
				m_vecOpaque.push_back(vecObjects[j].Get());
				break;
			case RENDER_DOMAIN::DOMAIN_MASKED:
				m_vecMasked.push_back(vecObjects[j].Get());
				break;
			case RENDER_DOMAIN::DOMAIN_TRANSPARENT:
				m_vecTrapsnarent.push_back(vecObjects[j].Get());
				break;			
			case RENDER_DOMAIN::DOMAIN_POSTPROCESS:
				m_vecPostProcess.push_back(vecObjects[j].Get());
				break;
			}
		}
	}
}

void CCamera::Render()
{
	g_Trans.matView = m_matView;
	g_Trans.matProj = m_matProj;

	// Domain 순서대로 렌더링 진행
	for (size_t i = 0; i < m_vecOpaque.size(); ++i)
		m_vecOpaque[i]->Render();

	for (size_t i = 0; i < m_vecMasked.size(); ++i)
		m_vecMasked[i]->Render();

	for (size_t i = 0; i < m_vecTrapsnarent.size(); ++i)
		m_vecTrapsnarent[i]->Render();

	for (size_t i = 0; i < m_vecPostProcess.size(); ++i)
		m_vecPostProcess[i]->Render();
}

void CCamera::LayerCheck(int _Idx)
{
	m_LayerCheck ^= (1 << _Idx);
}

void CCamera::SaveToLevelFile(FILE* _File)
{
	fwrite(&m_LayerCheck, sizeof(UINT), 1, _File);
	fwrite(&m_ProjType, sizeof(PROJ_TYPE), 1, _File);
	fwrite(&m_Far, sizeof(float), 1, _File);
	fwrite(&m_Width, sizeof(float), 1, _File);
	fwrite(&m_AspectRatio, sizeof(float), 1, _File);
	fwrite(&m_FOV, sizeof(float), 1, _File);
	fwrite(&m_OrthoScale, sizeof(float), 1, _File);
}

void CCamera::LoadFromLevelFile(FILE* _File)
{
	fread(&m_LayerCheck, sizeof(UINT), 1, _File);
	fread(&m_ProjType, sizeof(PROJ_TYPE), 1, _File);
	fread(&m_Far, sizeof(float), 1, _File);
	fread(&m_Width, sizeof(float), 1, _File);
	fread(&m_AspectRatio, sizeof(float), 1, _File);
	fread(&m_FOV, sizeof(float), 1, _File);
	fread(&m_OrthoScale, sizeof(float), 1, _File);
}
