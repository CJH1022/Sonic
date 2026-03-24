#include "pch.h"
#include "TileScriptEditor.h"
#include "PathMgr.h"
#include "func.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace
{
	float ClampFloat(float _Value, float _Min, float _Max)
	{
		if (_Value < _Min) return _Min;
		if (_Value > _Max) return _Max;
		return _Value;
	}

	bool IsLineType(TILETYPE _Type)
	{
		return CTileScript::IsLineTileType(_Type);
	}

	bool IsCircleType(TILETYPE _Type)
	{
		return CTileScript::IsCircleTileType(_Type);
	}

	bool IsCustomLineTypeValue(int _TypeValue)
	{
		return (int)TILETYPE::LINE_BLOCK_7 <= _TypeValue && _TypeValue <= (int)TILETYPE::LINE_BLOCK_20;
	}

	bool IsCustomCircleTypeValue(int _TypeValue)
	{
		return (int)TILETYPE::CIRCLE_BLOCK_5 <= _TypeValue && _TypeValue <= (int)TILETYPE::CIRCLE_BLOCK_20;
	}

	int ComputeOutCode(const ImVec2& _P, const ImVec2& _Min, const ImVec2& _Max)
	{
		const int LEFT = 1;
		const int RIGHT = 2;
		const int TOP = 4;
		const int BOTTOM = 8;

		int code = 0;
		if (_P.x < _Min.x) code |= LEFT;
		else if (_P.x > _Max.x) code |= RIGHT;

		if (_P.y < _Min.y) code |= TOP;
		else if (_P.y > _Max.y) code |= BOTTOM;

		return code;
	}

	bool ClipSegmentToRect(ImVec2& _P0, ImVec2& _P1, const ImVec2& _Min, const ImVec2& _Max)
	{
		const int LEFT = 1;
		const int RIGHT = 2;
		const int TOP = 4;
		const int BOTTOM = 8;

		int outCode0 = ComputeOutCode(_P0, _Min, _Max);
		int outCode1 = ComputeOutCode(_P1, _Min, _Max);

		while (true)
		{
			if (0 == (outCode0 | outCode1))
			{
				return true;
			}

			if (0 != (outCode0 & outCode1))
			{
				return false;
			}

			const int outCodeOut = (outCode0 != 0) ? outCode0 : outCode1;
			float x = 0.f;
			float y = 0.f;

			if (outCodeOut & TOP)
			{
				const float dy = _P1.y - _P0.y;
				if (fabsf(dy) < 1e-6f) return false;
				x = _P0.x + (_P1.x - _P0.x) * ((_Min.y - _P0.y) / dy);
				y = _Min.y;
			}
			else if (outCodeOut & BOTTOM)
			{
				const float dy = _P1.y - _P0.y;
				if (fabsf(dy) < 1e-6f) return false;
				x = _P0.x + (_P1.x - _P0.x) * ((_Max.y - _P0.y) / dy);
				y = _Max.y;
			}
			else if (outCodeOut & RIGHT)
			{
				const float dx = _P1.x - _P0.x;
				if (fabsf(dx) < 1e-6f) return false;
				y = _P0.y + (_P1.y - _P0.y) * ((_Max.x - _P0.x) / dx);
				x = _Max.x;
			}
			else // LEFT
			{
				const float dx = _P1.x - _P0.x;
				if (fabsf(dx) < 1e-6f) return false;
				y = _P0.y + (_P1.y - _P0.y) * ((_Min.x - _P0.x) / dx);
				x = _Min.x;
			}

			if (outCodeOut == outCode0)
			{
				_P0.x = x;
				_P0.y = y;
				outCode0 = ComputeOutCode(_P0, _Min, _Max);
			}
			else
			{
				_P1.x = x;
				_P1.y = y;
				outCode1 = ComputeOutCode(_P1, _Min, _Max);
			}
		}
	}
}

TileScriptEditor::TileScriptEditor()
	: EditorUI("TILE_SCRIPT_EDITOR")
	, m_Row(6)
	, m_Col(25)
	, m_EditRow(6)
	, m_EditCol(25)
	, m_BrushTypeValue((int)TILETYPE::LINE_BLOCK_3)
	, m_EditTypeValue((int)TILETYPE::LINE_BLOCK_3)
	, m_CellSize(26.f)
	, m_ShowTypeNumber(true)
	, m_ShowShapeOverlay(true)
	, m_PreviewMode(PREVIEW_MAP_EDIT)
	, m_DragLineHandle(-1)
	, m_StatusText("Ready")
{
	strcpy_s(m_szPresetRelativePath, "TileMap\\TileScriptPreset.txt");
	CTileScript::GetTileMapPlacement(m_MapPlacement);

	// 에디터 시작 시 저장된 프리셋을 우선 로드한다.
	// (기본 맵으로 먼저 초기화되면 Reload 버튼이 저장 파일을 덮어쓸 수 있음)
	if (!LoadPreset())
	{
		CTileScript::GetDefaultTileMap(m_Row, m_Col, m_vecTileValues);
		m_EditRow = (int)m_Row;
		m_EditCol = (int)m_Col;
		m_StatusText = "Preset not found. Loaded default map.";
	}
	else
	{
		m_StatusText = "Preset loaded.";
	}
}

