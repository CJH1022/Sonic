#pragma once
#include "Entity.h"

#include "GameObject.h"

class Layer :
    public Entity
{
private:
    vector<Ptr<GameObject>>     m_vecParents;   // Layer 에 소속된 최상위 부모타입 오브젝트들
    vector<Ptr<GameObject>>     m_vecAllObjects;// Layer 에 소속된 모든(부모, 자식타입) 오브젝트
    int                         m_LayerIdx;     // Layer 본인의 인덱스(0 ~ 31)

public:
    void AddObject(Ptr<GameObject> _Object);
    void RegisterObject(Ptr<GameObject> _Object) { m_vecAllObjects.push_back(_Object); }
    void DeregisterObject() {  m_vecAllObjects.clear(); }
    void DeregisterAsParent(Ptr<GameObject> _Object);

    const vector<Ptr<GameObject>>& GetParentObjects() { return m_vecParents; }
    const vector<Ptr<GameObject>>& GetAllObjects() { return m_vecAllObjects; }

public:
    void Begin();
    void Tick();
    void FinalTick();
    void Render();

public:
    Layer();
    Layer(const Layer& _Origin);
    virtual ~Layer();
    friend class ALevel;
};

