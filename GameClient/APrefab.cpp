#include "pch.h"
#include "APrefab.h"

APrefab::APrefab()
	: Asset(ASSET_TYPE::PREFAB)
{
}

APrefab::~APrefab()
{
}

GameObject* APrefab::Instantiate()
{
	if (nullptr == m_ProtoObj)
		return nullptr;

	return m_ProtoObj->Clone();
}

int APrefab::Save(const wstring& _FilePath)
{
	if (nullptr == m_ProtoObj)
		return E_FAIL;

	FILE* pFile = nullptr;
	_wfopen_s(&pFile, _FilePath.c_str(), L"wb");
	if (nullptr == pFile)
		return E_FAIL;

	m_ProtoObj->SaveToLevelFile(pFile);

	fclose(pFile);
	return S_OK;
}

int APrefab::Load(const wstring& _FilePath)
{
	FILE* pFile = nullptr;
	_wfopen_s(&pFile, _FilePath.c_str(), L"rb");
	if (nullptr == pFile)
		return E_FAIL;

	m_ProtoObj = new GameObject;
	m_ProtoObj->LoadFromLevelFile(pFile);

	fclose(pFile);
	return S_OK;
}
