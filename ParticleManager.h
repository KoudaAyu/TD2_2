#pragma once
#define NOMINMAX
#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <vector>
#include <random>
#include <list>
#include <string>
#include "Vector.h"
#include <unordered_map>
#include "Matrix4x4.h"
#include "Camera.h"
#include"TextureManager.h"
#include "DirectXCom.h"
#include <d3d12.h>
#include <wrl.h>
#include "Model.h"
#include "ModelManager.h"

class DirectXCom;
class SrvManager;

struct Particle {
	Vector3 position;
	Vector3 velocity;
	float   lifeTime;
	float   current;
	Vector4 color;
	float   scale;
	bool IsAlive() const { return current < lifeTime; }

	float rotation = 0.0f;
	float angularVel = 0.0f;

	// ★ 追加: 公転制御
	Vector3 center = { 0,0,0 };   // 公転の中心（原点にしたいなら {0,0,0}）
	float   orbitAngle = 0.0f;  // 現在角
	float   orbitAngularVel = 0.0f; // 角速度[rad/s]
	float   orbitRadius = 0.0f; // 半径
	bool    orbiting = false;   // 公転中フラグ

	float   radialSpeed = 0.0f;   // 半径の増分 [units/sec]
	float   radialAccel = 0.0f;   // 半径の加速度 [units/sec^2]（必要なければ0）

	// === 追加: 直線バースト用イージング ===
	Vector3 baseVelocity = { 0,0,0 }; // 初期の基準速度
	bool    easeOut = false;        // イージング有効/無効
	float   easePow = 1.0f;         // Ease指数（1=Linear, 2=Quad...）
};

struct ParticleGroup {
	// マテリアルデータ（テクスチャファイルパス と テクスチャ用SRVインデックス）
	std::string textureFilePath;
	uint32_t    textureSrvIndex = 0;

	// パーティクルのリスト（std::list<Particle> 型）
	std::list<Particle> particles;

	// インスタンシングデータ用SRVインデックス
	uint32_t instanceSrvIndex = 0;

	// インスタンシング用リソース
	Microsoft::WRL::ComPtr<ID3D12Resource> instanceResource;

	// インスタンス数
	uint32_t instanceCount = 0;

	// インスタンシングデータを書き込むためのポインタ
	void* instanceMappedPtr = nullptr;

	bool useMesh = false;
	Model* model = nullptr;
};



class ParticleManager {
public:
	// シングルトン
	static ParticleManager* GetInstance();

	// 初期化処理（スライドの指示通りの引数）
	void Initialize(DirectXCom* dx, SrvManager* srvMgr, Object3dCom* object3dCom);

	// 更新処理
	void Update(const Matrix4x4& view, const Matrix4x4& projection);

	// ParticleManager.h に既出の宣言が無ければ追加
	void Draw();

	// パーティクルの発生
	void Emit(const std::string name, const Vector3& position, uint32_t count);
	void Finalize();

	// パーティクルグループの生成
	void CreateParticleGroup(const std::string name, const std::string textureFilePath);
	void CreateParticleGroupFromModel(const std::string& name, const std::string& modelPath);

	// 追加宣言
	void EmitBurst8(const std::string& name,
		const Vector3& position,
		float speed, float scale, float life);
	// ヘッダ宣言（ParticleManager.h）
	void EmitBurst8Rotating(
		const std::string& name,
		const Vector3& center,
		float startRadius,
		float life,
		float angularVel,
		float scale,
		bool  alternateDir,
		float radialSpeed,     // ★ 追加: 半径の毎秒増分
		float radialAccel = 0  // ★ 追加: 半径の加速度（不要なら0）
	);

	// 外→内に渦巻きながら公転する8粒子を一括生成
	void EmitBurst8RotatingInward(
		const std::string& name,
		const Vector3& center,   // 公転の中心（原点なら {0,0,0}）
		float startRadius,       // 開始半径（外側から始めるので > 0 を推奨）
		float life,              // 寿命 [s]
		float angularVel,        // 角速度 [rad/s]（＋=反時計回り、−=時計回り）
		float scale,             // 粒子スケール
		float radialSpeedAbs,    // 収縮速度の大きさ [units/s]（正で指定 → 内向きに変換）
		float radialAccelAbs = 0 // 収縮加速度の大きさ [units/s^2]（正で指定 → 内向きに変換、既定0）
	);

private:
	ParticleManager() = default;
	~ParticleManager() = default;
	ParticleManager(const ParticleManager&) = delete;
	ParticleManager& operator=(const ParticleManager&) = delete;
	static ParticleManager* instance;

	// 引数で受け取ったポインタをメンバに記録する
	DirectXCom* dx_ = nullptr;
	SrvManager* srvMgr_ = nullptr;

	// ランダムエンジンの初期化
	std::mt19937 rng_{};

	// パイプライン生成（この段階ではRootSignatureだけ作成）
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;

	// 頂点データの初期化（座標等）→ 頂点リソース生成 → VBV作成 → リソースに書き込む
	struct Vertex { float x, y, z, w; float u, v; };
	std::vector<Vertex> vertices_;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_; // 頂点リソース
	D3D12_VERTEX_BUFFER_VIEW vbv_{};                      // VBV

	// ユーザが付けるグループ名をキーに、複数グループを保持
	std::unordered_map<std::string, ParticleGroup> particleGroups;

	struct TransformationMatrix { Matrix4x4 WVP; Matrix4x4 World; };

	Microsoft::WRL::ComPtr<ID3D12Resource> meshTransformCB_;
	TransformationMatrix* meshTransformPtr_ = nullptr;

	// （照明CBが未バインドだとデバッグ層が警告する場合があるのでダミーも用意）
	Microsoft::WRL::ComPtr<ID3D12Resource> meshLightCB_;
	struct DirectionalLight { Vector4 color; Vector3 direction; float intensity; };
	DirectionalLight* meshLightPtr_ = nullptr;

	// Object3dCom を借りるためのポインタ（Initialize時にもらう）
	Object3dCom* object3dCom_ = nullptr;

	Matrix4x4 viewProj_;

	// 1 draw = 1 slot
	static constexpr UINT kMaxMeshCB = 2048;
	static constexpr UINT kCBAlign = 256;
	static constexpr UINT AlignedCBSize = (sizeof(TransformationMatrix) + (kCBAlign - 1)) & ~(kCBAlign - 1);


	uint8_t* meshTransformCBBase_ = nullptr; // 先頭ポインタ（生ポインタでOK）
	uint32_t meshCBWriteIndex_ = 0;          // 今フレームの書き込み開始位置


private:
	void CreatePipeline_();          // パイプライン生成（RootSignatureのみ）
};
