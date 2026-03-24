#pragma once

#include "assets.h"
#include "PathMgr.h"
#include "ALevel.h"

class AssetMgr
	: public singleton<AssetMgr>
{
	SINGLE(AssetMgr)
private:
	map<wstring, Ptr<Asset>>	m_mapAsset[(UINT)ASSET_TYPE::END];
	bool						m_Changed;

public:
	void Init();
	bool IsChanged()
	{
		bool Changed = m_Changed;
		m_Changed = false;
		return Changed;
	}

private:
	void CreateEngineMesh();
	void CreateEngineShader();
	void CreateEngineTexture();
	void CreateEngineMaterial();
	void CreateEngineSprite();

public:
	void AddAsset(const wstring& _Key, Ptr<Asset> _Asset);

	void GetAssetNames(ASSET_TYPE _type, vector<wstring>& _vec);

	Ptr<Asset> FindAsset(ASSET_TYPE _Type, const wstring& _Key);

	template<typename T>
	Ptr<T> Find(const wstring& _Key);

	template<typename T>
	Ptr<T> Load(const wstring& _Key, const wstring& _RelativePath);
};

template<typename T>
ASSET_TYPE GetAssetType()
{	
	if constexpr (std::is_same_v<T, AMesh>)
		return ASSET_TYPE::MESH;		
	else if constexpr (std::is_same_v<T, AMaterial>)
		return ASSET_TYPE::MATERIAL;
	else if constexpr (std::is_same_v<T, AGraphicShader>)
		return ASSET_TYPE::GRAPHICSHADER;
	else if constexpr (std::is_same_v<T, ATexture>)
		return ASSET_TYPE::TEXTURE;
	else if constexpr (std::is_same_v<T, ASprite>)
		return ASSET_TYPE::SPRITE;
	else if constexpr (std::is_same_v<T, AFlipbook>)
		return ASSET_TYPE::FLIPBOOK;
	else if constexpr (std::is_same_v<T, ATileMap>)
		return ASSET_TYPE::TILEMAP;
	else if constexpr (std::is_same_v<T, ALevel>)
		return ASSET_TYPE::LEVEL;

	return ASSET_TYPE::END;
}

template<typename T>
Ptr<T> AssetMgr::Find(const wstring& _Key)
{		
	ASSET_TYPE Type = GetAssetType<T>();

	map<wstring, Ptr<Asset>>::iterator iter = m_mapAsset[(UINT)Type].find(_Key);

	if (iter == m_mapAsset[(UINT)Type].end())
		return nullptr;

	return (T*)iter->second.Get();
}

template<typename T>
Ptr<T> AssetMgr::Load(const wstring& _Key, const wstring& _RelativePath)
{
	// 동일키로 먼저 등록된 에셋이 있는지 확인
	Ptr<T> pAsset = Find<T>(_Key);
	
	// 동일키로 먼저 등록된 에셋이 있으면, 그걸 반환
	if (nullptr != pAsset)
	{
		pAsset->Load(CONTENT_PATH + _RelativePath);
		pAsset->SetRelativePath(_RelativePath);
		pAsset->SetKey(_Key);
		m_Changed = true;
		return pAsset;
	}

	// 에셋 객체 생성
	pAsset = new T;

	// 입력된 경로로부터 에셋 로딩작업 진행	
	pAsset->Load(CONTENT_PATH + _RelativePath);

	// T 타입에 해당하는 실제 AssetType 확인
	ASSET_TYPE type = GetAssetType<T>();
	
	// 맵에 에셋등록
	m_mapAsset[(UINT)type].insert(make_pair(_Key, pAsset.Get()));

	// 에셋이 자신이 매니저에 등롣될때 상요된 Key 와, 
	// 자신이 어떤 경로에 있는 파일로부터 로딩된 에셋인지 스스로 알 수 있도록 해줌
	pAsset->SetKey(_Key);
	pAsset->SetRelativePath(_RelativePath);

	m_Changed = true;

	return pAsset;
}

template<typename T>
Ptr<T> LoadAssetRef(FILE* _File)
{
	// Asset 이 Null 인지 아닌지 저장
	bool IsNull = false;
	fread(&IsNull, sizeof(bool), 1, _File);

	// Asset 의 Key, RelativePath 저장
	if (IsNull)
	{
		wstring Key = LoadWString(_File);
		wstring RelativePath = LoadWString(_File);
		return AssetMgr::GetInst()->Load<T>(Key, RelativePath);
	}

	return nullptr;
}


#define FIND(Type, Key) AssetMgr::GetInst()->Find<Type>(Key)
#define LOAD(Type, AssetPath) AssetMgr::GetInst()->Load<Type>(AssetPath, AssetPath)
