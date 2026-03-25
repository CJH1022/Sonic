#include "pch.h"
#include "ATileMap.h"

#include "AssetMgr.h"
#include "func.h"
#include "Source/Scripts/CTileScript.h"

namespace
{
	constexpr UINT TILEMAP_FILE_MAGIC = 0x50414D54; // TMAP
	constexpr UINT TILEMAP_FILE_VERSION = 4;
	constexpr UINT EMPTY_TILE_DEF_INDEX = 1;

	bool TryParseTileType(Ptr<ASprite> _Sprite, UINT& _OutType)
	{
		_OutType = EMPTY_TILE_DEF_INDEX;

		if (nullptr == _Sprite)
			return false;

		const wstring& key = _Sprite->GetKey();
		const wstring prefix = L"MapTest_";
		const size_t pos = key.rfind(prefix);
		if (pos == wstring::npos)
			return false;

		const wstring numberText = key.substr(pos + prefix.length());
		if (numberText.empty())
			return false;

		const int spriteIdx = _wtoi(numberText.c_str());
		if (spriteIdx < 0)
			return false;

		_OutType = (UINT)(spriteIdx + 1);
		return true;
	}

	TileShapeDesc MakeLineShape(float _A, float _B, float _C)
	{
		TileShapeDesc shape = {};
		shape.Mode = TILE_DRAW_MODE::LINE;
		shape.a = _A;
		shape.b = _B;
		shape.c = _C;
		return shape;
	}

	TileShapeDesc MakeCircleShape(float _Radius, float _CenterX, float _CenterY)
	{
		TileShapeDesc shape = {};
		shape.Mode = TILE_DRAW_MODE::CIRCLE;
		shape.r = _Radius;
		shape.centerX = _CenterX;
		shape.centerY = _CenterY;
		return shape;
	}

	TileShapeDesc MakeVerticalShape(float _X, float _C)
	{
		TileShapeDesc shape = {};
		shape.Mode = TILE_DRAW_MODE::VERTICAL;
		shape.a = _X;
		shape.c = _C;
		return shape;
	}

	void SaveTileShape(FILE* _File, const TileShapeDesc& _Shape)
	{
		UINT mode = (UINT)_Shape.Mode;
		fwrite(&mode, sizeof(UINT), 1, _File);
		fwrite(&_Shape.a, sizeof(float), 1, _File);
		fwrite(&_Shape.b, sizeof(float), 1, _File);
		fwrite(&_Shape.c, sizeof(float), 1, _File);
		fwrite(&_Shape.r, sizeof(float), 1, _File);
		fwrite(&_Shape.centerX, sizeof(float), 1, _File);
		fwrite(&_Shape.centerY, sizeof(float), 1, _File);
	}

	void LoadTileShape(FILE* _File, TileShapeDesc& _Shape)
	{
		UINT mode = 0;
		fread(&mode, sizeof(UINT), 1, _File);
		_Shape.Mode = (TILE_DRAW_MODE)mode;
		fread(&_Shape.a, sizeof(float), 1, _File);
		fread(&_Shape.b, sizeof(float), 1, _File);
		fread(&_Shape.c, sizeof(float), 1, _File);
		fread(&_Shape.r, sizeof(float), 1, _File);
		fread(&_Shape.centerX, sizeof(float), 1, _File);
		fread(&_Shape.centerY, sizeof(float), 1, _File);
	}
}

ATileMap::ATileMap()
	: Asset(ASSET_TYPE::TILEMAP)
	, m_Row(0)
	, m_Col(0)
{
	ResetToDefaultTileDefs();
}

ATileMap::~ATileMap()
{
}

