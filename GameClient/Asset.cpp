#include "pch.h"
#include "Asset.h"


Asset::Asset(ASSET_TYPE _Type)
	: m_Type(_Type)
{
}

Asset::Asset(const Asset& _Origin)
	: Entity(_Origin)
	, m_Key(_Origin.m_Key)
	, m_RelativePath()
	, m_Type(_Origin.m_Type)
{
}

Asset::~Asset()
{
}