TileScriptEditor::~TileScriptEditor()
{
}

void TileScriptEditor::EnsureTileBuffer()
{
	if (m_Row < 1) m_Row = 1;
	if (m_Col < 1) m_Col = 1;

	if (m_Row > 256) m_Row = 256;
	if (m_Col > 256) m_Col = 256;

	size_t required = (size_t)m_Row * (size_t)m_Col;
	if (m_vecTileValues.size() != required)
	{
		m_vecTileValues.assign(required, (int)TILETYPE::EMPTY_BLOCK);
	}
}

int TileScriptEditor::GetTileValue(UINT _Row, UINT _Col) const
{
	if (_Row >= m_Row || _Col >= m_Col)
		return (int)TILETYPE::EMPTY_BLOCK;

	return m_vecTileValues[(size_t)_Row * (size_t)m_Col + _Col];
}

void TileScriptEditor::SetTileValue(UINT _Row, UINT _Col, int _Value)
{
	if (_Row >= m_Row || _Col >= m_Col)
		return;

	if (!CTileScript::IsValidTileTypeValue(_Value))
		_Value = (int)TILETYPE::EMPTY_BLOCK;

	m_vecTileValues[(size_t)_Row * (size_t)m_Col + _Col] = _Value;
}

ImU32 TileScriptEditor::GetCellColor(int _TypeValue) const
{
	Vec4 color = CTileScript::GetDebugColorByTypeValue(_TypeValue);
	return ImGui::ColorConvertFloat4ToU32((ImVec4)color);
}

float TileScriptEditor::EvaluateTypeValue(TILETYPE _Type, float _X, float _Y) const
{
	if (_Type == TILETYPE::EMPTY_BLOCK)
		return 1.f;

	CTileScript::TILE_FORMULA_CONFIG cfg = {};
	if (!CTileScript::GetTileFormulaConfigByValue((int)_Type, cfg))
	{
		return 1.f;
	}

	if (IsLineType(_Type))
	{
		// f = c * (y - (b-a) * x - a)
		return cfg.c * (_Y - (cfg.b - cfg.a) * _X - cfg.a);
	}

	if (IsCircleType(_Type))
	{
		// f = r^2 - (x-cx)^2 - (y-cy)^2
		const float dx = _X - cfg.center_x;
		const float dy = _Y - cfg.center_y;
		return cfg.r * cfg.r - dx * dx - dy * dy;
	}

	return 1.f;
}

void TileScriptEditor::DrawTileMask(ImDrawList* _DrawList, const ImVec2& _Min, const ImVec2& _Max, TILETYPE _Type, bool _DrawOutline) const
{
	if (nullptr == _DrawList || _Type == TILETYPE::EMPTY_BLOCK)
		return;

	const float width = _Max.x - _Min.x;
	const float height = _Max.y - _Min.y;
	if (width <= 1.f || height <= 1.f)
		return;

	const ImU32 solidColor = IM_COL32(20, 20, 20, 150);
	const ImU32 outlineColor = IM_COL32(255, 210, 0, 255);

	// Line type can be drawn analytically, so avoid staircase artifacts.
	if (IsLineType(_Type))
	{
		CTileScript::TILE_FORMULA_CONFIG cfg = {};
		if (CTileScript::GetTileFormulaConfig(_Type, cfg))
		{
			const bool blockBelowLine = (cfg.c < 0.f);

			const ImVec2 pA = ImVec2(_Min.x, _Min.y + cfg.a * height);
			const ImVec2 pB = ImVec2(_Max.x, _Min.y + cfg.b * height);

			ImVec2 poly[4] = {};
			if (blockBelowLine)
			{
				poly[0] = pA;
				poly[1] = pB;
				poly[2] = ImVec2(_Max.x, _Max.y);
				poly[3] = ImVec2(_Min.x, _Max.y);
			}
			else
			{
				poly[0] = ImVec2(_Min.x, _Min.y);
				poly[1] = ImVec2(_Max.x, _Min.y);
				poly[2] = pB;
				poly[3] = pA;
			}

			_DrawList->AddConvexPolyFilled(poly, 4, solidColor);
			if (_DrawOutline)
			{
				_DrawList->AddLine(pA, pB, outlineColor, 2.f);
			}
			return;
		}
	}

	int kSubDiv = (int)(width / 4.f);
	if (kSubDiv < 12) kSubDiv = 12;
	if (kSubDiv > 40) kSubDiv = 40;

	for (int y = 0; y < kSubDiv; ++y)
	{
		for (int x = 0; x < kSubDiv; ++x)
		{
			const float nx = ((float)x + 0.5f) / (float)kSubDiv;
			const float ny = ((float)y + 0.5f) / (float)kSubDiv;

			if (EvaluateTypeValue(_Type, nx, ny) <= 0.f)
			{
				const ImVec2 p0 = ImVec2(_Min.x + width * ((float)x / (float)kSubDiv), _Min.y + height * ((float)y / (float)kSubDiv));
				const ImVec2 p1 = ImVec2(_Min.x + width * ((float)(x + 1) / (float)kSubDiv), _Min.y + height * ((float)(y + 1) / (float)kSubDiv));
				_DrawList->AddRectFilled(p0, p1, solidColor);
			}
		}
	}

	if (!_DrawOutline)
		return;

	if (IsLineType(_Type))
	{
		CTileScript::TILE_FORMULA_CONFIG cfg = {};
		if (CTileScript::GetTileFormulaConfig(_Type, cfg))
		{
			const ImVec2 p0 = ImVec2(_Min.x, _Min.y + cfg.a * height);
			const ImVec2 p1 = ImVec2(_Max.x, _Min.y + cfg.b * height);
			_DrawList->AddLine(p0, p1, outlineColor, 2.f);
		}
	}
	else if (IsCircleType(_Type))
	{
		CTileScript::TILE_FORMULA_CONFIG cfg = {};
		if (CTileScript::GetTileFormulaConfig(_Type, cfg))
		{
			// Draw only the arc part inside tile rect.
			const int kSlice = 96;
			for (int i = 0; i < kSlice; ++i)
			{
				const float t0 = XM_2PI * ((float)i / (float)kSlice);
				const float t1 = XM_2PI * ((float)(i + 1) / (float)kSlice);

				const ImVec2 p0 = ImVec2(
					_Min.x + (cfg.center_x + cosf(t0) * cfg.r) * width,
					_Min.y + (cfg.center_y + sinf(t0) * cfg.r) * height
				);
				const ImVec2 p1 = ImVec2(
					_Min.x + (cfg.center_x + cosf(t1) * cfg.r) * width,
					_Min.y + (cfg.center_y + sinf(t1) * cfg.r) * height
				);

				ImVec2 c0 = p0;
				ImVec2 c1 = p1;
				if (ClipSegmentToRect(c0, c1, _Min, _Max))
				{
					_DrawList->AddLine(c0, c1, outlineColor, 2.f);
				}
			}
		}
	}
}