void ATileMap::ResetToDefaultTileDefs()
{
	// 0번은 이전 포맷과의 호환을 위해 비워둔다.
	m_vecTileDefs.clear();
	m_vecTileDefs.resize((UINT)TILETYPE::LINE_BLOCK_VERTICAL + 1);

	m_vecTileDefs[0].Name = L"Unused";
	m_vecTileDefs[0].LegacyType = 0;
	m_vecTileDefs[0].Flags = TILE_FLAG_NONE;
	m_vecTileDefs[0].MainShape.Mode = TILE_DRAW_MODE::EMPTY;

	TileTypeDesc empty = {};
	empty.Name = L"Empty";
	empty.LegacyType = (UINT)TILETYPE::EMPTY_BLOCK;
	empty.Flags = TILE_FLAG_NONE;
	empty.MainShape.Mode = TILE_DRAW_MODE::EMPTY;
	m_vecTileDefs[(UINT)TILETYPE::EMPTY_BLOCK] = empty;

	TileTypeDesc line1 = {};
	line1.Name = L"Line Block 1";
	line1.LegacyType = (UINT)TILETYPE::LINE_BLOCK_1;
	line1.Flags = TILE_FLAG_SOLID | TILE_FLAG_ATTACHABLE;
	line1.MainShape = MakeLineShape(0.1f, 0.4f, -1.f);
	m_vecTileDefs[(UINT)TILETYPE::LINE_BLOCK_1] = line1;

	TileTypeDesc line2 = line1;
	line2.Name = L"Line Block 2";
	line2.LegacyType = (UINT)TILETYPE::LINE_BLOCK_2;
	line2.MainShape = MakeLineShape(0.4f, 0.8f, -1.f);
	m_vecTileDefs[(UINT)TILETYPE::LINE_BLOCK_2] = line2;

	TileTypeDesc line3 = line1;
	line3.Name = L"Line Block 3";
	line3.LegacyType = (UINT)TILETYPE::LINE_BLOCK_3;
	line3.MainShape = MakeLineShape(0.8f, 0.8f, -1.f);
	m_vecTileDefs[(UINT)TILETYPE::LINE_BLOCK_3] = line3;

	TileTypeDesc line4 = line1;
	line4.Name = L"Line Block 4";
	line4.LegacyType = (UINT)TILETYPE::LINE_BLOCK_4;
	line4.MainShape = MakeLineShape(0.8f, 0.4f, -1.f);
	m_vecTileDefs[(UINT)TILETYPE::LINE_BLOCK_4] = line4;

	TileTypeDesc line5 = line1;
	line5.Name = L"Line Block 5";
	line5.LegacyType = (UINT)TILETYPE::LINE_BLOCK_5;
	line5.MainShape = MakeLineShape(0.4f, 0.1f, -1.f);
	m_vecTileDefs[(UINT)TILETYPE::LINE_BLOCK_5] = line5;

	TileTypeDesc line6 = line1;
	line6.Name = L"Line Block 6";
	line6.LegacyType = (UINT)TILETYPE::LINE_BLOCK_6;
	line6.MainShape = MakeLineShape(0.1f, 0.1f, -1.f);
	m_vecTileDefs[(UINT)TILETYPE::LINE_BLOCK_6] = line6;

	TileTypeDesc circle1 = {};
	circle1.Name = L"Circle Block 1";
	circle1.LegacyType = (UINT)TILETYPE::CIRCLE_BLOCK_1;
	circle1.Flags = TILE_FLAG_SOLID | TILE_FLAG_ATTACHABLE;
	circle1.MainShape = MakeCircleShape(0.8f, 1.f, 1.f);
	m_vecTileDefs[(UINT)TILETYPE::CIRCLE_BLOCK_1] = circle1;

	TileTypeDesc circle2 = circle1;
	circle2.Name = L"Circle Block 2";
	circle2.LegacyType = (UINT)TILETYPE::CIRCLE_BLOCK_2;
	circle2.MainShape = MakeCircleShape(0.8f, 0.f, 1.f);
	m_vecTileDefs[(UINT)TILETYPE::CIRCLE_BLOCK_2] = circle2;

	TileTypeDesc circle3 = circle1;
	circle3.Name = L"Circle Block 3";
	circle3.LegacyType = (UINT)TILETYPE::CIRCLE_BLOCK_3;
	circle3.MainShape = MakeCircleShape(0.8f, 1.f, 0.f);
	circle3.CorrectionShape = MakeLineShape(0.8f, 0.8f, -1.f);
	circle3.CorrectionTrigger = TILE_CORRECTION_TRIGGER::HALF_CHECKER_FALSE;
	m_vecTileDefs[(UINT)TILETYPE::CIRCLE_BLOCK_3] = circle3;

	TileTypeDesc circle4 = circle1;
	circle4.Name = L"Circle Block 4";
	circle4.LegacyType = (UINT)TILETYPE::CIRCLE_BLOCK_4;
	circle4.MainShape = MakeCircleShape(0.8f, 0.f, 0.f);
	circle4.CorrectionShape = MakeLineShape(0.8f, 0.8f, -1.f);
	circle4.CorrectionTrigger = TILE_CORRECTION_TRIGGER::HALF_CHECKER_TRUE;
	m_vecTileDefs[(UINT)TILETYPE::CIRCLE_BLOCK_4] = circle4;

	TileTypeDesc vertical = {};
	vertical.Name = L"Vertical Wall";
	vertical.LegacyType = (UINT)TILETYPE::LINE_BLOCK_VERTICAL;
	vertical.Flags = TILE_FLAG_SOLID | TILE_FLAG_USE_CUSTOM_NORMAL;
	vertical.MainShape = MakeVerticalShape(0.5f, 1.f);
	vertical.CustomNormal = Vec2(-1.f, 0.f);
	m_vecTileDefs[(UINT)TILETYPE::LINE_BLOCK_VERTICAL] = vertical;
}

