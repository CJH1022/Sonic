#include "pch.h"
#include "CParticleRender.h"

#include "AssetMgr.h"
#include "CTransform.h"
#include "TimeMgr.h"
#include "KeyMgr.h"

CParticleRender::CParticleRender()
	: CRenderComponent(COMPONENT_TYPE::PARTICLE_RENDER)
	, m_ParticleBuffer(nullptr)
	, m_MaxParticle(1000)
	, m_Module{}
	, m_AccTime(0.f)
	, m_ModuleChanged(false)
{
	m_SpawnBuffer = new StructuredBuffer;
	m_SpawnBuffer->Create(sizeof(tSpawnCount), 1, SB_TYPE::SRV_UAV, true);

	m_ModuleBuffer = new StructuredBuffer;
	m_ModuleBuffer->Create(sizeof(tParticleModule), 1, SB_TYPE::SRV_ONLY, true);

	m_ParticleBuffer = new StructuredBuffer;
	m_ParticleBuffer->Create(sizeof(tParticle), m_MaxParticle, SB_TYPE::SRV_UAV, false);
	
	// 파티클 Tick 용 ComputeShader
	Ptr<AComputeShader> pCS = AssetMgr::GetInst()->Find<AComputeShader>(L"ParticleTickCS");

	if (nullptr == pCS)
	{
		pCS = new AParticleTickCS;
		pCS->SetName(L"ParticleTickCS");
		AssetMgr::GetInst()->AddAsset(pCS->GetName(), pCS.Get());
	}

	m_TickCS = (AParticleTickCS*)pCS.Get();
}

CParticleRender::CParticleRender(const CParticleRender& _Origin)
	: CRenderComponent(_Origin)
	, m_ParticleBuffer(nullptr)
	, m_SpawnBuffer(nullptr)
	, m_ModuleBuffer(nullptr)
	, m_TickCS(_Origin.m_TickCS)
	, m_MaxParticle(_Origin.m_MaxParticle)
	, m_Module(_Origin.m_Module)
	, m_ModuleChanged(false)
	, m_AccTime(0.f)
	, m_ParticleTex(_Origin.m_ParticleTex)
{
	m_SpawnBuffer = new StructuredBuffer;
	m_SpawnBuffer->Create(sizeof(tSpawnCount), 1, SB_TYPE::SRV_UAV, true);

	m_ModuleBuffer = new StructuredBuffer;
	m_ModuleBuffer->Create(sizeof(tParticleModule), 1, SB_TYPE::SRV_ONLY, true);

	m_ParticleBuffer = new StructuredBuffer;
	m_ParticleBuffer->Create(sizeof(tParticle), m_MaxParticle, SB_TYPE::SRV_UAV, false);

	if (nullptr == m_TickCS)
	{
		m_TickCS = new AParticleTickCS;
		m_TickCS->SetName(L"ParticleTickCS");
		AssetMgr::GetInst()->AddAsset(m_TickCS->GetName(), m_TickCS.Get());
	}
}

CParticleRender::~CParticleRender()
{
}

void CParticleRender::FinalTick()
{
	m_ModuleChanged = true;
	m_Module.ObjectWorldPos = Transform()->GetWorldPos();

	// 파티클 스폰 개수 계산
	CalcSpawnCount();

	// 모듈 내용이 변경된 경우, 모듈버퍼 업데이트
	if (m_ModuleChanged)
	{
		m_ModuleChanged = false;
		m_ModuleBuffer->SetData(&m_Module);
	}

	// 파티클 컴퓨트 쉐이더 실행
	{
		m_TickCS->SetSpawnBuffer(m_SpawnBuffer);
		m_TickCS->SetParticleBuffer(m_ParticleBuffer);
		m_TickCS->SetModuleBuffer(m_ModuleBuffer);
		m_TickCS->SetNoiseTex(AssetMgr::GetInst()->Load<ATexture>(L"Texture\\noise\\noise_03.jpg", L"Texture\\noise\\noise_03.jpg"));

		// 마우스 오른쪽 버튼 누루고 있을때는 잠시 정지
		if (KEY_PRESSED(KEY::RBTN))
			m_TickCS->SetStop(true);
		else
			m_TickCS->SetStop(false);

		// CS 실행
		m_TickCS->Execute();
	}	
}

