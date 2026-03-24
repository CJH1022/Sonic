#pragma once

#include "GameObject.h"
#include "CCamera.h"
#include "StructuredBuffer.h"

class RenderMgr
	: public singleton<RenderMgr>
{
	SINGLE(RenderMgr);
private:
	Ptr<CCamera>			m_MainCam;
	Ptr<CCamera>			m_EditorCam;

	Ptr<GameObject>			m_DbgObj;			// 디버그 렌더링을 위한 Dummy GameObject
	list<DbgInfo>			m_DbgInfoList;		// 디버그 요청 정보

	vector<Ptr<CLight2D>>	m_vecLight2D;		// 레벨 안에있는 모든 광원
	Ptr<StructuredBuffer>	m_Light2DBuffer;	// 광원의 데이터를 입력받을 구조화버퍼

	bool					m_bDebugRender;		// 디버그 렌더 기능 On / Off

public:
	void RegisterCamera(Ptr<CCamera> _Cam) { m_MainCam = _Cam; }
	void RegisterEditorCamera(Ptr<CCamera> _Cam) { m_EditorCam = _Cam; }

	Ptr<CCamera> GetPOVCamera() { return m_MainCam; }
	void AddDebugInfo(const DbgInfo& _Info)  {  if(m_bDebugRender) m_DbgInfoList.push_back(_Info); }
	void RegisterLight2D(Ptr<CLight2D> _Light2D) { m_vecLight2D.push_back(_Light2D); }

private:
	void Render_Start();
	void Render_End();
	void Render_Debug();

public:
	void Init();
	void Progress();
};

