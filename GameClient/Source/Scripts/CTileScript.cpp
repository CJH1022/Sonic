#include "pch.h"
#include "CTileScript.h"

#include "KeyMgr.h"
#include "TimeMgr.h"
#include "RenderMgr.h"
#include "CTransform.h"
#include "LevelMgr.h"
#include "GameObject.h"
#include "CCollider2D.h"
#include "TaskMgr.h"
#include "AssetMgr.h"
#include "CTileRender.h"
#include "CPlayerScript.h"

#include <filesystem>
#include <cstdio>
#include <cfloat>

namespace
{
    float GetMaskedFullBlockMinDistance(const Vec2& _Pos, unsigned char _Mask)
    {
        float minDist = FLT_MAX;

        if (_Mask & CTileScript::FULL_FACE_LEFT)
            minDist = min(minDist, _Pos.x);
        if (_Mask & CTileScript::FULL_FACE_RIGHT)
            minDist = min(minDist, 1.f - _Pos.x);
        if (_Mask & CTileScript::FULL_FACE_TOP)
            minDist = min(minDist, _Pos.y);
        if (_Mask & CTileScript::FULL_FACE_BOTTOM)
            minDist = min(minDist, 1.f - _Pos.y);

        return minDist;
    }

    unsigned char GetMaskedFullBlockClosestFace(const Vec2& _Pos, unsigned char _Mask)
    {
        float bestDist = FLT_MAX;
        unsigned char bestFace = CTileScript::FULL_FACE_NONE;

        auto considerFace = [&](unsigned char _Face, float _Distance)
            {
                if (!(_Mask & _Face))
                    return;

                if (_Distance < bestDist)
                {
                    bestDist = _Distance;
                    bestFace = _Face;
                }
            };

        considerFace(CTileScript::FULL_FACE_TOP, _Pos.y);
        considerFace(CTileScript::FULL_FACE_BOTTOM, 1.f - _Pos.y);
        considerFace(CTileScript::FULL_FACE_LEFT, _Pos.x);
        considerFace(CTileScript::FULL_FACE_RIGHT, 1.f - _Pos.x);

        return bestFace;
    }
}

void CTileScript::InitializeTileTypeUnlockState()
{
    if (s_bTileTypeUnlockInitialized)
        return;

    for (int i = 0; i < (int)TILETYPE::END; ++i)
    {
        s_TileTypeUnlocked[i] = false;
    }

    for (int typeValue = (int)TILETYPE::EMPTY_BLOCK; typeValue <= (int)TILETYPE::CIRCLE_BLOCK_4; ++typeValue)
    {
        s_TileTypeUnlocked[typeValue] = true;
    }

    s_TileTypeUnlocked[(int)TILETYPE::FULL_BLOCK] = true;

    s_bTileTypeUnlockInitialized = true;
}

bool CTileScript::IsValidTileTypeValue(int _TypeValue)
{
    return (int)TILETYPE::EMPTY_BLOCK <= _TypeValue
        && _TypeValue < (int)TILETYPE::END;
}

TILETYPE CTileScript::ToTileType(int _TypeValue)
{
    if (!IsValidTileTypeValue(_TypeValue))
        return TILETYPE::EMPTY_BLOCK;

    return (TILETYPE)_TypeValue;
}

bool CTileScript::IsLineTileType(TILETYPE _Type)
{
    return (TILETYPE::LINE_BLOCK_1 <= _Type && _Type <= TILETYPE::LINE_BLOCK_6)
        || (TILETYPE::LINE_BLOCK_CUSTOM_BEGIN <= _Type && _Type <= TILETYPE::LINE_BLOCK_CUSTOM_END);
}

bool CTileScript::IsCircleTileType(TILETYPE _Type)
{
    return (TILETYPE::CIRCLE_BLOCK_1 <= _Type && _Type <= TILETYPE::CIRCLE_BLOCK_4)
        || (TILETYPE::CIRCLE_BLOCK_CUSTOM_BEGIN <= _Type && _Type <= TILETYPE::CIRCLE_BLOCK_CUSTOM_END);
}

bool CTileScript::IsFullBlockTileType(TILETYPE _Type)
{
    return _Type == TILETYPE::FULL_BLOCK;
}

bool CTileScript::IsLineTileTypeValue(int _TypeValue)
{
    if (!IsValidTileTypeValue(_TypeValue))
        return false;
    return IsLineTileType((TILETYPE)_TypeValue);
}

bool CTileScript::IsCircleTileTypeValue(int _TypeValue)
{
    if (!IsValidTileTypeValue(_TypeValue))
        return false;
    return IsCircleTileType((TILETYPE)_TypeValue);
}

bool CTileScript::IsFullBlockTileTypeValue(int _TypeValue)
{
    if (!IsValidTileTypeValue(_TypeValue))
        return false;
    return IsFullBlockTileType((TILETYPE)_TypeValue);
}

bool CTileScript::IsUnlockedTileTypeValue(int _TypeValue)
{
    if (!IsValidTileTypeValue(_TypeValue))
        return false;

    InitializeTileTypeUnlockState();
    return s_TileTypeUnlocked[_TypeValue];
}

void CTileScript::GetEditableTileTypeValues(vector<int>& _OutTypeValues, bool _IncludeEmpty, bool _IncludeLocked)
{
    InitializeTileTypeUnlockState();
    _OutTypeValues.clear();

    if (_IncludeEmpty)
    {
        _OutTypeValues.push_back((int)TILETYPE::EMPTY_BLOCK);
    }

    for (int typeValue = (int)TILETYPE::LINE_BLOCK_1; typeValue < (int)TILETYPE::END; ++typeValue)
    {
        if (_IncludeLocked || s_TileTypeUnlocked[typeValue])
        {
            _OutTypeValues.push_back(typeValue);
        }
    }
}