void TileScriptEditor::DrawToolPanel()
{
	if (ImGui::BeginChild("TileScriptToolPanel", ImVec2(0.f, 0.f), true))
	{
		ImGui::TextColored(ImVec4(1.f, 0.9f, 0.3f, 1.f), "Tile Script Tools");
		ImGui::Separator();

		ImGui::InputInt("Rows", &m_EditRow);
		ImGui::InputInt("Cols", &m_EditCol);

		if (ImGui::Button("Apply Grid Size", ImVec2(-1.f, 0.f)))
		{
			int row = m_EditRow;
			int col = m_EditCol;

			if (row < 1) row = 1;
			if (col < 1) col = 1;
			if (row > 256) row = 256;
			if (col > 256) col = 256;

			vector<int> oldTiles = m_vecTileValues;
			UINT oldRow = m_Row;
			UINT oldCol = m_Col;

			m_Row = (UINT)row;
			m_Col = (UINT)col;
			m_vecTileValues.assign((size_t)m_Row * (size_t)m_Col, (int)TILETYPE::EMPTY_BLOCK);

			UINT copyRow = oldRow < (UINT)row ? oldRow : (UINT)row;
			UINT copyCol = oldCol < (UINT)col ? oldCol : (UINT)col;

			for (UINT r = 0; r < copyRow; ++r)
			{
				for (UINT c = 0; c < copyCol; ++c)
				{
					m_vecTileValues[(size_t)r * m_Col + c] = oldTiles[(size_t)r * oldCol + c];
				}
			}

			m_EditRow = (int)m_Row;
			m_EditCol = (int)m_Col;

			if (SavePreset())
				m_StatusText = "Grid size updated and preset saved.";
			else
				m_StatusText = "Grid size updated (preset save failed).";
		}

		ImGui::DragFloat("Cell Size", &m_CellSize, 0.25f, 12.f, 80.f, "%.1f");
		ImGui::Checkbox("Show Type Number", &m_ShowTypeNumber);
		ImGui::Checkbox("Shape Overlay", &m_ShowShapeOverlay);

		ImGui::Spacing();
		ImGui::SeparatorText("Map Transform (Render + Collision)");
		bool placementChanged = false;
		placementChanged |= ImGui::DragFloat("Map Pos X", &m_MapPlacement.map_pos_x, 1.f, -50000.f, 50000.f, "%.1f");
		placementChanged |= ImGui::DragFloat("Map Pos Y", &m_MapPlacement.map_pos_y, 1.f, -50000.f, 50000.f, "%.1f");
		placementChanged |= ImGui::DragFloat("Map Pos Z", &m_MapPlacement.map_pos_z, 1.f, -50000.f, 50000.f, "%.1f");
		placementChanged |= ImGui::DragFloat("Collision Offset X", &m_MapPlacement.collision_offset_x, 1.f, -50000.f, 50000.f, "%.1f");
		placementChanged |= ImGui::DragFloat("Collision Offset Y", &m_MapPlacement.collision_offset_y, 1.f, -50000.f, 50000.f, "%.1f");
		if (placementChanged)
		{
			CTileScript::SetTileMapPlacement(m_MapPlacement);
		}

		ImGui::Spacing();
		ImGui::SeparatorText("Preview Mode");

		if (ImGui::RadioButton("Map Edit", m_PreviewMode == PREVIEW_MAP_EDIT))
			m_PreviewMode = PREVIEW_MAP_EDIT;
		if (ImGui::RadioButton("Selected Type View", m_PreviewMode == PREVIEW_SELECTED_TYPE))
			m_PreviewMode = PREVIEW_SELECTED_TYPE;
		if (ImGui::RadioButton("All Types View", m_PreviewMode == PREVIEW_ALL_TYPE))
			m_PreviewMode = PREVIEW_ALL_TYPE;
		if (ImGui::RadioButton("Whole Map View", m_PreviewMode == PREVIEW_MAP_OVERVIEW))
			m_PreviewMode = PREVIEW_MAP_OVERVIEW;

		ImGui::Spacing();
		ImGui::SeparatorText("New Tile Type");

		if (ImGui::Button("Create Line Type", ImVec2(-1.f, 0.f)))
		{
			TILETYPE srcType = CTileScript::IsLineTileTypeValue(m_EditTypeValue)
				? CTileScript::ToTileType(m_EditTypeValue)
				: TILETYPE::LINE_BLOCK_3;

			TILETYPE newType = TILETYPE::EMPTY_BLOCK;
			if (CTileScript::CreateCustomTileType(false, srcType, newType))
			{
				m_BrushTypeValue = (int)newType;
				m_EditTypeValue = (int)newType;
				m_StatusText = "New line type created.";
			}
			else
			{
				m_StatusText = "No more free line type slots.";
			}
		}

		if (ImGui::Button("Create Circle Type", ImVec2(-1.f, 0.f)))
		{
			TILETYPE srcType = CTileScript::IsCircleTileTypeValue(m_EditTypeValue)
				? CTileScript::ToTileType(m_EditTypeValue)
				: TILETYPE::CIRCLE_BLOCK_1;

			TILETYPE newType = TILETYPE::EMPTY_BLOCK;
			if (CTileScript::CreateCustomTileType(true, srcType, newType))
			{
				m_BrushTypeValue = (int)newType;
				m_EditTypeValue = (int)newType;
				m_StatusText = "New circle type created.";
			}
			else
			{
				m_StatusText = "No more free circle type slots.";
			}
		}

		const bool canDeleteCustomType = IsCustomLineTypeValue(m_EditTypeValue) || IsCustomCircleTypeValue(m_EditTypeValue);
		if (!canDeleteCustomType)
		{
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("Delete Selected Type", ImVec2(-1.f, 0.f)))
		{
			int deleteTypeValue = m_EditTypeValue;
			size_t removedCellCount = 0;
			for (size_t i = 0; i < m_vecTileValues.size(); ++i)
			{
				if (m_vecTileValues[i] == deleteTypeValue)
				{
					m_vecTileValues[i] = (int)TILETYPE::EMPTY_BLOCK;
					++removedCellCount;
				}
			}

			if (CTileScript::DeleteCustomTileTypeByValue(deleteTypeValue))
			{
				m_BrushTypeValue = (int)TILETYPE::EMPTY_BLOCK;
				m_EditTypeValue = (int)TILETYPE::LINE_BLOCK_3;

				char msg[128] = {};
				sprintf_s(msg, "Type deleted. Cleared %zu painted cells.", removedCellCount);
				m_StatusText = msg;
			}
			else
			{
				m_StatusText = "Delete failed (built-in types cannot be deleted).";
			}
		}
		if (!canDeleteCustomType)
		{
			ImGui::EndDisabled();
		}

		ImGui::Spacing();
		ImGui::SeparatorText("Brush");

		vector<int> typeList;
		CTileScript::GetEditableTileTypeValues(typeList, true, false);

		for (size_t i = 0; i < typeList.size(); ++i)
		{
			int typeValue = typeList[i];
			TILETYPE type = CTileScript::ToTileType(typeValue);

			char label[64] = {};
			sprintf_s(label, "%02d - %s", typeValue, CTileScript::GetTileTypeNameByValue(typeValue));

			ImGui::PushID(typeValue);
			ImGui::ColorButton("##TypeColor", (ImVec4)CTileScript::GetDebugColorByTypeValue(typeValue), ImGuiColorEditFlags_NoTooltip, ImVec2(14.f, 14.f));
			ImGui::SameLine();

			bool selected = (m_BrushTypeValue == typeValue);
			if (ImGui::Selectable(label, selected))
			{
				m_BrushTypeValue = typeValue;
				if (typeValue != (int)TILETYPE::EMPTY_BLOCK)
				{
					m_EditTypeValue = typeValue;
				}
			}
			ImGui::PopID();
		}

		ImGui::Spacing();
		ImGui::Text("LMB drag: paint");
		ImGui::Text("RMB drag: erase -> EMPTY");

		ImGui::Spacing();
		ImGui::SeparatorText("Preset");
		ImGui::InputText("Relative Path", m_szPresetRelativePath, sizeof(m_szPresetRelativePath));

		if (ImGui::Button("Save Preset", ImVec2(-1.f, 0.f)))
		{
			if (SavePreset())
				m_StatusText = "Preset saved.";
			else
				m_StatusText = "Save failed.";
		}

		if (ImGui::Button("Load Preset", ImVec2(-1.f, 0.f)))
		{
			if (LoadPreset())
				m_StatusText = "Preset loaded.";
			else
				m_StatusText = "Load failed.";
		}

		if (ImGui::Button("Load Default Map", ImVec2(-1.f, 0.f)))
		{
			LoadDefaultMap();
			m_StatusText = "Default map loaded.";
		}

		if (ImGui::Button("Reload TestLevel", ImVec2(-1.f, 0.f)))
		{
			SavePreset();
			ChangeLevel(L"TestLevel");
			m_StatusText = "TestLevel reload requested.";
		}

		ImGui::Spacing();
		ImGui::TextWrapped("Status: %s", m_StatusText.c_str());

		DrawTypeConfigPanel();
	}
	ImGui::EndChild();
}

