#pragma once
#include "Asset.h"

class ATexture :
    public Asset
{
private:
    ScratchImage                        m_Image; // Content 폴더에 있는 이미지 파일을 메모리(SysMem) 로 불러드림
    ComPtr<ID3D11Texture2D>             m_Tex2D; // SystemMem 로 로딩한 픽셀 데이터를 GPU 메모리로 전송
    D3D11_TEXTURE2D_DESC                m_Desc;

    // 텍스쳐의 용도에 맞는 View 들
    ComPtr<ID3D11RenderTargetView>      m_RTV;
    ComPtr<ID3D11DepthStencilView>      m_DSV;
    ComPtr<ID3D11ShaderResourceView>    m_SRV;

    int                                 m_RecentNum;


public:
    void Binding(UINT _RegisterNum);
    void Clear();
    bool BuildRGBA8Image(ScratchImage& _OutImage) const;

    float GetWidth() { return m_Desc.Width; }
    float GetHeight() { return m_Desc.Height; }

    ComPtr<ID3D11RenderTargetView> GetRTV() { return m_RTV; }
    ComPtr<ID3D11DepthStencilView> GetDSV() { return m_DSV; }
    ComPtr<ID3D11ShaderResourceView> GetSRV() { return m_SRV; }


public:
    virtual int Load(const wstring& _FilePath);    

public:
    ATexture();
    virtual ~ATexture();
};

