#include "SpriteCom.h"
#include "Sprite.h" // Sprite実体

// 初期化
void SpriteCom::Initialize(DirectXCom* directXCom)
{
	// 引数を受け取ってメンバ変数に記録
	directXCom_ = directXCom;

	// グラフィックスパイプラインの生成
	CreateGraphicsPipeline();
}

/// <summary>
/// ルートシグネチャの作成
/// </summary>
void SpriteCom::CreateRootSignature()
{
	// RootSignatureの作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // 入力アセンブラーでの使用を許可

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};

	// SRV: t3, t4
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].BaseShaderRegister = 3;
	descriptorRange[0].RegisterSpace = 0;
	descriptorRange[0].OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// RootParemeter生成PuxelShaderのMaterialとVertexShaderのTransform
	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	rootParameters[0].ShaderVisibility =
		D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[0].Descriptor.ShaderRegister =
		0; // レジスタ番号0とバインド。b0の0と一致

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility =
		D3D12_SHADER_VISIBILITY_VERTEX;              // VertexShaderで使える
	rootParameters[1].Descriptor.ShaderRegister = 0; // レジスタ番号0を使用

	rootParameters[2].ParameterType =
		D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
	rootParameters[2].ShaderVisibility =
		D3D12_SHADER_VISIBILITY_ALL; // PixelShaderで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges =
		descriptorRange; // Tableの中身の配列を指定
	rootParameters[2].DescriptorTable.NumDescriptorRanges =
		_countof(descriptorRange); // Tableで管理する数

	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].Descriptor.ShaderRegister = 1;

	descriptionRootSignature.pParameters =
		rootParameters; // ルートパラメーター配列へのポインタ
	descriptionRootSignature.NumParameters =
		_countof(rootParameters); // 配列の長さ

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter =
		D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイアリニアフィルタ
	staticSamplers[0].AddressU =
		D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 0~1の範囲外をリピート
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 比較しない
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; // ありったけのMipmapを使う
	staticSamplers[0].ShaderRegister = 0;         // レジスタ番号0を使う
	staticSamplers[0].ShaderVisibility =
		D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	// シリアライズしてバイナリにする
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob,
		&errorBlob);

	if (FAILED(hr))
	{
		Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	// バイナリをもとに生成
	hr = directXCom_->GetDevice()->CreateRootSignature(
		0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));
}
/// <summary>
/// グラフィックスパイプラインの生成
/// </summary>
void SpriteCom::CreateGraphicsPipeline()
{

	// ルートシグネクチャの生成
	CreateRootSignature();

	// InputLayer
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION"; // セマンティック名
	inputElementDescs[0].SemanticIndex = 0;         // セマンティックインデックス
	inputElementDescs[0].Format =
		DXGI_FORMAT_R32G32B32A32_FLOAT; // 頂点のフォーマット
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;    // 入力要素の配列
	inputLayoutDesc.NumElements = _countof(inputElementDescs); // 入力要素の数

	// BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	// すべての色要素を書き込む
	blendDesc.RenderTarget[0].BlendEnable = TRUE; // ← ブレンドを有効化
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // ソースのα
	blendDesc.RenderTarget[0].DestBlend =
		D3D12_BLEND_INV_SRC_ALPHA; // デスティネーションのα
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;    // 加算ブレンド
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE; // αのソース
	blendDesc.RenderTarget[0].DestBlendAlpha =
		D3D12_BLEND_ZERO; // αのデスティネーション
	blendDesc.RenderTarget[0].BlendOpAlpha =
		D3D12_BLEND_OP_ADD; // αのブレンド演算
	blendDesc.RenderTarget[0].RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;

	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	// 裏面(時計回り)を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// Shaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob =
		directXCom_->CompileShader(L"Resources/shaders/Object3D.VS.hlsl", L"vs_6_0");
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob =
		directXCom_->CompileShader(L"Resources/shaders/Object3D.PS.hlsl", L"ps_6_0");
	assert(pixelShaderBlob != nullptr);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc{};
	graphicPipelineStateDesc.pRootSignature =
		rootSignature_.Get();                               // ルートシグネチャ
	graphicPipelineStateDesc.InputLayout = inputLayoutDesc; // 入力レイアウト
	graphicPipelineStateDesc.VS = {
		vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize() }; // 頂点シェーダーの設定
	graphicPipelineStateDesc.PS = {
		pixelShaderBlob->GetBufferPointer(),
		pixelShaderBlob->GetBufferSize() };           // ピクセルシェーダーの設定
	graphicPipelineStateDesc.BlendState = blendDesc; // ブレンドステートの設定
	graphicPipelineStateDesc.RasterizerState =
		rasterizerDesc; // ラスタライザーステートの設定
	// 書き込むRTVの情報
	graphicPipelineStateDesc.NumRenderTargets = 1;
	graphicPipelineStateDesc.RTVFormats[0] =
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // RTVのフォーマット
	// 利用するトロポジ(形状)のタイプ。三角形
	graphicPipelineStateDesc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// どのように画面に色を打ち込むか設定(気にしなくていい？)
	graphicPipelineStateDesc.SampleDesc.Count = 1; // マルチサンプルしない
	graphicPipelineStateDesc.SampleMask =
		D3D12_DEFAULT_SAMPLE_MASK; // サンプルマスクはデフォルト

	//// DepthStencilStateの設定
	//D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//// Depthの機能を有効化する
	//depthStencilDesc.DepthEnable = true;
	//// 書き込み
	//depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//// 比較関数はLessEqua。つまり、近ければ描画される
	//depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	//// DepthStencilの設定
	//graphicPipelineStateDesc.DepthStencilState = depthStencilDesc;
	//graphicPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	// 実際に生成
	hr = directXCom_->GetDevice()->CreateGraphicsPipelineState(
		&graphicPipelineStateDesc, IID_PPV_ARGS(&graphicPipelineState_));

	assert(vertexShaderBlob && "頂点シェーダーの読み込み失敗！");
	assert(pixelShaderBlob && "ピクセルシェーダーの読み込み失敗！");

	// パイプラインステートの生成に失敗した場合はエラー
	assert(SUCCEEDED(hr));
}

/// <summary>
/// 共通描画設定
/// </summary>
void SpriteCom::ApplyCommonRenderState()
{
	// RootSignatureを設定。PSOに設定しているけれど別途設定が必要
	directXCom_->GetCommandList()->SetGraphicsRootSignature(rootSignature_.Get());
	// パイプラインステートを設定
	directXCom_->GetCommandList()->SetPipelineState(graphicPipelineState_.Get());
	directXCom_->GetCommandList()->IASetPrimitiveTopology(
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

Sprite* SpriteCom::CreateSprite(const std::string& textureFilePath,
	const Vector2& position,
	const Vector2& scale,
	float rotation,
	const Vector2& anchorPoint,
	bool flipX,
	bool flipY)
{
	// 利用前に初期化済みかチェック
	assert(directXCom_ && "SpriteCom::Initialize を先に呼んでください");
	Sprite* sprite = new Sprite();
	sprite->Initialize(this, textureFilePath);

	// 初期値指定があれば上書き（Initializeでテクスチャサイズが scale_ に入るため 0 の時のみ保持）
	if (scale.x != 0.0f || scale.y != 0.0f)
	{
		sprite->SetScale(scale);
	}
	sprite->SetPosition(position);
	sprite->SetRotation(rotation);
	sprite->SetAnchorPoint(anchorPoint);
	sprite->SetFlipX(flipX);
	sprite->SetFlipY(flipY);
	// 初期設定反映
	sprite->Update();
	return sprite;
}