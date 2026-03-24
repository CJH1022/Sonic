#pragma once

#include "Asset.h"
#include "GameObject.h"

class APrefab
	: public Asset
{
private:
	Ptr<GameObject>	m_ProtoObj;

public:
	void SetObject(Ptr<GameObject> _Object) { m_ProtoObj = _Object; }
	GameObject* Instantiate();

public:
	virtual int Save(const wstring& _FilePath) override;
	virtual int Load(const wstring& _FilePath) override;
	CLONE(APrefab);

public:
	APrefab();
	virtual ~APrefab();
};