void TileScriptEditor::DrawTypeConfigPanel()
{
	ImGui::Spacing();
	ImGui::SeparatorText("Type Formula");

	TILETYPE curType = CTileScript::ToTileType(m_EditTypeValue);

	if (ImGui::BeginCombo("Edit Type", CTileScript::GetTileTypeNameByValue(m_EditTypeValue)))
	{
		vector<int> typeList;
		CTileScript::GetEditableTileTypeValues(typeList, false, false);

		for (size_t i = 0; i < typeList.size(); ++i)
		{
			int typeValue = typeList[i];
			TILETYPE t = CTileScript::ToTileType(typeValue);
			bool selected = (m_EditTypeValue == typeValue);
			if (ImGui::Selectable(CTileScript::GetTileTypeNameByValue(typeValue), selected))
			{
				m_EditTypeValue = typeValue;
				curType = t;
			}
		}
		ImGui::EndCombo();
	}

	CTileScript::TILE_FORMULA_CONFIG cfg = {};
	if (!CTileScript::GetTileFormulaConfigByValue(m_EditTypeValue, cfg))
		return;

	bool changed = false;

	if (IsLineType(curType))
	{
		ImGui::Text("Line: c * (y - (b-a)*x - a)");
		changed |= ImGui::DragFloat("a (Start Y)", &cfg.a, 0.005f, -2.f, 2.f, "%.3f");
		changed |= ImGui::DragFloat("b (End Y)", &cfg.b, 0.005f, -2.f, 2.f, "%.3f");
		changed |= ImGui::DragFloat("c (Direction)", &cfg.c, 0.01f, -5.f, 5.f, "%.3f");

		const float curSlope = cfg.b - cfg.a;
		float angleDeg = atan2f(curSlope, 1.f) * (180.f / XM_PI);
		float midY = (cfg.a + cfg.b) * 0.5f;

		if (ImGui::DragFloat("Slope Angle (deg)", &angleDeg, 0.1f, -89.f, 89.f, "%.2f"))
		{
			const float slope = tanf(angleDeg * (XM_PI / 180.f));
			cfg.a = midY - slope * 0.5f;
			cfg.b = midY + slope * 0.5f;
			changed = true;
		}

		if (ImGui::DragFloat("Line Mid Y", &midY, 0.005f, -2.f, 2.f, "%.3f"))
		{
			const float slope = cfg.b - cfg.a;
			cfg.a = midY - slope * 0.5f;
			cfg.b = midY + slope * 0.5f;
			changed = true;
		}

		if (ImGui::Button("Clamp a,b to [0,1]", ImVec2(-1.f, 0.f)))
		{
			cfg.a = ClampFloat(cfg.a, 0.f, 1.f);
			cfg.b = ClampFloat(cfg.b, 0.f, 1.f);
			changed = true;
		}

		ImGui::TextWrapped("Tip: Drag the two line handles in Selected Type View for direct slope editing.");
	}
	else
	{
		ImGui::Text("Circle: r^2 - (x-cx)^2 - (y-cy)^2");
		changed |= ImGui::DragFloat("r (Radius)", &cfg.r, 0.005f, 0.05f, 2.f, "%.3f");
		changed |= ImGui::DragFloat("center_x", &cfg.center_x, 0.005f, -1.f, 2.f, "%.3f");
		changed |= ImGui::DragFloat("center_y", &cfg.center_y, 0.005f, -1.f, 2.f, "%.3f");

		ImGui::Spacing();
		ImGui::SeparatorText("Loop Exit Fallback Line");
		changed |= ImGui::DragFloat("fallback a (Start Y)", &cfg.a, 0.005f, -2.f, 2.f, "%.3f");
		changed |= ImGui::DragFloat("fallback b (End Y)", &cfg.b, 0.005f, -2.f, 2.f, "%.3f");
		changed |= ImGui::DragFloat("fallback c (Direction)", &cfg.c, 0.01f, -5.f, 5.f, "%.3f");

		if (curType == TILETYPE::CIRCLE_BLOCK_3 || curType == TILETYPE::CIRCLE_BLOCK_4)
		{
			ImGui::TextWrapped("Used when half-check swaps this circle into line mode.");
		}
		else
		{
			ImGui::TextDisabled("Fallback line is mostly used by CIRCLE_3/CIRCLE_4.");
		}
	}

	if (changed)
	{
		CTileScript::SetTileFormulaConfigByValue(m_EditTypeValue, cfg);
	}

	if (ImGui::Button("Reset Formula Defaults", ImVec2(-1.f, 0.f)))
	{
		CTileScript::ResetTileFormulaConfigToDefault();
		m_StatusText = "Formula defaults restored.";
	}
}

