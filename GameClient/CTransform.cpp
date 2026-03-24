#include "pch.h"
#include "CTransform.h"

#include "Device.h"
#include "GameObject.h"

CTransform::CTransform()
	: Component(COMPONENT_TYPE::TRANSFORM)
	, m_RelativeScale(Vec3(1.f, 1.f, 1.f))
	, m_Dir{}
	, m_IndependentScale(false)
{
	m_Dir[(UINT)DIR::RIGHT] = Vec3(1.f, 0.f, 0.f);
	m_Dir[(UINT)DIR::UP]	= Vec3(0.f, 1.f, 0.f);
	m_Dir[(UINT)DIR::FRONT] = Vec3(0.f, 0.f, 1.f);
}

CTransform::~CTransform()
{
}



void CTransform::FinalTick()
{
	// 크기 -> 회전 -> 이동
	Matrix matTrans = XMMatrixTranslation(m_RelativePos.x, m_RelativePos.y, m_RelativePos.z);

	Matrix matSclae = XMMatrixScaling(m_RelativeScale.x, m_RelativeScale.y, m_RelativeScale.z);	

	Matrix matRot = XMMatrixRotationX(m_RelativeRot.x) 
				  * XMMatrixRotationY(m_RelativeRot.y) 
			      * XMMatrixRotationZ(m_RelativeRot.z);	

	// 방향벡터 계산
	m_Dir[(UINT)DIR::RIGHT] = Vec3(1.f, 0.f, 0.f);
	m_Dir[(UINT)DIR::UP]	= Vec3(0.f, 1.f, 0.f);
	m_Dir[(UINT)DIR::FRONT] = Vec3(0.f, 0.f, 1.f);

	// 변환행렬을 적용할 Vec3 벡터를 좌표성 데이터로 본다(동차좌표 1로 확장 -> 4행 이동정보 적용)
	// XMVector3TransformCoord(m_Dir[(UINT)DIR::RIGHT], matRot);

	// 변환행렬을 적용할 Vec3 벡터를 방향성 데이터로 본다(동차좌표 0로 확장 -> 4행 이동정보 무시)
	m_Dir[(UINT)DIR::RIGHT] = XMVector3TransformNormal(m_Dir[(UINT)DIR::RIGHT], matRot);
	m_Dir[(UINT)DIR::UP]	= XMVector3TransformNormal(m_Dir[(UINT)DIR::UP], matRot);
	m_Dir[(UINT)DIR::FRONT] = XMVector3TransformNormal(m_Dir[(UINT)DIR::FRONT], matRot);


	// 월드행렬 계산 ( 크기 x 회전 x 이동 )
	m_matWorld = matSclae * matRot * matTrans;

	// 부모 오브젝트가 있었다면
	if (nullptr != GetOwner()->GetParent())
	{
		// 부모 오브젝트의 크기에 영향을 받지 않겠다.
		if (m_IndependentScale)
		{
			Vec3 ParentScale = GetOwner()->GetParent()->Transform()->GetWorldScale();
			Matrix matParentScale = XMMatrixScaling(ParentScale.x, ParentScale.y, ParentScale.z);
			Matrix matParentScaleInv = XMMatrixInverse(nullptr, matParentScale);

			m_matWorld = m_matWorld * matParentScaleInv * GetOwner()->GetParent()->Transform()->GetWorldMat();
		}
		// 부모 오브젝트의 크기에 영향을 받는다.
		else
			m_matWorld *= GetOwner()->GetParent()->Transform()->GetWorldMat();		
	}
}

void CTransform::Binding()
{		
	g_Trans.matWorld = m_matWorld;	

	// 전역변수에 들어있는 오브젝트 위치 정보를 상수버퍼로 복사
	Device::GetInst()->GetCB(CB_TYPE::TRANSFORM)->SetData(&g_Trans);
	Device::GetInst()->GetCB(CB_TYPE::TRANSFORM)->Binding();
}




Vec3 CTransform::GetWorldScale()
{
	Vec3 vWorldScale = m_RelativeScale;	
	if (m_IndependentScale)
		return vWorldScale;

	Ptr<GameObject> pParent = GetOwner()->GetParent();
	while (nullptr != pParent)
	{
		vWorldScale *= pParent->Transform()->GetRelativeScale();

		if (pParent->Transform()->m_IndependentScale)
			break;

		pParent = pParent->GetParent();
	}

	return vWorldScale;
}