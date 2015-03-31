#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
                         "RootConstants(num32BitConstants = 4, b0) "


[RootSignature(MyRS1)]
float4 main(float4 pos : POSITION) : SV_POSITION
{
	return float4(pos.x, pos.y, .5, 1.);
}