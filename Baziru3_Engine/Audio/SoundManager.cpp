#include "SoundManager.h"
#include <xaudio2.h>
#include <wrl.h>
#include <unordered_map>
#include <fstream>
#include <cstring>
#include <cassert>

#pragma comment(lib, "xaudio2.lib")

using Microsoft::WRL::ComPtr;

// define static instance
SoundManager* SoundManager::instance = nullptr;

// GetInstance implementation
SoundManager* SoundManager::GetInstance()
{
    if (instance == nullptr) {
        instance = new SoundManager();
        instance->Initialize();
    }
    return instance;
}

bool SoundManager::Initialize() {
    // XAudio2の初期化
    HRESULT result = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(result)) {
        assert(0 && "XAudio2Create failed");
    }

    result = xAudio2_->CreateMasteringVoice(&masterVoice_);
    if (FAILED(result)) {
        assert(0 && "CreateMasteringVoice failed");
    }
    return true;
}

void SoundManager::Finalize() {
    SoundStopBGM();
    StopAllVoices();
    if (masterVoice_) {
        masterVoice_->DestroyVoice();
        masterVoice_ = nullptr;
    }
    xAudio2_.Reset();
}

SoundData SoundManager::SoundLoadWave(const char* filename) {
    // ファイルを開く
    std::ifstream file(filename, std::ios_base::binary);
    assert(file.is_open()); // 開けない → パスが不正 or カレントディレクトリがずれ

    // RIFFヘッダ
    RiffHeader riff{};
    file.read(reinterpret_cast<char*>(&riff), sizeof(riff));
    if (strncmp(riff.chunk.id, "RIFF", 4) != 0) { assert(0); }
    if (strncmp(riff.type, "WAVE", 4) != 0) { assert(0); }

    // 任意順に現れるチャンクを走査して fmt と data を見つける
    bool fmtFound = false;
    bool dataFound = false;

    ChunkHeader ch{};

    // 返却データ
    SoundData soundData{};

    // 走査ループ
    while (file.read(reinterpret_cast<char*>(&ch), sizeof(ch))) {
        if (strncmp(ch.id, "fmt ", 4) == 0) {
            // 可変長fmt: ch.size だけ読み取る
            soundData.fmtSize = ch.size;
            soundData.fmtRaw = new BYTE[ch.size];
            file.read(reinterpret_cast<char*>(soundData.fmtRaw), ch.size);

            // 先頭はWAVEFORMATEX互換
            const size_t copySize = std::min<size_t>(ch.size, sizeof(WAVEFORMATEX));
            std::memset(&soundData.wfex, 0, sizeof(WAVEFORMATEX));
            std::memcpy(&soundData.wfex, soundData.fmtRaw, copySize);

            fmtFound = true;
        }
        else if (strncmp(ch.id, "data", 4) == 0) {
            // 波形データ
            char* pBuffer = new char[ch.size];
            file.read(pBuffer, ch.size);
            soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
            soundData.bufferSize = ch.size;
            dataFound = true;
        }
        else {
            // 不要チャンクをスキップ（JUNK/LIST/fact/bext 等）
            file.seekg(ch.size, std::ios_base::cur);
        }

        if (fmtFound && dataFound) break;
    }

    // 必須チャンクが無ければ失敗
    if (!fmtFound || !dataFound) {
        assert(0 && "Invalid WAV: missing fmt or data chunk");
    }

    return soundData;
}

void SoundManager::SoundUnload(SoundData* soundData) {
    // 先にすべての再生を止めてから解放（ボイスが参照している可能性があるため）
    StopAllVoices();

    // バッファのメモリを解放（配列解放）
    delete[] soundData->pBuffer; soundData->pBuffer = nullptr;
    soundData->bufferSize = 0;
    // 追加: フォーマットの生データも解放
    delete[] soundData->fmtRaw; soundData->fmtRaw = nullptr; soundData->fmtSize = 0;
    soundData->wfex = {};
}

