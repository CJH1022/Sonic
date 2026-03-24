#include "pch.h"
#include "ImageEditor.h"
#include "AssetMgr.h"
#include "EditorUI.h"
#include "EditorMgr.h"
#include "ASprite.h"
#include "AssetUI.h"
#include <filesystem>
#include <algorithm>
#include "ListUI.h"
#include "AFlipbook.h"

static void DrawSectionTitle(const char* title)
{
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::TextColored(ImVec4(1.f, 0.85f, 0.2f, 1.f), "%s", title);
	ImGui::Spacing();
}

static void PushBlueButtonStyle()
{
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.20f, 0.25f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.45f, 0.75f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.35f, 0.65f, 1.f));
}

static void PushGreenButtonStyle()
{
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.35f, 0.22f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.50f, 0.30f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.42f, 0.20f, 1.f));
}

static void PopButtonStyle()
{
	ImGui::PopStyleColor(3);
}

ImageEditor::ImageEditor()
	: EditorUI("IMAGE_EDITOR")
	, m_bIsDragging(false)
	, m_iRows(1)
	, m_iCols(1)
	, m_vSliceSize(Vec2(64.f, 64.f))
	, m_vRegionStart(Vec2(0.f, 0.f))
	, m_vRegionEnd(Vec2(0.f, 0.f))
	, m_vBackgroundStart(Vec2(0.f, 0.f))
	, m_vBackgroundSize(Vec2(64.f, 64.f))
	, m_vCellOffset(Vec2(0.f, 0.f))
	, m_vCellSpriteSize(Vec2(64.f, 64.f))
	, m_AssetType(ASSET_TYPE::SPRITE)
	, m_curState(IMAGE_STATE::ATLAS)
	, m_pTargetFlipbook(nullptr)
	, m_iCurFrame(0)
	, m_fAccTime(0.f)
	, m_bPreviewPlaying(true)
	, m_bPreviewLoop(true)
	, m_fPreviewFPS(10.f)
	, m_bAutoTrimBlackBG(false)
	, m_iBlackThreshold(16)
	, m_iAlphaThreshold(4)
{
	m_vStartPos = Vec2(0.f, 0.f);
	m_pTargetSprite = new ASprite;
	m_pTargetAtlas = AssetMgr::GetInst()->Load<ATexture>(L"Link", L"Texture\\link.png");
	m_pTargetFlipbook = new AFlipbook;

	LoadAllSprites();
	LoadAllFlipbooks();
}

ImageEditor::~ImageEditor()
{
	if (m_pTargetFlipbook != nullptr)
	{
		delete m_pTargetFlipbook;
		m_pTargetFlipbook = nullptr;
	}
}

