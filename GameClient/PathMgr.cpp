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
	// 작업 디렉터리가 아니라 실행 파일 위치를 기준으로 Content 경로를 잡는다.
	// 그래야 외부 실행, 자동 플레이, VS/쉘 실행 방식 차이와 무관하게 동일하게 동작한다.
	GetModuleFileNameW(nullptr, m_ContentPath, _countof(m_ContentPath));

	int Len = (int)wcslen(m_ContentPath);

	for (int pass = 0; pass < 2; ++pass)
	{
		for (int i = Len - 1; 0 <= i; --i)
		{
			if (L'\\' == m_ContentPath[i] || L'/' == m_ContentPath[i])
			{
				m_ContentPath[i] = L'\0';
				Len = i;
				break;
			}
		}
	}

	wcscat_s(m_ContentPath, L"\\Content\\");
}
