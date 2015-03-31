#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
                         "RootConstants(num32BitConstants = 4, b0) "


cbuffer CONSTANT_BUF : register(b0)
{
	float4 color;
}

[RootSignature(MyRS1)]
float4 main() : SV_TARGET
{
	return color;
}