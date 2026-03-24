#pragma once


#include "ConstBuffer.h"

// Directx11 라이브러리 함수를 이용해서 
// GPU 를 제어할 수 있는 클래스
class Device
	: public singleton<Device>
{
	SINGLE(Device);
private:
	ComPtr<ID3D11Device>				m_Device;			// GPU 메모리 관리
	ComPtr<ID3D11DeviceContext>			m_Context;			// GPU 제어 및 렌더링, GPGPU(General Purpose) - ComputeShader 명령

	ComPtr<IDXGISwapChain>				m_SwapChain;		
	ComPtr<ID3D11Texture2D>				m_RenderTarget;
	ComPtr<ID3D11Texture2D>				m_DepthStencilTarget;

	ComPtr<ID3D11RenderTargetView>		m_RTV;
	ComPtr<ID3D11DepthStencilView>		m_DSV;

	ComPtr<ID3D11SamplerState>			m_arrSam[2];

	ComPtr<ID3D11RasterizerState>		m_RSState[(UINT)RS_TYPE::END];
	ComPtr<ID3D11DepthStencilState>		m_DSState[(UINT)DS_TYPE::END];
	ComPtr<ID3D11BlendState>			m_BSState[(UINT)BS_TYPE::END];
	
	

	HWND								m_hWnd;		   // 출력 윈도우
	Vec2								m_RenderResol; // 렌더타겟 해상도


	Ptr<ConstBuffer>					m_CB[(UINT)CB_TYPE::END];


public:
	int Init(HWND _hwnd, Vec2 _Resolution);
	void ClearTarget();
	void OMSetTarget();
	void Present() { m_SwapChain->Present(0, 0); }

	Vec2 GetRenderResolution() { return m_RenderResol; }
	ComPtr<ID3D11Device> GetDevice() {return m_Device;}
	ComPtr<ID3D11DeviceContext>	GetContext() { return	m_Context; }
	Ptr<ConstBuffer> GetCB(CB_TYPE _Type) { return m_CB[(UINT)_Type]; }

	ComPtr<ID3D11RasterizerState> GetRSState(RS_TYPE _Type) { return m_RSState[(UINT)_Type]; }
	ComPtr<ID3D11DepthStencilState> GetDSState(DS_TYPE _Type) { return m_DSState[(UINT)_Type]; }
	ComPtr<ID3D11BlendState> GetBSState(BS_TYPE _Type) { return m_BSState[(UINT)_Type]; }


private:
	int CreateSwapChain();
	int CreateBuffer();
	int CreateSampler();
	int CreateRasterizerState();
	int CreateDepthStencilState();
	int CreateBlendState();
	void CreateConstBuffer();
};


#define DEVICE	Device::GetInst()->GetDevice().Get()
#define CONTEXT Device::GetInst()->GetContext().Get()
