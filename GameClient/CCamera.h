#pragma once
#include "Component.h"

class GameObject;

enum class PROJ_TYPE
{
    ORTHOGRAPHIC,   // 직교투영
    PERSPECTIVE,    // 원근투영
};

class CCamera :
    public Component
{
private:
    UINT                    m_LayerCheck;   // 어떤 레이어만 화면에 렌더링 할 것인지 비트체크
    PROJ_TYPE               m_ProjType;     // 투영 방식

    float                   m_Far;          // 카메라 시야 최대거리
    float                   m_Width;        // 직교투영 가로길이
    float                   m_AspectRatio;  // 종횡비 (가로 / 세로), 세로대비 가로의 길이 비율

    float                   m_FOV;          // 원근투영 시야각

    float                   m_OrthoScale;   // 직교투영 배율

    Matrix                  m_matView;      // View 행렬
    Matrix                  m_matProj;      // Proj 행렬
        
    vector<GameObject*>     m_vecOpaque;
    vector<GameObject*>     m_vecMasked;
    vector<GameObject*>     m_vecTrapsnarent;
    vector<GameObject*>     m_vecPostProcess;

public:
    GET_SET(PROJ_TYPE, ProjType);
    GET_SET(float, Far);
    GET_SET(float, Width);
    GET_SET(float, AspectRatio);    
    GET_SET(float, OrthoScale);

    UINT GetLayerCheck() { return m_LayerCheck; }

    float GetFOV() { return m_FOV * (180 / XM_PI); }
    void SetFOV(float _Degree) { m_FOV = _Degree * (XM_PI / 180.f); }

public:
    virtual void Begin() override;
    virtual void FinalTick() override;

public:
    void LayerCheckAll() { m_LayerCheck = 0xffffffff; }
    void LayerCheckClear() { m_LayerCheck = 0; }
    void LayerCheck(int _Idx);

    void SortObject();
    void Render();

    CLONE(CCamera);
public:
    CCamera();
    virtual ~CCamera();
};

