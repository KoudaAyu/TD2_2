Texture2D gTex : register(t0);
SamplerState gSamp : register(s0);

struct PSIn
{
    float4 svpos : SV_Position;
    float2 uv : TEXCOORD0;
};

float4 main(PSIn pin) : SV_Target
{
    // テクスチャから色を取得
    float4 color = gTex.Sample(gSamp, pin.uv);

    // 透明度を強調したい場合（例: 半透明を強くしたいとき）
    color.a *= 1.0f; // ← アルファを半分にする

    // アルファ値をそのまま返す
    return color;
}