void ImageEditor::Tick_UI()
{
	OutputTitle(m_AssetType);

	Ptr<ATexture> pActiveAtlas = nullptr;
	if (m_curState == IMAGE_STATE::ATLAS)
		pActiveAtlas = m_pTargetAtlas;
	else if (m_curState == IMAGE_STATE::SPRITE && m_pTargetSprite.Get())
		pActiveAtlas = m_pTargetSprite->GetAtlas();

	if (nullptr == pActiveAtlas)
		return;

	static char szFileName[256] = "NewAsset";
	static bool bOpenSpriteList = false;
	static bool bOpenFlipbookList = false;
	static bool bShowPreview = true;

	float imgW = (float)pActiveAtlas->GetWidth();
	float imgH = (float)pActiveAtlas->GetHeight();

	auto ClampFloat = [](float value, float minValue, float maxValue) -> float
	{
		return max(minValue, min(value, maxValue));
	};

	auto ClampSettings = [&]()
	{
		m_iRows = max(1, m_iRows);
		m_iCols = max(1, m_iCols);

		m_vRegionStart.x = ClampFloat(m_vRegionStart.x, 0.f, imgW);
		m_vRegionStart.y = ClampFloat(m_vRegionStart.y, 0.f, imgH);
		m_vRegionEnd.x = ClampFloat(m_vRegionEnd.x, m_vRegionStart.x + 1.f, imgW);
		m_vRegionEnd.y = ClampFloat(m_vRegionEnd.y, m_vRegionStart.y + 1.f, imgH);

		m_vBackgroundStart.x = ClampFloat(m_vBackgroundStart.x, m_vRegionStart.x, m_vRegionEnd.x - 1.f);
		m_vBackgroundStart.y = ClampFloat(m_vBackgroundStart.y, m_vRegionStart.y, m_vRegionEnd.y - 1.f);

		const float maxBgW = max(1.f, m_vRegionEnd.x - m_vBackgroundStart.x);
		const float maxBgH = max(1.f, m_vRegionEnd.y - m_vBackgroundStart.y);
		m_vBackgroundSize.x = ClampFloat(m_vBackgroundSize.x, 1.f, maxBgW);
		m_vBackgroundSize.y = ClampFloat(m_vBackgroundSize.y, 1.f, maxBgH);

		m_vCellOffset.x = ClampFloat(m_vCellOffset.x, 0.f, max(0.f, m_vBackgroundSize.x - 1.f));
		m_vCellOffset.y = ClampFloat(m_vCellOffset.y, 0.f, max(0.f, m_vBackgroundSize.y - 1.f));

		const float maxSpriteW = max(1.f, m_vBackgroundSize.x - m_vCellOffset.x);
		const float maxSpriteH = max(1.f, m_vBackgroundSize.y - m_vCellOffset.y);
		m_vCellSpriteSize.x = ClampFloat(m_vCellSpriteSize.x, 1.f, maxSpriteW);
		m_vCellSpriteSize.y = ClampFloat(m_vCellSpriteSize.y, 1.f, maxSpriteH);

		m_vSliceSize = m_vBackgroundSize; // legacy synchronization
	};

	auto GetGridStep = [&]() -> Vec2
	{
		const float stepX = (m_iCols > 1) ? (m_vRegionEnd.x - m_vBackgroundStart.x - m_vBackgroundSize.x) / (float)(m_iCols - 1) : 0.f;
		const float stepY = (m_iRows > 1) ? (m_vRegionEnd.y - m_vBackgroundStart.y - m_vBackgroundSize.y) / (float)(m_iRows - 1) : 0.f;
		return Vec2(stepX, stepY);
	};

	auto GetCellBackgroundLT = [&](int _Row, int _Col) -> Vec2
	{
		Vec2 step = GetGridStep();
		return Vec2(m_vBackgroundStart.x + step.x * (float)_Col, m_vBackgroundStart.y + step.y * (float)_Row);
	};

	auto BuildSpriteRectFromCell = [&](int _Row, int _Col, Vec2& _OutCellLT, Vec2& _OutSpriteLT, Vec2& _OutBgSize, Vec2& _OutOffset, Vec2& _OutSpriteSize) -> bool
	{
		_OutCellLT = GetCellBackgroundLT(_Row, _Col);
		_OutBgSize = m_vBackgroundSize;
		_OutOffset = m_vCellOffset;
		_OutSpriteSize = m_vCellSpriteSize;
		_OutSpriteLT = _OutCellLT + _OutOffset;

		if (_OutCellLT.x < m_vRegionStart.x - 0.5f || _OutCellLT.y < m_vRegionStart.y - 0.5f)
			return false;
		if (_OutCellLT.x + _OutBgSize.x > m_vRegionEnd.x + 0.5f || _OutCellLT.y + _OutBgSize.y > m_vRegionEnd.y + 0.5f)
			return false;
		if (_OutSpriteLT.x + _OutSpriteSize.x > imgW + 0.5f || _OutSpriteLT.y + _OutSpriteSize.y > imgH + 0.5f)
			return false;

		return true;
	};

	if (m_vRegionEnd.x <= m_vRegionStart.x || m_vRegionEnd.y <= m_vRegionStart.y)
	{
		m_vRegionStart = Vec2(0.f, 0.f);
		m_vRegionEnd = Vec2(imgW, imgH);
		m_vBackgroundStart = m_vRegionStart;
	}

	ClampSettings();

	ImGui::Columns(2, nullptr, true);

	// =========================
	// LEFT PANEL
	// =========================
	ImGui::BeginChild("LeftPanel", ImVec2(0, 0), true);
	{
		DrawSectionTitle("Grid & Slice Settings");
		ImGui::PushItemWidth(170.f);
		const float maxSizeValue = max(imgW, imgH);

		ImGui::InputInt("Row", &m_iRows);
		ImGui::InputInt("Col", &m_iCols);
		ImGui::DragFloat2("Start Pos", (float*)&m_vRegionStart, 0.5f, 0.f, maxSizeValue, "%.0f px");
		ImGui::DragFloat2("End Pos", (float*)&m_vRegionEnd, 0.5f, 0.f, maxSizeValue, "%.0f px");
		ImGui::DragFloat2("Background Start Pos", (float*)&m_vBackgroundStart, 0.5f, 0.f, maxSizeValue, "%.0f px");
		ImGui::DragFloat2("Background Sprite Size", (float*)&m_vBackgroundSize, 0.5f, 1.f, maxSizeValue, "%.0f px");
		ImGui::DragFloat2("Sprite Size", (float*)&m_vCellSpriteSize, 0.5f, 1.f, maxSizeValue, "%.0f px");
		ImGui::DragFloat2("Sprite Offset", (float*)&m_vCellOffset, 0.5f, 0.f, maxSizeValue, "%.0f px");

		ClampSettings();
		ImGui::PopItemWidth();

		Vec2 step = GetGridStep();
		Vec2 totalSize = m_vRegionEnd - m_vRegionStart;

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f), "Grid: %d x %d", m_iCols, m_iRows);
		ImGui::TextColored(ImVec4(0.4f, 1.f, 1.f, 1.f), "Total Area: %.1f x %.1f", totalSize.x, totalSize.y);
		ImGui::TextColored(ImVec4(0.75f, 0.9f, 1.f, 1.f), "Background Start: %.1f, %.1f", m_vBackgroundStart.x, m_vBackgroundStart.y);
		ImGui::TextColored(ImVec4(0.72f, 0.72f, 0.72f, 1.f), "Background Step: %.1f, %.1f", step.x, step.y);
		ImGui::TextColored(ImVec4(1.f, 0.55f, 0.25f, 1.f), "Background Sprite Size: %.1f x %.1f", m_vBackgroundSize.x, m_vBackgroundSize.y);
		ImGui::TextColored(ImVec4(1.f, 0.85f, 0.2f, 1.f), "Sprite Size: %.1f x %.1f / Offset: %.1f, %.1f",
			m_vCellSpriteSize.x, m_vCellSpriteSize.y, m_vCellOffset.x, m_vCellOffset.y);
		ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.5f, 1.f), "Start Pos: %.0f, %.0f", m_vRegionStart.x, m_vRegionStart.y);
		ImGui::TextColored(ImVec4(1.f, 0.8f, 0.35f, 1.f), "End Pos: %.0f, %.0f", m_vRegionEnd.x, m_vRegionEnd.y);
		ImGui::TextColored(ImVec4(0.72f, 0.72f, 0.72f, 1.f), "Gray Box: Background Sprite");
		ImGui::TextColored(ImVec4(1.f, 0.85f, 0.2f, 1.f), "Yellow Box: Real Sprite");

		DrawSectionTitle("Save Settings");
		ImGui::InputText("File Name", szFileName, sizeof(szFileName));

		ImGui::Text("Mode:"); ImGui::SameLine();
		if (m_curState == IMAGE_STATE::ATLAS)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.70f, 1.f));
			if (ImGui::Button("ATLAS", ImVec2(90.f, 0.f))) m_curState = IMAGE_STATE::ATLAS;
			ImGui::PopStyleColor();
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.35f, 0.20f, 1.f));
			if (ImGui::Button("SPRITE", ImVec2(90.f, 0.f))) m_curState = IMAGE_STATE::SPRITE;
			ImGui::PopStyleColor();
		}

		PushGreenButtonStyle();
		if (ImGui::Button("Save Selected as Sprites", ImVec2(-1.f, 34.f)))
		{
			if (m_vecSelectedIndex.empty())
			{
				MessageBox(nullptr, L"선택된 영역이 없습니다!", L"Save Error", MB_OK);
			}
			else if (pActiveAtlas == nullptr)
			{
				MessageBox(nullptr, L"Atlas가 없습니다!", L"Save Error", MB_OK);
			}
			else
			{
				// 1. 저장 디렉토리 생성
				wstring spriteFolderPath = wstring(CONTENT_PATH) + L"Sprite\\";
				std::filesystem::create_directories(spriteFolderPath);

				// 2. 선택된 모든 인덱스 순회
				for (int idx : m_vecSelectedIndex)
				{
					int r = idx / m_iCols;
					int c = idx % m_iCols;
					Vec2 cellLT = {};
					Vec2 spriteLT = {};
					Vec2 bgSize = {};
					Vec2 offset = {};
					Vec2 spriteSize = {};
					if (!BuildSpriteRectFromCell(r, c, cellLT, spriteLT, bgSize, offset, spriteSize))
						continue;

					// 3. 임시 스프라이트 객체 설정
					ASprite tempSprite;
					tempSprite.SetAtlas(pActiveAtlas);
					tempSprite.SetLeftTopUV(Vec2(spriteLT.x / imgW, spriteLT.y / imgH));
					tempSprite.SetSliceUV(Vec2(spriteSize.x / imgW, spriteSize.y / imgH));
					tempSprite.SetBackgroundUV(Vec2(bgSize.x / imgW, bgSize.y / imgH));
					tempSprite.SetOffsetUV(Vec2(offset.x / imgW, offset.y / imgH));

					// 4. 파일 이름 결정 및 중복 체크
					wstring baseName = ToWString(szFileName);
					wstring finalRelativePath = L"Sprite\\" + baseName + L".sprite";
					wstring finalFullPath = wstring(CONTENT_PATH) + finalRelativePath;

					int count = 1;
					while (std::filesystem::exists(finalFullPath))
					{
						finalRelativePath = L"Sprite\\" + baseName + L"_" + std::to_wstring(count) + L".sprite";
						finalFullPath = wstring(CONTENT_PATH) + finalRelativePath;
						++count;
					}

					// 5. 파일 저장
					tempSprite.Save(finalFullPath);

					// 6. ✨ 엔진 AssetMgr에 즉시 로드 (리스트 새로고침 효과)
					// 파일명(키) 추출을 위해 확장자 제외
					wstring assetKey = std::filesystem::path(finalRelativePath).stem().wstring();
					AssetMgr::GetInst()->Load<ASprite>(assetKey, finalRelativePath);
				}

				// 7. 후처리
				m_vecSelectedIndex.clear();
				// 플립북 리스트도 갱신이 필요하다면 호출
				LoadAllSprites();

				MessageBox(nullptr, L"스프라이트 저장 및 리스트 등록 완료!", L"Success", MB_OK);
			}
		}

		if (ImGui::Button("Save Selected as Flipbook", ImVec2(-1.f, 34.f)))
		{
			if (m_vecSelectedIndex.empty()) MessageBox(nullptr, L"선택된 영역이 없습니다!", L"Save Error", MB_OK);
			else {
				std::filesystem::create_directories(wstring(CONTENT_PATH) + L"Sprite\\");
				std::filesystem::create_directories(wstring(CONTENT_PATH) + L"Flipbook\\");
				AFlipbook* pNewFlipbook = new AFlipbook;
				wstring flipName = ToWString(szFileName);
				int savedCount = 0;
				for (size_t i = 0; i < m_vecSelectedIndex.size(); ++i) {
					int idx = m_vecSelectedIndex[i];
					int r = idx / m_iCols;
					int c = idx % m_iCols;

					Vec2 cellLT = {};
					Vec2 spriteLT = {};
					Vec2 bgSize = {};
					Vec2 offset = {};
					Vec2 spriteSize = {};
					if (!BuildSpriteRectFromCell(r, c, cellLT, spriteLT, bgSize, offset, spriteSize))
						continue;

					wstring spriteKey = flipName + L"_frame_" + std::to_wstring(i);
					wstring spriteRelPath = L"Sprite\\" + spriteKey + L".sprite";
					wstring spriteFullPath = wstring(CONTENT_PATH) + spriteRelPath;
					ASprite tempSprite;
					tempSprite.SetAtlas(pActiveAtlas);
					tempSprite.SetLeftTopUV(Vec2(spriteLT.x / imgW, spriteLT.y / imgH));
					tempSprite.SetSliceUV(Vec2(spriteSize.x / imgW, spriteSize.y / imgH));
					tempSprite.SetBackgroundUV(Vec2(bgSize.x / imgW, bgSize.y / imgH));
					tempSprite.SetOffsetUV(Vec2(offset.x / imgW, offset.y / imgH));
					tempSprite.Save(spriteFullPath);
					Ptr<ASprite> pSavedSprite = AssetMgr::GetInst()->Load<ASprite>(spriteKey, spriteRelPath);
					if (pSavedSprite != nullptr)
					{
						pNewFlipbook->AddSprite(pSavedSprite);
						++savedCount;
					}
				}
				if (savedCount > 0)
				{
					pNewFlipbook->Save(wstring(CONTENT_PATH) + L"Flipbook\\" + flipName + L".flip");
					LoadAllFlipbooks(); // ✨ 새로고침
					MessageBox(nullptr, L"플립북 저장 완료!", L"Success", MB_OK);
				}
				else
				{
					MessageBox(nullptr, L"저장 가능한 셀이 없습니다. Region/Background/Offset/SpriteSize를 확인하세요.", L"Save Error", MB_OK);
				}
				delete pNewFlipbook;
			}
		}
		PopButtonStyle();

		DrawSectionTitle("Load");
		PushBlueButtonStyle();
		if (ImGui::Button("Open Load Sprite", ImVec2(-1.f, 34.f))) bOpenSpriteList = true;
		if (ImGui::Button("Open Load Flipbook", ImVec2(-1.f, 34.f))) bOpenFlipbookList = true;
		if (ImGui::Button("Toggle Preview Window", ImVec2(-1.f, 34.f))) bShowPreview = !bShowPreview;
		PopButtonStyle();
	}
	ImGui::EndChild();

	ImGui::NextColumn();

	// =========================
	// RIGHT PANEL (Atlas View)
	// =========================
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar;

	ImGui::BeginChild("RightPanel", ImVec2(0, 0), true, window_flags);
	{
		DrawSectionTitle("Atlas Selection");
		string curAtlasName = "None";
		if (m_pTargetAtlas != nullptr) curAtlasName = string(m_pTargetAtlas->GetKey().begin(), m_pTargetAtlas->GetKey().end());

		if (ImGui::BeginCombo("Select Atlas", curAtlasName.c_str()))
		{
			vector<wstring> vecTexNames;
			AssetMgr::GetInst()->GetAssetNames(ASSET_TYPE::TEXTURE, vecTexNames);
			for (const auto& name : vecTexNames) {
				string strName(name.begin(), name.end());
				if (ImGui::Selectable(strName.c_str())) {
					m_pTargetAtlas = AssetMgr::GetInst()->Find<ATexture>(name);
					m_vecSelectedIndex.clear();
				}
			}
			ImGui::EndCombo();
		}

		DrawSectionTitle("Atlas View");
		ImTextureID texID = (ImTextureID)pActiveAtlas->GetSRV().Get();
		ImVec2 vImgPos = ImGui::GetCursorScreenPos();
		ImGui::Image(texID, ImVec2(imgW, imgH));

		ImDrawList* draw = ImGui::GetWindowDrawList();
		ImGuiIO& io = ImGui::GetIO();

		// ✨ 기존 리스트에서 선택된 스프라이트 영역 표시 (빨간 박스)
		if (IMAGE_STATE::SPRITE == m_curState && m_pTargetSprite.Get())
		{
			Vec2 ltUV = m_pTargetSprite->GetLeftTopUV();
			Vec2 sliceUV = m_pTargetSprite->GetSliceUV();
			Vec2 bgUV = m_pTargetSprite->GetBackgroundUV();
			Vec2 offsetUV = m_pTargetSprite->GetOffsetUV();
			if (bgUV.x <= 0.f || bgUV.y <= 0.f)
			{
				bgUV = sliceUV;
				offsetUV = Vec2(0.f, 0.f);
			}

			Vec2 bgLTUV = ltUV - offsetUV;

			ImVec2 vSavedLT = ImVec2(vImgPos.x + ltUV.x * imgW, vImgPos.y + ltUV.y * imgH);
			ImVec2 vSavedRB = ImVec2(vSavedLT.x + sliceUV.x * imgW, vSavedLT.y + sliceUV.y * imgH);
			ImVec2 vBgLT = ImVec2(vImgPos.x + bgLTUV.x * imgW, vImgPos.y + bgLTUV.y * imgH);
			ImVec2 vBgRB = ImVec2(vBgLT.x + bgUV.x * imgW, vBgLT.y + bgUV.y * imgH);
			
			// 무지개 효과
			
			// 1. Hue 값 계산
			float hue = fmodf((float)ImGui::GetTime() * 1.0f, 1.0f);

			// 2. RGB 값을 담을 임시 변수들
			float r, g, b;

			// 3. HSV를 RGB로 변환 (ImGui 내부 함수 호출)
			ImGui::ColorConvertHSVtoRGB(hue, 1.0f, 1.0f, r, g, b);

			// 4. 변환된 r, g, b를 사용하여 ImVec4 생성 후 U32로 변환
			// 마지막 인자 1.0f는 Alpha(투명도)입니다.

			ImU32 aniColor = ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, 1.0f));
			// 계산된 aniColor를 사용
			draw->AddRect(vSavedLT, vSavedRB, aniColor, 0.0f, 0, 3.0f);
			draw->AddRect(vBgLT, vBgRB, IM_COL32(255, 180, 30, 255), 0.0f, 0, 2.0f);
		}

		ImGui::SetCursorScreenPos(vImgPos);
		ImGui::InvisibleButton("##Canvas", ImVec2(imgW, imgH));

		bool bHovered = ImGui::IsItemHovered();

		if (bHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			m_vecSelectedIndex.clear(); m_bIsDragging = false;
			if (m_pTargetFlipbook) { delete m_pTargetFlipbook; m_pTargetFlipbook = new AFlipbook; }
		}

		if (bHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			m_vStartPos = Vec2(io.MousePos.x, io.MousePos.y); m_bIsDragging = true;
		}

		float dragMinX = min(m_vStartPos.x, io.MousePos.x);
		float dragMaxX = max(m_vStartPos.x, io.MousePos.x);
		float dragMinY = min(m_vStartPos.y, io.MousePos.y);
		float dragMaxY = max(m_vStartPos.y, io.MousePos.y);

		if (m_bIsDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			m_bIsDragging = false;
			if (!io.KeyShift) {
				m_vecSelectedIndex.clear();
				if (m_pTargetFlipbook) { delete m_pTargetFlipbook; m_pTargetFlipbook = new AFlipbook; }
			}
			for (int i = 0; i < m_iRows; ++i) {
				for (int j = 0; j < m_iCols; ++j) {
					Vec2 cellLT = {};
					Vec2 spriteLT = {};
					Vec2 bgSize = {};
					Vec2 offset = {};
					Vec2 spriteSize = {};
					if (!BuildSpriteRectFromCell(i, j, cellLT, spriteLT, bgSize, offset, spriteSize))
						continue;

					ImVec2 p1 = ImVec2(vImgPos.x + cellLT.x, vImgPos.y + cellLT.y);
					ImVec2 p2 = ImVec2(p1.x + bgSize.x, p1.y + bgSize.y);
					float cx = (p1.x + p2.x) * 0.5f; float cy = (p1.y + p2.y) * 0.5f;
					if (cx >= dragMinX && cx <= dragMaxX && cy >= dragMinY && cy <= dragMaxY) {
						int index = i * m_iCols + j;
						if (std::find(m_vecSelectedIndex.begin(), m_vecSelectedIndex.end(), index) == m_vecSelectedIndex.end()) {
							m_vecSelectedIndex.push_back(index);
							Ptr<ASprite> pTemp = new ASprite;
							pTemp->SetAtlas(pActiveAtlas);
							pTemp->SetLeftTopUV(Vec2(spriteLT.x / imgW, spriteLT.y / imgH));
							pTemp->SetSliceUV(Vec2(spriteSize.x / imgW, spriteSize.y / imgH));
							pTemp->SetBackgroundUV(Vec2(bgSize.x / imgW, bgSize.y / imgH));
							pTemp->SetOffsetUV(Vec2(offset.x / imgW, offset.y / imgH));
							m_pTargetFlipbook->AddSprite(pTemp);
						}
					}
				}
			}
		}

		for (int i = 0; i < m_iRows; ++i) {
			for (int j = 0; j < m_iCols; ++j) {
				int curIdx = i * m_iCols + j;
				Vec2 cellLT = {};
				Vec2 spriteLT = {};
				Vec2 bgSize = {};
				Vec2 offset = {};
				Vec2 spriteSize = {};
				if (!BuildSpriteRectFromCell(i, j, cellLT, spriteLT, bgSize, offset, spriteSize))
					continue;

				ImVec2 p1 = ImVec2(floor(vImgPos.x + cellLT.x), floor(vImgPos.y + cellLT.y));
				ImVec2 p2 = ImVec2(p1.x + floor(bgSize.x), p1.y + floor(bgSize.y));
				ImVec2 sp1 = ImVec2(floor(vImgPos.x + spriteLT.x), floor(vImgPos.y + spriteLT.y));
				ImVec2 sp2 = ImVec2(sp1.x + floor(spriteSize.x), sp1.y + floor(spriteSize.y));

				draw->AddRect(p1, p2, IM_COL32(200, 200, 200, 100));
				draw->AddRect(sp1, sp2, IM_COL32(255, 210, 40, 220), 0.f, 0, 2.0f);
				if (std::find(m_vecSelectedIndex.begin(), m_vecSelectedIndex.end(), curIdx) != m_vecSelectedIndex.end()) {
					draw->AddRectFilled(p1, p2, IM_COL32(0, 255, 255, 80));
					draw->AddRect(p1, p2, IM_COL32(0, 255, 255, 255), 0.f, 0, 2.0f);
					draw->AddRect(sp1, sp2, IM_COL32(255, 210, 40, 255), 0.f, 0, 3.0f);
				}
			}
		}
		draw->AddRect(
			ImVec2(vImgPos.x + m_vRegionStart.x, vImgPos.y + m_vRegionStart.y),
			ImVec2(vImgPos.x + m_vRegionEnd.x, vImgPos.y + m_vRegionEnd.y),
			IM_COL32(80, 255, 120, 220), 0.f, 0, 2.0f);
		if (m_bIsDragging) draw->AddRect(ImVec2(dragMinX, dragMinY), ImVec2(dragMaxX, dragMaxY), IM_COL32(255, 255, 0, 255), 0.f, 0, 2.0f);
	}
	ImGui::EndChild();
	ImGui::Columns(1);

	// =========================
	// WINDOWS (SPRITE / FLIPBOOK LIST)
	// =========================
	if (bOpenSpriteList) {
		ImGui::SetNextWindowSize(ImVec2(320, 420), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Sprite List Window", &bOpenSpriteList)) {
			if (ImGui::BeginChild("SpriteScrollRegion", ImVec2(0, 0), true)) {
				vector<wstring> vecNames; AssetMgr::GetInst()->GetAssetNames(ASSET_TYPE::SPRITE, vecNames);
				for (const auto& name : vecNames) {
					string strName(name.begin(), name.end());
					if (ImGui::Selectable(strName.c_str())) {
						m_pTargetSprite = AssetMgr::GetInst()->Find<ASprite>(name);
						if (m_pTargetSprite != nullptr) {
							m_curState = IMAGE_STATE::SPRITE;
							m_pTargetAtlas = m_pTargetSprite->GetAtlas();
						}
						bOpenSpriteList = false;
					}
				}
			} ImGui::EndChild();
		} ImGui::End();
	}

	if (bOpenFlipbookList) {
		ImGui::SetNextWindowSize(ImVec2(320, 420), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Flipbook List Window", &bOpenFlipbookList)) {
			if (ImGui::BeginChild("FlipbookScrollRegion", ImVec2(0, 0), true)) {
				vector<wstring> vecFlipNames; AssetMgr::GetInst()->GetAssetNames(ASSET_TYPE::FLIPBOOK, vecFlipNames);
				for (const auto& name : vecFlipNames) {
					string strName(name.begin(), name.end());
					if (ImGui::Selectable(strName.c_str())) {
						m_pLoadedFlipbook = AssetMgr::GetInst()->Find<AFlipbook>(name);
						if (m_pLoadedFlipbook != nullptr && m_pLoadedFlipbook->GetSpriteCount() > 0) {
							m_curState = IMAGE_STATE::ATLAS; // 플립북 모드에서는 아틀라스 뷰 유지
							Ptr<ASprite> pFirstSprite = m_pLoadedFlipbook->GetSprite(0);
							if (pFirstSprite != nullptr) m_pTargetAtlas = pFirstSprite->GetAtlas();
						}
						m_iCurFrame = 0; m_fAccTime = 0.f; m_bPreviewPlaying = true;
						bOpenFlipbookList = false;
					}
				}
			} ImGui::EndChild();
		} ImGui::End();
	}

	// =========================
	// PREVIEW WINDOW
	// =========================
	AFlipbook* pPreviewFlipbook = (m_pTargetFlipbook && m_pTargetFlipbook->GetSpriteCount() > 0) ? m_pTargetFlipbook : m_pLoadedFlipbook.Get();
	if (bShowPreview && pPreviewFlipbook && pPreviewFlipbook->GetSpriteCount() > 0) {
		int spriteCount = pPreviewFlipbook->GetSpriteCount();
		if (m_iCurFrame >= spriteCount) m_iCurFrame = 0;
		ImGui::SetNextWindowSize(ImVec2(380, 500), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("FlipBook Preview", &bShowPreview)) {
			DrawSectionTitle("Controls");
			PushBlueButtonStyle();
			if (ImGui::Button("Play", ImVec2(90.f, 32.f))) m_bPreviewPlaying = true; ImGui::SameLine();
			if (ImGui::Button("Pause", ImVec2(90.f, 32.f))) m_bPreviewPlaying = false; ImGui::SameLine();
			if (ImGui::Button("Stop", ImVec2(90.f, 32.f))) { m_bPreviewPlaying = false; m_iCurFrame = 0; m_fAccTime = 0.f; }
			PopButtonStyle();
			ImGui::Checkbox("Loop", &m_bPreviewLoop);
			ImGui::SliderFloat("FPS", &m_fPreviewFPS, 1.f, 60.f);
			ImGui::SliderInt("Frame", &m_iCurFrame, 0, spriteCount - 1);
			if (m_bPreviewPlaying) {
				m_fAccTime += ImGui::GetIO().DeltaTime;
				float fFrameTime = 1.0f / m_fPreviewFPS;
				if (m_fAccTime >= fFrameTime) {
					m_fAccTime -= fFrameTime; ++m_iCurFrame;
					if (m_iCurFrame >= spriteCount) {
						if (m_bPreviewLoop) m_iCurFrame = 0;
						else { m_iCurFrame = spriteCount - 1; m_bPreviewPlaying = false; }
					}
				}
			}
			Ptr<ASprite> pCurSprite = pPreviewFlipbook->GetSprite(m_iCurFrame);
			if (pCurSprite != nullptr) {
				Ptr<ATexture> pAtlas = pCurSprite->GetAtlas();
				if (pAtlas != nullptr) {
					Vec2 ltUV = pCurSprite->GetLeftTopUV();
					Vec2 sliceUV = pCurSprite->GetSliceUV();
					Vec2 bgUV = pCurSprite->GetBackgroundUV();
					Vec2 offsetUV = pCurSprite->GetOffsetUV();
					ImTextureID previewTexID = (ImTextureID)pAtlas->GetSRV().Get();

					if (bgUV.x <= 0.f || bgUV.y <= 0.f)
					{
						bgUV = sliceUV;
						offsetUV = Vec2(0.f, 0.f);
					}

					auto clamp01 = [](float _v)->float { return max(0.f, min(1.f, _v)); };
					auto clamp01Vec2 = [&](Vec2& _v)
					{
						_v.x = clamp01(_v.x);
						_v.y = clamp01(_v.y);
					};

					Vec2 bgLTUV = ltUV - offsetUV;
					clamp01Vec2(ltUV);
					clamp01Vec2(bgLTUV);

					float bgAvailW = max(0.001f, 1.f - bgLTUV.x);
					float bgAvailH = max(0.001f, 1.f - bgLTUV.y);
					bgUV.x = max(0.f, min(bgUV.x, bgAvailW));
					bgUV.y = max(0.f, min(bgUV.y, bgAvailH));
					bgUV.x = max(1.f / pAtlas->GetWidth(), bgUV.x);
					bgUV.y = max(1.f / pAtlas->GetHeight(), bgUV.y);

					float imgW = (float)pAtlas->GetWidth();
					float imgH = (float)pAtlas->GetHeight();
					float spritePxX = sliceUV.x * imgW;
					float spritePxY = sliceUV.y * imgH;
					float bgPxX = bgUV.x * imgW;
					float bgPxY = bgUV.y * imgH;
					float offPxX = offsetUV.x * imgW;
					float offPxY = offsetUV.y * imgH;

					DrawSectionTitle("Frame Preview");
					float previewSize = 280.f;
					float windowWidth = ImGui::GetContentRegionAvail().x;
					float offsetX = (windowWidth - previewSize) * 0.5f;
					if (offsetX > 0.f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

					float scale = previewSize / max(bgPxX, bgPxY);
					ImVec2 pPreview = ImGui::GetCursorScreenPos();
					ImVec2 bgDrawSz = ImVec2(bgPxX * scale, bgPxY * scale);
					ImVec2 spriteDrawSz = ImVec2(spritePxX * scale, spritePxY * scale);
					float bgOffsetX = (previewSize - bgDrawSz.x) * 0.5f;
					float bgOffsetY = (previewSize - bgDrawSz.y) * 0.5f;

					ImVec2 bgPosMin = ImVec2(pPreview.x + bgOffsetX, pPreview.y + bgOffsetY);
					ImVec2 bgPosMax = ImVec2(bgPosMin.x + bgDrawSz.x, bgPosMin.y + bgDrawSz.y);
					ImVec2 spritePosMin = ImVec2(bgPosMin.x + offPxX * scale, bgPosMin.y + offPxY * scale);
					ImVec2 spritePosMax = ImVec2(spritePosMin.x + spriteDrawSz.x, spritePosMin.y + spriteDrawSz.y);

					ImDrawList* pDrawList = ImGui::GetWindowDrawList();
					pDrawList->AddImage(previewTexID, bgPosMin, bgPosMax,
						ImVec2(bgLTUV.x, bgLTUV.y),
						ImVec2(bgLTUV.x + bgUV.x, bgLTUV.y + bgUV.y));
					pDrawList->AddImage(previewTexID, spritePosMin, spritePosMax,
						ImVec2(ltUV.x, ltUV.y),
						ImVec2(ltUV.x + sliceUV.x, ltUV.y + sliceUV.y));

					ImGui::Dummy(ImVec2(previewSize, previewSize));
				}
			}
		} ImGui::End();
	}
}

void ImageEditor::LoadAllSprites()
{
	wstring strPath = wstring(CONTENT_PATH) + L"Sprite\\";
	if (!std::filesystem::exists(strPath)) std::filesystem::create_directories(strPath);

	for (const auto& entry : std::filesystem::directory_iterator(strPath)) {
		if (entry.is_regular_file() && entry.path().extension() == L".sprite") {
			wstring fileName = entry.path().stem().wstring();
			AssetMgr::GetInst()->Load<ASprite>(fileName, L"Sprite\\" + fileName + L".sprite");
		}
	}
}

void ImageEditor::LoadAllFlipbooks()
{
	wstring strPath = wstring(CONTENT_PATH) + L"Flipbook\\";
	if (!std::filesystem::exists(strPath)) std::filesystem::create_directories(strPath);
	for (const auto& entry : std::filesystem::directory_iterator(strPath)) {
		if (entry.is_regular_file() && entry.path().extension() == L".flip") {
			wstring fileName = entry.path().stem().wstring();
			AssetMgr::GetInst()->Load<AFlipbook>(fileName, L"Flipbook\\" + fileName + L".flip");
		}
	}
}
