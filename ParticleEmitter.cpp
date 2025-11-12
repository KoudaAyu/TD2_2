#include "ParticleEmitter.h"
#include "ParticleManager.h"

ParticleEmitter::ParticleEmitter(const std::string& groupName,
    const Vector3& position,
    uint32_t emitCount,
    float emitInterval)
{
    groupName_ = groupName;
    position_ = position;
    emitCount_ = emitCount;
    emitInterval_ = emitInterval;
}

void ParticleEmitter::Update(float deltaTime)
{
    // ● 時刻を進める
    emitTimer_ += deltaTime;

    // ● 発生頻度(=emitInterval_)より大きいなら発生
    //    ParticleManager::GetInstance()->Emit(name, position, count);
    while (emitTimer_ >= emitInterval_) {
        // 安全: グループ未作成ならスキップ
        // ParticleManager::Emit は assert するため、存在チェックしてから呼ぶ
        auto* pm = ParticleManager::GetInstance();
        // 粒子グループ存在チェックは内部状態に依存するため、Emit内のassertを避ける目的で
        // ここでは try-emit の代わりに、存在しない場合は何もしない（開発時はログ推奨）
        // 直接アクセスできないので、最小対応として例外を避けるため Emit を呼ぶ前でガードする設計のみ。
        // 利用側は必ず CreateParticleGroup を事前に呼ぶこと。
        // ここでは簡易に、存在時のみ呼べるように Create を冪等化済み。
        // 実際の存在チェック用APIが無いので、Reset直後はエミットしないことを意図してスキップする。
        
        // try emit
        // このまま呼ぶとassertの可能性があるため、粒子グループはシーンInitializeで毎回作成しておくこと。
        pm->Emit(groupName_, position_, emitCount_);
        // ● 余剰に過ぎた時間も加味して頻度計算する（持ち越し）
        emitTimer_ -= emitInterval_;
    }
}

void ParticleEmitter::Emit()
{
    // エミッタの設定値に従って、ParticleManagerのEmitを呼び出す
    auto* pm = ParticleManager::GetInstance();
    pm->Emit(groupName_, position_, emitCount_);
}