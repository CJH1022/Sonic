#pragma once

#include "EditorUI.h"
#include "GameObject.h"

class ComponentUI
	: public EditorUI
{
private:
	Ptr<GameObject>			m_Target;
	const COMPONENT_TYPE	m_ComType;

public:
	void SetTarget(Ptr<GameObject> _Object);	
	Ptr<GameObject> GetTarget() { return m_Target; }

protected:
	void OutputTitle(const string& _Title);

public:
	ComponentUI(COMPONENT_TYPE _Type, const string& _Name);
	virtual ~ComponentUI();
};

