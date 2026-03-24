#pragma once
#include "Entity.h"

enum SB_TYPE
{
    SRV_ONLY,
    SRV_UAV,
};

class StructuredBuffer :
    public Entity
{
private:
    ComPtr<ID3D11Buffer>                m_SB;
    ComPtr<ID3D11Buffer>                m_SB_Write;
    ComPtr<ID3D11Buffer>                m_SB_Read;

    ComPtr<ID3D11ShaderResourceView>    m_SRV;
    ComPtr<ID3D11UnorderedAccessView>   m_UAV;

    D3D11_BUFFER_DESC                   m_Desc;

    UINT                                m_ElementSize;
    UINT                                m_ElementCount;
    SB_TYPE                             m_SBType;

    int                                 m_RecentRegisterNum;
    int                                 m_RecentSRVNum;
    int                                 m_RecentUAVNum;
    bool                                m_SysMemMove;

public:
    int Create(UINT _ElementSize, UINT _ElementCount, SB_TYPE _Type, bool _bSysMove, void* _SysMem = nullptr);
    void SetData(void* _SysMem, UINT _DataSize = 0);
    void GetData(void* _DstSysMem, UINT _DataSize = 0);

    void Binding(UINT _RegisterNum);
    void Clear();

    void Binding_CS_SRV(UINT _RegisterNum);
    void Clear_CS_SRV();

    void Binding_CS_UAV(UINT _RegisterNum);
    void Clear_CS_UAV();

    UINT GetElementCount() { return m_ElementCount; }
    UINT GetElementSize() { return m_ElementSize; }
    UINT GetBufferSize() { return m_ElementCount * m_ElementSize; }

    ComPtr<ID3D11ShaderResourceView>   GetSRV() { return m_SRV; }
    ComPtr<ID3D11UnorderedAccessView>  GetUAV() { return m_UAV; }

public:
    StructuredBuffer();
    StructuredBuffer(const StructuredBuffer& _Origin);
    virtual ~StructuredBuffer();
};