void CParticleRender::Render()
{
	// GameObject 행렬정보 및 카메라 View, Proj 행렬 정보 바인딩
	Transform()->Binding();

	// 업데이트가 끝난 파티클들의 정보를 담고있는 파티클 버퍼를 t20 레지스터에 바인딩
	m_ParticleBuffer->Binding(20);

	// 파티클 동작(시뮬) 방식 정보(모듈버퍼) t21 레지스터에 바인딩
	m_ModuleBuffer->Binding(21);

	GetMaterial()->GetShader()->SetRSType(RS_TYPE::WIRE_FRAME);
	GetMaterial()->SetTexture(TEX_0, m_ParticleTex);
	GetMaterial()->SetScalar(VEC4_0, Transform()->GetWorldPos());

	// 파티클 재질 업데이트
	GetMaterial()->Binding();

	// 파티클 렌더링(인스턴싱 기법 활용)
	GetMesh()->Render_Particle(m_MaxParticle);

	// 바인딩 했던 버퍼들 다시 해제(다음 프레임에, CS 쪽에서 사용해야하기 때문)
	m_ParticleBuffer->Clear();
	m_ModuleBuffer->Clear();
}


void CParticleRender::CreateMaterial()
{
	if (nullptr == GetMesh())
		SetMesh(AssetMgr::GetInst()->Find<AMesh>(L"PointMesh"));

	if (nullptr != GetMaterial())
		return;

	Ptr<AMaterial> pMtrl = AssetMgr::GetInst()->Find<AMaterial>(L"ParticleMtrl");
	if (nullptr != pMtrl)
	{
		SetMaterial(pMtrl);
		return;
	}

	// 파티클 전용 렌더링 쉐이더
	Ptr<AGraphicShader> pShader = new AGraphicShader;
	pShader->SetName(L"ParticleRenderShader");

	pShader->CreateVertexShader(L"Shader\\particle.fx", "VS_Particle");
	pShader->CreateGeometryShader(L"Shader\\particle.fx", "GS_Particle");
	pShader->CreatePixelShader(L"Shader\\particle.fx", "PS_Particle");
	pShader->SetBSType(BS_TYPE::ONE_ONE);
	pShader->SetRSType(RS_TYPE::CULL_NONE);
	pShader->SetDSType(DS_TYPE::NO_TEST_NO_WRITE);
	pShader->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	AssetMgr::GetInst()->AddAsset(pShader->GetName(), pShader.Get());

	pMtrl = new AMaterial;
	pMtrl->SetName(L"ParticleMtrl");
	pMtrl->SetShader(pShader);
	pMtrl->SetDomain(RENDER_DOMAIN::DOMAIN_TRANSPARENT);
	AssetMgr::GetInst()->AddAsset(pMtrl->GetName(), pMtrl.Get());

	// 재질 설정
	SetMaterial(pMtrl);
}

void CParticleRender::CalcSpawnCount()
{
	// 엔진 DT 사용, 레벨 정지 시에도 시뮬레이션 확인해보기 위해서
	m_AccTime += E_DT;
	float Term = 1.f / m_Module.SpawnRate;

	tSpawnCount count = {};

	if (Term < m_AccTime)
	{
		m_AccTime -= Term;
		count.SpawnCount = 1;
	}

	if (m_Module.Module[(int)PARTICLE_MODULE::SPAWN_BURST] && 0 < m_Module.SpawnBurstRepeat)
	{
		static float AccTime = 0.f;
		AccTime += E_DT;

		if (m_Module.SpawnBurstRepeatTime < AccTime)
		{
			count.SpawnCount += m_Module.SpawnBurstCount;

			m_Module.SpawnBurstRepeat -= 1;
			AccTime -= m_Module.SpawnBurstRepeatTime;
		}
	}

	if (0 < count.SpawnCount)
	{
		m_SpawnBuffer->SetData(&count);
	}
}


void CParticleRender::SaveToLevelFile(FILE* _File)
{
	SaveAssetRef(_File, m_TickCS.Get());
	SaveAssetRef(_File, m_ParticleTex.Get());

	fwrite(&m_MaxParticle, sizeof(UINT), 1, _File);
	fwrite(&m_Module, sizeof(tParticleModule), 1, _File);
}

void CParticleRender::LoadFromLevelFile(FILE* _File)
{
	m_TickCS = (AParticleTickCS*)LoadAssetRef<AComputeShader>(_File).Get();
	m_ParticleTex = LoadAssetRef<ATexture>(_File);

	fread(&m_MaxParticle, sizeof(UINT), 1, _File);
	fread(&m_Module, sizeof(tParticleModule), 1, _File);
}