bool CTileScript::CreateCustomTileType(bool _Circle, TILETYPE _CopyFrom, TILETYPE& _OutNewType)
{
    InitializeTileTypeUnlockState();
    if (!s_bFormulaInitialized)
        ResetTileFormulaConfigToDefault();

    const int startType = _Circle ? (int)TILETYPE::CIRCLE_BLOCK_CUSTOM_BEGIN : (int)TILETYPE::LINE_BLOCK_CUSTOM_BEGIN;
    const int endType = _Circle ? (int)TILETYPE::CIRCLE_BLOCK_CUSTOM_END : (int)TILETYPE::LINE_BLOCK_CUSTOM_END;

    int newTypeValue = -1;
    for (int i = startType; i <= endType; ++i)
    {
        if (!s_TileTypeUnlocked[i])
        {
            newTypeValue = i;
            break;
        }
    }

    if (newTypeValue == -1)
        return false;

    TILE_FORMULA_CONFIG srcCfg = {};
    TILETYPE fallbackType = _Circle ? TILETYPE::CIRCLE_BLOCK_1 : TILETYPE::LINE_BLOCK_3;

    if ((_Circle && IsCircleTileType(_CopyFrom)) || (!_Circle && IsLineTileType(_CopyFrom)))
    {
        GetTileFormulaConfig(_CopyFrom, srcCfg);
    }
    else
    {
        GetTileFormulaConfig(fallbackType, srcCfg);
    }

    s_TileTypeUnlocked[newTypeValue] = true;
    s_FormulaConfig[newTypeValue] = srcCfg;
    _OutNewType = (TILETYPE)newTypeValue;
    return true;
}

bool CTileScript::DeleteCustomTileType(TILETYPE _Type)
{
    return DeleteCustomTileTypeByValue((int)_Type);
}

bool CTileScript::DeleteCustomTileTypeByValue(int _TypeValue)
{
    if (!IsValidTileTypeValue(_TypeValue))
        return false;

    const bool isCustomLine = ((int)TILETYPE::LINE_BLOCK_CUSTOM_BEGIN <= _TypeValue && _TypeValue <= (int)TILETYPE::LINE_BLOCK_CUSTOM_END);
    const bool isCustomCircle = ((int)TILETYPE::CIRCLE_BLOCK_CUSTOM_BEGIN <= _TypeValue && _TypeValue <= (int)TILETYPE::CIRCLE_BLOCK_CUSTOM_END);

    if (!isCustomLine && !isCustomCircle)
        return false;

    InitializeTileTypeUnlockState();
    if (!s_bFormulaInitialized)
        ResetTileFormulaConfigToDefault();

    s_TileTypeUnlocked[_TypeValue] = false;

    if (IsLineTileTypeValue(_TypeValue))
    {
        s_FormulaConfig[_TypeValue] = s_FormulaConfig[(int)TILETYPE::LINE_BLOCK_3];
    }
    else if (IsCircleTileTypeValue(_TypeValue))
    {
        const TILE_FORMULA_CONFIG lineFallback = s_FormulaConfig[(int)TILETYPE::LINE_BLOCK_3];
        s_FormulaConfig[_TypeValue] = TILE_FORMULA_CONFIG{ lineFallback.a, lineFallback.b, lineFallback.c, 0.8f, 0.5f, 0.5f };
    }

    return true;
}

bool CTileScript::IsSpecialTileType(TILETYPE _Type)
{
    return _Type == TILETYPE::CIRCLE_BLOCK_3
        || _Type == TILETYPE::CIRCLE_BLOCK_4;
}

Vec4 CTileScript::GetDebugColorByType(TILETYPE _Type)
{
    if (_Type == TILETYPE::EMPTY_BLOCK)
    {
        return Vec4(1.f, 1.f, 1.f, 1.f);
    }

    if (_Type == TILETYPE::FULL_BLOCK)
    {
        return Vec4(0.2f, 0.6f, 1.f, 1.f);
    }

    if (IsSpecialTileType(_Type))
    {
        return Vec4(1.f, 0.3f, 0.3f, 1.f);
    }

    return Vec4(0.f, 0.f, 0.f, 1.f);
}

Vec4 CTileScript::GetDebugColorByTypeValue(int _TypeValue)
{
    return GetDebugColorByType(ToTileType(_TypeValue));
}

const char* CTileScript::GetTileTypeName(TILETYPE _Type)
{
    return GetTileTypeNameByValue((int)_Type);
}

const char* CTileScript::GetTileTypeNameByValue(int _TypeValue)
{
    if (!IsValidTileTypeValue(_TypeValue))
        return "UNKNOWN";

    if (_TypeValue == (int)TILETYPE::EMPTY_BLOCK)
        return "EMPTY";

    if (_TypeValue == (int)TILETYPE::FULL_BLOCK)
        return "FULL_BLOCK";

    static char s_Buf[64] = {};

    if (IsLineTileTypeValue(_TypeValue))
    {
        int lineNo = 0;
        if (_TypeValue <= (int)TILETYPE::LINE_BLOCK_6)
            lineNo = _TypeValue - (int)TILETYPE::LINE_BLOCK_1 + 1;
        else
            lineNo = _TypeValue - (int)TILETYPE::LINE_BLOCK_CUSTOM_BEGIN + 7;

        if (lineNo <= 6)
            sprintf_s(s_Buf, "LINE_%d", lineNo);
        else
            sprintf_s(s_Buf, "LINE_CUSTOM_%d", lineNo);
        return s_Buf;
    }

    if (IsCircleTileTypeValue(_TypeValue))
    {
        int circleNo = 0;
        if (_TypeValue <= (int)TILETYPE::CIRCLE_BLOCK_4)
            circleNo = _TypeValue - (int)TILETYPE::CIRCLE_BLOCK_1 + 1;
        else
            circleNo = _TypeValue - (int)TILETYPE::CIRCLE_BLOCK_CUSTOM_BEGIN + 5;

        if (circleNo <= 4)
            sprintf_s(s_Buf, "CIRCLE_%d", circleNo);
        else
            sprintf_s(s_Buf, "CIRCLE_CUSTOM_%d", circleNo);
        return s_Buf;
    }

    return "UNKNOWN";
}

