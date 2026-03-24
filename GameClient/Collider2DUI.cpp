#include "pch.h"
#include "Collider2DUI.h"

Collider2DUI::Collider2DUI()
	: ComponentUI(COMPONENT_TYPE::COLLIDER2D, "Collider2D")
{
}

Collider2DUI::~Collider2DUI()
{
}

void Collider2DUI::Tick_UI()
{
	OutputTitle("Collider2D");


	ImGui::Text("Offset");
	ImGui::SameLine(100);
	Vec2 Offset = GetTarget()->Collider2D()->GetOffset();
	if (ImGui::DragFloat2("##Offset", Offset, 0.01f))
		GetTarget()->Collider2D()->SetOffset(Offset);

	ImGui::Text("Scale");
	ImGui::SameLine(100);
	Vec2 Scale = GetTarget()->Collider2D()->GetScale();
	if (ImGui::DragFloat2("##Scale", Scale, 0.01f))
		GetTarget()->Collider2D()->SetScale(Scale);
}