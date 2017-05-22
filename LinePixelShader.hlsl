// Per-pixel color data passed to the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR0;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
	return float4(input.color, 0.f);
}