void CTileScript::ResetTileFormulaConfigToDefault()
{
    InitializeTileTypeUnlockState();

    for (int i = 0; i < (int)TILETYPE::END; ++i)
    {
        s_FormulaConfig[i] = TILE_FORMULA_CONFIG{ 0.f, 0.f, -1.f, 0.8f, 0.5f, 0.5f };
    }

    s_FormulaConfig[(int)TILETYPE::LINE_BLOCK_1] = TILE_FORMULA_CONFIG{ 0.1f, 0.4f, -1.f, 0.8f, 0.5f, 0.5f };
    s_FormulaConfig[(int)TILETYPE::LINE_BLOCK_2] = TILE_FORMULA_CONFIG{ 0.4f, 0.8f, -1.f, 0.8f, 0.5f, 0.5f };
    s_FormulaConfig[(int)TILETYPE::LINE_BLOCK_3] = TILE_FORMULA_CONFIG{ 0.8f, 0.8f, -1.f, 0.8f, 0.5f, 0.5f };
    s_FormulaConfig[(int)TILETYPE::LINE_BLOCK_4] = TILE_FORMULA_CONFIG{ 0.8f, 0.4f, -1.f, 0.8f, 0.5f, 0.5f };
    s_FormulaConfig[(int)TILETYPE::LINE_BLOCK_5] = TILE_FORMULA_CONFIG{ 0.4f, 0.1f, -1.f, 0.8f, 0.5f, 0.5f };
    s_FormulaConfig[(int)TILETYPE::LINE_BLOCK_6] = TILE_FORMULA_CONFIG{ 0.1f, 0.1f, -1.f, 0.8f, 0.5f, 0.5f };

    const TILE_FORMULA_CONFIG lineFallback = s_FormulaConfig[(int)TILETYPE::LINE_BLOCK_3];
    s_FormulaConfig[(int)TILETYPE::CIRCLE_BLOCK_1] = TILE_FORMULA_CONFIG{ lineFallback.a, lineFallback.b, lineFallback.c, 0.8f, 1.f, 1.f };
    s_FormulaConfig[(int)TILETYPE::CIRCLE_BLOCK_2] = TILE_FORMULA_CONFIG{ lineFallback.a, lineFallback.b, lineFallback.c, 0.8f, 0.f, 1.f };
    s_FormulaConfig[(int)TILETYPE::CIRCLE_BLOCK_3] = TILE_FORMULA_CONFIG{ lineFallback.a, lineFallback.b, lineFallback.c, 0.8f, 1.f, 0.f };
    s_FormulaConfig[(int)TILETYPE::CIRCLE_BLOCK_4] = TILE_FORMULA_CONFIG{ lineFallback.a, lineFallback.b, lineFallback.c, 0.8f, 0.f, 0.f };

    for (int i = (int)TILETYPE::LINE_BLOCK_CUSTOM_BEGIN; i <= (int)TILETYPE::LINE_BLOCK_CUSTOM_END; ++i)
    {
        s_FormulaConfig[i] = s_FormulaConfig[(int)TILETYPE::LINE_BLOCK_3];
    }
    for (int i = (int)TILETYPE::CIRCLE_BLOCK_CUSTOM_BEGIN; i <= (int)TILETYPE::CIRCLE_BLOCK_CUSTOM_END; ++i)
    {
        s_FormulaConfig[i] = TILE_FORMULA_CONFIG{ 0.f, 0.f, -1.f, 0.8f, 0.5f, 0.5f };
    }

    s_bFormulaInitialized = true;
}

bool CTileScript::SetTileFormulaConfig(TILETYPE _Type, const TILE_FORMULA_CONFIG& _Config)
{
    return SetTileFormulaConfigByValue((int)_Type, _Config);
}

bool CTileScript::GetTileFormulaConfig(TILETYPE _Type, TILE_FORMULA_CONFIG& _OutConfig)
{
    return GetTileFormulaConfigByValue((int)_Type, _OutConfig);
}

bool CTileScript::SetTileFormulaConfigByValue(int _TypeValue, const TILE_FORMULA_CONFIG& _Config)
{
    if (!IsValidTileTypeValue(_TypeValue))
        return false;

    if (_TypeValue == (int)TILETYPE::EMPTY_BLOCK)
        return false;

    if (!s_bFormulaInitialized)
        ResetTileFormulaConfigToDefault();

    s_FormulaConfig[_TypeValue] = _Config;
    return true;
}

bool CTileScript::GetTileFormulaConfigByValue(int _TypeValue, TILE_FORMULA_CONFIG& _OutConfig)
{
    if (!IsValidTileTypeValue(_TypeValue))
        return false;

    if (!s_bFormulaInitialized)
        ResetTileFormulaConfigToDefault();

    _OutConfig = s_FormulaConfig[_TypeValue];
    return true;
}

void CTileScript::SetTileMapPlacement(const TILE_MAP_PLACEMENT& _Placement)
{
    s_MapPlacement = _Placement;
}

void CTileScript::GetTileMapPlacement(TILE_MAP_PLACEMENT& _OutPlacement)
{
    _OutPlacement = s_MapPlacement;
}

void CTileScript::GetDefaultTileMap(UINT& _OutRow, UINT& _OutCol, vector<int>& _OutTileValues)
{
    _OutRow = 6;
    _OutCol = 25;

    static const int kDefaultMap[6][25] =
    {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 8, 9, 1, 1, 1, 8, 9},
        {7, 7, 7, 7, 2, 3, 4, 4, 4, 4, 5, 6, 7, 7, 2, 3, 4, 4, 10, 11, 4, 4, 4, 10, 11},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    };

    _OutTileValues.assign(_OutRow * _OutCol, (int)TILETYPE::EMPTY_BLOCK);

    for (UINT row = 0; row < _OutRow; ++row)
    {
        for (UINT col = 0; col < _OutCol; ++col)
        {
            _OutTileValues[row * _OutCol + col] = kDefaultMap[row][col];
        }
    }
}