void TileScriptEditor::DrawCanvasPanel()
{
	ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar;

	if (ImGui::BeginChild("TileScriptCanvasPanel", ImVec2(0.f, 0.f), true, flags))
	{
		ImGui::TextColored(ImVec4(0.5f, 1.f, 0.9f, 1.f), "Tile Layout (Direct Visual)");
		ImGui::Separator();

		ImVec2 canvasPos = ImGui::GetCursorScreenPos();
		ImVec2 canvasSize = ImVec2(m_CellSize * (float)m_Col, m_CellSize * (float)m_Row);

		ImGui::InvisibleButton("##TileCanvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

		bool hovered = ImGui::IsItemHovered();
		ImVec2 mousePos = ImGui::GetIO().MousePos;

		int hoveredCol = (int)((mousePos.x - canvasPos.x) / m_CellSize);
		int hoveredRow = (int)((mousePos.y - canvasPos.y) / m_CellSize);

		bool validHoveredCell =
			(0 <= hoveredRow && hoveredRow < (int)m_Row) &&
			(0 <= hoveredCol && hoveredCol < (int)m_Col);

		if (hovered && validHoveredCell)
		{
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				SetTileValue((UINT)hoveredRow, (UINT)hoveredCol, m_BrushTypeValue);
			}
			if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
			{
				SetTileValue((UINT)hoveredRow, (UINT)hoveredCol, (int)TILETYPE::EMPTY_BLOCK);
			}
		}

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		for (UINT row = 0; row < m_Row; ++row)
		{
			for (UINT col = 0; col < m_Col; ++col)
			{
				int typeValue = GetTileValue(row, col);
				TILETYPE type = CTileScript::ToTileType(typeValue);

				ImVec2 p0 = ImVec2(canvasPos.x + (float)col * m_CellSize, canvasPos.y + (float)row * m_CellSize);
				ImVec2 p1 = ImVec2(p0.x + m_CellSize, p0.y + m_CellSize);

				drawList->AddRectFilled(p0, p1, GetCellColor(typeValue));
				if (m_ShowShapeOverlay)
				{
					DrawTileMask(drawList, p0, p1, type, true);
				}
				drawList->AddRect(p0, p1, IM_COL32(90, 90, 90, 255));

				if (m_ShowTypeNumber)
				{
					char typeText[8] = {};
					sprintf_s(typeText, "%d", typeValue);

					ImU32 textColor = (typeValue == (int)TILETYPE::EMPTY_BLOCK)
						? IM_COL32(20, 20, 20, 255)
						: IM_COL32(245, 245, 245, 255);

					drawList->AddText(ImVec2(p0.x + 3.f, p0.y + 2.f), textColor, typeText);
				}
			}
		}

		if (validHoveredCell)
		{
			ImVec2 h0 = ImVec2(canvasPos.x + (float)hoveredCol * m_CellSize, canvasPos.y + (float)hoveredRow * m_CellSize);
			ImVec2 h1 = ImVec2(h0.x + m_CellSize, h0.y + m_CellSize);
			drawList->AddRect(h0, h1, IM_COL32(255, 220, 0, 255), 0.f, 0, 2.f);
		}
	}
	ImGui::EndChild();
}

