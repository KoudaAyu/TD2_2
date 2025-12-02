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

				// ★ 公転モードなら角度→位置で更新。そうでなければ従来の力学
				if (p.orbiting) {
					// 角度更新
					p.orbitAngle += p.orbitAngularVel * dt;

					// ★ 半径を拡大（内→外）
					p.radialSpeed += p.radialAccel * dt;      // 加速したい時だけ有効
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
				p.radialSpeed += p.radialAccel * dt;      // 加速したい時だけ有効
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
		srvMgr_->PreDraw();

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
				Matrix4x4 R = MakeRotateZMatrix(p.rotation); // apply self-rotation
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
		std::uniform_real_distribution<float> speedDist(0.08f, 0.15f);
		std::uniform_real_distribution<float> scaleDist(0.10f, 0.20f);
		std::uniform_real_distribution<float> lifeDist(1.0f, 2.0f);

		std::uniform_real_distribution<float> angVelDist(-6.0f, 6.0f);

		// ★ omni用のチューニング（出す間隔を長くしたいので寿命も長め）
		if (name == "default") {
			speedDist = std::uniform_real_distribution<float>{ 0.01f, 0.10f }; // 速く
			scaleDist = std::uniform_real_distribution<float>{ 1.0f, 1.0f }; // でかく
			lifeDist = std::uniform_real_distribution<float>{ 0.5f , 1.0f };  // 長生き
		}

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

		for (uint32_t i = 0; i < count; ++i) {
			Particle p{};
			p.position = position;
			p.lifeTime = lifeDist(rng_);
			p.current = 0.0f;
			p.color = { 1,1,1,0.1f };
			p.scale = scaleDist(rng_);

			
			if (group.useMesh) {
				p.angularVel = angVelDist(rng_);
			} else {
				p.angularVel = 0.0f;
			}

			if (name == "default") {
				// 全方向ランダムに飛ばす（ただし Z 変化させたくないので z=0 に固定）
				Vector3 dir = randomDirOnSphere();
				float spd = speedDist(rng_);
				p.velocity = { dir.x * spd, dir.y * spd, 0.0f }; // ← z 成分を 0 に固定
			} else if (name == "defaultMesh") {
				// 全方向ランダムに飛ばす（ただし Z 変化させたくないので z=0 に固定）
				Vector3 dir = randomDirOnSphere();
				float spd = speedDist(rng_);
				p.velocity = { dir.x * spd, dir.y * spd, 0.0f }; // ← z 成分を 0 に固定
			} else {
				// デフォルト（上向きに飛ぶ）
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


	// ParticleManager.cpp
	void ParticleManager::CreateParticleGroupFromModel(const std::string & name, const std::string & modelPath)
	{
		// 既に同名グループがある場合はスキップ（idempotent）
		auto it = particleGroups.find(name);
		if (it != particleGroups.end()) {
			return;
		}

		ParticleGroup g{};
		g.useMesh = true;
		g.model = ModelManager::GetInstance()->FindModel(modelPath); // 既にLoad済み前提
		assert(g.model && "ModelManager::LoadModel(modelPath) を先に呼んでください");

		// ★ インスタンス用StructuredBufferは不要（1 粒子ずつ描く）
		// テクスチャは Model::Draw が自分でバインドするのでSRVも不要

		particleGroups.emplace(name, std::move(g));
	}

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

			// 直線バーストの基準速度（フレーム単位）
			p.baseVelocity = dir * speed;
			p.velocity = p.baseVelocity;

			p.lifeTime = life;
			p.current = 0.0f;
			p.scale = scale;
			p.color = { 1,1,1,1 };

			// イージング有効化：外へ行くほど遅くなる（Ease-Out）
			p.easeOut = true;
			p.easePow = 2.0f;   // おすすめ: 2.0 = Quad（ふわっと減速）
			// 好みで 1.5 ～ 3.0 を試してOK

			// 公転は使わない
			p.orbiting = false;

			
			if (group.useMesh) p.angularVel = angVelDist(rng_);
			else p.angularVel = 0.0f;

			group.particles.push_back(p);
		}
	}


	// 指定中心を軸に公転する8粒子を同時生成
	// 追加引数: startRadius=0, radialSpeed>0, radialAccel=0 で内→外へ
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
			p.orbitAngle = step * i;
			p.orbitAngularVel = angularVel;

			// ★ 渦巻きパラメータ
			p.radialSpeed = radialSpeed;     // 正で外へ
			p.radialAccel = radialAccel;

			// 初期位置（開始半径）
			const float c = std::cos(p.orbitAngle);
			const float s = std::sin(p.orbitAngle);
			p.position = { center.x + c * p.orbitRadius,
					center.y + s * p.orbitRadius,
					center.z };

			// そのほか
			p.velocity = { 0,0,0 };          // 公転で位置制御するので未使用
			p.rotation = 0.0f;
			p.angularVel = 0.0f;              // 粒子自身のスピンは使わない
			p.scale = scale;
			p.color = { 1,1,1,1 };
			p.lifeTime = life;
			p.current = 0.0f;
			p.orbiting = true;

		
			if (group.useMesh) p.angularVel = angVelDist(rng_);

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
		const float inwardSpeed = -std::abs(radialSpeedAbs);
		const float inwardAccel = -std::abs(radialAccelAbs);

		for (int i = 0; i < kCount; ++i) {
			Particle p{};

			// 公転の初期状態
			p.center = center;
			p.orbitRadius = startRadius;      // 外側スタート
			p.orbitAngle = step * i;         // 円周上に等配
			p.orbitAngularVel = angularVel;       // 全員同方向（符号でCW/CCWを決定）
			p.orbiting = true;

			// 渦巻き（外→内）パラメータ
			p.radialSpeed = inwardSpeed;      // 半径が毎秒減る
			p.radialAccel = inwardAccel;      // 必要に応じてさらに減速（強い収束）

			// 初期位置（開始半径）
			const float c = std::cos(p.orbitAngle);
			const float s = std::sin(p.orbitAngle);
			p.position = { center.x + c * p.orbitRadius,
					   center.y + s * p.orbitRadius,
					   center.z };

			// 自走速度は未使用（公転で位置を決定）
			p.velocity = { 0.0f, 0.0f, 0.0f };

			// 見た目
			p.scale = scale;
			p.color = { 1,1,1,1 };

			// 寿命
			p.lifeTime = life;
			p.current = 0.0f;

		
			if (group.useMesh) p.angularVel = angVelDist(rng_);

		
		

			group.particles.push_back(p);
		}
	}