bool CTileScript::SaveTileScriptPreset(const wstring& _FilePath, UINT _Row, UINT _Col, const vector<int>& _TileValues)
{
    if (0 == _Row || 0 == _Col)
        return false;

    if (_TileValues.size() != (size_t)_Row * (size_t)_Col)
        return false;

    if (!s_bFormulaInitialized)
        ResetTileFormulaConfigToDefault();
    InitializeTileTypeUnlockState();

    std::filesystem::path path(_FilePath);
    if (!path.parent_path().empty())
    {
        std::filesystem::create_directories(path.parent_path());
    }

    FILE* pFile = nullptr;
    _wfopen_s(&pFile, _FilePath.c_str(), L"wb");
    if (nullptr == pFile)
        return false;

    const UINT kMagic = 0x31505354; // 'TSP1'
    fwrite(&kMagic, sizeof(UINT), 1, pFile);
    fwrite(&_Row, sizeof(UINT), 1, pFile);
    fwrite(&_Col, sizeof(UINT), 1, pFile);

    UINT mapCount = (UINT)_TileValues.size();
    fwrite(&mapCount, sizeof(UINT), 1, pFile);
    if (0 < mapCount)
    {
        fwrite(_TileValues.data(), sizeof(int), mapCount, pFile);
    }

    UINT configCount = (UINT)((int)TILETYPE::END - (int)TILETYPE::EMPTY_BLOCK);
    fwrite(&configCount, sizeof(UINT), 1, pFile);

    for (int typeValue = (int)TILETYPE::EMPTY_BLOCK; typeValue < (int)TILETYPE::END; ++typeValue)
    {
        const TILE_FORMULA_CONFIG& cfg = s_FormulaConfig[typeValue];
        fwrite(&typeValue, sizeof(int), 1, pFile);
        fwrite(&cfg, sizeof(TILE_FORMULA_CONFIG), 1, pFile);
    }

    UINT unlockedCount = 0;
    for (int typeValue = (int)TILETYPE::LINE_BLOCK_CUSTOM_BEGIN; typeValue <= (int)TILETYPE::LINE_BLOCK_CUSTOM_END; ++typeValue)
    {
        if (s_TileTypeUnlocked[typeValue])
            ++unlockedCount;
    }

    for (int typeValue = (int)TILETYPE::CIRCLE_BLOCK_CUSTOM_BEGIN; typeValue <= (int)TILETYPE::CIRCLE_BLOCK_CUSTOM_END; ++typeValue)
    {
        if (s_TileTypeUnlocked[typeValue])
            ++unlockedCount;
    }

    fwrite(&unlockedCount, sizeof(UINT), 1, pFile);
    for (int typeValue = (int)TILETYPE::LINE_BLOCK_CUSTOM_BEGIN; typeValue <= (int)TILETYPE::LINE_BLOCK_CUSTOM_END; ++typeValue)
    {
        if (s_TileTypeUnlocked[typeValue])
            fwrite(&typeValue, sizeof(int), 1, pFile);
    }

    for (int typeValue = (int)TILETYPE::CIRCLE_BLOCK_CUSTOM_BEGIN; typeValue <= (int)TILETYPE::CIRCLE_BLOCK_CUSTOM_END; ++typeValue)
    {
        if (s_TileTypeUnlocked[typeValue])
            fwrite(&typeValue, sizeof(int), 1, pFile);
    }

    const UINT kMetaMagic = 0x50414D54; // 'TMAP'
    fwrite(&kMetaMagic, sizeof(UINT), 1, pFile);
    fwrite(&s_MapPlacement, sizeof(TILE_MAP_PLACEMENT), 1, pFile);

    fclose(pFile);

    return true;
}

