#pragma once
#include "Entity.h"

class Asset :
    public Entity
{
private:
    wstring                 m_Key;          // 에셋을 찾을때 사용할 이름
    wstring                 m_RelativePath; // 상대 경로(실제 Content 폴더에 저장되어있는 위치)

    // RTTI : Runtime Type Identification, 
    // 런타임 도중에, 객체의 실제 타입을 알아내는 기술
    const ASSET_TYPE        m_Type;         // 에셋 타입
    
public:
    ASSET_TYPE GetType() { return m_Type; }

public:
    const wstring& GetKey() { return m_Key; }
    const wstring& GetRelativePath() { return m_RelativePath; }

private:
    void SetKey(const wstring& _Key) { m_Key = _Key; }
    void SetRelativePath(const wstring& _Path) { m_RelativePath = _Path; }


public:
    virtual int Load(const wstring& _FilePath) { return S_OK; };
    virtual int Save(const wstring& _FilePath) { return S_OK; };

public:
    // 기본생성자 제거, 자신이 어떤 타입의 Asset 인지 입력으로 받는 버전의 생성자만 남겨둠
    Asset(ASSET_TYPE _Type);
    Asset(const Asset& _Origin);
    virtual ~Asset();

    friend class AssetMgr;
};

