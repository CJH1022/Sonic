#include "pch.h"
#include "PathMgr.h"

wchar_t CPathMgr::g_szSolutionPath[255] = L"";
wchar_t CPathMgr::g_szResPath[255] = L"";
wchar_t CPathMgr::g_szIncPath[255] = L"";
wchar_t CPathMgr::g_szProjPath[255] = L"";
wchar_t CPathMgr::g_szRelativePath[255] = L"";
wchar_t CPathMgr::g_szFile[255] = L"";
wchar_t CPathMgr::g_szExt[50] = L"";

void CPathMgr::init()
{
	GetCurrentDirectory(255, g_szSolutionPath);

	wcscpy_s(g_szResPath, 255, g_szSolutionPath);

	wcscat_s(g_szResPath, L"\\Game\\Content\\");
	
	GetCurrentDirectory(255, g_szIncPath);
	
	wcscat_s(g_szIncPath, L"\\External\\Include\\");

	GetCurrentDirectory(255, g_szProjPath);
	
	wcscat_s(g_szProjPath, L"\\");
}

wchar_t * CPathMgr::GetResPath()
{
	return g_szResPath;
}

wchar_t * CPathMgr::GetIncludePath()
{
	return g_szIncPath;
}

wchar_t * CPathMgr::GetProjectPath()
{
	return g_szProjPath;
}

wchar_t * CPathMgr::GetFileName(const wchar_t * _strPath)
{
	_wsplitpath_s(_strPath, NULL, 0, NULL, 0, g_szFile, 255, NULL, 0);
	return g_szFile;
}

wchar_t * CPathMgr::GetExt(const wchar_t * _strPath)
{
	_wsplitpath_s(_strPath, NULL, 0, NULL, 0, nullptr, 0, g_szExt, 50);
	return g_szExt;
}

wchar_t * CPathMgr::GetRelativePath(const wchar_t * _pFullPath)
{
	wmemset(g_szRelativePath, 0, 255);

	wstring str = _pFullPath;
	size_t iLen = wcslen(g_szResPath);
	str = str.substr(iLen, str.length() - iLen).c_str();
	wcscpy_s(g_szRelativePath, 255, str.c_str());
	return g_szRelativePath;
}

wchar_t * CPathMgr::GetSolutionPath()
{
	return g_szSolutionPath;
}
