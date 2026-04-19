#pragma once
#include "Asset.h"
#include "StructuredBuffer.h"

class AComputeShader : public Asset
{
protected:
    ComPtr<ID3DBlob>            m_CSBlob;
    ComPtr<ID3D11ComputeShader> m_CS;

    MtrlConst                   m_Const;

    const UINT                  m_GroupPerThreadX;
    const UINT                  m_GroupPerThreadY;
    const UINT                  m_GroupPerThreadZ;

    UINT                        m_GroupX;
    UINT                        m_GroupY;
    UINT                        m_GroupZ;


protected:
    int CreateComputeShader(const wstring& _RelativePath, const string& _FuncName);

public:
    int Execute();

private:
    virtual int Binding() = 0;
    virtual void CalcGroupNum() = 0;
    virtual void Clear() = 0;

    
    AComputeShader* Clone() { return nullptr; }
public:
    AComputeShader();
    AComputeShader(const wstring& _RelativePath, const string& _FuncName, UINT _GroupPerX, UINT _GroupPerY, UINT _GroupPerZ);
    ~AComputeShader();
};

