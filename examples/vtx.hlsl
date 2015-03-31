#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
                         "DescriptorTable(CBV(b0)) "


[RootSignature(MyRS1)]
float4 main(float4 pos : POSITION) : SV_POSITION
{
	return float4(pos.x, pos.y, .5, 1.);
}