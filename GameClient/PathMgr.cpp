#include "pch.h"
#include "PathMgr.h"

#include "Engine.h"

PathMgr::PathMgr()
	: m_ContentPath{}
{	
}


PathMgr::~PathMgr()
{
}

void PathMgr::Init()
{
	// ContentPath 경로 찾기
	// 실행파일이 있는 Bin 폴더 경로를 찾아낸다.
	// 디버깅 모드에서도 똑같이 동작하게 하기 위해서, 
	// 프로젝트 구성설정, 디버깅 탭에 작업 디렉터리를 실행파일 경로로 설정해준다.
	GetCurrentDirectory(255, m_ContentPath);
	//SetWindowText(Engine::GetInst()->GetMainWndHwnd(), m_ContentPath);

	int Len = wcslen(m_ContentPath);

	for (int i = Len - 1; 0 <= i; --i)
	{
		if ('\\' == m_ContentPath[i])
		{
			m_ContentPath[i] = '\0';
			break;
		}
	}

	wcscat_s(m_ContentPath, L"\\Content\\");
}