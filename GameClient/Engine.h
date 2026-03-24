#pragma once


// 게임 프로그램의 최상위 관리자
class Engine
	: public singleton<Engine>
{
	SINGLE(Engine)
private:
	HINSTANCE	m_hInst;
	HWND		m_hWnd;
	Vec2		m_Resolution;

	bool		m_EditorMode;

public:
	HINSTANCE GetInstance() { return m_hInst; }	
	HWND GetMainWndHwnd() { return m_hWnd; }
	Vec2 GetResolution() { return m_Resolution; }

public:
	int Init(HINSTANCE _hInst, UINT _Width, UINT _Height, bool _EditorMode);
	int Progress();
};
