#pragma once

enum class KEY
{
	Q,
	W,
	E,
	R,
	A,
	S,
	D,
	F,
	Z,
	X,
	C,
	V,

	LBTN,
	RBTN,
	MBTN,

	LEFT,
	RIGHT,
	UP,
	DOWN,

	ENTER,
	ALT,
	CTRL,
	LSHIFT,
	RSHIFT,
	SPACE,
	ESC,

	HOME,
	END,

	F1, F2, F3, F4, F5,
	F6, F7, F8, F9, 

	KEY_END,
};


// None  TAP Pressed  Released None
//  |     |  |			  |     |
//  V     V  V            V     V 
//  ____				 _______________
//	    \_______________/
enum KEY_STATE
{
	NONE,
	TAP,
	PRESSED,
	RELEASED,	
};

struct KeyInfo
{	
	KEY_STATE	State;		// 키의 현재 상태
	bool	    Pressed;	// 이전에 눌린적이 있는지 아닌지
};

class KeyMgr
	: public singleton<KeyMgr>
{
	SINGLE(KeyMgr)
private:
	vector<KeyInfo>		m_vecKeys;

	Vec2				m_MousePos;		// 이번 프레임 마우스 좌표
	Vec2				m_MousePrevPos;	// 이전 프레임 마우스 좌표
	Vec2				m_MouseDir;

	bool				m_WheelChanged;
	int					m_Wheel;  // 1, 0, -1

	bool				m_Active;

public:
	KEY_STATE GetKeyState(KEY _Key) { return m_vecKeys[(UINT)_Key].State; }
	GET(Vec2, MousePos);
	GET(Vec2, MousePrevPos);
	GET(Vec2, MouseDir);

	bool IsActive() { return m_Active; }
	void SetActive(bool _Active) { m_Active = _Active; }

	int GetMouseWheel() { return m_Wheel; } 
	void SetMouseWheel(int _Wheel) 
	{ 
		m_WheelChanged = true;
		m_Wheel = _Wheel / abs(_Wheel);
	}


public:
	void Init();
	void Tick();
};

#define KEY_CHECK(key, state) KeyMgr::GetInst()->GetKeyState(key) == state

#define KEY_TAP(key)		KEY_CHECK(key, KEY_STATE::TAP)
#define KEY_PRESSED(key)	KEY_CHECK(key, KEY_STATE::PRESSED)
#define KEY_RELEASED(key)	KEY_CHECK(key, KEY_STATE::RELEASED)
#define KEY_NONE(key)		KEY_CHECK(key, KEY_STATE::NONE)





