#include "pch.h"
#include "AMesh.h"

#include "Device.h"

AMesh::AMesh()
	: Asset(ASSET_TYPE::MESH)
	, m_VBDesc{}
	, m_IBDesc{}
	, m_VtxSysMem(nullptr)
	, m_IdxSysMem(nullptr)
{
}

AMesh::~AMesh()
{
	delete[] m_VtxSysMem;
	delete[] m_IdxSysMem;
}

int AMesh::Create(Vtx* _VtxSysMem, UINT _VtxCount, UINT* _IdxSysMem, UINT _IdxCount)
{		
	m_VtxCount = _VtxCount;
	m_IdxCount = _IdxCount;

	m_VBDesc.ByteWidth = sizeof(Vtx) * m_VtxCount;	// 버퍼의 크기	
	m_VBDesc.Usage = D3D11_USAGE_DEFAULT;
	m_VBDesc.CPUAccessFlags = 0;
	m_VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	
	D3D11_SUBRESOURCE_DATA tSub = {};
	tSub.pSysMem = _VtxSysMem;

	if (FAILED(DEVICE->CreateBuffer(&m_VBDesc, &tSub, m_VB.GetAddressOf())))
	{
		return E_FAIL;
	}

	// 버텍스 데이터를 동적할당 해서 따로 저장해둔다.
	m_VtxSysMem = new Vtx[_VtxCount];
	for (int i = 0; i < _VtxCount; ++i)
	{
		m_VtxSysMem[i] = _VtxSysMem[i];
	}
	
	
	// 버퍼의 크기
	m_IBDesc.ByteWidth = sizeof(UINT) * m_IdxCount;
	
	m_IBDesc.Usage = D3D11_USAGE_DEFAULT;
	m_IBDesc.CPUAccessFlags = 0;

	// 버퍼 용도
	m_IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	// 처음 버퍼 생성할때 전달시킬 데이터의 시작주소를 Sub 구조체에 담아서 CreateBuffer 함수에 넣어준다.
	tSub = {};
	tSub.pSysMem = _IdxSysMem;

	if (FAILED(DEVICE->CreateBuffer(&m_IBDesc, &tSub, m_IB.GetAddressOf())))
	{
		return E_FAIL;
	}

	// 인덱스 데이터를 동적할당 해서 따로 저장해둔다.
	m_IdxSysMem = new UINT[_IdxCount];
	for (int i = 0; i < _IdxCount; ++i)
	{
		m_IdxSysMem[i] = _IdxSysMem[i];
	}

	return S_OK;
}

void AMesh::Binding()
{
	// 정점을 프로그래머가 설계하기 때문에, 전달 한 버텍스 버퍼안에서, 하나의 정점의 단위크기를 알려줘야 한다.
	UINT Stride = sizeof(Vtx);
	UINT Offset = 0;
	CONTEXT->IASetVertexBuffers(0, 1, m_VB.GetAddressOf(), &Stride, &Offset);

	// 인덱스 버퍼 설정, 정점 버퍼안에 있는 정점을 가리키는 인덱스 정보, 
	// 인덱스 숫자 하나의 크기가 몇 바이트인지 픽셀포맷으로 알려준다.
	CONTEXT->IASetIndexBuffer(m_IB.Get(), DXGI_FORMAT_R32_UINT, 0);
}

void AMesh::Render()
{
	Binding();

	// 렌더링 파이프라인 시작
	// Draw 가 호출되기 전까지 설정해놓은 세팅을 기반으로 실제 렌더링 파이프라인이 실행됨
	// 그 이전까지는 각 단계별로 실행할 옵션을 설정만 함
	CONTEXT->DrawIndexed(m_IdxCount, 0, 0);
}
