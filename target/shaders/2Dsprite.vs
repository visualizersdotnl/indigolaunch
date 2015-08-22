
struct VS_INPUT
{
	float3 position : SV_POSITION;
	float4 color : COLOR0;
	float2 UV : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float4 color : COLOR0;
	float2 UV : TEXCOORD0;
};

VS_OUTPUT main(in VS_INPUT input)
{
	VS_OUTPUT output;
	output.position = float4(input.position, 1.f);
	output.color = input.color;
	output.UV = input.UV;
	return output;
}