bool CTileScript::LoadTileScriptPreset(const wstring& _FilePath, UINT& _OutRow, UINT& _OutCol, vector<int>& _OutTileValues, bool _ApplyFormula)
{
    FILE* pFile = nullptr;
    _wfopen_s(&pFile, _FilePath.c_str(), L"rb");
    if (nullptr == pFile)
        return false;

    UINT magic = 0;
    fread(&magic, sizeof(UINT), 1, pFile);
    if (magic != 0x31505354)
    {
        fclose(pFile);
        return false;
    }

    fread(&_OutRow, sizeof(UINT), 1, pFile);
    fread(&_OutCol, sizeof(UINT), 1, pFile);

    if (0 == _OutRow || 0 == _OutCol || _OutRow > 1024 || _OutCol > 1024)
    {
        fclose(pFile);
        return false;
    }

    UINT mapCount = 0;
    fread(&mapCount, sizeof(UINT), 1, pFile);
    if (mapCount != (UINT)(_OutRow * _OutCol))
    {
        fclose(pFile);
        return false;
    }

    _OutTileValues.assign(mapCount, (int)TILETYPE::EMPTY_BLOCK);
    if (0 < mapCount)
    {
        fread(_OutTileValues.data(), sizeof(int), mapCount, pFile);
    }

    for (UINT i = 0; i < mapCount; ++i)
    {
        if (!IsValidTileTypeValue(_OutTileValues[i]))
            _OutTileValues[i] = (int)TILETYPE::EMPTY_BLOCK;
    }

    InitializeTileTypeUnlockState();
    for (UINT i = 0; i < mapCount; ++i)
    {
        if (((int)TILETYPE::LINE_BLOCK_CUSTOM_BEGIN <= _OutTileValues[i] && _OutTileValues[i] <= (int)TILETYPE::LINE_BLOCK_CUSTOM_END)
            || ((int)TILETYPE::CIRCLE_BLOCK_CUSTOM_BEGIN <= _OutTileValues[i] && _OutTileValues[i] <= (int)TILETYPE::CIRCLE_BLOCK_CUSTOM_END))
            s_TileTypeUnlocked[_OutTileValues[i]] = true;
    }

    UINT configCount = 0;
    fread(&configCount, sizeof(UINT), 1, pFile);

    if (_ApplyFormula && !s_bFormulaInitialized)
        ResetTileFormulaConfigToDefault();

    for (UINT i = 0; i < configCount; ++i)
    {
        int typeValue = 0;
        TILE_FORMULA_CONFIG cfg = {};
        fread(&typeValue, sizeof(int), 1, pFile);
        fread(&cfg, sizeof(TILE_FORMULA_CONFIG), 1, pFile);

        if (_ApplyFormula && IsValidTileTypeValue(typeValue))
        {
            s_FormulaConfig[typeValue] = cfg;
        }
    }

    UINT unlockedCount = 0;
    if (1 == fread(&unlockedCount, sizeof(UINT), 1, pFile))
    {
        for (UINT i = 0; i < unlockedCount; ++i)
        {
            int unlockedType = 0;
            if (1 != fread(&unlockedType, sizeof(int), 1, pFile))
                break;

            const bool validCustomType =
                ((int)TILETYPE::LINE_BLOCK_CUSTOM_BEGIN <= unlockedType && unlockedType <= (int)TILETYPE::LINE_BLOCK_CUSTOM_END)
                || ((int)TILETYPE::CIRCLE_BLOCK_CUSTOM_BEGIN <= unlockedType && unlockedType <= (int)TILETYPE::CIRCLE_BLOCK_CUSTOM_END);

            if (IsValidTileTypeValue(unlockedType) && validCustomType)
            {
                s_TileTypeUnlocked[unlockedType] = true;
            }
        }
    }

    UINT metaMagic = 0;
    if (1 == fread(&metaMagic, sizeof(UINT), 1, pFile))
    {
        if (metaMagic == 0x50414D54)
        {
            TILE_MAP_PLACEMENT placement = s_MapPlacement;
            if (1 == fread(&placement, sizeof(TILE_MAP_PLACEMENT), 1, pFile))
            {
                s_MapPlacement = placement;
            }
        }
    }

    fclose(pFile);

    if (_ApplyFormula)
        s_bFormulaInitialized = true;

    return true;
}

CTileScript::CTileScript()
    : CScript(SCRIPT_TYPE::TILESCRIPT)
{
}

CTileScript::~CTileScript()
{
}

void CTileScript::Begin()
{
    if (Collider2D())
    {
        Collider2D()->AddDynamicBeginOverlap(this, (COLLISION_EVENT)&CTileScript::BeginOverlap);
        Collider2D()->AddDynamicEndOverlap(this, (COLLISION_EVENT)&CTileScript::EndOverlap);
        Collider2D()->AddDynamicOverlap(this, (COLLISION_EVENT)&CTileScript::Overlap);
    }
}

void CTileScript::Tick()
{
    DbgInfo info = {};
    info.Color = GetDebugColorByType(m_eType);
    info.DepthTest = false;
    info.Life = 0.f;

    Vec3 vWorldPos = Transform()->GetWorldPos();
    Vec3 vWorldScale = Transform()->GetRelativeScale();

    bool bDrawCircle = false;
    if (m_eType == TILETYPE::CIRCLE_BLOCK_3)
    {
        bDrawCircle = s_bHalfChecker;
    }
    else if (m_eType == TILETYPE::CIRCLE_BLOCK_4)
    {
        bDrawCircle = !s_bHalfChecker;
    }
    else if (IsCircleTileType(m_eType))
    {
        bDrawCircle = true;
    }

    if (bDrawCircle)
    {
        info.Shape = DBG_SHAPE::CIRCLE;
        info.Pos = Vec3(
            vWorldPos.x + (center_x - 0.5f) * vWorldScale.x,
            vWorldPos.y - (center_y - 0.5f) * vWorldScale.y,
            vWorldPos.z
        );
        info.Scale = Vec3(r * vWorldScale.x * 2.f, r * vWorldScale.y * 2.f, 1.f);
    }
    else
    {
        info.Shape = DBG_SHAPE::RECT;
        info.Pos = vWorldPos;
        info.Scale = vWorldScale;
    }

    RenderMgr::GetInst()->AddDebugInfo(info);
}

void CTileScript::BeginOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    Overlap(_OwnCollider, _OtherCollider);

    auto pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer == nullptr)
        return;

    if (m_eType == TILETYPE::CIRCLE_BLOCK_3)
    {
        Vec3 vPlayerPos = _OtherCollider->GetOwner()->Transform()->GetWorldPos();
        Vec3 vTilePos = Transform()->GetWorldPos();
        Vec3 vTileScale = Transform()->GetRelativeScale();
  
        float LeftX = vTilePos.x - vTileScale.x * 0.5f;
        if (vPlayerPos.x < LeftX)
        {
            if (s_bHalfChecker == true)
            {
                s_bHalfChecker = false;
                RefreshHalfCheckerTiles();
            }
        }
    }

    if (m_eType == TILETYPE::CIRCLE_BLOCK_4)
    {
        Vec3 vPlayerPos = _OtherCollider->GetOwner()->Transform()->GetWorldPos();
        Vec3 vTilePos = Transform()->GetWorldPos();
        Vec3 vTileScale = Transform()->GetRelativeScale();
     
        float RightX = vTilePos.x + vTileScale.x * 0.5f;

        if (vPlayerPos.x > RightX)
        {
            if (s_bHalfChecker == false)
            {
                s_bHalfChecker = true;
                RefreshHalfCheckerTiles();
            }
        }
    }

    if (m_eType == TILETYPE::CIRCLE_BLOCK_1)
    {
        if (s_bHalfChecker == false)
        {
            s_bHalfChecker = true;
            RefreshHalfCheckerTiles();
        }
    }
    else if (m_eType == TILETYPE::CIRCLE_BLOCK_2)
    {
        if (s_bHalfChecker == true)
        {
            s_bHalfChecker = false;
            RefreshHalfCheckerTiles();
        }
    }
        
}

