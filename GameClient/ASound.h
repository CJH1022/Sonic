#pragma once
#include "Asset.h"

class ASound :
    public Asset
{
private:
    FMOD::Sound*            m_Sound;        // Sound 객체
    list<FMOD::Channel*>	m_listChannel;  // Sound 가 재생되고 있는 채널 리스트

public:
    // _iRoopCount : 0 (반복재생),  _fVolume : 0 ~ 1(Volume), _bOverlap : 같은 사운드를 중첩해서 켤 수 있는지
    int Play(int _iRoopCount, float _fVolume, bool _bOverlap);
    void RemoveChannel(FMOD::Channel* _pTargetChannel);
    void Stop();
    bool IsPlaying() const;

    // 0 ~ 1
    void SetVolume(float _f, int _iChannelIdx);

public:
    virtual int Load(const wstring& _FilePath) override;
    virtual int Save(const wstring& _FilePath) override { return S_OK; }

public:
    CLONE(ASound);
    ASound(bool _EngineRes = false);
    ~ASound();

    friend class CAssetMgr;
};