void TileScriptEditor::DrawSelectedTypePreviewPanel()
{
	if (ImGui::BeginChild("TileTypePreviewPanel", ImVec2(0.f, 0.f), true))
	{
		TILETYPE type = CTileScript::ToTileType(m_EditTypeValue);
		ImGui::TextColored(ImVec4(0.5f, 1.f, 0.9f, 1.f), "Selected Type Preview");
		ImGui::Text("Type: %s (%d)", CTileScript::GetTileTypeNameByValue(m_EditTypeValue), m_EditTypeValue);
		ImGui::Separator();

		ImVec2 avail = ImGui::GetContentRegionAvail();
		float previewSize = avail.x < avail.y ? avail.x : avail.y;
		previewSize -= 20.f;
		if (previewSize < 180.f)
			previewSize = 180.f;

		ImVec2 canvasSize(previewSize, previewSize);
		ImVec2 canvasPos = ImGui::GetCursorScreenPos();

		ImGui::InvisibleButton("##SingleTypePreview", canvasSize, ImGuiButtonFlags_MouseButtonLeft);
		bool hovered = ImGui::IsItemHovered();

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 p0 = canvasPos;
		ImVec2 p1 = ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);
		drawList->AddRectFilled(p0, p1, IM_COL32(255, 255, 255, 255));
		drawList->AddRect(p0, p1, IM_COL32(120, 120, 120, 255), 0.f, 0, 2.f);

		DrawTileMask(drawList, p0, p1, type, true);

		if (IsCircleType(type))
		{
			CTileScript::TILE_FORMULA_CONFIG cfg = {};
			if (CTileScript::GetTileFormulaConfigByValue(m_EditTypeValue, cfg))
			{
				const ImVec2 pA = ImVec2(p0.x, p0.y + cfg.a * canvasSize.y);
				const ImVec2 pB = ImVec2(p1.x, p0.y + cfg.b * canvasSize.y);
				drawList->AddLine(pA, pB, IM_COL32(255, 140, 0, 220), 2.f);
			}
		}

		if (IsLineType(type))
		{
			CTileScript::TILE_FORMULA_CONFIG cfg = {};
			if (CTileScript::GetTileFormulaConfigByValue(m_EditTypeValue, cfg))
			{
				const ImVec2 handle0(p0.x, p0.y + cfg.a * canvasSize.y);
				const ImVec2 handle1(p1.x, p0.y + cfg.b * canvasSize.y);

				drawList->AddCircleFilled(handle0, 6.f, IM_COL32(255, 90, 90, 255));
				drawList->AddCircleFilled(handle1, 6.f, IM_COL32(90, 180, 255, 255));

				ImVec2 mousePos = ImGui::GetIO().MousePos;
				const float d0x = mousePos.x - handle0.x;
				const float d0y = mousePos.y - handle0.y;
				const float d1x = mousePos.x - handle1.x;
				const float d1y = mousePos.y - handle1.y;

				if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					if (d0x * d0x + d0y * d0y <= 10.f * 10.f)
						m_DragLineHandle = 0;
					else if (d1x * d1x + d1y * d1y <= 10.f * 10.f)
						m_DragLineHandle = 1;
				}

				if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
				{
					m_DragLineHandle = -1;
				}
				else if (m_DragLineHandle != -1)
				{
					float localY = (mousePos.y - p0.y) / canvasSize.y;
					localY = ClampFloat(localY, -2.f, 2.f);

					if (m_DragLineHandle == 0)
						cfg.a = localY;
					else
						cfg.b = localY;

					CTileScript::SetTileFormulaConfigByValue(m_EditTypeValue, cfg);
				}
			}
		}

		ImGui::Spacing();
		if (IsLineType(type))
		{
			ImGui::TextWrapped("Drag red/blue points to change line slope directly.");
		}
		else if (IsCircleType(type))
		{
			ImGui::TextWrapped("Orange line shows loop-exit fallback line (editable in Type Formula panel).");
		}
	}
	ImGui::EndChild();
}

