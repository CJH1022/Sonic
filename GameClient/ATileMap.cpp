#include "pch.h"
#include "ATileMap.h"

#include "AssetMgr.h"

ATileMap::ATileMap()
	: Asset(ASSET_TYPE::TILEMAP)
	, m_Row(0)
	, m_Col(0)
{
}

ATileMap::~ATileMap()
{
}

void ATileMap::SetRowCol(UINT _Row, UINT _Col)
{
	m_Row = _Row;
	m_Col = _Col;
	m_vecSpriteInfo.resize(m_Row * m_Col);
}

void ATileMap::SetSprite(UINT _Row, UINT _Col, Ptr<ASprite> _Sprite)
{
	if (nullptr == m_Atlas || _Sprite->GetAtlas() != m_Atlas)
		return;

	// 2 차원 행렬 좌표를 1차원 인덱스로 변환
	int Idx = _Row * m_Col + _Col;
	m_vecSpriteInfo[Idx] = _Sprite;
}

int ATileMap::Save(const wstring& _FilePath)
{
	FILE* pFile = nullptr;
	_wfopen_s(&pFile, _FilePath.c_str(), L"wb");
		

	fwrite(&m_Row, sizeof(UINT), 1, pFile);
	fwrite(&m_Col, sizeof(UINT), 1, pFile);
	fwrite(&m_TileSize, sizeof(Vec2), 1, pFile);
	
	SaveAssetRef(pFile, m_Atlas.Get());

	UINT SpriteCount = (UINT)m_vecSpriteInfo.size();
	fwrite(&SpriteCount, sizeof(UINT), 1, pFile);

	for (const auto& Sprite : m_vecSpriteInfo)
	{
		SaveAssetRef(pFile, Sprite.Get());
	}	

	fclose(pFile);
	return 0;
}

int ATileMap::Load(const wstring& _FilePath)
{
	FILE* pFile = nullptr;
	_wfopen_s(&pFile, _FilePath.c_str(), L"rb");

	UINT Row = 0, Col = 0;
	fread(&Row, sizeof(UINT), 1, pFile);
	fread(&Col, sizeof(UINT), 1, pFile);
	SetRowCol(Row, Col);

	fread(&m_TileSize, sizeof(Vec2), 1, pFile);

	m_Atlas = LoadAssetRef<ATexture>(pFile);

	UINT SpriteCount = 0;
	fread(&SpriteCount, sizeof(UINT), 1, pFile);

	for (UINT i = 0; i < SpriteCount; ++i)
	{
		m_vecSpriteInfo[i] = LoadAssetRef<ASprite>(pFile);
	}

	fclose(pFile);
	return 0;
}