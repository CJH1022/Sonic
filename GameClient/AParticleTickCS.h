#pragma once
#include "AComputeShader.h"
#include "ATexture.h"

class AParticleTickCS :
    public AComputeShader
{
private:
    Ptr<StructuredBuffer>   m_ParticleBuffer;
    Ptr<StructuredBuffer>   m_SpawnBuffer;
    Ptr<StructuredBuffer>   m_ModuleBuffer;
    Ptr<ATexture>           m_NoiseTex;

public:
    void SetParticleBuffer(Ptr<StructuredBuffer> _Buffer) { m_ParticleBuffer = _Buffer; }
    void SetSpawnBuffer(Ptr<StructuredBuffer> _Buffer) { m_SpawnBuffer = _Buffer; }
    void SetModuleBuffer(Ptr<StructuredBuffer> _Buffer) { m_ModuleBuffer = _Buffer; }
    void SetNoiseTex(Ptr<ATexture> _Noise) { m_NoiseTex = _Noise; }
    void SetStop(bool _Stop) { m_Const.iArr[1] = _Stop; }

public:
    virtual int Binding() override;
    virtual void CalcGroupNum() override;
    virtual void Clear() override;

public:
    AParticleTickCS();
    ~AParticleTickCS();
};

