#include "Fade.h"

#include<algorithm>
#include <cassert>
#include <cmath>
#include "Random.h"
#include "Logger.h"

static float EaseOutBack(float t)
{
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float p = t - 1.0f;
    return 1.0f + c3 * p * p * p + c1 * p * p;
}

static float EaseOutCubic(float t)
{
    float p = t - 1.0f;
    return p * p * p + 1.0f;
}

static float SmoothPulse(float t)
{
    return std::sin(t * 3.14159265f);
}

static Vector4 LerpColor(const Vector4 &a, const Vector4 &b, float t)
{
    return { a.x + (b.x - a.x) * t,
             a.y + (b.y - a.y) * t,
             a.z + (b.z - a.z) * t,
             a.w + (b.w - a.w) * t };
}

Fade::~Fade()
{
    // cleanup sprites
    if (fadeSprite_) { delete fadeSprite_; fadeSprite_ = nullptr; }
    if (flashSprite_) { delete flashSprite_; flashSprite_ = nullptr; }
    if (scanline_) { delete scanline_; scanline_ = nullptr; }
    for (auto*s : bars_) { if (s) delete s; }
    for (auto*s : barEdges_) { if (s) delete s; }
    for (auto*s : barHighlights_) { if (s) delete s; }
    bars_.clear();
    barEdges_.clear();
    barHighlights_.clear();
    barPhase_.clear();
}

void Fade::Initialize(SpriteCom* spriteCom)
{
    // SpriteCom を注入して保持
    spriteCom_ = spriteCom;
    assert(spriteCom_ && "SpriteCom is null. Call Fade::Initialize with a valid SpriteCom*.");

    // create a full-screen dark overlay sprite used as base
    fadeSprite_ = spriteCom_->CreateSprite("Resources/white.png",
        { 0.0f,0.0f },
        { 1280.0f,720.0f },
        0.0f,
        { 0.0f,0.0f },
        false,
        false);
    // use black color to overlay
    if (fadeSprite_) { fadeSprite_->SetColor({0,0,0,1}); fadeSprite_->Update(); }

    // a flash sprite (white) used briefly for dramatic cut
    flashSprite_ = spriteCom_->CreateSprite("Resources/white.png",
        { 0.0f,0.0f },
        { 1280.0f,720.0f },
        0.0f,
        { 0.0f,0.0f },
        false,
        false);
    if (flashSprite_) { flashSprite_->SetColor({1,1,1,0}); flashSprite_->Update(); }

    // try to query actual screen size if available
    if (spriteCom_ && spriteCom_->GetDirectXCom()) {
        screenW_ = static_cast<float>(spriteCom_->GetDirectXCom()->GetClientWidth());
        screenH_ = static_cast<float>(spriteCom_->GetDirectXCom()->GetClientHeight());
    }

    // prepare vertical bars, edges and highlight sprites
    bars_.clear(); barEdges_.clear(); barHighlights_.clear(); barPhase_.clear();
    float barW = screenW_ / static_cast<float>(barCount_);
    for (int i = 0; i < barCount_; ++i) {
        // main bar
        Sprite* s = spriteCom_->CreateSprite("Resources/white.png",
            { -barW, 0.0f },
            { barW + 2.0f, screenH_ },
            0.0f,
            { 0.0f, 0.0f },
            false,
            false);
        if (s) { s->SetColor(barColor_); s->Update(); }
        bars_.push_back(s);

        // thin edge (slightly darker) on right side
        Sprite* edge = spriteCom_->CreateSprite("Resources/white.png",
            { -2.0f, 0.0f },
            { 6.0f, screenH_ },
            0.0f,
            { 0.0f, 0.0f },
            false,
            false);
        if (edge) { edge->SetColor(edgeColor_); edge->Update(); }
        barEdges_.push_back(edge);

        // highlight sweep sprite (narrow vertical gradient look)
        Sprite* hl = spriteCom_->CreateSprite("Resources/white.png",
            { -barW, 0.0f },
            { barW * highlightWidthRatio_, screenH_ },
            0.0f,
            { 0.0f, 0.0f },
            false,
            false);
        if (hl) { hl->SetColor(highlightColor_); hl->Update(); }
        barHighlights_.push_back(hl);

        // phase
        barPhase_.push_back(Random::GeneratorFloat(0.0f, 6.2831853f));
    }

    // scanline: long thin red sweep
    float scanH = (std::max)(4.0f, screenH_ * 0.006f);
    scanline_ = spriteCom_->CreateSprite("Resources/white.png",
        { 0.0f, -scanH },
        { screenW_, scanH },
        0.0f,
        { 0.0f, 0.0f },
        false,
        false);
    if (scanline_) { scanline_->SetColor(scanlineColor_); scanline_->Update(); }
}

