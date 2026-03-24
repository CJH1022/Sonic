#pragma once
#include "Component.h"

#include "AMesh.h"
#include "AMaterial.h"

class CRenderComponent :
    public Component
{
private:
    Ptr<AMesh>          m_Mesh;

    Ptr<AMaterial>      m_Mtrl;         // 현재 사용중인 재질
    Ptr<AMaterial>      m_SharedMtrl;   // 공유 재질(에셋매니저의 관리를 받는..)
    Ptr<AMaterial>      m_DynamicMtrl;  // 나만의 동적 재질(쓰고 버리는..)

public:
    GET_SET(Ptr<AMesh>, Mesh);
    
    void SetMaterial(Ptr<AMaterial> _Mtrl);
    Ptr<AMaterial> GetMaterial() { return m_Mtrl; }
    Ptr<AMaterial> GetSharedMaterial();


public:
    Ptr<AMaterial> CreateDynamicMaterial();


public:
    virtual void Init() override;
    virtual void Render() = 0;

    // 자신이 사용할 재질 로딩 및 생성
    virtual void CreateMaterial() = 0; 

    virtual void SaveToLevelFile(FILE* _File) override;
    virtual void LoadFromLevelFile(FILE* _File) override;

public:
    CRenderComponent(COMPONENT_TYPE _Type);
    CRenderComponent(const CRenderComponent& _Origin);
    virtual ~CRenderComponent();
};

