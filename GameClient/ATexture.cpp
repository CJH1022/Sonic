#include "pch.h"
#include "ATexture.h"

#include "Device.h"

ATexture::ATexture()
	: Asset(ASSET_TYPE::TEXTURE)
	, m_Desc{}
	, m_RecentNum(-1)
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
