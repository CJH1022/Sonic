#pragma once
#include "EditorUI.h"

struct TreeNode
    : public Entity
{
public:
    string                  Str;
    string                  Key;
    DWORD_PTR               Data;

    TreeNode*               Parent;
    vector<Ptr<TreeNode>>   vecChildNode;

    bool                    Framed;

    class TreeUI*           m_Owner; // 노드를 소유하고 있는 TreeUI

private:
    void ClickCheck();
    void DragCheck();
    void DropCheck();

public:
    void AddChildNode(Ptr<TreeNode> _Node)
    {
        vecChildNode.push_back(_Node);
        _Node->Parent = this;
    }

    void SetFramed(bool _Frame) { Framed = _Frame; }

public:
    void Tick();

public:
    TreeNode();
    virtual ~TreeNode();
};


class TreeUI :
    public EditorUI
{
private:
    vector<Ptr<TreeNode>>   m_vecNode;
    Ptr<TreeNode>           m_Selected; // 트리 소속 노드들 중에서 선택된 노드
    Ptr<TreeNode>           m_DragNode; // 트리 소속 노드들 중에서 드래그 중인 노드
    Ptr<TreeNode>           m_DropNode; // 트리 소속 노드들 중에서 드랍 당한 노드

    EditorUI*               m_Inst;
    DELEGATE_1              m_MemFunc;

    string                  m_DropKey;

    EditorUI*               m_DDInst;
    DELEGATE_2              m_DDMemFunc;


public:
    virtual void Tick_UI() override;

public:
    void AddDynamicSelect(EditorUI* _Inst, DELEGATE_1 _MemFunc) { m_Inst = _Inst; m_MemFunc = _MemFunc; }
    void AddDynamicDragDrop(EditorUI* _Inst, DELEGATE_2 _MemFunc) { m_DDInst = _Inst; m_DDMemFunc = _MemFunc; }

    void Clear() { m_vecNode.clear(); }
    Ptr<TreeNode> AddItem(Ptr<TreeNode> _ParentNode,  string _String, DWORD_PTR _Data = 0);

    void RegisterSelected(Ptr<TreeNode> _Node);
    void RegisterDragged(Ptr<TreeNode> _Node) { m_DragNode = _Node; }
    void RegisterDropped(Ptr<TreeNode> _Node) { m_DropNode = _Node; }
    
    Ptr<TreeNode> GetSelected() { return m_Selected; }

    void SetDropKey(const string& _DropKey) { m_DropKey = _DropKey; }
    const string& GetDropKey() { return m_DropKey; }

public:
    TreeUI();
    virtual ~TreeUI();
};