void CTileScript::EndOverlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    auto pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer == nullptr)
        return;

    if (m_eType == TILETYPE::CIRCLE_BLOCK_2 && pPlayer->GetIsGround() == true)
    {
        Vec3 vPlayerPos = _OtherCollider->GetOwner()->Transform()->GetWorldPos();
        Vec3 vTilePos = Transform()->GetWorldPos();
        Vec3 vTileScale = Transform()->GetRelativeScale();

        // CIRCLE_BLOCK_2의 좌측 경계(= 원 중앙 세로 분할선)를 기준으로
        // 왼쪽으로 빠져나간 경우에는 half-check 상태를 유지한다.
        float splitX = vTilePos.x - vTileScale.x * 0.5f;
        bool bExitToRight = (vPlayerPos.x >= splitX);

        bool bNextHalfChecker = bExitToRight ? false : true;
        if (s_bHalfChecker != bNextHalfChecker)
        {
            s_bHalfChecker = bNextHalfChecker;
            RefreshHalfCheckerTiles();
        }
    }
}

void CTileScript::RefreshHalfCheckerTiles()
{
    Ptr<ALevel> pCurLevel = LevelMgr::GetInst()->GetCurLevel();
    if (pCurLevel == nullptr)
        return;

    for (int layerIdx = 0; layerIdx < MAX_LAYER; ++layerIdx)
    {
        const vector<Ptr<GameObject>>& vecObjects = pCurLevel->GetLayer(layerIdx)->GetAllObjects();
        for (const Ptr<GameObject>& pObject : vecObjects)
        {
            if (pObject == nullptr)
                continue;

            auto pTileScript = pObject->GetScript<CTileScript>();
            if (pTileScript == nullptr)
                continue;

            if (pTileScript->m_eType != TILETYPE::CIRCLE_BLOCK_3 &&
                pTileScript->m_eType != TILETYPE::CIRCLE_BLOCK_4)
            {
                continue;
            }

            pTileScript->TileMapSetting(pTileScript->m_eType);
        }
    }
}

