#ifndef _PARTICLE_TICK
#define _PARTICLE_TICK

#include "value.fx"
#include "particle_func.fx"

RWStructuredBuffer<tParticle> g_Buffer : register(u0);
RWStructuredBuffer<tSpawnCount> g_Spawn : register(u1);
StructuredBuffer<tParticleModule> g_Module : register(t20);

#define MAX_PARTICLE g_int_0
#define STOP         g_int_1



[numthreads(256, 1, 1)]
void CS_ParticleTick(int3 _ID : SV_DispatchThreadID)
{
    if (MAX_PARTICLE <= _ID.x)
        return;
    
    float E_DT = EngineDT;
    if (STOP)
    {
        E_DT = 0.f;
    }
    
    if (false == g_Buffer[_ID.x].Active)
    {        
        int LimitCount = 0;
        InterlockedAdd(g_Spawn[0].SpawnCount, -1, LimitCount);
        if (0 < LimitCount)
        {
            float NormalizedThreadID = (float) _ID.x / (float) MAX_PARTICLE;
            ParticleInit(g_Buffer[_ID.x], g_Module[0], g_tex_0, g_btex_0, NormalizedThreadID);
        }
      
        
        int Success = 0;
        
        while (g_Spawn[0].SpawnCount)
        {
            int SpawnCount = g_Spawn[0].SpawnCount;
            int Input = SpawnCount - 1;
            int Origin = 0;
        
            InterlockedCompareExchange(g_Spawn[0].SpawnCount, SpawnCount, Input, Origin);
        
            if (Origin == SpawnCount)
            {
                Success = 1;
                break;
            }
        }
        
        if (Success)
        {
            float NormalizedThreadID = (float) _ID.x / (float) MAX_PARTICLE;
            ParticleInit(g_Buffer[_ID.x], g_Module[0], g_tex_0, g_btex_0, NormalizedThreadID);
        }
    }
    else
    {
        tParticle Particle = g_Buffer[_ID.x];
        
        Particle.NormalizedAge = Particle.Age / Particle.Life;
        Particle.Force.xyz = float3(0.f, 0.f, 0.f);
        
        if (g_Module[0].Module[5])
        {
            if (g_Module[0].NoiseForceTerm <= Particle.NoiseForceAccTime)
            {
                Particle.NoiseForceAccTime -= g_Module[0].NoiseForceTerm;
                
                float NormalizedThreadID = (float) _ID.x / (float) MAX_PARTICLE;
                
                float3 vRandom = GetRandom(NormalizedThreadID, g_tex_0);
                float3 vNoiseForce = normalize(vRandom.xyz - 0.5f);

                Particle.NoiseForceDir = vNoiseForce;
            }
                        
            Particle.Force.xyz += Particle.NoiseForceDir * g_Module[0].NoiseForceScale;
            Particle.NoiseForceAccTime += E_DT;
        }
        
        float3 vAccel = Particle.Force / Particle.Mass;
        
        Particle.Velocity.xyz += vAccel * E_DT;
                
        if (0 == g_Module[0].SpaceType)
        {
            Particle.LocalPos.xyz += Particle.Velocity * E_DT;
            Particle.WorldPos.xyz = Particle.LocalPos.xyz + g_Module[0].ObjectWorldPos.xyz;
        }
        else
        {
            Particle.LocalPos += Particle.Velocity * E_DT;
            Particle.WorldPos += Particle.Velocity * E_DT;
        }
        
        
        if (g_Module[0].Module[3])
        {
            float CurScale = ((g_Module[0].EndScale - g_Module[0].StartScale) * Particle.NormalizedAge + g_Module[0].StartScale);
            Particle.WorldScale.xyz = Particle.WorldInitScale.xyz * CurScale;
        }
        
        if (g_Module[0].Module[4])
        {
            if (Particle.NormalizedAge < g_Module[0].DestNormalizedAge)
            {
                float Gradient = (g_Module[0].LimitSpeed - length(Particle.Velocity)) / (g_Module[0].DestNormalizedAge - Particle.NormalizedAge);
                float NADT = E_DT / Particle.Life;
            
                float NewSpeed = length(Particle.Velocity) + (Gradient * NADT);
                Particle.Velocity = normalize(Particle.Velocity) * NewSpeed;
            }
        }
                
        // Render Module
        if (g_Module[0].Module[6])
        {
            Particle.Color = (g_Module[0].EndColor - g_Module[0].StartColor) * Particle.NormalizedAge + g_Module[0].StartColor;

            if (g_Module[0].FadeOut)
            {
                float fRatio = saturate(1.f - (Particle.NormalizedAge - g_Module[0].StartRatio) / (1.f - g_Module[0].StartRatio));
                Particle.Color.a = fRatio;
            }
        }
        
        
        // Age Check
        Particle.Age += E_DT;
        if (Particle.Life < Particle.Age)
        {
            Particle.Active = false;
            g_Buffer[_ID.x] = Particle;
            return;
        }
                
        g_Buffer[_ID.x] = Particle;
    }
}

#endif