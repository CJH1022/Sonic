#pragma once
#include "Asset.h"


enum class SHADER_PARAM
{
    INT,
    FLOAT,
    VEC2,
    VEC4,
    MAT,
    TEX,
};

struct ShaderParam
{
    SHADER_PARAM    Type;
    int             Index;
    wstring         Desc;
    int             Step;    // ImGui 에서 값의 변화량
    bool            IsInput; // ImGui 에서 표현할 위젯 스타일
};



// 에셋 - 공유자원
// 렌더링 파이프라인 자체를 하나의 에셋으로 본다
class AGraphicShader :
    public Asset
{
private:    
    ComPtr<ID3DBlob>			m_VSBlob; // HLSL 로 작성한 VS 함수를 컴파일한 어셈블리코드를 저장시킬 버퍼
    ComPtr<ID3DBlob>			m_PSBlob; // HLSL 로 작성한 PS 함수를 컴파일한 어셈블리코드를 저장시킬 버퍼

    ComPtr<ID3D11VertexShader>	m_VS;
    ComPtr<ID3D11PixelShader>	m_PS;

    RS_TYPE                     m_RSType; // 레스터라이져 컬링모드
    DS_TYPE                     m_DSType;
    BS_TYPE                     m_BSType; // 블랜딩 공식 설정
    
    ComPtr<ID3D11InputLayout>	m_Layout; // 정점 쉐이더에 입력으로 들어오는 정점 하나의 구성정보
    D3D11_PRIMITIVE_TOPOLOGY    m_Topology; // 렌더링 과정에서, 정점들을 어떤 도형으로 인지할 것인지

    vector<ShaderParam>         m_vecShaderParam;

public:
    void AddShaderParam(SHADER_PARAM _Type, int _Idx, const wstring& _Desc, int _Step = 0, bool IsInput = true)
    {
        m_vecShaderParam.push_back(ShaderParam{ _Type , _Idx, _Desc , _Step , IsInput });
    }
    const vector<ShaderParam>& GetShaderParam() { return m_vecShaderParam; }


public:
    int CreateVertexShader(const wstring& _RelativeFilePath, const string& _FuncName);
    int CreatePixelShader(const wstring& _RelativeFilePath, const string& _FuncName);

    GET_SET(D3D11_PRIMITIVE_TOPOLOGY, Topology);
    GET_SET(RS_TYPE, RSType);
    GET_SET(DS_TYPE, DSType);
    GET_SET(BS_TYPE, BSType);    
    
    void Binding();

public:
    AGraphicShader();
    virtual ~AGraphicShader();
};

