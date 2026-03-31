#include "pch.h"
#include "ATileMap.h"

#include "AssetMgr.h"
#include "Source/Scripts/CTileScript.h"

namespace
{
    constexpr UINT kTileMapFileMagic = 0x50414D54; // 'TMAP'
    constexpr UINT kTileMapPlacementVersion = 5;
    constexpr UINT kEmptyLegacyTileType = 1;

    void SkipWideString(FILE* _File)
    {
        int len = 0;
        fread(&len, sizeof(int), 1, _File);
        if (0 < len)
            fseek(_File, sizeof(wchar_t) * len, SEEK_CUR);
    }

    void SkipTileShape(FILE* _File)
    {
        UINT mode = 0;
        float values[6] = {};
        fread(&mode, sizeof(UINT), 1, _File);
        fread(values, sizeof(float), 6, _File);
    }

    UINT ClampPreviewLegacyType(UINT _LegacyType)
    {
        if (_LegacyType <= kEmptyLegacyTileType)
            return kEmptyLegacyTileType;

        if (_LegacyType == (UINT)TILETYPE::FULL_BLOCK)
            return (UINT)TILETYPE::LINE_BLOCK_6;

        if ((UINT)TILETYPE::LINE_BLOCK_CUSTOM_BEGIN <= _LegacyType
            && _LegacyType <= (UINT)TILETYPE::LINE_BLOCK_CUSTOM_END)
        {
            return (UINT)TILETYPE::LINE_BLOCK_3;
        }

        if ((UINT)TILETYPE::CIRCLE_BLOCK_CUSTOM_BEGIN <= _LegacyType
            && _LegacyType <= (UINT)TILETYPE::CIRCLE_BLOCK_CUSTOM_END)
        {
            return (UINT)TILETYPE::CIRCLE_BLOCK_1;
        }

        return _LegacyType;
    }

    Ptr<ASprite> LoadPreviewSprite(UINT _LegacyType)
    {
        const UINT previewType = ClampPreviewLegacyType(_LegacyType);
        int spriteIdx = (int)previewType - 1;
        if (spriteIdx < 0)
            spriteIdx = 0;

        wchar_t key[64] = {};
        wchar_t relativePath[128] = {};
        swprintf_s(key, L"MapTest_%d", spriteIdx);
        swprintf_s(relativePath, L"Sprite\\MapTest_%d.sprite", spriteIdx);
        return AssetMgr::GetInst()->Load<ASprite>(key, relativePath);
    }
}

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
    if (nullptr == pFile)
        return E_FAIL;

    UINT magic = 0;
    fread(&magic, sizeof(UINT), 1, pFile);

    if (magic == kTileMapFileMagic)
    {
        UINT version = 0;
        fread(&version, sizeof(UINT), 1, pFile);

        UINT layoutMode = 0;
        if (version >= kTileMapPlacementVersion)
            fread(&layoutMode, sizeof(UINT), 1, pFile);

        m_LayoutMode = (layoutMode == (UINT)TILEMAP_LAYOUT_MODE::PLACEMENT)
            ? TILEMAP_LAYOUT_MODE::PLACEMENT
            : TILEMAP_LAYOUT_MODE::GRID;

        UINT row = 0;
        UINT col = 0;
        fread(&row, sizeof(UINT), 1, pFile);
        fread(&col, sizeof(UINT), 1, pFile);
        SetRowCol(row, col);

        fread(&m_TileSize, sizeof(Vec2), 1, pFile);

        UINT defCount = 0;
        fread(&defCount, sizeof(UINT), 1, pFile);
        vector<UINT> legacyTypes(defCount, kEmptyLegacyTileType);

        for (UINT i = 0; i < defCount; ++i)
        {
            SkipWideString(pFile);
            fread(&legacyTypes[i], sizeof(UINT), 1, pFile);

            UINT flags = 0;
            fread(&flags, sizeof(UINT), 1, pFile);
            SkipTileShape(pFile);
            SkipTileShape(pFile);

            UINT correctionTrigger = 0;
            Vec2 customNormal = {};
            fread(&correctionTrigger, sizeof(UINT), 1, pFile);
            fread(&customNormal, sizeof(Vec2), 1, pFile);
        }

        UINT tileCount = 0;
        fread(&tileCount, sizeof(UINT), 1, pFile);
        vector<UINT> tileTypeIndices(tileCount, 0);
        for (UINT i = 0; i < tileCount; ++i)
        {
            fread(&tileTypeIndices[i], sizeof(UINT), 1, pFile);
        }

        UINT scaleCount = 0;
        fread(&scaleCount, sizeof(UINT), 1, pFile);
        if (0 < scaleCount)
            fseek(pFile, sizeof(Vec2) * scaleCount, SEEK_CUR);

        if (version >= kTileMapPlacementVersion)
        {
            UINT placementCount = 0;
            fread(&placementCount, sizeof(UINT), 1, pFile);

            if (0 < placementCount)
            {
                const size_t placementBytes = (sizeof(UINT) + sizeof(Vec2) + sizeof(Vec2) + sizeof(float) + sizeof(float)) * (size_t)placementCount;
                fseek(pFile, (long)placementBytes, SEEK_CUR);
            }
        }

        m_Atlas = FIND(ATexture, L"MapTest");

        if (m_LayoutMode == TILEMAP_LAYOUT_MODE::GRID)
        {
            const UINT maxCount = min((UINT)m_vecSpriteInfo.size(), tileCount);
            for (UINT i = 0; i < maxCount; ++i)
            {
                UINT typeIdx = tileTypeIndices[i];
                UINT legacyType = kEmptyLegacyTileType;
                if (typeIdx < legacyTypes.size())
                    legacyType = legacyTypes[typeIdx];

                m_vecSpriteInfo[i] = LoadPreviewSprite(legacyType);
            }
        }
        else
        {
            // Placement 기반 타일맵은 현재 grid sprite 렌더 경로로 직접 그리지 않는다.
            // 레벨 로드만 정상 통과시키고, 저장된 오브젝트/충돌 데이터를 그대로 사용한다.
            m_vecSpriteInfo.clear();
        }

        fclose(pFile);
        return 0;
    }

    fseek(pFile, 0, SEEK_SET);

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

    m_LayoutMode = TILEMAP_LAYOUT_MODE::GRID;
	fclose(pFile);
	return 0;
}