void CTileScript::Overlap(CCollider2D* _OwnCollider, CCollider2D* _OtherCollider)
{
    if (_OtherCollider->GetOwner()->GetName() != L"Player")
        return;

    auto pPlayer = _OtherCollider->GetOwner()->GetScript<CPlayerScript>();
    if (pPlayer == nullptr)
        return;

    const bool wasGround = pPlayer->GetIsGround();
    GetTileState();

    Vec3 vPlayerPos = _OtherCollider->GetOwner()->Transform()->GetWorldPos();
    Vec3 vPlayerScale = _OtherCollider->GetOwner()->Transform()->GetRelativeScale();

    Vec3 vTilePos = Transform()->GetWorldPos();
    Vec3 vTileScale = Transform()->GetRelativeScale();

    Vec2 vFootPos = Vec2(vPlayerPos.x, vPlayerPos.y);

    if (wasGround)
    {
        float fRotZ = _OtherCollider->GetOwner()->Transform()->GetRelativeRot().z;
        Vec2 vLocalDown = Vec2(sinf(fRotZ), -cosf(fRotZ));
        vFootPos += vLocalDown * (vPlayerScale.y * 0.5f);
    }
    else
    {
        // 낙하 중에는 회전된 사각형 모서리 대신 원형 하단점을 사용해 착지 판정 안정화
        float halfW = fabsf(vPlayerScale.x) * 0.5f;
        float halfH = fabsf(vPlayerScale.y) * 0.5f;
        float radius = (halfW < halfH) ? halfW : halfH;
        vFootPos.y -= radius;
    }

    float left = vTilePos.x - vTileScale.x * 0.5f;
    float top = vTilePos.y + vTileScale.y * 0.5f;

    Vec2 localPos = {};
    localPos.x = (vFootPos.x - left) / vTileScale.x;
    localPos.y = (top - vFootPos.y) / vTileScale.y;

    const float edgeEpsilon = 0.02f;
    if (localPos.x < -edgeEpsilon || localPos.x > 1.f + edgeEpsilon ||
        localPos.y < -edgeEpsilon || localPos.y > 1.f + edgeEpsilon)
    {
        return;
    }

    if (localPos.x < 0.f) localPos.x = 0.f;
    else if (localPos.x > 1.f) localPos.x = 1.f;
    if (localPos.y < 0.f) localPos.y = 0.f;
    else if (localPos.y > 1.f) localPos.y = 1.f;

    float fValue = GetFvalue(localPos);
    const float detachThreshold = wasGround ? 0.02f : 0.005f;
    const float transitionSnapThreshold = wasGround ? 0.08f : detachThreshold;
    if (fValue > transitionSnapThreshold)
    {
        // 경계면에서 타일 간 콜백 순서로 지면 판정이 깜빡이는 현상 방지
        return;
    }
    const bool bTransitionSurface = (fValue > detachThreshold);

    Vec2 normalLocal = {};
    float mag = 0.f;
    GetNormal(localPos, normalLocal, mag);

    if (mag <= 0.001f)
    {
        return;
    }

    Vec2 worldNormal = Vec2(
        normalLocal.x * vTileScale.x,
        -normalLocal.y * vTileScale.y
    );

    float worldNormalLen = sqrtf(worldNormal.x * worldNormal.x + worldNormal.y * worldNormal.y);
    if (worldNormalLen <= 0.001f)
        return;

    worldNormal.x /= worldNormalLen;
    worldNormal.y /= worldNormalLen;

    // 1. 보간된 법선 결정 (경계면 지터 완화)
    Vec2 vOldNormal = pPlayer->GetNormal();
    if (fabsf(vOldNormal.x) < 0.0001f && fabsf(vOldNormal.y) < 0.0001f)
        vOldNormal = worldNormal;
    else
        vOldNormal.Normalize();

    float normalDot = vOldNormal.x * worldNormal.x + vOldNormal.y * worldNormal.y;
    if (normalDot > 1.f) normalDot = 1.f;
    if (normalDot < -1.f) normalDot = -1.f;

    float normalBlend = wasGround ? 0.2f : 1.f;
    if (m_TileState == TILESTATE::CIRCLE_BLOCK && wasGround)
        normalBlend = 0.3f;
    if (wasGround)
    {
        if (normalDot < 0.75f)
            normalBlend = 1.f;
        else if (normalDot < 0.9f && normalBlend < 0.6f)
            normalBlend = 0.6f;
    }
    if (bTransitionSurface)
        normalBlend = 1.f;

    vCurNormal = Lerp(vOldNormal, worldNormal, normalBlend);
    if (fabsf(vCurNormal.x) < 0.0001f && fabsf(vCurNormal.y) < 0.0001f)
        vCurNormal = worldNormal;
    else
        vCurNormal.Normalize();

    pPlayer->SetNormal(vCurNormal);

    // 2. [중요] Push 값의 월드 단위 변환
    // localNormal의 magnitude(mag)는 로컬 기준이므로, 
    // 실제 밀어낼 거리(World Distance)는 타일 스케일을 반영해야 합니다.
    if (fValue < 0.f)
    {
        float push = (-fValue / mag);

        // 타일의 평균 스케일을 곱해 로컬 push를 월드 push로 변환합니다.
        // (정밀도를 위해 해당 방향의 스케일 성분을 고려하는 것이 좋음)
        float worldPush = push * ((vTileScale.x + vTileScale.y) * 0.5f) + 0.001f;
        const float maxPush = 12.f;
        if (worldPush > maxPush)
            worldPush = maxPush;

        Vec3 vPos = _OtherCollider->GetOwner()->Transform()->GetRelativePos();

        // 3. 보간된 방향 * 월드 거리로 보정
        vPos.x += vCurNormal.x * worldPush;
        vPos.y += vCurNormal.y * worldPush;

        _OtherCollider->GetOwner()->Transform()->SetRelativePos(vPos);
    }

    Vec2 tangent = Vec2(vCurNormal.y, -vCurNormal.x);

    Vec2 v = pPlayer->GetVelocity();

    float rawVn = v.x * vCurNormal.x + v.y * vCurNormal.y;
    const float incomingVn = rawVn;
    float vt = v.x * tangent.x + v.y * tangent.y;

    const bool isCircleTile = (m_TileState == TILESTATE::CIRCLE_BLOCK);
    const float breakDropSpeedThreshold = 100.f;
    const bool blockedByWallLikeTile =
        pPlayer->IsBreakOrRollAction() &&
        (fabsf(vCurNormal.y) < 0.35f) &&
        (fabsf(incomingVn) > 20.f);

    const bool bBreakCircleDrop =
        isCircleTile &&
        pPlayer->GetAction() == ActionState::Break &&
        (fabsf(vCurNormal.y) < 0.35f) &&
        (pPlayer->GetBreakSpeed() < breakDropSpeedThreshold);
    if (bBreakCircleDrop)
    {
        Vec2 vDrop = pPlayer->GetVelocity();
        vDrop.x = 0.f;
        pPlayer->SetVelocity(vDrop);
        pPlayer->SetIsGround(false);
        pPlayer->StopBlockedAction();
        return;
    }

    if (blockedByWallLikeTile && pPlayer->GetAction() == ActionState::Break)
    {
        pPlayer->RequestBreakWallStop();
        pPlayer->SetVelocity(Vec2(0.f, 0.f));
        return;
    }

    if (rawVn < 0.f)
    {
        v.x -= vCurNormal.x * rawVn;
        v.y -= vCurNormal.y * rawVn;
        rawVn = 0.f;
    }

    // 위쪽/옆면 속도가 있을 때 + 히스테리시스
    const float stickMinSpeed = 500.f;
    const float normalEnterY = 0.65f;
    const float normalKeepY = 0.45f;
    const float vnEnterTolerance = 5.f;
    const float vnKeepTolerance = 80.f;
    const float normalThreshold = wasGround ? normalKeepY : normalEnterY;
    const float vnTolerance = wasGround ? vnKeepTolerance : vnEnterTolerance;

    bool bSpeedStick = (fabsf(vt) > stickMinSpeed) && (vCurNormal.y > 0.2f);
    bool bCanStick =
        (rawVn <= vnTolerance) &&
        (vCurNormal.y > normalThreshold || bSpeedStick);

    if (bTransitionSurface && vCurNormal.y < normalThreshold)
        bCanStick = false;

    if (blockedByWallLikeTile)
    {
        if (pPlayer->GetAction() == ActionState::Break)
            pPlayer->RequestBreakWallStop();
        else
            pPlayer->StopBlockedAction();

        if (pPlayer->GetAction() == ActionState::Break)
        {
            Vec2 vBlocked = pPlayer->GetVelocity();
            vBlocked.x = 0.f;
            pPlayer->SetVelocity(vBlocked);
            return;
        }
    }

    if (bCanStick)
    {
        pPlayer->SetIsGround(true);
        pPlayer->SetNormal(vCurNormal);
        pPlayer->SetGroundTangent(tangent);

        // 바닥 마찰
        if (vCurNormal.y > 0.f)
        {
            float dt = DT;
            float friction = 100.f;

            if (vt > 0.f)
            {
                vt -= friction * dt;
                if (vt < 0.f) vt = 0.f;
            }
            else if (vt < 0.f)
            {
                vt += friction * dt;
                if (vt > 0.f) vt = 0.f;
            }
        }

        // Ground에서는 접선 성분만 유지
        v = tangent * vt;
  
    }
    else
    {
        const bool definiteAirborne =
            (rawVn > vnKeepTolerance) ||
            (vCurNormal.y < 0.2f && fabsf(vt) < stickMinSpeed);

        if (definiteAirborne)
            pPlayer->SetIsGround(false);

        pPlayer->SetNormal(vCurNormal);
        pPlayer->SetGroundTangent(tangent);

        // 옆/윗부분에서 속도 부족으로 떨어질 때 위로 튀지 않게 보정
        if (vCurNormal.y <= 0.65f && fabsf(vt) < stickMinSpeed)
        {
            if (v.y > 0.f)
            {
                v.y = 0.f;
            } 
        }
    }

    pPlayer->SetVelocity(v);
}

