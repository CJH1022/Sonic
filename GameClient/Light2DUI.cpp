#include "pch.h"
#include "Light2DUI.h"

#include "CLight2D.h"

Light2DUI::Light2DUI()
	: ComponentUI(COMPONENT_TYPE::LIGHT2D, "Light2DUI")
{
}

Light2DUI::~Light2DUI()
{
}

void Light2DUI::Tick_UI()
{
	OutputTitle("Light2D");

	Ptr<CLight2D> pLight2D = GetTarget()->Light2D();

	Vec3 vColor = pLight2D->GetLightColor();

	ImGui::Text("Light Color");
	ImGui::SameLine(100);
	if (ImGui::ColorEdit3("##LightColor", vColor))
	{
		pLight2D->SetLightColor(vColor);
	}
}