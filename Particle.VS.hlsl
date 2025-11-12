// 入力（POSITION0, TEXCOORD0）
struct VSIn
{
    float4 pos : POSITION0;
    float2 uv : TEXCOORD0;
};

// 1インスタンスにつき1個のWVP行列（t1 にバインド）
StructuredBuffer<float4x4> gWVP : register(t1);

// 出力
struct VSOut
{
    float4 svpos : SV_Position;
    float2 uv : TEXCOORD0;
};

// SV_InstanceID は関数引数で受ける
VSOut main(VSIn vin, uint instId : SV_InstanceID)
{
    VSOut vout;
    float4x4 wvp = gWVP[instId];
    vout.svpos = mul(vin.pos, wvp);
    vout.uv = vin.uv;
    return vout;
}
