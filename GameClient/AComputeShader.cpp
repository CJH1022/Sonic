#include "pch.h"
#include "AComputeShader.h"

#include "Device.h"
#include "ConstBuffer.h"
#include "PathMgr.h"

AComputeShader::AComputeShader()
	: Asset(ASSET_TYPE::COMPUTESHADER)
	, m_GroupPerThreadX(1)
	, m_GroupPerThreadY(1)
	, m_GroupPerThreadZ(1)
	, m_GroupX(1)
	, m_GroupY(1)
	, m_GroupZ(1)
{
}

AComputeShader::AComputeShader(const wstring& _RelativePath, const string& _FuncName
	, UINT _GroupPerX, UINT _GroupPerY, UINT _GroupPerZ)
	: Asset(ASSET_TYPE::COMPUTESHADER)
	, m_GroupPerThreadX(_GroupPerX)
	, m_GroupPerThreadY(_GroupPerY)
	, m_GroupPerThreadZ(_GroupPerZ)
	, m_GroupX(1)
	, m_GroupY(1)
	, m_GroupZ(1)
{
	CreateComputeShader(_RelativePath, _FuncName);
}

AComputeShader::~AComputeShader()
{
}

int AComputeShader::CreateComputeShader(const wstring& _RelativePath, const string& _FuncName)
{
	wstring strFilePath = CONTENT_PATH + _RelativePath;

	UINT Flag = 0;

#ifdef _DEBUG
	Flag = D3DCOMPILE_DEBUG;
#endif

	if (FAILED(D3DCompileFromFile(strFilePath.c_str()
		, nullptr
		, D3D_COMPILE_STANDARD_FILE_INCLUDE
		, _FuncName.c_str(), "cs_5_0", Flag, 0
		, m_CSBlob.GetAddressOf(), nullptr)))
	{		
		
		MessageBoxA(nullptr, "컴퓨터 쉐이더 컴파일 실패", "컴퓨터 쉐이더 컴파일 실패", MB_OK);		
		return E_FAIL;
	}

	if (FAILED(DEVICE->CreateComputeShader(m_CSBlob->GetBufferPointer()
		, m_CSBlob->GetBufferSize()
		, nullptr
		, m_CS.GetAddressOf())))
	{
		return E_FAIL;
	}

	return S_OK;
}

int AComputeShader::Execute()
{
	if (FAILED(Binding()))
	{
		assert(nullptr);
		return E_FAIL;
	}

	CalcGroupNum();

	// 재질상수버퍼 재활용
	static Ptr<ConstBuffer> pBuffer = Device::GetInst()->GetCB(CB_TYPE::MATERIAL);
	pBuffer->SetData(&m_Const, sizeof(MtrlConst));
	pBuffer->Binding_CS();

	// 컴퓨트 쉐이더 바인딩
	CONTEXT->CSSetShader(m_CS.Get(), nullptr, 0);

	// 실행
	CONTEXT->Dispatch(m_GroupX, m_GroupY, m_GroupZ);

	// 바인딩 리소스 해제
	Clear();
	CONTEXT->CSSetShader(nullptr, nullptr, 0);

	return S_OK;
}
