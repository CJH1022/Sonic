#include "pch.h"
#include "ASprite.h"


#include "AssetMgr.h"

ASprite::ASprite()
	: Asset(ASSET_TYPE::SPRITE)
	, m_LeftTopUV(Vec2(0.f, 0.f))
	, m_SliceUV(Vec2(0.f, 0.f))
	, m_BackgroundUV(Vec2(0.f, 0.f))
	, m_OffsetUV(Vec2(0.f, 0.f))
{
}

ASprite::~ASprite()
{
}

int ASprite::Save(const wstring& _FilePath)
{
	FILE* pFile = nullptr;
	_wfopen_s(&pFile, _FilePath.c_str(), L"wb");

	// 가리키고 있던 텍스쳐가 누군지 저장
	SaveAssetRef(pFile, m_Atlas.Get());	

	// LeftTop, Slice 정보 저장
	fwrite(&m_LeftTopUV, sizeof(Vec2), 1, pFile);
	fwrite(&m_SliceUV, sizeof(Vec2), 1, pFile);
	fwrite(&m_BackgroundUV, sizeof(Vec2), 1, pFile);
	fwrite(&m_OffsetUV, sizeof(Vec2), 1, pFile);

	fclose(pFile);

	return 0;
}

int ASprite::Load(const wstring& _FilePath)
{
	FILE* pFile = nullptr;
	_wfopen_s(&pFile, _FilePath.c_str(), L"rb");

	// 저장할때 가리키던 텍스쳐를 다시 가리키기
	m_Atlas = LoadAssetRef<ATexture>(pFile);

	// LeftTop, Slice 정보 불러오기
	fread(&m_LeftTopUV, sizeof(Vec2), 1, pFile);
	fread(&m_SliceUV, sizeof(Vec2), 1, pFile);
	fread(&m_BackgroundUV, sizeof(Vec2), 1, pFile);
	fread(&m_OffsetUV, sizeof(Vec2), 1, pFile);

	fclose(pFile);

	return 0;
}