void ATileMap::SetRowCol(UINT _Row, UINT _Col)
{
	m_Row = _Row;
	m_Col = _Col;
	m_vecTileType.assign(m_Row * m_Col, EMPTY_TILE_DEF_INDEX);
	m_vecTileScale.assign(m_Row * m_Col, Vec2(1.f, 1.f));
}

void ATileMap::Resize(UINT _Row, UINT _Col)
{
	vector<UINT> oldTiles = m_vecTileType;
	vector<Vec2> oldScales = m_vecTileScale;
	const UINT oldRow = m_Row;
	const UINT oldCol = m_Col;

	m_Row = _Row;
	m_Col = _Col;
	m_vecTileType.assign(m_Row * m_Col, EMPTY_TILE_DEF_INDEX);
	m_vecTileScale.assign(m_Row * m_Col, Vec2(1.f, 1.f));

	const UINT copyRow = (oldRow < m_Row) ? oldRow : m_Row;
	const UINT copyCol = (oldCol < m_Col) ? oldCol : m_Col;

	for (UINT row = 0; row < copyRow; ++row)
	{
		for (UINT col = 0; col < copyCol; ++col)
		{
			m_vecTileType[row * m_Col + col] = oldTiles[row * oldCol + col];
			m_vecTileScale[row * m_Col + col] = oldScales[row * oldCol + col];
		}
	}
}

void ATileMap::SetTileType(UINT _Row, UINT _Col, UINT _Type)
{
	if (_Row >= m_Row || _Col >= m_Col)
		return;

	if (m_vecTileDefs.size() <= _Type)
		_Type = EMPTY_TILE_DEF_INDEX;

	const UINT idx = _Row * m_Col + _Col;
	m_vecTileType[idx] = _Type;
}

UINT ATileMap::GetTileType(UINT _Row, UINT _Col) const
{
	if (_Row >= m_Row || _Col >= m_Col)
		return EMPTY_TILE_DEF_INDEX;

	const UINT idx = _Row * m_Col + _Col;
	return m_vecTileType[idx];
}

void ATileMap::SetTileScale(UINT _Row, UINT _Col, const Vec2& _Scale)
{
	if (_Row >= m_Row || _Col >= m_Col)
		return;

	Vec2 scale = _Scale;

	if (scale.x < 0.1f) scale.x = 0.1f;
	else if (scale.x > 1.f) scale.x = 1.f;

	if (scale.y < 0.1f) scale.y = 0.1f;
	else if (scale.y > 1.f) scale.y = 1.f;

	const UINT idx = _Row * m_Col + _Col;
	m_vecTileScale[idx] = scale;
}

Vec2 ATileMap::GetTileScale(UINT _Row, UINT _Col) const
{
	if (_Row >= m_Row || _Col >= m_Col)
		return Vec2(1.f, 1.f);

	const UINT idx = _Row * m_Col + _Col;
	if (idx >= m_vecTileScale.size())
		return Vec2(1.f, 1.f);

	return m_vecTileScale[idx];
}

void ATileMap::SetSprite(UINT _Row, UINT _Col, Ptr<ASprite> _Sprite)
{
	UINT tileType = EMPTY_TILE_DEF_INDEX;
	if (!TryParseTileType(_Sprite, tileType))
		return;

	SetTileType(_Row, _Col, tileType);
}

