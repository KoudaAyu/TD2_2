#include "Object3dCom.h"

/// <summary>
/// 初期化
/// </summary>
void Object3dCom::Initialize(DirectXCom *directXCom) {
  directXCom_ = directXCom;

  CreateGraphicsPipeline();
}

/// <summary>
/// 共通描画設定
/// </summary>
void Object3dCom::ApplyCommonRenderState() {
  // RootSignatureを設定。PSOに設定しているけれど別途設定が必要
  directXCom_->GetCommandList()->SetGraphicsRootSignature(rootSignature_.Get());
  // デフォルトは CCW を表面として扱うPSO
  directXCom_->GetCommandList()->SetPipelineState(pipelineFrontCCW_.Get());
  directXCom_->GetCommandList()->IASetPrimitiveTopology(
      D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

/// <summary>
/// ルートシグネチャの作成
/// </summary>
void Object3dCom::CreateRootSignature() {
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

  if (FAILED(hr)) {
    Logger::Log(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
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
void Object3dCom::CreateGraphicsPipeline(){

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
  blendDesc.RenderTarget[0].RenderTargetWriteMask =
      D3D12_COLOR_WRITE_ENABLE_ALL;

  // Shaderをコンパイルする
  Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob =
      directXCom_->CompileShader(L"Resources/shaders/Object3D.VS.hlsl", L"vs_6_0");
  assert(vertexShaderBlob != nullptr);

  Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob =
      directXCom_->CompileShader(L"Resources/shaders/Object3D.PS.hlsl", L"ps_6_0");
  assert(pixelShaderBlob != nullptr);

  // DepthStencilStateの設定
  D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
  depthStencilDesc.DepthEnable = true;
  depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

  // ベースとなるPSO desc
  D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
  desc.pRootSignature = rootSignature_.Get();
  desc.InputLayout = inputLayoutDesc;
  desc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()};
  desc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};
  desc.BlendState = blendDesc;
  desc.NumRenderTargets = 1;
  desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  desc.SampleDesc.Count = 1;
  desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
  desc.DepthStencilState = depthStencilDesc;
  desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

  // 共通のラスタライザ設定
  D3D12_RASTERIZER_DESC rast{};
  rast.CullMode = D3D12_CULL_MODE_BACK;      // バックフェイスカリング
  rast.FillMode = D3D12_FILL_MODE_SOLID;
  rast.DepthClipEnable = TRUE;

  // 1) Front = CCW
  rast.FrontCounterClockwise = TRUE;
  desc.RasterizerState = rast;
  hr = directXCom_->GetDevice()->CreateGraphicsPipelineState(
      &desc, IID_PPV_ARGS(&pipelineFrontCCW_));
  assert(SUCCEEDED(hr));

  // 2) Front = CW
  rast.FrontCounterClockwise = FALSE;
  desc.RasterizerState = rast;
  hr = directXCom_->GetDevice()->CreateGraphicsPipelineState(
      &desc, IID_PPV_ARGS(&pipelineFrontCW_));
  assert(SUCCEEDED(hr));
}