void TileScriptEditor::DrawAllTypePreviewPanel()
{
	if (ImGui::BeginChild("AllTypePreviewPanel", ImVec2(0.f, 0.f), true))
	{
		ImGui::TextColored(ImVec4(0.5f, 1.f, 0.9f, 1.f), "All Types");
		ImGui::TextWrapped("Select brush/edit type directly from visual tile cards.");
		ImGui::Separator();

		const float tileSize = 105.f;
		const float spacing = 8.f;

		vector<int> typeList;
		CTileScript::GetEditableTileTypeValues(typeList, true, false);

		int idx = 0;
		for (size_t i = 0; i < typeList.size(); ++i, ++idx)
		{
			int typeValue = typeList[i];
			if (idx > 0 && idx % 3 != 0)
				ImGui::SameLine(0.f, spacing);

			TILETYPE type = CTileScript::ToTileType(typeValue);

			ImGui::PushID(typeValue);
			ImGui::BeginGroup();
			ImGui::Text("%02d %s", typeValue, CTileScript::GetTileTypeNameByValue(typeValue));

			ImVec2 pos = ImGui::GetCursorScreenPos();
			ImGui::InvisibleButton("##TypePreviewButton", ImVec2(tileSize, tileSize), ImGuiButtonFlags_MouseButtonLeft);
			if (ImGui::IsItemClicked())
			{
				m_BrushTypeValue = typeValue;
				if (typeValue != (int)TILETYPE::EMPTY_BLOCK)
					m_EditTypeValue = typeValue;

				m_StatusText = "Selected from all-type preview.";
			}

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 p0 = pos;
			ImVec2 p1 = ImVec2(pos.x + tileSize, pos.y + tileSize);
			drawList->AddRectFilled(p0, p1, GetCellColor(typeValue));
			DrawTileMask(drawList, p0, p1, type, true);
			drawList->AddRect(p0, p1, IM_COL32(100, 100, 100, 255));

			if (m_BrushTypeValue == typeValue)
			{
				drawList->AddRect(p0, p1, IM_COL32(255, 215, 0, 255), 0.f, 0, 2.5f);
			}

			ImGui::EndGroup();
			ImGui::PopID();
		}
	}
	ImGui::EndChild();
}

