#pragma once

class Bullet
{
public:
	Bullet();
	~Bullet();
	void Initialize();
	void Update();
	void Draw();

public:
	bool IsActive() const { return isActive_; }
	void SetActive(bool isActive) { isActive_ = isActive; }

private:

	bool isActive_ = false;

private:
};