void SoundManager::SoundPlayWave(const SoundData& soundData, bool loop)
{
    // 既定SE音量を使って再生
    SoundPlayWave(soundData, loop, seVolume_);
}

void SoundManager::SoundPlayWave(const SoundData& soundData, bool loop, float volume)
{
    IXAudio2SourceVoice* pSourceVoice = nullptr;
    // 拡張fmtがある場合はそれを優先
    const WAVEFORMATEX* pWf = soundData.fmtRaw ? reinterpret_cast<const WAVEFORMATEX*>(soundData.fmtRaw)
                                               : &soundData.wfex;
    HRESULT hr = xAudio2_->CreateSourceVoice(&pSourceVoice, pWf);
    assert(SUCCEEDED(hr));

    XAUDIO2_BUFFER buf{};
    buf.pAudioData = soundData.pBuffer;
    buf.AudioBytes = soundData.bufferSize;
    buf.Flags = XAUDIO2_END_OF_STREAM;
    buf.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0; // ループ再生指定

    hr = pSourceVoice->SubmitSourceBuffer(&buf);
    assert(SUCCEEDED(hr));

    // 音量設定（0.0f～1.0f）
    float clamped = (volume < 0.0f) ? 0.0f : (volume > 1.0f ? 1.0f : volume);
    pSourceVoice->SetVolume(clamped);

    hr = pSourceVoice->Start();
    assert(SUCCEEDED(hr));

    // トラッキング
    voices_.push_back(pSourceVoice);
}

// BGM dedicated API implementations
void SoundManager::SoundPlayBGM(const SoundData& soundData, bool loop, float volume)
{
    // If a BGM is already playing, stop it first
    SoundStopBGM();

    IXAudio2SourceVoice* pSourceVoice = nullptr;
    const WAVEFORMATEX* pWf = soundData.fmtRaw ? reinterpret_cast<const WAVEFORMATEX*>(soundData.fmtRaw)
                                               : &soundData.wfex;
    HRESULT hr = xAudio2_->CreateSourceVoice(&pSourceVoice, pWf);
    assert(SUCCEEDED(hr));

    XAUDIO2_BUFFER buf{};
    buf.pAudioData = soundData.pBuffer;
    buf.AudioBytes = soundData.bufferSize;
    buf.Flags = XAUDIO2_END_OF_STREAM;
    buf.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

    hr = pSourceVoice->SubmitSourceBuffer(&buf);
    assert(SUCCEEDED(hr));

    float clamped = (volume < 0.0f) ? 0.0f : (volume > 1.0f ? 1.0f : volume);
    pSourceVoice->SetVolume(clamped);

    hr = pSourceVoice->Start();
    assert(SUCCEEDED(hr));

    // store dedicated BGM voice
    bgmVoice_ = pSourceVoice;
}

void SoundManager::SoundSetBGMVolume(float volume)
{
    if (!bgmVoice_) return;
    float clamped = (volume < 0.0f) ? 0.0f : (volume > 1.0f ? 1.0f : volume);
    bgmVoice_->SetVolume(clamped);
}

void SoundManager::SoundStopBGM()
{
    if (!bgmVoice_) return;
    bgmVoice_->Stop();
    bgmVoice_->FlushSourceBuffers();
    bgmVoice_->DestroyVoice();
    bgmVoice_ = nullptr;
}

void SoundManager::StopAllVoices()
{
    for (auto* v : voices_) {
        if (!v) continue;
        v->Stop();
        v->FlushSourceBuffers();
        v->DestroyVoice();
    }
    voices_.clear();
}

void SoundManager::SetSEVolume(float volume)
{
    // 0～1にクランプ
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    seVolume_ = volume;
}

void SoundManager::SetMasterVolume(float volume)
{
    if (!masterVoice_) return;
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    // IXAudio2MasteringVoice has SetVolume method
    masterVoice_->SetVolume(volume);
}


