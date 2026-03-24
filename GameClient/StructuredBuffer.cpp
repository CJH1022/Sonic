#include "pch.h"
#include "StructuredBuffer.h"

#include "Device.h"

StructuredBuffer::StructuredBuffer()
	: m_Desc{}
	, m_ElementSize(0)
	, m_ElementCount(0)
	, m_SBType(SB_TYPE::SRV_ONLY)
	, m_RecentRegisterNum(-1)
	, m_RecentSRVNum(-1)
	, m_RecentUAVNum(-1)
	, m_SysMemMove(false)
{
}

StructuredBuffer::StructuredBuffer(const StructuredBuffer& _Origin)
	: Entity(_Origin)
	, m_SB(nullptr)
	, m_SB_Write(nullptr)
	, m_SB_Read(nullptr)
	, m_SRV(nullptr)
	, m_UAV(nullptr)
	, m_Desc{}
	, m_ElementSize(_Origin.m_ElementSize)
	, m_ElementCount(_Origin.m_ElementCount)
	, m_SBType(_Origin.m_SBType)
	, m_RecentRegisterNum(-1)
	, m_RecentSRVNum(-1)
	, m_RecentUAVNum(-1)
	, m_SysMemMove(_Origin.m_SysMemMove)
{
	Create(m_ElementSize, m_ElementCount, m_SBType, m_SysMemMove);

	if (nullptr != _Origin.m_SB)
		CONTEXT->CopyResource(m_SB.Get(), _Origin.m_SB.Get());
}

StructuredBuffer::~StructuredBuffer()
{
}

int StructuredBuffer::Create(UINT _ElementSize, UINT _ElementCount, SB_TYPE _Type, bool _SysMemMove, void* _SysMem)
{
	m_SB = nullptr;
	m_SB_Write = nullptr;
	m_SB_Read = nullptr;
	m_SRV = nullptr;

	m_ElementSize = _ElementSize;
	m_ElementCount = _ElementCount;
	m_SysMemMove = _SysMemMove;
	m_SBType = _Type;


	m_Desc.ByteWidth = m_ElementSize * m_ElementCount;

	if (SRV_ONLY == m_SBType)
		m_Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	else if (SRV_UAV == m_SBType)
		m_Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

	m_Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	m_Desc.CPUAccessFlags = 0;
	m_Desc.Usage = D3D11_USAGE_DEFAULT;
	m_Desc.StructureByteStride = m_ElementSize;

	HRESULT hr = S_OK;

	if (nullptr != _SysMem)
	{
		D3D11_SUBRESOURCE_DATA tSub = {};
		tSub.pSysMem = _SysMem;
		hr = DEVICE->CreateBuffer(&m_Desc, &tSub, m_SB.GetAddressOf());
	}
	else
	{
		hr = DEVICE->CreateBuffer(&m_Desc, nullptr, m_SB.GetAddressOf());
	}

	if (FAILED(hr))
	{
		errno_t err = GetLastError();
		assert(nullptr);
		return E_FAIL;
	}

	// ShaderResourceView
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.ViewDimension = D3D_SRV_DIMENSION_BUFFER;
	SRVDesc.BufferEx.NumElements = _ElementCount;

	if (FAILED(DEVICE->CreateShaderResourceView(m_SB.Get(), &SRVDesc, m_SRV.GetAddressOf())))
	{
		assert(nullptr);
		return E_FAIL;
	}

	// UAV
	if (SB_TYPE::SRV_UAV == m_SBType)
	{
		// UnorderedAccessView
		D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		UAVDesc.Buffer.NumElements = _ElementCount;

		if (FAILED(DEVICE->CreateUnorderedAccessView(m_SB.Get(), &UAVDesc, m_UAV.GetAddressOf())))
		{
			assert(nullptr);
			return E_FAIL;
		}
	}

	// SystemMemMove
	if (!m_SysMemMove)
		return S_OK;

	// Write
	D3D11_BUFFER_DESC WriteDesc = m_Desc;
	WriteDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	WriteDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	WriteDesc.Usage = D3D11_USAGE_DYNAMIC;

	if (nullptr != _SysMem)
	{
		D3D11_SUBRESOURCE_DATA tSub = {};
		tSub.pSysMem = _SysMem;
		hr = DEVICE->CreateBuffer(&WriteDesc, &tSub, m_SB_Write.GetAddressOf());
	}
	else
	{
		hr = DEVICE->CreateBuffer(&WriteDesc, nullptr, m_SB_Write.GetAddressOf());
	}


	// Read
	D3D11_BUFFER_DESC ReadBuffer = m_Desc;
	ReadBuffer.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	ReadBuffer.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	ReadBuffer.Usage = D3D11_USAGE_DEFAULT;

	if (nullptr != _SysMem)
	{
		D3D11_SUBRESOURCE_DATA tSub = {};
		tSub.pSysMem = _SysMem;
		hr = DEVICE->CreateBuffer(&ReadBuffer, &tSub, m_SB_Read.GetAddressOf());
	}
	else
	{
		hr = DEVICE->CreateBuffer(&ReadBuffer, nullptr, m_SB_Read.GetAddressOf());
	}

	if (FAILED(hr))
	{
		assert(nullptr);
		return E_FAIL;
	}

	return S_OK;
}

