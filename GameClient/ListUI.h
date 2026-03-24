#pragma once

#include "EditorUI.h"

class ListUI :
    public EditorUI
{
private:
    vector<string>      m_vecList;
    int                 m_SelectedIdx;
    string              m_SelectedString;

    EditorUI*           m_Inst;
    DELEGATE_1          m_MemFunc;


public:
    void AddString(const string& _Str) { m_vecList.push_back(_Str); }
    void AddString(const wstring& _WStr) { m_vecList.push_back(string(_WStr.begin(), _WStr.end())); }
    void AddString(const vector<string>& _vecStr) { m_vecList.insert(m_vecList.end(), _vecStr.begin(), _vecStr.end()); }
    void AddString(const vector<wstring>& _vecWStr)
    {
        for (size_t i = 0; i < _vecWStr.size(); ++i)
        {
            AddString(_vecWStr[i]);
        }
    }

    void AddDelegate(EditorUI* _Inst, DELEGATE_1 _MemFunc) { m_Inst = _Inst; m_MemFunc = _MemFunc; }

    const string& GetSelectedString() { return m_SelectedString; }

public:
    virtual void Tick_UI() override;
    virtual void Activate() override;
    virtual void Deactivate() override;


public:
    ListUI();
    virtual ~ListUI();
};

