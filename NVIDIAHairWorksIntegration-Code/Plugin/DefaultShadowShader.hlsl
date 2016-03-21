#include "GFSDK_HairWorks_ShaderCommon.h" 

//////////////////////////////////////////////////////////////////////////////
// constant buffer for render parameters
//////////////////////////////////////////////////////////////////////////////
cbuffer cbPerFrame : register(b0)
{
	GFSDK_Hair_ConstantBuffer	g_hairConstantBuffer; // hairworks portion of constant buffer data
}

//SamplerState shadowSampler : register(s0);

//Texture2D   g_shadowTexture      :   register(t0);



//////////////////////////////////////////////////////////////////////////////////
// Pixel shader for shadow rendering pass
//////////////////////////////////////////////////////////////////////////////////
float ps_main(GFSDK_Hair_PixelShaderInput input) : SV_Target
{
	//float3 HairWorksShadow = GFSDK_Hair_ScreenToView(input.position, g_hairConstantBuffer).xyz;
	//float UnityShadow = g_shadowTexture.Sample(shadowSampler, HairWorksShadow.xy).r;
	//if (UnityShadow < HairWorksShadow.z)
	//return UnityShadow;
	//return HairWorksShadow.z;

	return GFSDK_Hair_ScreenToView(input.position, g_hairConstantBuffer).z;
}