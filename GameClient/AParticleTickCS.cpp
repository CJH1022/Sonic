#include "pch.h"
#include "AParticleTickCS.h"
#include "Device.h"

AParticleTickCS::AParticleTickCS()
	: AComputeShader(L"Shader\\particle_tick.fx", "CS_ParticleTick", 256, 1, 1)
	, m_ParticleBuffer(nullptr)
{
}

AParticleTickCS::~AParticleTickCS()
{
}

int AParticleTickCS::Binding()
{
	if (nullptr == m_ParticleBuffer)
		return E_FAIL;
		
	m_ParticleBuffer->Binding_CS_UAV(0);	
	m_SpawnBuffer->Binding_CS_UAV(1);	
	m_ModuleBuffer->Binding_CS_SRV(20);

	// 파티클 버퍼 최대개수 값
	m_Const.iArr[0] = (int)m_ParticleBuffer->GetElementCount();

	// 노이즈 텍스쳐 바인딩
	if (nullptr != m_NoiseTex)
	{
		m_NoiseTex->Binding_CS_SRV(0);
		m_Const.IsTex[0] = 1;
	}

	// 넘길게 없으면 해당 레지스터 nullptr 로 밀어버리기
	else
	{	
		ID3D11ShaderResourceView* pSRV = nullptr;
		ID3D11DeviceContext* pContext = Device::GetInst()->GetContext().Get();
		pContext->CSSetShaderResources(0, 1, &pSRV);
		m_Const.IsTex[0] = 0;
	}

	return S_OK;
}

void AParticleTickCS::CalcGroupNum()
{
	m_GroupX = m_ParticleBuffer->GetElementCount() / m_GroupPerThreadX;
	if (m_ParticleBuffer->GetElementCount() % m_GroupPerThreadX)
		m_GroupX += 1;

	m_GroupY = 1;
	m_GroupZ = 1;
}

void AParticleTickCS::Clear()
{
	m_ParticleBuffer->Clear_CS_UAV();
	m_ParticleBuffer = nullptr;

	m_SpawnBuffer->Clear_CS_UAV();
	m_SpawnBuffer = nullptr;

	m_ModuleBuffer->Clear_CS_SRV();
	m_ModuleBuffer = nullptr;

	if (nullptr != m_NoiseTex)
	{
		m_NoiseTex->Clear_CS_SRV(0);
		m_NoiseTex = nullptr;
	}
}