void TileScriptEditor::DrawWholeMapOverviewPanel()
{
	if (ImGui::BeginChild("TileScriptMapOverviewPanel", ImVec2(0.f, 0.f), true))
	{
		ImGui::TextColored(ImVec4(0.5f, 1.f, 0.9f, 1.f), "Whole Map Overview");
		ImGui::TextWrapped("See the entire map in one fitted view.");
		ImGui::Separator();

		ImVec2 avail = ImGui::GetContentRegionAvail();
		if (avail.x < 20.f || avail.y < 20.f)
		{
			ImGui::EndChild();
			return;
		}

		const float cellW = avail.x / (float)m_Col;
		const float cellH = avail.y / (float)m_Row;
		float cell = cellW < cellH ? cellW : cellH;
		cell -= 1.f;
		if (cell < 2.f) cell = 2.f;

		const float totalW = cell * (float)m_Col;
		const float totalH = cell * (float)m_Row;
		const float startX = ImGui::GetCursorScreenPos().x + (avail.x - totalW) * 0.5f;
		const float startY = ImGui::GetCursorScreenPos().y + (avail.y - totalH) * 0.5f;

		ImVec2 canvasPos(startX, startY);
		ImVec2 canvasSize(totalW, totalH);
		ImGui::SetCursorScreenPos(canvasPos);
		ImGui::InvisibleButton("##WholeMapCanvas", canvasSize);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		for (UINT row = 0; row < m_Row; ++row)
		{
			for (UINT col = 0; col < m_Col; ++col)
			{
				int typeValue = GetTileValue(row, col);
				TILETYPE type = CTileScript::ToTileType(typeValue);

				ImVec2 p0(canvasPos.x + (float)col * cell, canvasPos.y + (float)row * cell);
				ImVec2 p1(p0.x + cell, p0.y + cell);

				drawList->AddRectFilled(p0, p1, GetCellColor(typeValue));
				if (m_ShowShapeOverlay)
				{
					DrawTileMask(drawList, p0, p1, type, false);
				}
				drawList->AddRect(p0, p1, IM_COL32(90, 90, 90, 255));
			}
		}
	}
	ImGui::EndChild();
}

void TileScriptEditor::LoadDefaultMap()
{
	CTileScript::GetDefaultTileMap(m_Row, m_Col, m_vecTileValues);
	m_EditRow = (int)m_Row;
	m_EditCol = (int)m_Col;
}

bool TileScriptEditor::SavePreset()
{
	EnsureTileBuffer();
	CTileScript::SetTileMapPlacement(m_MapPlacement);

	string relativePath = m_szPresetRelativePath;
	if (relativePath.empty())
		relativePath = "TileMap\\TileScriptPreset.txt";

	wstring fullPath = wstring(CONTENT_PATH) + wstring(relativePath.begin(), relativePath.end());
	return CTileScript::SaveTileScriptPreset(fullPath, m_Row, m_Col, m_vecTileValues);
}

bool TileScriptEditor::LoadPreset()
{
	string relativePath = m_szPresetRelativePath;
	if (relativePath.empty())
		relativePath = "TileMap\\TileScriptPreset.txt";

	wstring fullPath = wstring(CONTENT_PATH) + wstring(relativePath.begin(), relativePath.end());

	UINT row = 0;
	UINT col = 0;
	vector<int> tiles;

	if (!CTileScript::LoadTileScriptPreset(fullPath, row, col, tiles, true))
		return false;

	m_Row = row;
	m_Col = col;
	m_vecTileValues = tiles;
	EnsureTileBuffer();
	m_EditRow = (int)m_Row;
	m_EditCol = (int)m_Col;
	CTileScript::GetTileMapPlacement(m_MapPlacement);

	return true;
}

void TileScriptEditor::Tick_UI()
{
	EnsureTileBuffer();

	ImGui::Columns(2, "TileScriptEditorColumns", true);
	ImGui::SetColumnWidth(0, 340.f);

	DrawToolPanel();

	ImGui::NextColumn();

	switch (m_PreviewMode)
	{
	case PREVIEW_SELECTED_TYPE:
		DrawSelectedTypePreviewPanel();
		break;
	case PREVIEW_ALL_TYPE:
		DrawAllTypePreviewPanel();
		break;
	case PREVIEW_MAP_OVERVIEW:
		DrawWholeMapOverviewPanel();
		break;
	case PREVIEW_MAP_EDIT:
	default:
		DrawCanvasPanel();
		break;
	}

	ImGui::Columns(1);
}
