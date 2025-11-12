#pragma once
#include <string>
#include <cstdint>      // ← uint32_t
#include <Windows.h>    // WAVEFORMATEX, BYTE
#include <wrl.h>        // ← ComPtr
#include <xaudio2.h>    // ← IXAudio2, IXAudio2MasteringVoice
#include <vector>

struct SoundData {
	// 波型フォーマット（基本部）
	WAVEFORMATEX wfex{};
	// fmtチャンクの生データ（可変長: WAVEFORMATEX 互換先頭 + 追加領域）
	BYTE* fmtRaw = nullptr;
	uint32_t fmtSize = 0;
	// バッファの先頭アドレス（波形データ）
	BYTE* pBuffer = nullptr;
	// バッファのサイズ
	unsigned int bufferSize = 0;
};

// チャンクヘッダ
struct ChunkHeader {
	char id[4];    // チャンクID
	uint32_t size; // チャンクのサイズ
};
// RIFFヘッダチャンク
struct RiffHeader {
	ChunkHeader chunk; // チャンクヘッダ
	char type[4];      // フォーマット（"WAVE"）
};

struct FormatChunk {
	ChunkHeader chunk; // fmt
	WAVEFORMATEX fmt;  // フォーマット情報
};

class SoundManager
{
public:
	bool Initialize();  // ← 初期化をまとめる
	void Finalize();    // ← 終了処理

	SoundData SoundLoadWave(const char* filename);
	void SoundUnload(SoundData* soundData);
	void SoundPlayWave(const SoundData& soundData, bool loop = false);
	// 音量指定版（0.0f ～ 1.0f 推奨）
	void SoundPlayWave(const SoundData& soundData, bool loop, float volume);

	void StopAllVoices(); // すべての再生音を停止・破棄

	// SEのデフォルト音量を設定（0.0f ～ 1.0f）
	void SetSEVolume(float volume);
	float GetSEVolume() const { return seVolume_; }
	// マスターボリューム設定（0.0f ～ 1.0f）
	void SetMasterVolume(float volume);

private:
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
	IXAudio2MasteringVoice* masterVoice_ = nullptr;
	std::vector<IXAudio2SourceVoice*> voices_{}; // アクティブなSourceVoice
	float seVolume_ = 1.0f; // デフォルトSE音量
};

