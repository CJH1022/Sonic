#pragma once
#include "Entity.h"

#include "components.h"

#define GET_COMPONENT(COM_NAME, COM_TYPE)     Ptr<C##COM_NAME> COM_NAME() { return (C##COM_NAME*)m_Com[(UINT)COMPONENT_TYPE::COM_TYPE].Get(); }


class GameObject :
    public Entity
{
private:
    Ptr<Component>          m_Com[(UINT)COMPONENT_TYPE::END];
    Ptr<CRenderComponent>   m_RenderCom;
    vector<Ptr<CScript>>    m_vecScripts;

    GameObject*             m_Parent;
    vector<Ptr<GameObject>> m_vecChild;

    // GameObject 본인이 속한 Layer Index, 
    // -1 인 경우, 어떤 레이어에도 속하지 않는다 == 레벨안에 있지 않은 오브젝트
    int                     m_LayerIdx; 
    bool                    m_Dead;

public:
    // 레벨이 처음 시작될때 호출되는 함수
    void Begin();

    // 매 프레임마다 할 일
    void Tick();

    // 매 프레임마다 Tick 이후에 뒷 수습작업 수행
    void FinalTick();
    void FinalTick_Editor();

    // 자신을 타겟에 그림
    void Render();

public:
    void AddComponent(Ptr<Component> _Com);
    void RemoveScript(CScript* _Script);
    Ptr<Component> GetComponent(COMPONENT_TYPE _Type) { return m_Com[(UINT)_Type]; }
    const vector<Ptr<CScript>>& GetScripts() const { return m_vecScripts; }
    template<typename T>
    Ptr<T> GetScript();
    bool IsDescendantOf(Ptr<GameObject> _Object) const;

    void AddChild(Ptr<GameObject> _Child);
    void DisconnectWithParent();
    void RegisterAsParent();
    void DeregisterAsParent();


    Ptr<GameObject> GetParent() { return m_Parent; }
    Ptr<GameObject> GetChild(int _idx) { return m_vecChild[_idx]; }
    const vector<Ptr<GameObject>>& GetChild() { return m_vecChild; }

    bool IsDead() { return m_Dead; }
    void Destroy();

    // 특정 컴포넌트를 다운캐스팅해서 바로 리턴
    GET_COMPONENT(Light2D, LIGHT2D);
    GET_COMPONENT(TileRender, TILE_RENDER);
    GET_COMPONENT(FlipbookRender, FLIPBOOK_RENDER);
    GET_COMPONENT(SpriteRender, SPRITE_RENDER);
    GET_COMPONENT(Transform, TRANSFORM);
    GET_COMPONENT(MeshRender, MESHRENDER);
    GET_COMPONENT(BillboardRender, BILLBOARD_RENDER);
    GET_COMPONENT(Camera, CAMERA);
    GET_COMPONENT(Collider2D, COLLIDER2D);

    Ptr<CRenderComponent> GetRenderCom() { return m_RenderCom; }

public:
    void SaveToLevelFile(FILE* _File);
    void LoadFromLevelFile(FILE* _File);

private:
    void RegisterLayer();

public:
    CLONE(GameObject);

public:
    GameObject();
    GameObject(const GameObject& _Origin);
    virtual ~GameObject();

    friend class Layer;
    friend class TaskMgr;
};


bool IsValid(Ptr<GameObject>& _Object);

template<typename T>
inline Ptr<T> GameObject::GetScript()
{
    for (size_t i = 0; i < m_vecScripts.size(); ++i)
    {
        T* pScript = dynamic_cast<T*>(m_vecScripts[i].Get());
        if (nullptr == pScript)
            continue;

        return pScript;
    }

    return nullptr;
}
