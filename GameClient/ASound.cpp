#include "pch.h"
#include "ASound.h"

#include "Engine.h"
#include "AssetMgr.h"

FMOD_RESULT CHANNEL_CALLBACK(FMOD_CHANNELCONTROL* channelcontrol, FMOD_CHANNELCONTROL_TYPE controltype
	, FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype
	, void* commanddata1, void* commanddata2);

ASound::ASound(bool _EngineRes)
	: Asset(ASSET_TYPE::SOUND)
	, m_Sound(nullptr)
{
}

ASound::~ASound()
{
	if (nullptr != m_Sound)
	{
		if (FMOD_SYSTEM != nullptr)
		{
			Stop();
			m_Sound->release();
		}

		m_listChannel.clear();
		m_Sound = nullptr;
	}
}

int ASound::Play(int _iRoopCount, float _fVolume, bool _bOverlap)
{
    if (m_Sound == nullptr || FMOD_SYSTEM == nullptr)
        return E_FAIL;

	if (_iRoopCount <= -1)
	{
		assert(nullptr);
	}

	// 중첩재생 X + 이미 다른채널에서 재생중 ==> 재생 불가
	if (!_bOverlap && !m_listChannel.empty())
	{
		return E_FAIL;
	}

	_iRoopCount -= 1;

	FMOD::Channel* pChannel = nullptr;
	FMOD_SYSTEM->playSound(m_Sound, nullptr, false, &pChannel);

	// 재생을 했는데, 재생중인 채널이 없다 --> 실패
	if (nullptr == pChannel)
		return E_FAIL;

	pChannel->setVolume(_fVolume);

	pChannel->setCallback(&CHANNEL_CALLBACK);
	pChannel->setUserData(this);

	pChannel->setMode(FMOD_LOOP_NORMAL);
	pChannel->setLoopCount(_iRoopCount);

	// 어떤 채널에서 Sound 가 재생중인지 기록
	m_listChannel.push_back(pChannel);

	int iIdx = -1;
	pChannel->getIndex(&iIdx);

	return iIdx;
}

void ASound::Stop()
{
	list<FMOD::Channel*>::iterator iter;

	while (!m_listChannel.empty())
	{
		iter = m_listChannel.begin();
		(*iter)->stop();
	}
}

bool ASound::IsPlaying() const
{
    return !m_listChannel.empty();
}

void ASound::SetVolume(float _f, int _iChannelIdx)
{
	list<FMOD::Channel*>::iterator iter = m_listChannel.begin();

	int iIdx = -1;
	for (; iter != m_listChannel.end(); ++iter)
	{
		(*iter)->getIndex(&iIdx);
		if (_iChannelIdx == iIdx)
		{
			(*iter)->setVolume(_f);
			return;
		}
	}
}

void ASound::RemoveChannel(FMOD::Channel* _pTargetChannel)
{
	list<FMOD::Channel*>::iterator iter = m_listChannel.begin();
	for (; iter != m_listChannel.end(); ++iter)
	{
		if (*iter == _pTargetChannel)
		{
			m_listChannel.erase(iter);
			return;
		}
	}
}

int ASound::Load(const wstring& _FilePath)
{
    if (FMOD_SYSTEM == nullptr)
        return E_FAIL;

    const int utf8Len = WideCharToMultiByte(CP_UTF8, 0, _FilePath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8Len <= 0)
        return E_FAIL;

    string path;
    path.resize((size_t)utf8Len - 1);
    WideCharToMultiByte(CP_UTF8, 0, _FilePath.c_str(), -1, path.data(), utf8Len, nullptr, nullptr);

	FMOD_RESULT result = FMOD_SYSTEM->createSound(path.c_str(), FMOD_DEFAULT, nullptr, &m_Sound);
	if (FMOD_OK != result)
	{
		return E_FAIL;
	}

	return S_OK;
}

// =========
// Call Back
// =========
FMOD_RESULT CHANNEL_CALLBACK(FMOD_CHANNELCONTROL* channelcontrol, FMOD_CHANNELCONTROL_TYPE controltype
	, FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype
	, void* commanddata1, void* commanddata2)
{
	FMOD::Channel* cppchannel = (FMOD::Channel*)channelcontrol;
	ASound* pSound = nullptr;

	switch (controltype)
	{
	// 사운즈 재생 종료시 발생하는 이벤트
	case FMOD_CHANNELCONTROL_CALLBACK_END:
	{
		cppchannel->getUserData((void**)&pSound);
		pSound->RemoveChannel(cppchannel);
	}
	break;
	}

	return FMOD_OK;
}
