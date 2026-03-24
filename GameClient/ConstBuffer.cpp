#include "pch.h"
#include "ConstBuffer.h"

#include "Device.h"

ConstBuffer::ConstBuffer()
	: m_Desc{}
	, m_Type(CB_TYPE::END)
{
}

ConstBuffer::~ConstBuffer()
{
}

int ConstBuffer::Create(CB_TYPE _Type, UINT _Size)
{
	m_Type = _Type;

	// 상수 버퍼 생성
	// 버퍼의 크기, 16 의 배수
	m_Desc.ByteWidth = _Size;

	// CPU 를 통해서 버퍼의 내용을 쓰거나, 읽을 수 있는지 
	// D3D11_USAGE_DYNAMIC + D3D11_CPU_ACCESS_WRITE
	// 버퍼를 생성한 이후에도, CPU 를 통해서 버퍼의 내용을 수정할 수 있다.
	m_Desc.Usage = D3D11_USAGE_DYNAMIC;
	m_Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	// 버퍼 용도
	m_Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	// 처음 버퍼 생성할때 전달시킬 데이터의 시작주소를 Sub 구조체에 담아서 CreateBuffer 함수에 넣어준다.
	if (FAILED(DEVICE->CreateBuffer(&m_Desc, nullptr, m_CB.GetAddressOf())))
	{
		return E_FAIL;
	}

    return S_OK;
}

void ConstBuffer::SetData(void* _SysMem, UINT _Size)
{
	if (_Size == 0)
	{
		_Size = m_Desc.ByteWidth;
	}

	D3D11_MAPPED_SUBRESOURCE tMapSub = {};
	CONTEXT->Map(m_CB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &tMapSub);
	memcpy(tMapSub.pData, _SysMem, _Size);
	CONTEXT->Unmap(m_CB.Get(), 0);
}

void ConstBuffer::Binding()
{
	CONTEXT->VSSetConstantBuffers((UINT)m_Type/*상수버퍼를 바인딩할 레지스터 번호*/, 1, m_CB.GetAddressOf());
	CONTEXT->HSSetConstantBuffers((UINT)m_Type/*상수버퍼를 바인딩할 레지스터 번호*/, 1, m_CB.GetAddressOf());
	CONTEXT->DSSetConstantBuffers((UINT)m_Type/*상수버퍼를 바인딩할 레지스터 번호*/, 1, m_CB.GetAddressOf());
	CONTEXT->GSSetConstantBuffers((UINT)m_Type/*상수버퍼를 바인딩할 레지스터 번호*/, 1, m_CB.GetAddressOf());
	CONTEXT->PSSetConstantBuffers((UINT)m_Type/*상수버퍼를 바인딩할 레지스터 번호*/, 1, m_CB.GetAddressOf());
}