void CTileScript::TileMapSetting(TILETYPE i)
{
    TileMapSetting((int)i);
}

void CTileScript::TileMapSetting(int _TypeValue)
{
    if (!IsValidTileTypeValue(_TypeValue))
        _TypeValue = (int)TILETYPE::EMPTY_BLOCK;

    if (!s_bFormulaInitialized)
        ResetTileFormulaConfigToDefault();

    m_eType = (TILETYPE)_TypeValue;
    m_FullBlockFaceMask = FULL_FACE_ALL;

    auto SetLineFormula = [this](const TILE_FORMULA_CONFIG& cfg)
        {
            a = cfg.a;
            b = cfg.b;
            c = cfg.c;

            f = [this](float x, float y) { return c * (y - (b - a) * x - a); };
            dfdx = [this](float x, float y) { return c * (a - b); };
            dfdy = [this](float x, float y) { return c; };
        };

    auto SetCircleFormula = [this](const TILE_FORMULA_CONFIG& cfg)
        {
            c = cfg.c;
            r = cfg.r;
            center_x = cfg.center_x;
            center_y = cfg.center_y;

            f = [this](float x, float y)
                {
                    const float dist2 = (x - center_x) * (x - center_x) + (y - center_y) * (y - center_y);
                    return c * (dist2 - r * r);
                };
            dfdx = [this](float x, float y) { return c * 2.f * (x - center_x); };
            dfdy = [this](float x, float y) { return c * 2.f * (y - center_y); };
        };

    auto SetFullFormula = [this]()
        {
            f = [](float x, float y)
                {
                    const float distLeft = x;
                    const float distRight = 1.f - x;
                    const float distTop = y;
                    const float distBottom = 1.f - y;
                    const float minDist = min(min(distLeft, distRight), min(distTop, distBottom));
                    return -minDist;
                };
            dfdx = [](float x, float y) { return 0.f; };
            dfdy = [](float x, float y) { return -1.f; };
        };

    if (m_eType == TILETYPE::EMPTY_BLOCK)
    {
        f = [](float x, float y) { return 1.0f; };
        dfdx = [](float x, float y) { return 1.f; };
        dfdy = [](float x, float y) { return 1.f; };
        return;
    }

    if (m_eType == TILETYPE::FULL_BLOCK)
    {
        SetFullFormula();
        return;
    }

    if (m_eType == TILETYPE::CIRCLE_BLOCK_3)
    {
        if (!s_bHalfChecker)
            SetLineFormula(s_FormulaConfig[_TypeValue]);
        else
            SetCircleFormula(s_FormulaConfig[_TypeValue]);
        return;
    }

    if (m_eType == TILETYPE::CIRCLE_BLOCK_4)
    {
        if (s_bHalfChecker)
            SetLineFormula(s_FormulaConfig[_TypeValue]);
        else
            SetCircleFormula(s_FormulaConfig[_TypeValue]);
        return;
    }

    if (IsLineTileType(m_eType))
    {
        SetLineFormula(s_FormulaConfig[_TypeValue]);
        return;
    }

    if (IsCircleTileType(m_eType))
    {
        SetCircleFormula(s_FormulaConfig[_TypeValue]);
        return;
    }

    f = [](float x, float y) { return -1.0f; };
    dfdx = [](float x, float y) { return 0.f; };
    dfdy = [](float x, float y) { return 0.f; };
}

float CTileScript::GetFvalue(Vec2 _pos)
{
    if (m_eType == TILETYPE::FULL_BLOCK)
    {
        const unsigned char faceMask =
            (m_FullBlockFaceMask == FULL_FACE_NONE) ? FULL_FACE_ALL : m_FullBlockFaceMask;
        const float minDist = GetMaskedFullBlockMinDistance(_pos, faceMask);
        if (minDist == FLT_MAX)
            return 1.f;

        return -minDist;
    }

    if (!f)
        return -1.f;

    return f(_pos.x, _pos.y);
}

void CTileScript::GetNormal(Vec2 _pos, Vec2& _normal, float& _mag)
{
    if (m_eType == TILETYPE::FULL_BLOCK)
    {
        const unsigned char faceMask =
            (m_FullBlockFaceMask == FULL_FACE_NONE) ? FULL_FACE_ALL : m_FullBlockFaceMask;
        const unsigned char closestFace = GetMaskedFullBlockClosestFace(_pos, faceMask);
        if (closestFace == FULL_FACE_NONE)
        {
            _normal = Vec2(0.f, 0.f);
            _mag = 0.f;
            return;
        }

        _mag = 1.f;

        if (closestFace == FULL_FACE_TOP)
            _normal = Vec2(0.f, -1.f);
        else if (closestFace == FULL_FACE_BOTTOM)
            _normal = Vec2(0.f, 1.f);
        else if (closestFace == FULL_FACE_LEFT)
            _normal = Vec2(-1.f, 0.f);
        else
            _normal = Vec2(1.f, 0.f);

        return;
    }

    if (!dfdx || !dfdy)
    {
        _normal = Vec2(0.f, 0.f);
        _mag = 0.f;
        return;
    }

    float dx = dfdx(_pos.x, _pos.y);
    float dy = dfdy(_pos.x, _pos.y);

    _mag = sqrtf(dx * dx + dy * dy);

    if (_mag > 0.f)
    {
        _normal.x = dx / _mag;
        _normal.y = dy / _mag;
    }
    else
    {
        _normal = Vec2(0.f, 0.f);
    }
}
