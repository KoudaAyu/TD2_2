#include "ParticleManager.h"
#include "DirectXCom.h"
#include "SrvManager.h"
#include <cassert>
#include <cstring> // std::memcpy

using Microsoft::WRL::ComPtr;

ParticleManager* ParticleManager::instance = nullptr;

ParticleManager* ParticleManager::GetInstance() {
	if (!instance) instance = new ParticleManager();
	return instance;
}

bool ParticleManager::HasGroup(const std::string& name) const {
    return particleGroups.find(name) != particleGroups.end();
}

void ParticleManager::Initialize(DirectXCom* dx, SrvManager* srvMgr, Object3dCom* object3dCom)
{
	// ● 引数でDirectXCommonとSRVマネージャのポインタを受け取ってメンバ変数に記録する。
	dx_ = dx;
	srvMgr_ = srvMgr;

	// ● ランダムエンジンの初期化
	std::random_device rd;
	rng_ = std::mt19937(rd());

	object3dCom_ = object3dCom;



	meshLightCB_ = dx_->CreateBufferResource(sizeof(DirectionalLight));
	meshLightCB_->Map(0, nullptr, reinterpret_cast<void**>(&meshLightPtr_));
	meshLightPtr_->color = { 1,1,1,1 };
	meshLightPtr_->direction = { 0,-1,0 };
	meshLightPtr_->intensity = 0.0f; // ライト無効相当（Model 側は enableLighting=false）でも念のため


	// ● パイプライン生成
	CreatePipeline_();

	// ● 頂点データの初期化（座標等）
// ここではパーティクル1枚分のローカル四角形（-0.5～0.5）だけを用意
	vertices_.clear();
	vertices_.reserve(4);
	vertices_.push_back({ -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f }); // 左下
	vertices_.push_back({ -0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f }); // 左上
	vertices_.push_back({ 0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f }); // 右下
	vertices_.push_back({ 0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f }); // 右上

	// ● 頂点リソース生成
	const size_t vbSize = sizeof(Vertex) * vertices_.size();
	vertexBuffer_ = dx_->CreateBufferResource(vbSize);
	assert(vertexBuffer_ != nullptr);

	// ● 頂点リソースに頂点データを書き込む
	void* mapped = nullptr;
	HRESULT hr = vertexBuffer_->Map(0, nullptr, &mapped);
	assert(SUCCEEDED(hr));
	std::memcpy(mapped, vertices_.data(), vbSize);
	vertexBuffer_->Unmap(0, nullptr);

	// ● 頂点バッファビュー（VBV）作成
	vbv_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vbv_.SizeInBytes = static_cast<UINT>(vbSize);
	vbv_.StrideInBytes = sizeof(Vertex);

	// 以前は sizeof(TransformationMatrix) だけ → ここを「スロット数分」確保
	meshTransformCB_ = dx_->CreateBufferResource(AlignedCBSize * kMaxMeshCB);
	meshTransformCB_->Map(0, nullptr, reinterpret_cast<void**>(&meshTransformCBBase_));
	meshTransformPtr_ = reinterpret_cast<TransformationMatrix*>(meshTransformCBBase_); // 互換のため残してもOK
	*meshTransformPtr_ = { MakeIdentity4x4(), MakeIdentity4x4() };

}

