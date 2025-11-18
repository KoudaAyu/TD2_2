#pragma once

#include"Sprite.h"
#include"SpriteCom.h"

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

private:
	//フェードの継続時間
	float duration_ = 1.0f;
	//経過時間
	float counter_ = 0.0f;

private:
	Sprite* fadeSprite_ = nullptr;
	SpriteCom* spriteCom_ = nullptr;

	State status_ = State::kNone;
};