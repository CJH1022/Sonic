#include "pch.h"
#include "ALevel.h"



ALevel::ALevel()
	: Asset(ASSET_TYPE::LEVEL)
	, m_Matrix{}
	, m_Changed(false)
{
	for (int i = 0; i < MAX_LAYER; ++i)
	{
		m_arrLayer[i].m_LayerIdx = i;
	}
}

ALevel::~ALevel()
{
}

void ALevel::AddObject(int _LayerIdx, Ptr<GameObject> _Object)
{
	m_arrLayer[_LayerIdx].AddObject(_Object);
}

void ALevel::Deregister()
{
	for (UINT i = 0; i < MAX_LAYER; ++i)
	{
		m_arrLayer[i].DeregisterObject();
	}	
}


void ALevel::Begin()
{
	for (UINT i = 0; i < MAX_LAYER; ++i)
	{
		m_arrLayer[i].Begin();
	}
}

void ALevel::Tick()
{
	for (UINT i = 0; i < MAX_LAYER; ++i)
	{
		m_arrLayer[i].Tick();
	}
}

void ALevel::FinalTick()
{
	for (UINT i = 0; i < MAX_LAYER; ++i)
	{
		m_arrLayer[i].FinalTick();
	}
}

void ALevel::CheckCollisionLayer(UINT _LayerIdx1, UINT _LayerIdx2)
{
	UINT Row = _LayerIdx1;
	UINT Col = _LayerIdx2;

	// 더 작은 레이어 인덱스를 행 으로 사용한다.
	if (_LayerIdx2 < _LayerIdx1)
	{
		Row = _LayerIdx2;
		Col = _LayerIdx1;
	}
	
	m_Matrix[Row] ^= (1 << Col);
}

void ALevel::CheckCollisionLayer(const wstring& _LayerName1, const wstring& _LayerName2)
{

}

Ptr<GameObject> ALevel::FindObjectByName(const wstring& _Name)
{
	for (UINT i = 0; i < MAX_LAYER; ++i)
	{
		const vector<Ptr<GameObject>>& vecParents = m_arrLayer[i].GetParentObjects();

		for (size_t i = 0; i < vecParents.size(); ++i)
		{
			list<Ptr<GameObject>>  queue;
			queue.push_back(vecParents[i]);

			while (!queue.empty())
			{
				Ptr<GameObject> pObject = queue.front();
				queue.pop_front();

				// 찾았다
				if (pObject->GetName() == _Name)
					return pObject;

				const vector<Ptr<GameObject>>& vecChild = pObject->GetChild();
				for (size_t j = 0; j < vecChild.size(); ++j)
				{
					queue.push_back(vecChild[j]);
				}
			}
			
		}
	}

	// 없다
	return nullptr;
}

int ALevel::Save(const wstring& _FilePath)
{
	FILE* pFile = nullptr;
	_wfopen_s(&pFile, _FilePath.c_str(), L"wb");
	if (nullptr == pFile)
		return E_FAIL;

	SaveWString(pFile, GetName());
	fwrite(m_Matrix, sizeof(UINT), MAX_LAYER, pFile);

	for (UINT i = 0; i < MAX_LAYER; ++i)
	{
		SaveWString(pFile, m_arrLayer[i].GetName());

		const vector<Ptr<GameObject>>& vecParents = m_arrLayer[i].GetParentObjects();
		size_t ParentCount = vecParents.size();
		fwrite(&ParentCount, sizeof(size_t), 1, pFile);

		for (const auto& Object : vecParents)
		{
			Object->SaveToLevelFile(pFile);
		}
	}

	fclose(pFile);
	return S_OK;
}

int ALevel::Load(const wstring& _FilePath)
{
	FILE* pFile = nullptr;
	_wfopen_s(&pFile, _FilePath.c_str(), L"rb");
	if (nullptr == pFile)
		return E_FAIL;

	for (UINT i = 0; i < MAX_LAYER; ++i)
	{
		m_arrLayer[i].Clear();
		m_arrLayer[i].m_LayerIdx = i;
	}

	SetName(LoadWString(pFile));
	fread(m_Matrix, sizeof(UINT), MAX_LAYER, pFile);

	for (UINT i = 0; i < MAX_LAYER; ++i)
	{
		wstring LayerName = LoadWString(pFile);
		m_arrLayer[i].SetName(LayerName);

		size_t ParentCount = 0;
		fread(&ParentCount, sizeof(size_t), 1, pFile);

		for (size_t j = 0; j < ParentCount; ++j)
		{
			Ptr<GameObject> pObject = new GameObject;
			pObject->LoadFromLevelFile(pFile);
			AddObject(i, pObject);
		}
	}

	fclose(pFile);
	m_Changed = false;
	return S_OK;
}