// ParticleManager.cpp
void ParticleManager::Update(const Matrix4x4& view, const Matrix4x4& projection)
{
	const float dt = 1.0f / 60.0f;

	// Draw用
	viewProj_ = Multiply(view, projection);
	meshCBWriteIndex_ = 0;

	// ビルボード行列（回転用：面内の回転はこの空間で行う）
	Matrix4x4 matBillboard = view;
	matBillboard.m[3][0] = matBillboard.m[3][1] = matBillboard.m[3][2] = 0.0f;
	matBillboard.m[0][3] = matBillboard.m[1][3] = matBillboard.m[2][3] = 0.0f;
	matBillboard.m[3][3] = 1.0f;
	matBillboard = Inverse(matBillboard);

	for (auto& [name, group] : particleGroups) {

		// ───────────────
		// メッシュ粒子
		// ───────────────
		if (group.useMesh) {
			for (auto it = group.particles.begin(); it != group.particles.end();) {
				Particle& p = *it;
				p.current += dt;
				if (p.current >= p.lifeTime) { it = group.particles.erase(it); continue; }

				// 回転更新（メッシュ粒子にも自転を追加）
				p.rotation += p.angularVel * dt;
				// 3D回転も更新
				p.rotation3.x += p.angularVel3.x * dt;
				p.rotation3.y += p.angularVel3.y * dt;
				p.rotation3.z += p.angularVel3.z * dt;

				// ★ 公転モードなら角度→位置で更新。そうでなければ従来の力学
				if (p.orbiting) {
					// 角度更新
					p.orbitAngle += p.orbitAngularVel * dt;

					// ★ 半径を拡大（内→外）
					p.orbitRadius += p.radialSpeed * dt;      // ここで半径が伸びる

					// 位置再計算
					const float c = std::cos(p.orbitAngle);
					const float s = std::sin(p.orbitAngle);
					p.position = { p.center.x + c * p.orbitRadius,
								   p.center.y + s * p.orbitRadius,
								   p.center.z };
				} else {
					// ▼▼ ここから追加（イージング） ▼▼
					float t = (p.lifeTime > 0.0f) ? (p.current / p.lifeTime) : 1.0f; // 0→1
					if (p.easeOut) {
						float w = std::max(0.0f, 1.0f - t);  // 外に行くほど小さく
						w = std::pow(w, p.easePow);          // 指数で減速の度合い調整
						p.velocity = p.baseVelocity * w;     // 初期速度×ウェイト
					}
					// ▲▲ ここまで追加 ▲▲

					if (name == "up_gravity") { p.velocity += Vector3{ 0.0f, -0.01f, 0.0f }; }
					p.position += p.velocity;
				}



				++it;
			}
			// OBJ はインスタンス書き込み不要
			continue;
		}

		// ───────────────
		// 板ポリ粒子（テクスチャ）
// ───────────────
		group.instanceCount = 0;
		constexpr UINT kMaxInstance = 1024;

		for (auto it = group.particles.begin(); it != group.particles.end();) {
			Particle& p = *it;

			p.current += dt;
			if (p.current >= p.lifeTime) { it = group.particles.erase(it); continue; }

			// ★ 公転 or 従来の力学
			if (p.orbiting) {
				// 角度更新
				p.orbitAngle += p.orbitAngularVel * dt;

				// ★ 半径を拡大（内→外）
// p.radialSpeed += p.radialAccel * dt;      // 加速したい時だけ有効
				p.orbitRadius += p.radialSpeed * dt;      // ここで半径が伸びる

				// 位置再計算
				const float c = std::cos(p.orbitAngle);
				const float s = std::sin(p.orbitAngle);
				p.position = { p.center.x + c * p.orbitRadius,
							   p.center.y + s * p.orbitRadius,
							   p.center.z };
			} else {
				// ▼▼ ここから追加（イージング） ▼▼
				float t = (p.lifeTime > 0.0f) ? (p.current / p.lifeTime) : 1.0f; // 0→1
				if (p.easeOut) {
					float w = std::max(0.0f, 1.0f - t);
					w = std::pow(w, p.easePow);
					p.velocity = p.baseVelocity * w;
				}
				// ▲▲ ここまで追加 ▲▲

				if (name == "up_gravity") { p.velocity += Vector3{ 0.0f, -0.01f, 0.0f }; }
				p.position += p.velocity;
			}



			// 粒子自身のスピン（必要なら保持）
            p.rotation += p.angularVel * dt;

			if (group.instanceCount >= kMaxInstance || !group.instanceMappedPtr) { ++it; continue; }

			// S * Billboard * Rz * T
			Matrix4x4 S = MakeScaleMatrix({ p.scale, p.scale, p.scale });
			Matrix4x4 R = MakeRotateZMatrix(p.rotation);
			Matrix4x4 T = MakeTranslateMatrix(p.position);

			Matrix4x4 W = Multiply(S, Multiply(matBillboard, Multiply(R, T)));
			Matrix4x4 WVP = Multiply(W, viewProj_);

			auto* instData = reinterpret_cast<Matrix4x4*>(group.instanceMappedPtr);
			instData[group.instanceCount] = WVP;
			++group.instanceCount;
			++it;
		}
	}
}

	void ParticleManager::Draw()
	{
		auto* cl = dx_->GetCommandList();

		// =============================
		// 1) テクスチャ粒子（板ポリ）
		// =============================
		// ※ 1回だけ共通セット
		// SRVヒープをバインド（SrvManager の仕様に合わせて）
// srvMgr_->PreDraw();

		cl->SetGraphicsRootSignature(rootSignature_.Get()); // ← CreatePipeline_ で作った粒子用RS
		if (pso_) { cl->SetPipelineState(pso_.Get()); }

		cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		cl->IASetVertexBuffers(0, 1, &vbv_);

		for (auto& [name, g] : particleGroups) {
			if (g.useMesh) continue;                                // 板ポリだけ
			if (g.instanceCount == 0) continue;                     // 今フレームの粒子が無ければスキップ
			if (!g.instanceMappedPtr) continue;                     // 念のため

			// t0: 粒子テクスチャ
			srvMgr_->SetGraphicsRootDescriptorTable(0, g.textureSrvIndex);
			// t1: インスタンス(WVP) StructuredBuffer
			srvMgr_->SetGraphicsRootDescriptorTable(1, g.instanceSrvIndex);

			// 4頂点 × instanceCount
			cl->DrawInstanced(4, g.instanceCount, 0, 0);
		}

		// =============================
		// 2) メッシュ粒子（OBJ）
// =============================
		object3dCom_->ApplyCommonRenderState();                      // RootSig/PSO/IA（三角形）
		cl->SetGraphicsRootConstantBufferView(3, meshLightCB_->GetGPUVirtualAddress());

		for (auto& [name, g] : particleGroups) {
			if (!g.useMesh) continue;
			if (!g.model) continue;
			if (g.particles.empty()) continue;

			for (auto it = g.particles.begin(); it != g.particles.end(); ++it) {
				if (meshCBWriteIndex_ >= kMaxMeshCB) break;

				const Particle& p = *it;

				// 書き込み先スロット
				const D3D12_GPU_VIRTUAL_ADDRESS gpuAddr =
					meshTransformCB_->GetGPUVirtualAddress() +
					static_cast<UINT64>(AlignedCBSize) * meshCBWriteIndex_;
            auto* slot = reinterpret_cast<TransformationMatrix*>(
                meshTransformCBBase_ + static_cast<size_t>(AlignedCBSize) * meshCBWriteIndex_);
				Matrix4x4 S = MakeScaleMatrix({ p.scale, p.scale, p.scale });
				// build full 3D rotation from rotation3
				Matrix4x4 Rz = MakeRotateZMatrix(p.rotation3.z);
				Matrix4x4 Ry = MakeRotateYMatrix(p.rotation3.y);
				Matrix4x4 Rx = MakeRotateXMatrix(p.rotation3.x);
				Matrix4x4 R = Multiply(Rx, Multiply(Ry, Rz));
				Matrix4x4 T = MakeTranslateMatrix(p.position);
				Matrix4x4 world = Multiply(Multiply(S, R), T);
				slot->World = world;
				slot->WVP = Multiply(world, viewProj_);

				// b1(Vertex) にこのスロットを指す
				cl->SetGraphicsRootConstantBufferView(1, gpuAddr);

				// b0(Material) / t3(Texture) は Model 側がセットして Draw してくれる
				g.model->Draw(cl);

				++meshCBWriteIndex_;
			}
		}


	}



	void ParticleManager::CreatePipeline_()
	{
		using Microsoft::WRL::ComPtr;

		// ── 1) RootSignature（t0: Texture SRV, t1: Instance(WVP) SRV, s0: StaticSampler）──
		D3D12_DESCRIPTOR_RANGE ranges[2]{};
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // t0
		ranges[0].NumDescriptors = 1;
		ranges[0].BaseShaderRegister = 0;

		ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // t1
		ranges[1].NumDescriptors = 1;
		ranges[1].BaseShaderRegister = 1;

		D3D12_ROOT_PARAMETER params[2]{};
		params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // t0→PS
		params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		params[0].DescriptorTable = { 1, &ranges[0] };

		params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // t1→VS
		params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		params[1].DescriptorTable = { 1, &ranges[1] };

		// Static Sampler (s0)
		D3D12_STATIC_SAMPLER_DESC samp{};
		samp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samp.AddressU = samp.AddressV = samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samp.ShaderRegister = 0; // s0
		samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC rsDesc{};
		rsDesc.NumParameters = _countof(params);
		rsDesc.pParameters = params;
		rsDesc.NumStaticSamplers = 1;
		rsDesc.pStaticSamplers = &samp;
		rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> sigBlob, errBlob;
		HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
		if (FAILED(hr)) {
			if (errBlob) OutputDebugStringA((const char*)errBlob->GetBufferPointer());
			assert(false);
		}
		hr = dx_->GetDevice()->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(),
			IID_PPV_ARGS(&rootSignature_));
		assert(SUCCEEDED(hr));

		// ── 2) “同じ rootSignature_” を使って PSO を作る ──
		
		ComPtr<IDxcBlob> vs = dx_->CompileShader(L"Resources/shaders/Particle.VS.hlsl", L"vs_6_0");
		ComPtr<IDxcBlob> ps = dx_->CompileShader(L"Resources/shaders/Particle.PS.hlsl", L"ps_6_0");

		
		D3D12_INPUT_ELEMENT_DESC inputElems[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_BLEND_DESC blend{};
		blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blend.RenderTarget[0].BlendEnable = TRUE;
		blend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		blend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

		D3D12_RASTERIZER_DESC rast{};
		rast.FillMode = D3D12_FILL_MODE_SOLID;
		rast.CullMode = D3D12_CULL_MODE_NONE;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = rootSignature_.Get();                  // ★同じRSを渡す
		psoDesc.InputLayout = { inputElems, _countof(inputElems) };
		psoDesc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
		psoDesc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
		psoDesc.BlendState = blend;
		psoDesc.RasterizerState = rast;
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // ←あなたのバックバッファに合わせて
		psoDesc.SampleDesc.Count = 1;

		hr = dx_->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso_));
		assert(SUCCEEDED(hr));
	}

	void ParticleManager::CreateParticleGroup(const std::string name,
		const std::string textureFilePath)
	{
		// 既に同名グループがある場合は再作成せずにスキップ（idempotent）
		auto it = particleGroups.find(name);
		if (it != particleGroups.end()) {
			return;
		}

		ParticleGroup newGroup{};
		newGroup.textureFilePath = textureFilePath;

		// ★ TextureManager に読み込みを依頼（未読み込みなら内部でロード & SRV作成してくれる想定）
		TextureManager::GetInstance()->LoadTexture(textureFilePath);

		// ★ TextureManager が作った SRV のインデックスをもらう（←これを使う！）
		newGroup.textureSrvIndex = TextureManager::GetInstance()->GetSrvIndex(textureFilePath);

		// ▼▼ ここは今まで通り：インスタンス用のStructuredBufferを自前で作る ▼▼
		const UINT kMaxInstance = 1024;
		size_t instanceBufferSize = sizeof(Matrix4x4) * kMaxInstance;
		newGroup.instanceResource = dx_->CreateBufferResource(instanceBufferSize);
		newGroup.instanceSrvIndex = srvMgr_->Allocate();
		srvMgr_->CreateSRVforStructureBuffer(
			newGroup.instanceSrvIndex, newGroup.instanceResource.Get(),
			kMaxInstance, sizeof(Matrix4x4));

		HRESULT hr = newGroup.instanceResource->Map(0, nullptr, &newGroup.instanceMappedPtr);
		assert(SUCCEEDED(hr));
		newGroup.instanceCount = 0;

		particleGroups[name] = std::move(newGroup);
	}



	void ParticleManager::Emit(const std::string name, const Vector3 & position, uint32_t count)
	{
		auto it = particleGroups.find(name);
		if (it == particleGroups.end()) return; // silently ignore missing group
		ParticleGroup& group = it->second;

		std::uniform_real_distribution<float> u01(0.0f, 1.0f);
		auto rand01 = [&] { return u01(rng_); };

		// ランダム分布（後で名前ごとに上書き）
// std::uniform_real_distribution<float> speedDist(0.08f, 0.15f);
		std::uniform_real_distribution<float> speedDist(0.01f, 0.05f); // 感じて変更
		std::uniform_real_distribution<float> scaleDist(0.10f, 0.20f);
		std::uniform_real_distribution<float> lifeDist(1.0f, 2.0f);

		std::uniform_real_distribution<float> angVelDist(-6.0f, 6.0f);

		// ★ omni用のチューニング（出す間隔を長くしたいので寿命も長め）
// if (name == "default") {
// 	speedDist = std::uniform_real_distribution<float>{ 0.01f, 0.10f }; // 速く
// 	scaleDist = std::uniform_real_distribution<float>{ 1.0f, 1.0f }; // でかく
// 	lifeDist = std::uniform_real_distribution<float>{ 0.5f , 1.0f };  // 長生き
// }

		if (name == "defaultMesh") {
			speedDist = std::uniform_real_distribution<float>{ 0.01f, 0.10f }; // 速く
			scaleDist = std::uniform_real_distribution<float>{ 0.5f, 0.5f }; // でかく
			lifeDist = std::uniform_real_distribution<float>{ 0.5f , 1.0f };  // 長生き
		}

		auto randomDirOnSphere = [&]() {
			float u = rand01();
			float v = rand01();
			float cosT = 2.0f * u - 1.0f;
			float sinT = std::sqrt(std::max(0.0f, 1.0f - cosT * cosT));
			float phi = 6.283185307f * v;
			return Vector3{ sinT * std::cos(phi), cosT, sinT * std::sin(phi) };
			};

		// variations: color jitter and scale jitter distributions
		std::uniform_real_distribution<float> colorJit(-0.18f, 0.18f);
		std::uniform_real_distribution<float> scaleJit(0.85f, 1.25f);

		for (uint32_t i = 0; i < count; ++i) {
			Particle p{};
			p.position = position;
			p.lifeTime = lifeDist(rng_);
			p.current = 0.0f;
			// base color slightly transparent
			p.color = { 1,1,1,0.1f };
			p.scale = scaleDist(rng_);

			
			if (group.useMesh) {
				p.angularVel = angVelDist(rng_);
                // initialize 3D rotation and angular velocity for mesh particles
                std::uniform_real_distribution<float> ang3Init(0.0f, 6.283185307f);
                std::uniform_real_distribution<float> ang3Vel(-1.5f, 1.5f);
                p.rotation3 = { ang3Init(rng_), ang3Init(rng_), ang3Init(rng_) };
                p.angularVel3 = { ang3Vel(rng_), ang3Vel(rng_), ang3Vel(rng_) };
			} else {
				p.angularVel = 0.0f;
			}

			if (name == "default") {
				// widen color and scale variety for boss/emphasis
				float cj = colorJit(rng_);
				p.color = { 1.0f + cj, 0.9f + cj*0.5f, 0.6f + cj*0.2f, 0.9f };
				p.scale *= scaleJit(rng_);
				// velocity mostly upwards
				Vector3 dir = randomDirOnSphere();
				float spd = speedDist(rng_);
				p.velocity = { dir.x * spd, dir.y * spd, 0.0f };
			} else if (name == "defaultMesh") {
				float cj = colorJit(rng_);
				p.color = { 0.9f + cj*0.2f, 0.9f + cj*0.2f, 1.0f + cj*0.1f, 1.0f };
				p.scale *= scaleJit(rng_);
				Vector3 dir = randomDirOnSphere();
				float spd = speedDist(rng_);
				p.velocity = { dir.x * spd, dir.y * spd, 0.0f };
			} else {
				// fallback varied default
				float cj = colorJit(rng_);
				p.color = { 1.0f + cj, 1.0f + cj*0.2f, 1.0f + cj*0.1f, 0.85f + (rand01()*0.15f) };
				p.scale *= 0.9f + rand01()*0.6f;
				float spd = speedDist(rng_);
				p.velocity = { 0.0f, spd, 0.0f };
			}

			group.particles.push_back(p);
		}
	}


	void ParticleManager::Finalize()
	{
		for (auto& [name, group] : particleGroups) {
			if (!group.useMesh) {
				if (group.instanceResource) {
					if (group.instanceMappedPtr) {
						group.instanceResource->Unmap(0, nullptr);
						group.instanceMappedPtr = nullptr;
					}
					group.instanceResource.Reset();
				}

			}
			// メッシュ粒子：Model側の解放は ModelManager の責務
		}

		// ★★★ ここを追加：CBのUnmapと解放 ★★★
		if (meshTransformCB_) {
			if (meshTransformCBBase_) {
				meshTransformCB_->Unmap(0, nullptr);
				meshTransformCBBase_ = nullptr;
				meshTransformPtr_ = nullptr;
			}
			meshTransformCB_.Reset();
		}
		if (meshLightCB_) {
			// lightCBは Map しているので Unmap する
			if (meshLightPtr_) {
				meshLightCB_->Unmap(0, nullptr);
				meshLightPtr_ = nullptr;
			}
			meshLightCB_.Reset();
		}

		vertexBuffer_.Reset();
		pso_.Reset();
		rootSignature_.Reset();
		particleGroups.clear();
	}


	// This function emits an 8-way burst (original implementation)
	void ParticleManager::EmitBurst8(const std::string& name,
		const Vector3& position,
		float speed, float scale, float life)
	{
		auto it = particleGroups.find(name);
		if (it == particleGroups.end()) return; // silently ignore missing group
		ParticleGroup& group = it->second;

		std::uniform_real_distribution<float> angVelDist(-6.0f, 6.0f);
		// 8方向（45度ごと）
		for (int i = 0; i < 8; ++i) {
			float angle = DirectX::XM_2PI / 8.0f * i;
			Vector3 dir = { std::cos(angle), std::sin(angle), 0.0f };

			Particle p{};
			p.position = position;

			// add slight random jitter to scale/color/speed to reduce repetition
			float sj = std::uniform_real_distribution<float>(0.75f, 1.35f)(rng_);
			float cj = std::uniform_real_distribution<float>(-0.22f, 0.22f)(rng_);

			p.baseVelocity = dir * speed * sj;
			p.velocity = p.baseVelocity;

			p.lifeTime = life * (0.72f + std::uniform_real_distribution<float>(0.0f, 0.56f)(rng_));
			p.current = 0.0f;
			p.scale = scale * sj;

			// color: angle-based shift + jitter for variety
			float hue = (angle / (DirectX::XM_2PI));
			p.color = { std::clamp(1.0f - 0.6f * hue + cj, 0.0f, 1.0f),
			            std::clamp(0.2f + 0.8f * hue + cj * 0.35f, 0.0f, 1.0f),
			            std::clamp(0.1f + 0.5f * (1.0f - hue) + cj * 0.15f, 0.0f, 1.0f),
			            1.0f };

			// イージング有効化：外へ行くほど遅くなる（Ease-Out）
			p.easeOut = true;
			p.easePow = 1.8f + std::uniform_real_distribution<float>(-0.3f, 0.6f)(rng_);
			// 公転は使わない
			p.orbiting = false;

			// give slight spin variation for textured particles so they look different
			if (group.useMesh) {
				p.angularVel = angVelDist(rng_);
                // 3D rotation for mesh
                std::uniform_real_distribution<float> ang3Init(0.0f, 6.283185307f);
                std::uniform_real_distribution<float> ang3Vel(-2.0f, 2.0f);
                p.rotation3 = { ang3Init(rng_), ang3Init(rng_), ang3Init(rng_) };
                p.angularVel3 = { ang3Vel(rng_), ang3Vel(rng_), ang3Vel(rng_) };
			}
			else p.angularVel = std::uniform_real_distribution<float>(-2.0f, 2.0f)(rng_);

			group.particles.push_back(p);

			// spawn small secondary sparks in a fallback group to add uniqueness
			auto itSpark = particleGroups.find("default");
			if (itSpark != particleGroups.end()) {
				ParticleGroup& sparkGroup = itSpark->second;
				int sparks = 1 + (std::uniform_int_distribution<int>(0,2)(rng_));
				for (int si = 0; si < sparks; ++si) {
					Particle sp{};
					sp.position = position + Vector3{ dir.x * 0.2f * si, dir.y * 0.2f * si, 0.0f };
					float sjs = std::uniform_real_distribution<float>(0.35f, 0.9f)(rng_);
					sp.scale = (scale * 0.35f) * sjs;
					sp.lifeTime = std::uniform_real_distribution<float>(0.25f, 0.65f)(rng_);
					sp.current = 0.0f;
					float spd = std::uniform_real_distribution<float>(speed * 0.3f, speed * 1.2f)(rng_);
					sp.velocity = { dir.x * spd * (0.6f + std::uniform_real_distribution<float>(-0.4f,0.6f)(rng_)),
					                  dir.y * spd * (0.6f + std::uniform_real_distribution<float>(-0.4f,0.6f)(rng_)),
					                  0.0f };
					sp.color = { 1.0f, 0.85f + std::uniform_real_distribution<float>(-0.2f,0.2f)(rng_), 0.6f + std::uniform_real_distribution<float>(-0.2f,0.2f)(rng_), 0.9f };
					sparkGroup.particles.push_back(sp);
				}
			}
		}
	}


	void ParticleManager::EmitBurst8Rotating(
		const std::string& name,
		const Vector3& center,
		float startRadius,
		float life,
		float angularVel,
		float scale,
		bool  alternateDir,
		float radialSpeed,     // ★ 追加: 半径の毎秒増分
		float radialAccel  // ★ 追加: 半径の加速度（不要なら0）
	)
	{
		auto it = particleGroups.find(name);
		if (it == particleGroups.end()) return; // silently ignore missing group
		ParticleGroup& group = it->second;

		std::uniform_real_distribution<float> angVelDist(-6.0f, 6.0f);
		constexpr int kCount = 8;
		const float step = 2.0f * 3.14159265358979323846f / float(kCount);

		for (int i = 0; i < kCount; ++i) {
			Particle p{};
			p.center = center;
			p.orbitRadius = startRadius;     // ★ 0から始めると内側スタート
			p.orbitAngle = step * i + std::uniform_real_distribution<float>(-0.2f,0.2f)(rng_); // jitter start angle
			// small per-particle angular velocity variation
			float av = angularVel * (1.0f + std::uniform_real_distribution<float>(-0.25f,0.25f)(rng_));
			if (alternateDir && (i % 2 == 1)) av = -av;
			p.orbitAngularVel = av;

			// ★ 渦巻きパラメータ
// p.radialSpeed = radialSpeed * (1.0f + std::uniform_real_distribution<float>(-0.35f,0.6f)(rng_));
			p.radialSpeed = radialSpeed;
			p.radialAccel = radialAccel;

			// 初期位置（開始半径）
// const float c = std::cos(p.orbitAngle);
// const float s = std::sin(p.orbitAngle);
// p.position = { center.x + c * p.orbitRadius,
// 			   center.y + s * p.orbitRadius,
// 			   center.z };
			p.position = center;

			// そのほか
			p.velocity = { 0,0,0 };          // 公転で位置制御するので未使用
			p.rotation = std::uniform_real_distribution<float>(0.0f, 6.2831853f)(rng_);
			p.angularVel = std::uniform_real_distribution<float>(-2.5f, 2.5f)(rng_);
			p.scale = scale * std::uniform_real_distribution<float>(0.8f, 1.4f)(rng_);
			// slight color variance
			float cj = std::uniform_real_distribution<float>(-0.28f, 0.28f)(rng_);
			p.color = { std::clamp(0.9f + cj*0.3f, 0.0f, 1.0f), 0.8f + cj*0.2f, 0.6f + cj*0.15f, 1.0f };
			p.lifeTime = life * (0.85f + std::uniform_real_distribution<float>(-0.12f, 0.28f)(rng_));
			p.current = 0.0f;
			p.orbiting = true;

			if (group.useMesh) {
				p.angularVel += angVelDist(rng_);
                // init 3D rotation and angular velocity
                std::uniform_real_distribution<float> ang3Init(0.0f, 6.283185307f);
                std::uniform_real_distribution<float> ang3Vel(-1.2f, 1.8f);
                p.rotation3 = { ang3Init(rng_), ang3Init(rng_), ang3Init(rng_) };
                p.angularVel3 = { ang3Vel(rng_), ang3Vel(rng_), ang3Vel(rng_) };
			}

			group.particles.push_back(p);
		}
	}


	void ParticleManager::EmitBurst8RotatingInward(
		const std::string& name,
		const Vector3& center,
		float startRadius,
		float life,
		float angularVel,
		float scale,
		float radialSpeedAbs,
		float radialAccelAbs)
	{
		auto it = particleGroups.find(name);
		if (it == particleGroups.end()) return; // silently ignore missing group
		ParticleGroup& group = it->second;

		std::uniform_real_distribution<float> angVelDist(-6.0f, 6.0f);
		constexpr int kCount = 8;
		const float step = 2.0f * 3.14159265358979323846f / float(kCount);

		// 内向きに確実に収束させるため、速度・加速度は負符号で設定
		const float inwardBase = -std::abs(radialSpeedAbs);
		const float inwardAccelBase = -std::abs(radialAccelAbs);

		for (int i = 0; i < kCount; ++i) {
			Particle p{};

			// 公転の初期状態
			p.center = center;
			p.orbitRadius = startRadius + std::uniform_real_distribution<float>(-1.2f,1.8f)(rng_);      // 外側スタート with jitter
			p.orbitAngle = step * i + std::uniform_real_distribution<float>(-0.35f,0.35f)(rng_);         // jitter
			// vary angular direction/velocity slightly
			float sign = (i % 2 == 0) ? 1.0f : -1.0f;
			p.orbitAngularVel = (angularVel + std::uniform_real_distribution<float>(-1.2f,1.6f)(rng_)) * sign;
			p.orbiting = true;

			// 渦巻き（外→内）パラメータ with per-particle variance
			p.radialSpeed = inwardBase * (0.6f + std::uniform_real_distribution<float>(-0.25f,0.6f)(rng_));
			p.radialAccel = inwardAccelBase * (0.6f + std::uniform_real_distribution<float>(-0.25f,0.6f)(rng_));

			// 初期位置（開始半径）
// const float c = std::cos(p.orbitAngle);
// const float s = std::sin(p.orbitAngle);
// p.position = { center.x + c * p.orbitRadius,
// 			   center.y + s * p.orbitRadius,
// 			   center.z };
			p.position = center;

			// 見た目
			p.scale = scale * std::uniform_real_distribution<float>(0.7f, 1.5f)(rng_);
			float hueJ = std::uniform_real_distribution<float>(-0.25f, 0.25f)(rng_);
			p.color = { std::clamp(1.0f + hueJ*0.4f, 0.0f, 1.0f), 0.85f + hueJ*0.3f, 0.6f + hueJ*0.2f, 1.0f };

			// 寿命
			p.lifeTime = life * (0.85f + std::uniform_real_distribution<float>(-0.15f, 0.25f)(rng_));
			p.current = 0.0f;

			if (group.useMesh) {
				p.angularVel = angVelDist(rng_);
                std::uniform_real_distribution<float> ang3Init(0.0f, 6.283185307f);
                std::uniform_real_distribution<float> ang3Vel(-1.4f, 1.6f);
                p.rotation3 = { ang3Init(rng_), ang3Init(rng_), ang3Init(rng_) };
                p.angularVel3 = { ang3Vel(rng_), ang3Vel(rng_), ang3Vel(rng_) };
			}

			group.particles.push_back(p);
		}
	}

	void ParticleManager::EmitCustom(const std::string& name, const Vector3& position, uint32_t count, const Vector4& baseColor, float scaleMin, float scaleMax)
{
    auto it = particleGroups.find(name);
    if (it == particleGroups.end()) return;
    ParticleGroup& group = it->second;

    std::uniform_real_distribution<float> u01(0.0f, 1.0f);
    std::uniform_real_distribution<float> colorJ(-0.18f, 0.18f);
    std::uniform_real_distribution<float> scaleD(scaleMin, scaleMax);
    std::uniform_real_distribution<float> spd(0.02f, 0.18f);

    for (uint32_t i=0;i<count;++i) {
        Particle p{};
        p.position = position + Vector3{ (u01(rng_)-0.5f)*0.6f, (u01(rng_)-0.5f)*0.6f, (u01(rng_)-0.5f)*0.2f };
        p.scale = scaleD(rng_);
        float cj = colorJ(rng_);
        p.color = { std::clamp(baseColor.x + cj, 0.0f, 1.0f), std::clamp(baseColor.y + cj*0.5f, 0.0f, 1.0f), std::clamp(baseColor.z + cj*0.3f, 0.0f, 1.0f), baseColor.w };
        p.lifeTime = 0.6f + u01(rng_)*1.2f;
        p.current = 0.0f;
        Vector3 dir = { (u01(rng_)-0.5f), (u01(rng_)-0.5f), (u01(rng_)*0.6f) };
        float len = std::sqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
        if (len > 1e-6f) dir = { dir.x/len, dir.y/len, dir.z/len };
        p.baseVelocity = { dir.x*spd(rng_), dir.y*spd(rng_), dir.z*spd(rng_) };
        p.velocity = p.baseVelocity;
        p.angularVel = std::uniform_real_distribution<float>(-4.0f,4.0f)(rng_);
        if (group.useMesh) {
            std::uniform_real_distribution<float> ang3Init(0.0f, 6.283185307f);
            std::uniform_real_distribution<float> ang3Vel(-2.0f, 2.0f);
            p.rotation3 = { ang3Init(rng_), ang3Init(rng_), ang3Init(rng_) };
            p.angularVel3 = { ang3Vel(rng_), ang3Vel(rng_), ang3Vel(rng_) };
        }
        group.particles.push_back(p);
    }
}

void ParticleManager::CreateParticleGroupFromModel(const std::string& name, const std::string& modelPath)
{
    // If group already exists, do nothing
    auto it = particleGroups.find(name);
    if (it != particleGroups.end()) return;

    ParticleGroup g{};
    g.useMesh = true;

    // Ensure model is loaded
    ModelManager::GetInstance()->LoadModel(modelPath);
    g.model = ModelManager::GetInstance()->FindModel(modelPath);
    assert(g.model && "CreateParticleGroupFromModel: model not found after LoadModel");

    particleGroups.emplace(name, std::move(g));
}
