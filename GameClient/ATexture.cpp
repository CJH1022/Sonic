#include "pch.h"
#include "ATexture.h"

#include "Device.h"

ATexture::ATexture()
	: Asset(ASSET_TYPE::TEXTURE)
	, m_Desc{}
	, m_RecentNum(-1)
	, m_RecentSRV_CS(-1)
	, m_RecentUAV_CS(-1)
{
}

ATexture::~ATexture()
{
}

void ATexture::Binding(UINT _RegisterNum)
{
	m_RecentNum = _RegisterNum;

	CONTEXT->VSSetShaderResources(m_RecentNum, 1, m_SRV.GetAddressOf());
	CONTEXT->GSSetShaderResources(m_RecentNum, 1, m_SRV.GetAddressOf());
	CONTEXT->HSSetShaderResources(m_RecentNum, 1, m_SRV.GetAddressOf());
	CONTEXT->DSSetShaderResources(m_RecentNum, 1, m_SRV.GetAddressOf());
	CONTEXT->PSSetShaderResources(m_RecentNum, 1, m_SRV.GetAddressOf());
}

void ATexture::Clear()
{
	if (-1 == m_RecentNum)
		return;

	ID3D11ShaderResourceView* pSRV = nullptr;

	CONTEXT->VSSetShaderResources(m_RecentNum, 1, &pSRV);
	CONTEXT->GSSetShaderResources(m_RecentNum, 1, &pSRV);
	CONTEXT->HSSetShaderResources(m_RecentNum, 1, &pSRV);
	CONTEXT->DSSetShaderResources(m_RecentNum, 1, &pSRV);
	CONTEXT->PSSetShaderResources(m_RecentNum, 1, &pSRV);

	m_RecentNum = -1;
}

bool ATexture::BuildRGBA8Image(ScratchImage& _OutImage) const
{
	_OutImage.Release();

	if (nullptr == m_Image.GetImages() || 0 == m_Image.GetImageCount())
		return false;

	const TexMetadata metadata = m_Image.GetMetadata();
	HRESULT hr = Convert(m_Image.GetImages()
		, m_Image.GetImageCount()
		, metadata
		, DXGI_FORMAT_R8G8B8A8_UNORM
		, TEX_FILTER_DEFAULT
		, TEX_THRESHOLD_DEFAULT
		, _OutImage);

	return SUCCEEDED(hr);
}

void ATexture::Binding_CS_SRV(UINT _RegisterNum)
{
	m_RecentSRV_CS = static_cast<int>(_RegisterNum);
	CONTEXT->CSSetShaderResources(_RegisterNum, 1, m_SRV.GetAddressOf());
}

void ATexture::Binding_CS_UAV(UINT _RegisterNum)
{
	m_RecentUAV_CS = static_cast<int>(_RegisterNum);
	UINT i = static_cast<UINT>(-1);
	CONTEXT->CSSetUnorderedAccessViews(_RegisterNum, 1, m_UAV.GetAddressOf(), &i);
}

void ATexture::Clear_CS_SRV(int _RegisterNum)
{
	ID3D11ShaderResourceView* pSRV = nullptr;

	if (_RegisterNum == -1)
	{
		if (-1 == m_RecentSRV_CS)
			return;

		CONTEXT->CSSetShaderResources(m_RecentSRV_CS, 1, &pSRV);
		m_RecentSRV_CS = -1;
		return;
	}

	CONTEXT->CSSetShaderResources(_RegisterNum, 1, &pSRV);
	if (m_RecentSRV_CS == _RegisterNum)
		m_RecentSRV_CS = -1;
}

void ATexture::Clear_CS_UAV()
{
	if (-1 == m_RecentUAV_CS)
		return;

	ID3D11UnorderedAccessView* pUAV = nullptr;
	UINT i = static_cast<UINT>(-1);
	CONTEXT->CSSetUnorderedAccessViews(m_RecentUAV_CS, 1, &pUAV, &i);
	m_RecentUAV_CS = -1;
}

