// From http://http.developer.nvidia.com/GPUGems3/gpugems3_ch40.html

cbuffer parameter : register(b0, space0)
{
//	float sigma;
};
Texture2D<float4> source : register(t0, space1);
RWTexture2D<float4> dest : register(u0, space2);

groupshared float4 local_src[8 + 2 * 8][2];

[numthreads(8, 2, 1)]
void main( uint3 DTid : SV_DispatchThreadID, uint3 Lid : SV_GroupThreadID)
{
	const float sigma = 10.;
	local_src[Lid.x][Lid.y] = source.Load(DTid.xyz + int3(-8, 0, 0));
	local_src[Lid.x + 8][Lid.y] = source.Load(DTid.xyz);
	local_src[Lid.x + 16][Lid.y] = source.Load(DTid.xyz + int3(8, 0, 0));

	GroupMemoryBarrier();

	float g0 = 1.0 / (sqrt(2.0 * 3.14) * sigma);
	float g1 = exp(-.5 / (sigma * sigma));
	float g2 = g1 * g1;

	float4 sum = local_src[Lid.x + 8][Lid.y] * g0;
	g0 *= g1;
	g1 *= g2;
	float total_weight = g0;
	for (int j = 1; j < 8; j++) {
		total_weight += g0;
		sum += local_src[8 + Lid.x - j][Lid.y] * g0;
		total_weight += g0;
		sum += local_src[8 + Lid.x + j][Lid.y] * g0;
		g0 *= g1;
		g1 *= g2;
	}
	dest[DTid.xy] = sum / total_weight;
}