void Fade::Update()
{
    switch (status_)
    {
        case State::kNone:
            // 何もしない
            break;
        case State::kFadeIn:
            FadeIn();
            break;
        case State::kFadeOut:
            FadeOut();
            break;
    }
}

void Fade::Draw()
{
    // If not active, don't draw anything to avoid residual sprites causing flashes
    if (status_ == State::kNone) return;

    const float kEpsilon = 0.01f; // raised threshold to avoid faint residual draw

    // draw highlights over bars first, then bars and edges
    for (int i = 0; i < static_cast<int>(barHighlights_.size()); ++i) {
        Sprite* hl = barHighlights_[i];
        if (!hl) continue;
        Vector4 hc = hl->GetColor();
        if (hc.w > kEpsilon) hl->Draw();
    }
    for (int i = 0; i < static_cast<int>(bars_.size()); ++i) {
        Sprite* s = bars_[i];
        Sprite* edge = barEdges_[i];
        if (s) { Vector4 sc = s->GetColor(); if (sc.w > kEpsilon) s->Draw(); }
        if (edge) { Vector4 ec = edge->GetColor(); if (ec.w > kEpsilon) edge->Draw(); }
    }

    // draw scanline above bars for strong Eva-like sweep
    if (scanline_) { Vector4 sc = scanline_->GetColor(); if (sc.w > kEpsilon) scanline_->Draw(); }

    if (fadeSprite_) { Vector4 fc = fadeSprite_->GetColor(); if (fc.w > kEpsilon) fadeSprite_->Draw(); }
    if (flashSprite_) { Vector4 fc2 = flashSprite_->GetColor(); if (fc2.w > kEpsilon) flashSprite_->Draw(); }
}

void Fade::Start(State state, float duration)
{
    status_ = state;
    duration_ = duration;
    counter_ = 0.0f;

    float barW = screenW_ / static_cast<float>(barCount_);
    for (int i = 0; i < static_cast<int>(bars_.size()); ++i) {
        Sprite* s = bars_[i];
        Sprite* edge = barEdges_[i];
        Sprite* hl = barHighlights_[i];
        if (!s || !edge || !hl) continue;

        if (state == State::kFadeIn) {
            // bars cover screen at start
            s->SetPosition({ static_cast<float>(i) * barW, 0.0f });
            edge->SetPosition({ static_cast<float>(i) * barW + (barW - 4.0f), 0.0f });
            hl->SetPosition({ static_cast<float>(i) * barW + barW * 0.5f, 0.0f });
        } else {
            // fade out: bars offscreen bottom
            s->SetPosition({ static_cast<float>(i) * barW, screenH_ + 10.0f });
            edge->SetPosition({ static_cast<float>(i) * barW + (barW - 4.0f), screenH_ + 10.0f });
            hl->SetPosition({ static_cast<float>(i) * barW + barW * 0.5f, screenH_ + 10.0f });
        }

        s->SetRotation(0.0f);
        s->SetScale({ barW + 2.0f, screenH_ });
        Vector4 c = s->GetColor(); c.w = 1.0f; s->SetColor(c);
        s->Update();

        edge->SetScale({ 6.0f, screenH_ });
        Vector4 ec = edge->GetColor(); ec.w = 1.0f; edge->SetColor(ec); edge->Update();

        hl->SetScale({ barW * highlightWidthRatio_, screenH_ });
        Vector4 hc = hl->GetColor(); hc.w = 0.0f; hl->SetColor(hc); hl->Update();

        // randomize phase
        barPhase_[i] = Random::GeneratorFloat(0.0f, 6.2831853f);
    }

    if (fadeSprite_) { fadeSprite_->SetColor({0,0,0,1}); fadeSprite_->Update(); }
    if (flashSprite_) { flashSprite_->SetColor({1,1,1,0}); flashSprite_->Update(); }

    // position scanline start
    if (scanline_) {
        float sh = scanline_->GetScale().y;
        if (state == State::kFadeIn) {
            scanline_->SetPosition({ 0.0f, -sh - 8.0f });
        } else {
            scanline_->SetPosition({ 0.0f, screenH_ + sh + 8.0f });
        }
        scanline_->Update();
    }
}

