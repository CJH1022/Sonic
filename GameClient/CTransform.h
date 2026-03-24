#pragma once
#include "Component.h"

class CTransform :
    public Component
{
private:
    Vec3        m_RelativePos;
    Vec3        m_RelativeScale;
    Vec3        m_RelativeRot;

    Vec3        m_Dir[(UINT)DIR::END];
    Matrix      m_matWorld;
    bool        m_IndependentScale; // 부모 오브젝트의 크기는 무시

public:
    Vec3 GetRelativePos() { return m_RelativePos; }
    Vec3 GetRelativeScale() { return m_RelativeScale; }
    Vec3 GetRelativeRot() { return m_RelativeRot; }

    void SetRelativePos(Vec3 _Pos) { m_RelativePos = _Pos; }
    void SetRelativeScale(Vec3 _Scale) { m_RelativeScale = _Scale; }
    void SetRelativeRot(Vec3 _Rotation) { m_RelativeRot = _Rotation; }

    Vec3 GetWorldPos() { return m_matWorld.Translation(); }
    Vec3 GetWorldScale();

    void SetIndependentScale(bool _On) { m_IndependentScale = _On; }
    bool IsIndependentScale() { return m_IndependentScale; }

    Vec3 GetDir(DIR _Type) { return m_Dir[(UINT)_Type]; }

    const Matrix& GetWorldMat() { return m_matWorld; }
    void SetWorldMat(const Matrix& _matWorld) { m_matWorld = _matWorld; }


public:
    // 세팅된 위치, 크기, 회전 정보를 하나의 월드행렬로 묶어준다.
    virtual void FinalTick() override;

    // 데이터를 GPU 메모리로 전송
    void Binding();

    CLONE(CTransform);
public:
    CTransform();
    virtual ~CTransform();
};

