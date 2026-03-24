#include "pch.h"
#include "RenderMgr.h"

#include "Device.h"
#include "LevelMgr.h"
#include "AssetMgr.h"
#include "TimeMgr.h"
#include "KeyMgr.h"

RenderMgr::RenderMgr()
	: m_bDebugRender(true)
{

}

RenderMgr::~RenderMgr()
{

}

void RenderMgr::Init()
{
	m_DbgObj = new GameObject;
	m_DbgObj->AddComponent(new CTransform);
	m_DbgObj->AddComponent(new CMeshRender);
	m_DbgObj->MeshRender()->SetMaterial(FIND(AMaterial, L"DbgMtrl"));

	m_Light2DBuffer = new StructuredBuffer;
}

void RenderMgr::Progress()
{
	if (KEY_TAP(KEY::F9))
		m_bDebugRender ? m_bDebugRender = false : m_bDebugRender = true;
	

	// 레더링 시작전에 할 일
	Render_Start();

	// Level 의 상태가 Play 상태면, 등록된 MainCam 으로 렌더링
	if (LEVEL_STATE::PLAY == LevelMgr::GetInst()->GetLevelState())
	{
		// 카메라 기반 렌더링
		if (nullptr == m_MainCam)
			return;

		// 카메라를 이용해서 레벨안에 있는 물체들을 렌더링
		m_MainCam->SortObject();
		m_MainCam->Render();		
	}

	// Level 의 상태가 Pause, Stop 상태면, 등록된 EditorCam 로 렌더링
	else
	{
		// 카메라 기반 렌더링
		if (nullptr == m_EditorCam)
			return;

		// 카메라를 이용해서 레벨안에 있는 물체들을 렌더링
		m_EditorCam->SortObject();
		m_EditorCam->Render();		
	}

	// 디버그 렌더링 요청 처리
	if (m_bDebugRender)
		Render_Debug();

	Render_End();
}

void RenderMgr::Render_Start()
{
	// 타겟 설정
	Device::GetInst()->OMSetTarget();

	// 렌더타겟 클리어
	Device::GetInst()->ClearTarget();

	// 등록받은 Light2D 의 광원 정보를 구조화버퍼에 담는다.
	// 구조화버퍼를 특정 t 레지스터에 바인딩 한다.
	vector<Light2DInfo> vecInfo;
	for (size_t i = 0; i < m_vecLight2D.size(); ++i)
	{
		vecInfo.push_back(m_vecLight2D[i]->GetInfo());
	}

	// 등록된 광원이 최소 1개 이상인 경우에만 데이터를 구조화버퍼로 보낸다.
	if (!vecInfo.empty())
	{
		// 구조화버퍼 공간이 모자라면 재확장 및 데이터 전달
		if (vecInfo.size() > m_Light2DBuffer->GetElementCount())
			m_Light2DBuffer->Create(sizeof(Light2DInfo), vecInfo.size(), SB_TYPE::SRV_ONLY, true, vecInfo.data());

		// 공간이 여유가 있으면 바로 광원데이터 전달
		else
			m_Light2DBuffer->SetData(vecInfo.data(), sizeof(Light2DInfo) * vecInfo.size());

		// t12 레지스터로 바인딩
		m_Light2DBuffer->Binding(12);
	}

	g_Global.Light2DCount = m_vecLight2D.size();

	// Globa 데이터를 상수버퍼를 통해서 b2 레지스터에 바인딩
	Device::GetInst()->GetCB(CB_TYPE::GLOBAL)->SetData(&g_Global);
	Device::GetInst()->GetCB(CB_TYPE::GLOBAL)->Binding();
}

void RenderMgr::Render_End()
{
	// 구조화버퍼 클리어
	// 등록받았던 광원들 해제
	m_Light2DBuffer->Clear();
	m_vecLight2D.clear();
}

void RenderMgr::Render_Debug()
{
	list<DbgInfo>::iterator iter = m_DbgInfoList.begin();
	for (; iter != m_DbgInfoList.end();)
	{
		// Mesh 설정
		switch ((*iter).Shape)
		{
		case DBG_SHAPE::RECT:
			m_DbgObj->MeshRender()->SetMesh(FIND(AMesh, L"RectMesh_LineStrip"));
			break;
		case DBG_SHAPE::CIRCLE:
			m_DbgObj->MeshRender()->SetMesh(FIND(AMesh, L"CircleMesh_LineStrip"));
			break;
		case DBG_SHAPE::CUBE:
			m_DbgObj->MeshRender()->SetMesh(FIND(AMesh, L"CubeMesh"));
			break;
		case DBG_SHAPE::SPHERE:
			m_DbgObj->MeshRender()->SetMesh(FIND(AMesh, L"SphereMesh"));
			break;
		}

		// Transform 설정

		if ((*iter).matWorld == XMMatrixIdentity())
		{
			m_DbgObj->Transform()->SetRelativePos((*iter).Pos);
			m_DbgObj->Transform()->SetRelativeScale((*iter).Scale);
			m_DbgObj->Transform()->SetRelativeRot((*iter).Rotation);
			m_DbgObj->Transform()->FinalTick();
		}
		else
		{
			m_DbgObj->Transform()->SetWorldMat((*iter).matWorld);
		}		
		
		// Material 설정
		m_DbgObj->MeshRender()->GetMaterial()->SetScalar(VEC4_0, (*iter).Color);

		if ((*iter).DepthTest)
			m_DbgObj->MeshRender()->GetMaterial()->GetShader()->SetDSType(DS_TYPE::LESS);
		else
			m_DbgObj->MeshRender()->GetMaterial()->GetShader()->SetDSType(DS_TYPE::NO_TEST_NO_WRITE);

		// Render 요청
		m_DbgObj->Render();

		// 렌더링 시간 누적
		(*iter).Age += E_DT;

		// 최대 수명에 도달하면 정보 삭제
		if ((*iter).Life < (*iter).Age)
			iter = m_DbgInfoList.erase(iter);
		else
			++iter;
	}
}