void StructuredBuffer::SetData(void* _SysMem, UINT _DataSize)
{
	assert(m_Desc.ByteWidth >= _DataSize);

	if (0 == _DataSize)
		_DataSize = m_Desc.ByteWidth;

	// SysMem -> WriteBuffer
	D3D11_MAPPED_SUBRESOURCE mapSub = {};
	CONTEXT->Map(m_SB_Write.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapSub);
	memcpy(mapSub.pData, _SysMem, _DataSize);
	CONTEXT->Unmap(m_SB_Write.Get(), 0);

	// WirteBuffer -> MainBuffer
	CONTEXT->CopyResource(m_SB.Get(), m_SB_Write.Get());
}

void StructuredBuffer::GetData(void* _DstSysMem, UINT _DataSize)
{
	assert(m_Desc.ByteWidth >= _DataSize);

	if (0 == _DataSize)
		_DataSize = m_Desc.ByteWidth;

	// Main -> Read
	CONTEXT->CopyResource(m_SB_Read.Get(), m_SB.Get());

	// Read -> SysMem
	D3D11_MAPPED_SUBRESOURCE mapSub = {};
	CONTEXT->Map(m_SB_Read.Get(), 0, D3D11_MAP_READ, 0, &mapSub);
	memcpy(_DstSysMem, mapSub.pData, _DataSize);
	CONTEXT->Unmap(m_SB_Read.Get(), 0);
}

void StructuredBuffer::Binding(UINT _RegisterNum)
{
	m_RecentRegisterNum = _RegisterNum;

	CONTEXT->VSSetShaderResources(_RegisterNum, 1, m_SRV.GetAddressOf());
	CONTEXT->HSSetShaderResources(_RegisterNum, 1, m_SRV.GetAddressOf());
	CONTEXT->DSSetShaderResources(_RegisterNum, 1, m_SRV.GetAddressOf());
	CONTEXT->GSSetShaderResources(_RegisterNum, 1, m_SRV.GetAddressOf());
	CONTEXT->PSSetShaderResources(_RegisterNum, 1, m_SRV.GetAddressOf());
}

void StructuredBuffer::Clear()
{
	if (-1 == m_RecentRegisterNum)
		return;

	ID3D11ShaderResourceView* pSRV = nullptr;
	CONTEXT->VSSetShaderResources(m_RecentRegisterNum, 1, &pSRV);
	CONTEXT->HSSetShaderResources(m_RecentRegisterNum, 1, &pSRV);
	CONTEXT->DSSetShaderResources(m_RecentRegisterNum, 1, &pSRV);
	CONTEXT->GSSetShaderResources(m_RecentRegisterNum, 1, &pSRV);
	CONTEXT->PSSetShaderResources(m_RecentRegisterNum, 1, &pSRV);
	m_RecentRegisterNum = -1;
}

void StructuredBuffer::Binding_CS_SRV(UINT _RegisterNum)
{
	m_RecentSRVNum = _RegisterNum;
	CONTEXT->CSSetShaderResources(_RegisterNum, 1, m_SRV.GetAddressOf());
}

void StructuredBuffer::Clear_CS_SRV()
{
	ID3D11ShaderResourceView* pSRV = nullptr;
	CONTEXT->CSSetShaderResources(m_RecentSRVNum, 1, &pSRV);
	m_RecentSRVNum = -1;
}

void StructuredBuffer::Binding_CS_UAV(UINT _RegisterNum)
{
	m_RecentUAVNum = _RegisterNum;
	UINT i = -1;
	CONTEXT->CSSetUnorderedAccessViews(_RegisterNum, 1, m_UAV.GetAddressOf(), &i);
}

void StructuredBuffer::Clear_CS_UAV()
{
	ID3D11UnorderedAccessView* pUAV = nullptr;
	UINT i = -1;
	CONTEXT->CSSetUnorderedAccessViews(m_RecentUAVNum, 1, &pUAV, &i);
	m_RecentUAVNum = -1;
}