void Fade::FadeIn()
{
    counter_ += 1.0f / 60.0f;
    if (counter_ >= duration_) counter_ = duration_;
    float t = duration_ > 0.0f ? std::clamp(counter_ / duration_, 0.0f, 1.0f) : 1.0f;

    // overlay fades out quickly
    if (fadeSprite_) {
        float alpha = 1.0f - EaseOutCubic(t);
        // We'll clamp overlay alpha to not exceed any visible bar/scanline alpha to avoid
        // a frame where bars have left but the overlay still dims the screen.
        float maxCompAlpha = 0.0f;
        // check bars
        for (auto*s : bars_) { if (s) { Vector4 c = s->GetColor(); if (c.w > maxCompAlpha) maxCompAlpha = c.w; } }
        for (auto*s : barEdges_) { if (s) { Vector4 c = s->GetColor(); if (c.w > maxCompAlpha) maxCompAlpha = c.w; } }
        for (auto*s : barHighlights_) { if (s) { Vector4 c = s->GetColor(); if (c.w > maxCompAlpha) maxCompAlpha = c.w; } }
        if (scanline_) { Vector4 sc = scanline_->GetColor(); if (sc.w > maxCompAlpha) maxCompAlpha = sc.w; }
        if (flashSprite_) { Vector4 fc = flashSprite_->GetColor(); if (fc.w > maxCompAlpha) maxCompAlpha = fc.w; }

        // Debug logging to trace alpha behavior
        {
            std::string msg = "FadeIn t=" + std::to_string(t) + ", overlayAlpha(before)=" + std::to_string(1.0f - EaseOutCubic(t)) + ", maxCompAlpha=" + std::to_string(maxCompAlpha);
            Logger::Log(msg);
        }

        // clamp overlay so it won't remain visible when all components are gone
        if (maxCompAlpha < alpha) alpha = maxCompAlpha;

        Vector4 c = fadeSprite_->GetColor(); c.w = alpha; fadeSprite_->SetColor(c); fadeSprite_->Update();
    }

    float barW = screenW_ / static_cast<float>(barCount_);
    for (int i = 0; i < static_cast<int>(bars_.size()); ++i) {
        Sprite* s = bars_[i];
        Sprite* edge = barEdges_[i];
        Sprite* hl = barHighlights_[i];
        if (!s || !edge || !hl) continue;

        float delay = i * (barGapDelay_ * 0.7f); // tighter spacing for dramatic look
        float raw = std::clamp((t - delay) / (1.0f - delay), 0.0f, 1.0f);
        float localT = EaseOutBack(raw);

        // move up offscreen
        float yStart = 0.0f;
        float yFinal = -screenH_ - 60.0f;
        float y = yStart + (yFinal - yStart) * localT;

        // slight staggered x offset for depth
        // compute horizontal dispersion: bars move outward from center as they leave
        float xBase = static_cast<float>(i) * barW;
        float barCenter = xBase + 0.5f * barW;
        float centerNorm = (barCenter - 0.5f * screenW_) / (0.5f * screenW_); // -1..1
        float horizAmp = 0.6f * screenW_; // how far bars travel horizontally
        float horiz = centerNorm * horizAmp * localT; // progress outward with localT
        float jitter = std::sin(raw * 3.14159f + barPhase_[i]) * 2.0f * (1.0f - raw);
        float x = xBase + horiz + jitter;

        // color leans slightly toward magenta as bars release for more 'anime' tone
        Vector4 accent = { 0.9f, 0.1f, 0.6f, 1.0f };
        float colorLerp = 0.25f * std::abs(centerNorm) * raw;
        Vector4 targetCol = LerpColor(barColor_, accent, colorLerp);

        s->SetPosition({ x, y });
        s->SetScale({ barW + 6.0f - raw * 6.0f, screenH_ * (0.98f + raw * 0.05f) });
        Vector4 sc = s->GetColor();
        sc = LerpColor(sc, targetCol, 0.5f * raw);
        sc.w = 1.0f - EaseOutCubic(raw);
        s->SetColor(sc); s->Update();

        // edge follows right side
        edge->SetPosition({ x + barW - 4.0f, y });
        Vector4 ec = edge->GetColor(); ec.w = sc.w * 0.9f; edge->SetColor(ec); edge->Update();

        // highlight sweeps quickly across bar during move with pulsing width
        float hlX = x + raw * (barW - hl->GetScale().x);
        float hlPulse = 1.0f + 0.6f * (1.0f - std::fabs(0.5f - raw) * 2.0f);
        hl->SetScale({ hl->GetScale().x * hlPulse, hl->GetScale().y });
        hl->SetPosition({ hlX, y });
        Vector4 hc = hl->GetColor(); hc.w = (raw > 0.1f && raw < 0.9f) ? (0.9f * (1.0f - std::fabs(0.5f - raw) * 2.0f)) : 0.0f; hl->SetColor(hc); hl->Update();
    }

    // scanline sweep downward faster than main progress for a streak effect with pulse
    if (scanline_) {
        float sh = scanline_->GetScale().y;
        float range = screenH_ + sh * 2.0f;
        float scanT = std::clamp(t * 1.6f, 0.0f, 1.0f);
        float sy = -sh - 8.0f + EaseOutCubic(scanT) * range;
        scanline_->SetPosition({ 0.0f, sy });
        float glow = 0.6f + 0.4f * SmoothPulse(t * 3.0f);
        Vector4 sc = scanline_->GetColor(); sc.w = (1.0f - t) * scanlineColor_.w * glow; scanline_->SetColor(sc); scanline_->Update();
    }

    // small white flash near end for cinematic punch (stronger)
    if (t > 0.78f && flashSprite_) {
        float f = (t - 0.78f) / 0.22f; if (f > 1.0f) f = 1.0f;
        Vector4 fc = flashSprite_->GetColor(); fc.w = f * 0.95f; flashSprite_->SetColor(fc); flashSprite_->Update();
    }

    if (counter_ >= duration_) {
        status_ = State::kNone;
        // ensure everything hidden
        if (fadeSprite_) { Vector4 c = fadeSprite_->GetColor(); c.w = 0.0f; fadeSprite_->SetColor(c); fadeSprite_->Update(); }
        if (flashSprite_) { Vector4 fc = flashSprite_->GetColor(); fc.w = 0.0f; flashSprite_->SetColor(fc); flashSprite_->Update(); }
        if (scanline_) { Vector4 sc = scanline_->GetColor(); sc.w = 0.0f; scanline_->SetColor(sc); scanline_->Update(); }
        for (auto*s : bars_) { if (s) { Vector4 cc = s->GetColor(); cc.w = 0.0f; s->SetColor(cc); s->Update(); } }
        for (auto*s : barEdges_) { if (s) { Vector4 cc = s->GetColor(); cc.w = 0.0f; s->SetColor(cc); s->Update(); } }
        for (auto*s : barHighlights_) { if (s) { Vector4 cc = s->GetColor(); cc.w = 0.0f; s->SetColor(cc); s->Update(); } }

        Logger::Log("FadeIn complete: status set to kNone, all component alphas set to 0");
    }
}