int ATileMap::AddTileType(const TileTypeDesc& _Desc)
{
	TileTypeDesc desc = _Desc;
	if (desc.Name.empty())
		desc.Name = L"Custom Tile";

	if (desc.LegacyType == 0)
		desc.LegacyType = (UINT)TILETYPE::EMPTY_BLOCK;

	m_vecTileDefs.push_back(desc);
	return (int)m_vecTileDefs.size() - 1;
}

int ATileMap::DuplicateTileType(UINT _Idx)
{
	if (_Idx >= m_vecTileDefs.size())
		return -1;

	TileTypeDesc desc = m_vecTileDefs[_Idx];
	desc.Name += L" Copy";
	m_vecTileDefs.push_back(desc);
	return (int)m_vecTileDefs.size() - 1;
}

void ATileMap::RemoveTileType(UINT _Idx)
{
	// 0: unused, 1: empty 는 항상 남겨둔다.
	if (_Idx <= EMPTY_TILE_DEF_INDEX || _Idx >= m_vecTileDefs.size())
		return;

	m_vecTileDefs.erase(m_vecTileDefs.begin() + _Idx);

	for (size_t i = 0; i < m_vecTileType.size(); ++i)
	{
		if (m_vecTileType[i] == _Idx)
			m_vecTileType[i] = EMPTY_TILE_DEF_INDEX;
		else if (m_vecTileType[i] > _Idx)
			--m_vecTileType[i];
	}
}

TileTypeDesc* ATileMap::GetTileTypeDesc(UINT _Idx)
{
	if (_Idx >= m_vecTileDefs.size())
		return nullptr;

	return &m_vecTileDefs[_Idx];
}

const TileTypeDesc* ATileMap::GetTileTypeDesc(UINT _Idx) const
{
	if (_Idx >= m_vecTileDefs.size())
		return nullptr;

	return &m_vecTileDefs[_Idx];
}