int ATexture::Load(const wstring& _FilePath)
{	
	wchar_t szExt[10] = {};
	_wsplitpath_s(_FilePath.c_str(), nullptr, 0, nullptr, 0, nullptr, 0, szExt, 10);
	wstring strExt = szExt;

	HRESULT hr = S_OK;

	// .dds
	if (L".dds" == strExt)
	{	
		hr = LoadFromDDSFile(_FilePath.c_str(), DDS_FLAGS_NONE, nullptr, m_Image);
	}
	// .tga
	else if (L".tga" == strExt)
	{	
		hr = LoadFromTGAFile(_FilePath.c_str(), nullptr, m_Image);
	}
	// WIC(Window Image Component) .png, .jpg, .jpeg, .bmp
	else
	{
		hr = LoadFromWICFile(_FilePath.c_str(), WIC_FLAGS_NONE, nullptr, m_Image);
	}

	if (FAILED(hr))
	{
		MessageBox(nullptr, L"텍스쳐 시스템메모리 로딩 실패", L"텍스쳐 로딩 실패", MB_OK);
		return E_FAIL;
	}	

	// SysMem			->	GPU
	// ScratchImage		->	Texture2D	
	// Texture2D 생성
	
	// Texture2D -> RTV  ->
	//           -> DSV  ->
	//           -> SRV  ->
	// View 생성

	// ScratcgImage 에 로딩된 이미지 데이터를 기반으로 Texture2D 를 생성하고, 
	// 다시 이걸로 ShaderResourceView 까지 만들어서 ShaderResourceView 주소를 알려줌
	if(FAILED(CreateShaderResourceView(DEVICE, m_Image.GetImages()
							, m_Image.GetImageCount(), m_Image.GetMetadata()
							, m_SRV.GetAddressOf())))
	{
		MessageBox(nullptr, L"ShaderResourveView 생성 실패", L"텍스쳐 로딩 실패", MB_OK);
		return E_FAIL;
	}

	// 생성된 SRV 를 이용해서, 먼저 만들어진 Texture2D 의 주소를 알아냄
	m_SRV->GetResource((ID3D11Resource**)m_Tex2D.GetAddressOf());

	// Texture2D 를 생성할때 세팅한 Desc 옵션정보를 알아냄
	m_Tex2D->GetDesc(&m_Desc);

	return S_OK;
}

int ATexture::Create(UINT _Width, UINT _Height, DXGI_FORMAT _format, UINT _Flag, D3D11_USAGE _usage)
{
	m_Desc = {};
	m_Desc.Format = _format;
	m_Desc.ArraySize = 1;
	m_Desc.Width = _Width;
	m_Desc.Height = _Height;
	m_Desc.BindFlags = _Flag;
	m_Desc.Usage = _usage;
	m_Desc.CPUAccessFlags = (_usage == D3D11_USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;
	m_Desc.MipLevels = 1;
	m_Desc.SampleDesc.Count = 1;
	m_Desc.SampleDesc.Quality = 0;

	if (FAILED(DEVICE->CreateTexture2D(&m_Desc, nullptr, m_Tex2D.GetAddressOf())))
		return E_FAIL;

	if (m_Desc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
	{
		if (FAILED(DEVICE->CreateDepthStencilView(m_Tex2D.Get(), nullptr, m_DSV.GetAddressOf())))
			return E_FAIL;
	}
	else
	{
		if (m_Desc.BindFlags & D3D11_BIND_RENDER_TARGET)
		{
			if (FAILED(DEVICE->CreateRenderTargetView(m_Tex2D.Get(), nullptr, m_RTV.GetAddressOf())))
				return E_FAIL;
		}

		if (m_Desc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
		{
			if (FAILED(DEVICE->CreateShaderResourceView(m_Tex2D.Get(), nullptr, m_SRV.GetAddressOf())))
				return E_FAIL;
		}

		if (m_Desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS)
		{
			if (FAILED(DEVICE->CreateUnorderedAccessView(m_Tex2D.Get(), nullptr, m_UAV.GetAddressOf())))
				return E_FAIL;
		}
	}

	return S_OK;
}
