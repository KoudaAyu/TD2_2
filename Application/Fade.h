#pragma once

#include"Sprite.h"
#include"SpriteCom.h"
#include <vector>

class Fade
{
public:

	enum class State
	{
		kNone,
		kFadeIn,
		kFadeOut,
	};

	Fade() = default;
	~Fade();
	void Initialize(SpriteCom* spriteCom);
	void Update();
	void Draw();

	/// <summary>
	/// フェード開始
	/// </summary>
	/// <param name="state">フェードの状態</param>
	/// <param name="duration">フェードの経過時間</param>
	void Start(State state, float duration);

	/// <summary>
	/// フェードイン処理
	/// </summary>
	void FadeIn();

	/// <summary>
	/// フェードアウト処理
	/// </summary>
	void FadeOut();

	/// <summary>
	/// フェード終了
	/// </summary>
	void Stop();

	/// <summary>
	/// フェード終了判定
	/// </summary>
	/// <returns>フェード状態による分岐</returns>
	bool IsFinished() const;

	/// <summary>
	/// フェード中かどうか (kNone 以外はアクティブ)
	/// </summary>
	bool IsActive() const;

	/// <summary>
	/// 現在のフェード状態を取得
	/// </summary>
	State GetState() const;

private:
	//フェードの継続時間
	float duration_ = 1.0f;
	//経過時間
	float counter_ = 0.0f;

private:
	Sprite* fadeSprite_ = nullptr; // dark overlay
	Sprite* flashSprite_ = nullptr; // white flash used for cinematic cut
	Sprite* scanline_ = nullptr; // moving scanline for Eva-like effect
	SpriteCom* spriteCom_ = nullptr;

	State status_ = State::kNone;

	// ----- Evangelion-ish effect members -----
	// vertical bars used to create the Eva-style wipe effect
	std::vector<Sprite*> bars_;
	std::vector<Sprite*> barEdges_; // thin darker edge to simulate separation
	std::vector<Sprite*> barHighlights_; // moving highlights that sweep across
	int barCount_ = 12; // number of vertical bars
	float barGapDelay_ = 0.03f; // delay multiplier between bars
	Vector4 barColor_ = { 0.75f, 0.05f, 0.1f, 1.0f }; // dark red tint
	Vector4 edgeColor_ = { 0.1f, 0.02f, 0.02f, 1.0f };
	
	// per-bar phase/random offset for nicer motion
	std::vector<float> barPhase_;

	// highlights parameters
	float highlightWidthRatio_ = 0.12f; // width relative to bar width
	Vector4 highlightColor_ = { 1.0f, 0.85f, 0.6f, 0.85f };

	// scanline parameters
	float scanlineSpeed_ = 2200.0f; // px/sec
	Vector4 scanlineColor_ = { 1.0f, 0.2f, 0.2f, 0.25f };

	// helper state
	float screenW_ = 1280.0f;
	float screenH_ = 720.0f;
	
};