int ATileMap::Save(const wstring& _FilePath)
{
	FILE* pFile = nullptr;
	_wfopen_s(&pFile, _FilePath.c_str(), L"wb");
	if (nullptr == pFile)
		return E_FAIL;

	fwrite(&TILEMAP_FILE_MAGIC, sizeof(UINT), 1, pFile);
	fwrite(&TILEMAP_FILE_VERSION, sizeof(UINT), 1, pFile);
	fwrite(&m_Row, sizeof(UINT), 1, pFile);
	fwrite(&m_Col, sizeof(UINT), 1, pFile);
	fwrite(&m_TileSize, sizeof(Vec2), 1, pFile);

	UINT defCount = (UINT)m_vecTileDefs.size();
	fwrite(&defCount, sizeof(UINT), 1, pFile);
	for (const TileTypeDesc& def : m_vecTileDefs)
	{
		SaveWString(pFile, def.Name);
		fwrite(&def.LegacyType, sizeof(UINT), 1, pFile);
		fwrite(&def.Flags, sizeof(UINT), 1, pFile);
		SaveTileShape(pFile, def.MainShape);
		SaveTileShape(pFile, def.CorrectionShape);

		UINT trigger = (UINT)def.CorrectionTrigger;
		fwrite(&trigger, sizeof(UINT), 1, pFile);
		fwrite(&def.CustomNormal, sizeof(Vec2), 1, pFile);
	}

	UINT tileCount = (UINT)m_vecTileType.size();
	fwrite(&tileCount, sizeof(UINT), 1, pFile);
	for (UINT tileType : m_vecTileType)
	{
		fwrite(&tileType, sizeof(UINT), 1, pFile);
	}

	UINT scaleCount = (UINT)m_vecTileScale.size();
	fwrite(&scaleCount, sizeof(UINT), 1, pFile);
	for (const Vec2& tileScale : m_vecTileScale)
	{
		fwrite(&tileScale, sizeof(Vec2), 1, pFile);
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

	if (magic == TILEMAP_FILE_MAGIC)
	{
		UINT version = 0;
		fread(&version, sizeof(UINT), 1, pFile);

		UINT row = 0;
		UINT col = 0;
		fread(&row, sizeof(UINT), 1, pFile);
		fread(&col, sizeof(UINT), 1, pFile);
		SetRowCol(row, col);

		fread(&m_TileSize, sizeof(Vec2), 1, pFile);

		if (version >= 3)
		{
			UINT defCount = 0;
			fread(&defCount, sizeof(UINT), 1, pFile);

			m_vecTileDefs.clear();
			m_vecTileDefs.resize(defCount);

			for (UINT i = 0; i < defCount; ++i)
			{
				m_vecTileDefs[i].Name = LoadWString(pFile);
				fread(&m_vecTileDefs[i].LegacyType, sizeof(UINT), 1, pFile);
				fread(&m_vecTileDefs[i].Flags, sizeof(UINT), 1, pFile);
				LoadTileShape(pFile, m_vecTileDefs[i].MainShape);
				LoadTileShape(pFile, m_vecTileDefs[i].CorrectionShape);

				UINT trigger = 0;
				fread(&trigger, sizeof(UINT), 1, pFile);
				m_vecTileDefs[i].CorrectionTrigger = (TILE_CORRECTION_TRIGGER)trigger;
				fread(&m_vecTileDefs[i].CustomNormal, sizeof(Vec2), 1, pFile);
			}

			if (m_vecTileDefs.size() <= EMPTY_TILE_DEF_INDEX)
			{
				ResetToDefaultTileDefs();
			}
		}
		else
		{
			ResetToDefaultTileDefs();
		}

		UINT tileCount = 0;
		fread(&tileCount, sizeof(UINT), 1, pFile);

		const UINT maxCount = (UINT)m_vecTileType.size();
		const UINT readCount = (tileCount < maxCount) ? tileCount : maxCount;

		for (UINT i = 0; i < readCount; ++i)
		{
			fread(&m_vecTileType[i], sizeof(UINT), 1, pFile);
			if (m_vecTileDefs.size() <= m_vecTileType[i])
				m_vecTileType[i] = EMPTY_TILE_DEF_INDEX;
		}

		for (UINT i = readCount; i < tileCount; ++i)
		{
			UINT dummy = 0;
			fread(&dummy, sizeof(UINT), 1, pFile);
		}

		m_vecTileScale.assign(m_Row * m_Col, Vec2(1.f, 1.f));

		if (version >= 4)
		{
			UINT scaleCount = 0;
			fread(&scaleCount, sizeof(UINT), 1, pFile);

			const UINT maxScaleCount = (UINT)m_vecTileScale.size();
			const UINT readScaleCount = (scaleCount < maxScaleCount) ? scaleCount : maxScaleCount;

			for (UINT i = 0; i < readScaleCount; ++i)
			{
				fread(&m_vecTileScale[i], sizeof(Vec2), 1, pFile);
				if (m_vecTileScale[i].x < 0.1f) m_vecTileScale[i].x = 0.1f;
				else if (m_vecTileScale[i].x > 1.f) m_vecTileScale[i].x = 1.f;
				if (m_vecTileScale[i].y < 0.1f) m_vecTileScale[i].y = 0.1f;
				else if (m_vecTileScale[i].y > 1.f) m_vecTileScale[i].y = 1.f;
			}

			for (UINT i = readScaleCount; i < scaleCount; ++i)
			{
				Vec2 dummy = Vec2(1.f, 1.f);
				fread(&dummy, sizeof(Vec2), 1, pFile);
			}
		}
	}
	else
	{
		// 예전 스프라이트 기반 tilemap 파일 로더
		fseek(pFile, 0, SEEK_SET);

		UINT row = 0;
		UINT col = 0;
		fread(&row, sizeof(UINT), 1, pFile);
		fread(&col, sizeof(UINT), 1, pFile);
		SetRowCol(row, col);

		fread(&m_TileSize, sizeof(Vec2), 1, pFile);
		LoadAssetRef<ATexture>(pFile);

		ResetToDefaultTileDefs();

		UINT spriteCount = 0;
		fread(&spriteCount, sizeof(UINT), 1, pFile);

		const UINT maxCount = (UINT)m_vecTileType.size();
		for (UINT i = 0; i < spriteCount; ++i)
		{
			Ptr<ASprite> pSprite = LoadAssetRef<ASprite>(pFile);
			if (i >= maxCount)
				continue;

			UINT tileType = EMPTY_TILE_DEF_INDEX;
			if (TryParseTileType(pSprite, tileType))
			{
				m_vecTileType[i] = tileType;
			}
		}

		m_vecTileScale.assign(m_Row * m_Col, Vec2(1.f, 1.f));
	}

	fclose(pFile);
	return 0;
}
