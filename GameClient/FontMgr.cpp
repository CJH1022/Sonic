#include "pch.h"
#include "FontMgr.h"
#include "Device.h"

FontMgr::FontMgr()
	: m_FW1Factory(nullptr)
	, m_FontWrapper(nullptr)
{
}

FontMgr::~FontMgr()
{
	if (nullptr != m_FW1Factory)
		m_FW1Factory->Release();

	if (nullptr != m_FontWrapper)
		m_FontWrapper->Release();
}

void FontMgr::Init()
{
	if (FAILED(FW1CreateFactory(FW1_VERSION, &m_FW1Factory)))
	{
		assert(nullptr);
	}

	if (FAILED(m_FW1Factory->CreateFontWrapper(DEVICE, L"Arial", &m_FontWrapper)))
	{
		assert(nullptr);
	}
}

void FontMgr::DrawFont(const wchar_t* _pStr, float _fPosX, float _fPosY, float _fFontSize, UINT _Color)
{
	m_FontWrapper->DrawString(
		CONTEXT,
		_pStr,				// String
		_fFontSize,			// Font size
		_fPosX,				// Window X position
		_fPosY,				// Window Y position
		_Color,				// Text color, 0xAaBbGgRr
		FW1_RESTORESTATE    // Flags (for example FW1_RESTORESTATE to keep context states unchanged)
	);
}