void Fade::FadeOut()
{
    counter_ += 1.0f / 60.0f;
    if (counter_ >= duration_) counter_ = duration_;
    float t = duration_ > 0.0f ? std::clamp(counter_ / duration_, 0.0f, 1.0f) : 1.0f;

    float barW = screenW_ / static_cast<float>(barCount_);
    for (int i = 0; i < static_cast<int>(bars_.size()); ++i) {
        Sprite* s = bars_[i];
        Sprite* edge = barEdges_[i];
        Sprite* hl = barHighlights_[i];
        if (!s || !edge || !hl) continue;

        float delay = i * (barGapDelay_ * 0.9f);
        float raw = std::clamp((t - delay) / (1.0f - delay), 0.0f, 1.0f);
        float localT = EaseOutCubic(raw);

        float xBase = static_cast<float>(i) * barW;
        // bars start horizontally offset and converge to xBase as they cover the screen
        float barCenter = xBase + 0.5f * barW;
        float centerNorm = (barCenter - 0.5f * screenW_) / (0.5f * screenW_); // -1..1
        float horizAmp = 0.6f * screenW_;
        float horizStart = centerNorm * horizAmp; // initial offset
        float horiz = horizStart * (1.0f - localT); // converge to zero

        float startY = screenH_ + 60.0f;
        float y = startY + (0.0f - startY) * localT;

        float x = xBase + horiz;

        s->SetPosition({ x, y });
        s->SetScale({ barW + 4.0f - (1.0f - raw) * 4.0f, screenH_ * (0.95f + raw * 0.05f) });
        Vector4 sc = s->GetColor(); sc.w = localT; s->SetColor(sc); s->Update();

        edge->SetPosition({ x + barW - 4.0f, y });
        Vector4 ec = edge->GetColor(); ec.w = sc.w * 0.9f; edge->SetColor(ec); edge->Update();

        // highlight appears near leading edge with pulsing width
        float hlX = x + (1.0f - (1.0f - raw) * 0.9f) * (barW - hl->GetScale().x);
        float hlPulse = 1.0f + 0.8f * SmoothPulse(raw * 2.2f);
        hl->SetScale({ hl->GetScale().x * hlPulse, hl->GetScale().y });
        hl->SetPosition({ hlX, y });
        Vector4 hc = hl->GetColor(); hc.w = (raw > 0.1f) ? (0.6f * raw) : 0.0f; hl->SetColor(hc); hl->Update();
    }

    // dark overlay grows
    if (fadeSprite_) { Vector4 c = fadeSprite_->GetColor(); c.w = t * 0.95f; fadeSprite_->SetColor(c); fadeSprite_->Update(); }

    // scanline: sweep upward slightly delayed for impact and pulse stronger
    if (scanline_) {
        float sh = scanline_->GetScale().y;
        float range = screenH_ + sh * 2.0f;
        float scanT = std::clamp((t - 0.08f) / 0.92f, 0.0f, 1.0f);
        float sy = screenH_ + sh + 8.0f - EaseOutCubic(scanT) * range;
        scanline_->SetPosition({ 0.0f, sy });
        float glow = 0.6f + 0.6f * SmoothPulse(t * 4.0f);
        Vector4 sc = scanline_->GetColor(); sc.w = t * scanlineColor_.w * glow; scanline_->SetColor(sc); scanline_->Update();
    }

    // flashes: mid pulse then final strong pulse
    if (flashSprite_) {
        float mid = 0.45f;
        float midWidth = 0.12f;
        float fmid = 0.0f;
        if (t > mid && t < mid + midWidth) {
            fmid = (t - mid) / midWidth;
            if (fmid > 1.0f) fmid = 1.0f;
        }
        float fend = 0.0f;
        if (t > 0.82f) {
            fend = (std::min)(1.0f, (t - 0.82f) / 0.18f);
        }
        Vector4 fc = flashSprite_->GetColor();
        fc.w = (std::max)(fmid * 0.7f, fend * 1.0f);
        flashSprite_->SetColor(fc);
        flashSprite_->Update();
    }

    if (counter_ >= duration_) {
        status_ = State::kNone;
    }
}

void Fade::Stop()
{
    // stop the fade by setting status to none and resetting the counter
    status_ = State::kNone;
    counter_ = 0.0f;
}

bool Fade::IsFinished() const
{
    return counter_ >= duration_;
}

bool Fade::IsActive() const
{
    return status_ != State::kNone;
}

Fade::State Fade::GetState() const
{
    return status_;
}
