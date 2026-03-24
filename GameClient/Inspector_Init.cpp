#include "pch.h"
#include "Inspector.h"

#include "TransformUI.h"
#include "CameraUI.h"
#include "Collider2DUI.h"
#include "Light2DUI.h"
#include "MeshRenderUI.h"
#include "SpriteRenderUI.h"
#include "FlipbookRenderUI.h"
#include "TileRenderUI.h"

#include "MeshUI.h"
#include "MaterialUI.h"
#include "TextureUI.h"
#include "SoundUI.h"
#include "GraphicShaderUI.h"
#include "ComputeShaderUI.h"
#include "LevelUI.h"
#include "SpriteUI.h"
#include "FlipbookUI.h"
#include "TileMapUI.h"
#include "PrefabUI.h"

#define ADD_COMPONENT_UI(ComponentType, type, Size) m_arrComUI[(UINT)ComponentType] = new type;\
													m_arrComUI[(UINT)ComponentType]->SetSizeAsChild(Size);\
													AddChildUI(m_arrComUI[(UINT)ComponentType].Get());

#define ADD_ASSET_UI(AssetType, type) m_arrAssetUI[(UINT)AssetType] = new type;\
									  AddChildUI(m_arrAssetUI[(UINT)AssetType].Get());



void Inspector::CreateChildUI()
{
	ADD_COMPONENT_UI(COMPONENT_TYPE::TRANSFORM,			TransformUI,		Vec2(0.f, 200.f));
	ADD_COMPONENT_UI(COMPONENT_TYPE::COLLIDER2D,		Collider2DUI,		Vec2(0.f, 200.f));
	ADD_COMPONENT_UI(COMPONENT_TYPE::CAMERA,			CameraUI,			Vec2(0.f, 300.f));
	ADD_COMPONENT_UI(COMPONENT_TYPE::LIGHT2D,			Light2DUI,			Vec2(0.f, 200.f));
	ADD_COMPONENT_UI(COMPONENT_TYPE::MESHRENDER,		MeshRenderUI,		Vec2(0.f, 200.f));
	ADD_COMPONENT_UI(COMPONENT_TYPE::SPRITE_RENDER,		SpriteRenderUI,		Vec2(0.f, 200.f));
	ADD_COMPONENT_UI(COMPONENT_TYPE::FLIPBOOK_RENDER,	FlipbookRenderUI,	Vec2(0.f, 200.f));
	ADD_COMPONENT_UI(COMPONENT_TYPE::TILE_RENDER,		TileRenderUI,		Vec2(0.f, 200.f));

	ADD_ASSET_UI(ASSET_TYPE::MESH, MeshUI);
	ADD_ASSET_UI(ASSET_TYPE::MATERIAL, MaterialUI);
	ADD_ASSET_UI(ASSET_TYPE::TEXTURE, TextureUI);
	ADD_ASSET_UI(ASSET_TYPE::SOUND, SoundUI);
	ADD_ASSET_UI(ASSET_TYPE::GRAPHICSHADER, GraphicShaderUI);
	ADD_ASSET_UI(ASSET_TYPE::COMPUTESHADER, ComputeShaderUI);
	ADD_ASSET_UI(ASSET_TYPE::LEVEL, LevelUI);
	ADD_ASSET_UI(ASSET_TYPE::SPRITE, SpriteUI);
	ADD_ASSET_UI(ASSET_TYPE::FLIPBOOK, FlipbookUI);
	ADD_ASSET_UI(ASSET_TYPE::TILEMAP, TileMapUI);
	ADD_ASSET_UI(ASSET_TYPE::PREFAB, PrefabUI);
}