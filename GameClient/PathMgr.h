#pragma once


class PathMgr
	: public singleton<PathMgr>
{
	SINGLE(PathMgr)
private:
	wchar_t		m_ContentPath[255];

public:
	void Init();
	const wchar_t* GetContentPath() { return m_ContentPath; }
};

#define CONTENT_PATH PathMgr::GetInst()->GetContentPath()