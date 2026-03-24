#pragma once
#include "Component.h"

class CLight2D :
    public Component
{
private:
    Light2DInfo     m_Info;

public:
    virtual void FinalTick() override;    

public:
    const Light2DInfo& GetInfo() { return m_Info; }

    LIGHT_TYPE GetLightType() { return m_Info.Type; }
    void SetLightType(LIGHT_TYPE _Type) { m_Info.Type = _Type; }

    Vec3 GetLightColor() { return m_Info.Color; }
    void SetLightColor(Vec3 _Color) { m_Info.Color = _Color; }

    Vec3 GetLightDir() { return m_Info.LightDir; }
    
    Vec3 GetAmbient() { return m_Info.Ambient; }
    void SetAmbient(Vec3 _Ambient) { m_Info.Ambient = _Ambient; }

    float GetRadius() { return m_Info.Radius; }
    void SetRadius(float _R) { m_Info.Radius = _R; }

    float GetAngle() { return m_Info.Angle; }
    void SetAngle(float _Angle) { m_Info.Angle = _Angle; }

    virtual void SaveToLevelFile(FILE* _File) override;
    virtual void LoadFromLevelFile(FILE* _File) override;

    CLONE(CLight2D);
public:
    CLight2D();
    virtual ~CLight2D();
};

