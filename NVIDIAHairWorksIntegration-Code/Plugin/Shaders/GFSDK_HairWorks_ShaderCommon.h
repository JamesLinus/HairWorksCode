// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright (c) 2013-2015 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

#ifndef __HAIRWORKS_SHADER_COMMON_H_
#define __HAIRWORKS_SHADER_COMMON_H_

#ifndef TWO_PI
#define TWO_PI 3.141592 * 2.0f
#endif

#ifndef FLT_EPSILON
#define FLT_EPSILON 0.00000001f
#endif

#ifdef _CPP // to include this header in CPP file

#ifndef float4
#define float4			gfsdk_float4
#endif

#ifndef float3
#define float3			gfsdk_float3
#endif

#ifndef float2
#define float2			gfsdk_float2
#endif

#ifndef float4x4
#define float4x4		gfsdk_float4x4
#endif

#ifndef row_major
#define row_major		
#endif

#ifndef float4x4
#define float4x4		gfsdk_float4x4
#endif

#ifndef NOINTERPOLATION
#define	NOINTERPOLATION					
#endif

#endif

/////////////////////////////////////////////////////////////////////////////////////////////
struct GFSDK_Hair_ShaderAttributes
{
	float3	P;			// world coord position
	float3	T;			// world space tangent vector
	float3	N;			// world space normal vector at the root
	float4	texcoords; // texture coordinates on hair root 
						// .xy: texcoord on the hair root
						// .z: texcoord along the hair
						// .w: texcoord along the hair quad
	float3	V;			// world space view vector
	float	hairID;		// unique hair identifier
};

//////////////////////////////////////////////////////////////////////////////
// basic hair material from constant buffer
//////////////////////////////////////////////////////////////////////////////
// 9 float4
struct GFSDK_Hair_Material 
{
	// 3 float4
	float4			rootColor; 
	float4			tipColor; 
	float4			specularColor; 

	// 4 floats (= 1 float4)
	float			diffuseBlend;
	float			diffuseScale;
	float			diffuseHairNormalWeight;
	float			_diffuseUnused_; // for alignment and future use

	// 4 floats (= 1 float4)
	float			specularPrimaryScale;
	float			specularPrimaryPower;
	float			specularPrimaryBreakup;
	float			specularNoiseScale;

	// 4 floats (= 1 float4)
	float			specularSecondaryScale;
	float			specularSecondaryPower;
	float			specularSecondaryOffset;
	float			_specularUnused_; // for alignment and future use

	// 4 floats (= 1 float4)
	float			rootTipColorWeight;
	float			rootTipColorFalloff;
	float			shadowSigma;
	float			strandBlendScale;

	// 4 floats (= 1 float4)
	float			glintStrength;
	float			glintCount;
	float			glintExponent;
	float			rootAlphaFalloff;
};

//////////////////////////////////////////////////////////////////////////////
// Use this data structure to manage all hair related cbuffer data within your own cbuffer
struct GFSDK_Hair_ConstantBuffer
{
	// camera information 
	row_major	float4x4	inverseViewProjection; // inverse of view projection matrix
	row_major	float4x4	inverseProjection; // inverse of projection matrix
	row_major	float4x4	inverseViewport; // inverse of viewport transform
	row_major	float4x4	inverseViewProjectionViewport; // inverse of world to screen matrix

	float4					camPosition;		  // position of camera center
	float4					modelCenter; // center of the growth mesh model

	// shared settings 
	int						useRootColorTexture;
	int						useTipColorTexture; 
	int						useStrandTexture;
	int						useSpecularTexture;

	int						receiveShadows;		
	int						shadowUseLeftHanded;
	float					__shadowReserved1__;
	float					__shadowReserved2__;

	int						strandBlendMode;
	int						colorizeMode;	
	int						strandPointCount;
	int						__reserved__;

	float					lodDistanceFactor;		
	float					lodDetailFactor;		
	float					lodAlphaFactor;
	float					__reservedLOD___;


	GFSDK_Hair_Material		defaultMaterial; 

	// noise table
	float4					_noiseTable[256]; // 1024 floats

};

// Codes below are for use with hlsl shaders only
#ifndef _CPP 

#ifndef SAMPLE_LEVEL
#define SAMPLE_LEVEL( _texture, _sampler, _coord, _level )	_texture.SampleLevel( _sampler, _coord, _level )
#endif

#ifndef SYS_POSITION
#define SYS_POSITION					SV_Position
#endif

#ifndef NOINTERPOLATION
#define	NOINTERPOLATION					nointerpolation
#endif

//////////////////////////////////////////////////////////////////////////////
// return normalized noise (0..1) from a unique hash id
inline float GFSDK_Hair_GetNormalizedNoise(unsigned int hash, GFSDK_Hair_ConstantBuffer hairConstantBuffer)
{
	const unsigned int mask = 1023; 
	unsigned int id = hash & mask;

	unsigned int noiseIdx1 = id / 4;
	unsigned int noiseIdx2 = id % 4;

	return hairConstantBuffer._noiseTable[noiseIdx1][noiseIdx2];
}

//////////////////////////////////////////////////////////////////////////////
// return signed noise (-1 .. 1) from a unique hash id
inline float GFSDK_Hair_GetSignedNoise(unsigned int hash, GFSDK_Hair_ConstantBuffer hairConstantBuffer)
{
	float v = GFSDK_Hair_GetNormalizedNoise(hash, hairConstantBuffer);
	return 2.0f * (v - 0.5f);
}

//////////////////////////////////////////////////////////////////////////////
// return vector noise [-1..1, -1..1, -1..1] from a unique hash id
inline float3 GFSDK_Hair_GetVectorNoise(unsigned int seed, GFSDK_Hair_ConstantBuffer hairConstantBuffer)
{
	float x = GFSDK_Hair_GetSignedNoise(seed, hairConstantBuffer);
	float y = GFSDK_Hair_GetSignedNoise(seed + 1229, hairConstantBuffer);
	float z = GFSDK_Hair_GetSignedNoise(seed + 2131, hairConstantBuffer);

	return float3(x,y,z);
}

//////////////////////////////////////////////////////////////////////////////
inline float3 GFSDK_Hair_ComputeHairShading(
	float3 Lcolor, // light color and illumination
	float3 Ldir, // light direction

	float3 V, // view vector
	float3 N, // surface normal
	float3 T, // hair tangent

	float3 diffuseColor, // diffuse albedo
	float3 specularColor, // specularity

	float diffuseBlend,
	float primaryScale,
	float primaryShininess,
	float secondaryScale,
	float secondaryShininess,
	float secondaryOffset
	)
{
	// diffuse hair shading
	float TdotL = clamp(dot( T , Ldir), -1.0f, 1.0f);
	float diffuseSkin = max(0, dot( N, Ldir));
	float diffuseHair = sqrt( 1.0f - TdotL*TdotL );
	
	float diffuseSum = lerp(diffuseHair, diffuseSkin, diffuseBlend);
	
	// primary specular
	float3 H = normalize(V + Ldir);
	float TdotH = clamp(dot(T, H), -1.0f, 1.0f);
	float specPrimary = sqrt(1.0f - TdotH*TdotH);
	specPrimary = pow(max(0, specPrimary), primaryShininess);

	// secondary
	TdotH = clamp(TdotH + secondaryOffset, -1.0, 1.0);
	float specSecondary = sqrt(1 - TdotH*TdotH);
	specSecondary = pow(max(0, specSecondary), secondaryShininess);

	// specular sum
	float specularSum = primaryScale * specPrimary + secondaryScale * specSecondary;

	float3 output = diffuseSum * (Lcolor * diffuseColor) + specularSum * (Lcolor * specularColor);

	return output;
}

//////////////////////////////////////////////////////////////////////////////
// Compute shaded color for hair (diffuse + specular)
//////////////////////////////////////////////////////////////////////////////
inline float3 GFSDK_Hair_ComputeHairShading(
	float3						Lcolor,
	float3						Ldir,
	GFSDK_Hair_ShaderAttributes	attr,
	GFSDK_Hair_Material			mat,
	float3						hairColor
	)
{
	return GFSDK_Hair_ComputeHairShading(
		Lcolor, Ldir,
		attr.V, attr.N, attr.T,
		hairColor,
		mat.specularColor.rgb,
		mat.diffuseBlend,
		mat.specularPrimaryScale,
		mat.specularPrimaryPower,
		mat.specularSecondaryScale,
		mat.specularSecondaryPower,
		mat.specularSecondaryOffset);
}

//////////////////////////////////////////////////////////////////////////////
inline float GFSDK_Hair_SampleBilinear(
	float val00, float val10, float val01, float val11, float u, float v)
{
	float val0 = lerp(val00, val10, u);
	float val1 = lerp(val01, val11, u);
	float val  = lerp(val0, val1, v);
	return val;
}

//////////////////////////////////////////////////////////////////////////////
// Compute structured noise in 1D
//////////////////////////////////////////////////////////////////////////////
inline float GFSDK_Hair_ComputeStructuredNoise(
	float						noiseCount,
	float						seed,
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer
)
{
	// seed along hair length
	float hash = noiseCount * seed;
	float noiseSeed = floor(hash);
	float noiseFrac = hash - noiseSeed - 0.5f;
	
	// seed for neighboring sample
	float seedNeighbor = (noiseFrac < 0) ? noiseSeed - 1.0f : noiseSeed + 1.0f;
	seedNeighbor = max(0, seedNeighbor);

	// sample 4 noise values for bilinear interpolation
	float seedSample0 = noiseSeed;
	float seedSample1 = seedNeighbor;

	float noise0 = GFSDK_Hair_GetNormalizedNoise(seedSample0, hairConstantBuffer);
	float noise1 = GFSDK_Hair_GetNormalizedNoise(seedSample1, hairConstantBuffer);

	// interpolated noise sample
	float noise = lerp(noise0, noise1, abs(noiseFrac));

	// scale noise by user param
	return noise;
}

//////////////////////////////////////////////////////////////////////////////
// Compute glint (dirty sparkels) term
//////////////////////////////////////////////////////////////////////////////
inline float GFSDK_Hair_ComputeHairGlint(
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer,
	GFSDK_Hair_Material			mat,
	GFSDK_Hair_ShaderAttributes	attr
)
{
	// read material parameters
	float glintSize			= mat.glintCount;
	float glintPower		= mat.glintExponent;

	// seed along hair length
	float lengthHash = glintSize * attr.texcoords.z;
	float lengthSeed = floor(lengthHash);
	float lengthFrac = lengthHash - lengthSeed - 0.5f;
	
	// seed for neighboring sample
	float lengthSeedNeighbor = (lengthFrac < 0) ? lengthSeed - 1.0f : lengthSeed + 1.0f;
	lengthSeedNeighbor = max(0, lengthSeedNeighbor);

	// sample 4 noise values for bilinear interpolation
	float seedSample0 = attr.hairID + lengthSeed;
	float seedSample1 = attr.hairID + lengthSeedNeighbor;

	float noise0 = GFSDK_Hair_GetNormalizedNoise(seedSample0, hairConstantBuffer);
	float noise1 = GFSDK_Hair_GetNormalizedNoise(seedSample1, hairConstantBuffer);

	// interpolated noise sample
	float noise = lerp(noise0, noise1, abs(lengthFrac));

	// apply gamma like power function
	noise = pow(noise, glintPower);

	// scale noise by user param
	return noise;
}

//////////////////////////////////////////////////////////////////////////////
// Compute diffuse shading term only (no albedo is used)
//////////////////////////////////////////////////////////////////////////////
inline float GFSDK_Hair_ComputeHairDiffuseShading(
	float3		Ldir, // light direction
	float3		T,
	float3		N,
	float		diffuseScale,
	float		diffuseBlend
	)
{
	// diffuse hair shading
	float TdotL = clamp(dot( T , Ldir), -1.0f, 1.0f);

	float diffuseSkin = max(0, dot( N, Ldir));
	float diffuseHair = sqrt( 1.0f - TdotL*TdotL );

	float diffuse = lerp(diffuseHair, diffuseSkin, diffuseBlend);
	float result = diffuseScale * saturate(diffuse);

	return max(0,result);
}

//////////////////////////////////////////////////////////////////////////////
// Compute specular shading term only
//////////////////////////////////////////////////////////////////////////////
inline float GFSDK_Hair_ComputeHairSpecularShading(
	float		hairID,

	float3		Ldir, 
	float3		V,
	float3		T,
	float3		N,

	float		primaryScale,
	float		primaryShininess,
	float		secondaryScale,
	float		secondaryShininess,
	float		secondaryOffset,
	float		diffuseBlend,

	float		primaryBreakup = 0.0f

	)
{
	uint	hash = asuint(hairID * 17938401.0f);
	float	noiseVal = float(hash % 1024) / 1024.0f;
	float	signedNoise = noiseVal - 0.5f;

	float specPrimaryOffset = primaryBreakup * signedNoise;

	// primary specular
	float3 H				= normalize(V + Ldir);
	float TdotH				= clamp(dot(T, H), -1.0f, 1.0f);

	float TdotHshifted		= clamp(TdotH + specPrimaryOffset, -1.0f, 1.0f);
	float specPrimary		= sqrt(1.0f - TdotHshifted*TdotHshifted);

	specPrimary				= pow(max(0, specPrimary), primaryShininess);

	// secondary
	TdotH					= clamp(TdotH + secondaryOffset, -1.0, 1.0);
	float specSecondary		= sqrt(1 - TdotH*TdotH);
	specSecondary			= pow(max(0, specSecondary), secondaryShininess);

	// specular sum
	float specularSum = primaryScale * specPrimary + secondaryScale * specSecondary;

	// visibility due to diffuse normal
	float visibilityScale = lerp(1.0f, saturate(dot(N, Ldir)), diffuseBlend);
	specularSum *= visibilityScale;

	return max(0,specularSum);
}

//////////////////////////////////////////////////////////////////////////////
// Compute specular shading term only
//////////////////////////////////////////////////////////////////////////////
inline float GFSDK_Hair_ComputeHairSpecularShading(
	float3 Ldir, // light direction
	GFSDK_Hair_ShaderAttributes	attr,
	GFSDK_Hair_Material			mat,
	
	float glint = 0.0f)
{
	return GFSDK_Hair_ComputeHairSpecularShading(
		attr.hairID, 

		Ldir, 
		attr.V, attr.T, attr.N, 		
		mat.specularPrimaryScale,
		mat.specularPrimaryPower,
		mat.specularSecondaryScale,
		mat.specularSecondaryPower,
		mat.specularSecondaryOffset,
		mat.diffuseBlend,
		mat.specularPrimaryBreakup
		);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Computes blending factor between root and tip
//////////////////////////////////////////////////////////////////////////////////////////////
inline float GFSDK_Hair_GetRootTipRatio(const float s,  GFSDK_Hair_Material mat)
{
	float ratio = s;

	// add bias for root/tip color variation
	if (mat.rootTipColorWeight < 0.5f)
	{
		float slope = 2.0f * mat.rootTipColorWeight;
		ratio = slope * ratio;
	}
	else
	{
		float slope = 2.0f * (1.0f - mat.rootTipColorWeight) ;
		ratio = slope * (ratio - 1.0f) + 1.0f;
	}

	// modify ratio for falloff
	float slope = 1.0f / (mat.rootTipColorFalloff + 0.001f);
	ratio = saturate(0.5f + slope * (ratio - 0.5f));

	return ratio;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Returns hair color from textures for this hair fragment.
//////////////////////////////////////////////////////////////////////////////////////////////
inline float3 GFSDK_Hair_SampleHairColorTex(
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer,
	GFSDK_Hair_Material			mat, 
	SamplerState				texSampler, 
	Texture2D					rootColorTex, 
	Texture2D					tipColorTex, 
	float3						texcoords)
{
	float3 rootColor = mat.rootColor.rgb;
	float3 tipColor = mat.tipColor.rgb;

	if (hairConstantBuffer.useRootColorTexture)
		rootColor *= rootColorTex.SampleLevel(texSampler, float2(texcoords.x, 1-texcoords.y), 0);
	if (hairConstantBuffer.useTipColorTexture)
		tipColor *= tipColorTex.SampleLevel(texSampler, float2(texcoords.x, 1-texcoords.y), 0);

	float ratio = GFSDK_Hair_GetRootTipRatio(texcoords.z, mat);

	float3 hairColor = lerp(rootColor, tipColor, ratio);

	return hairColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Returns hair color from textures for this hair fragment.
//////////////////////////////////////////////////////////////////////////////////////////////
inline float3 GFSDK_Hair_SampleHairColorStrandTex(
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer,
	GFSDK_Hair_Material			mat, 
	SamplerState				texSampler, 
	Texture2D					rootColorTex, 
	Texture2D					tipColorTex, 
	Texture2D					strandColorTex, 
	float4						texcoords)
{
	float3 rootColor = mat.rootColor.rgb;
	float3 tipColor = mat.tipColor.rgb;

	if (hairConstantBuffer.useRootColorTexture)
		rootColor = (SAMPLE_LEVEL( rootColorTex, texSampler, texcoords.xy, 0 )).rgb;  
	if (hairConstantBuffer.useTipColorTexture)
		tipColor = (SAMPLE_LEVEL( tipColorTex, texSampler, texcoords.xy, 0 )).rgb;  

	float ratio = GFSDK_Hair_GetRootTipRatio(texcoords.z, mat);

	float3 hairColor = lerp(rootColor, tipColor, ratio);

	if (hairConstantBuffer.useStrandTexture)
	{
		float3 strandColor = (SAMPLE_LEVEL( strandColorTex, texSampler, texcoords.zw, 0 )).rgb;  

		switch(hairConstantBuffer.strandBlendMode)
		{
			case 0:
				hairColor = mat.strandBlendScale * strandColor;
				break;
			case 1:
				hairColor = lerp(hairColor, hairColor * strandColor, mat.strandBlendScale);
				break;
			case 2:
				hairColor += mat.strandBlendScale * strandColor;
				break;
			case 3:
				hairColor += mat.strandBlendScale * (strandColor - 0.5f);
				break;
		}
	}

	return hairColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Returns blended hair color between root and tip color
//////////////////////////////////////////////////////////////////////////////////////////////
float3 GFSDK_Hair_SampleHairColor(GFSDK_Hair_Material mat, float4 texcoords)
{
	float3 rootColor = mat.rootColor.rgb;
	float3 tipColor = mat.tipColor.rgb;

	float ratio = GFSDK_Hair_GetRootTipRatio(texcoords.z, mat);

	float3 hairColor = lerp(rootColor, tipColor, ratio);

	return hairColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Computes target alpha based on hair length alpha control
//////////////////////////////////////////////////////////////////////////////////////////////
float GFSDK_Hair_ComputeAlpha(
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer,
	GFSDK_Hair_Material			mat,
	GFSDK_Hair_ShaderAttributes attr 
	)
{
	float lengthScale = attr.texcoords.z;

	float rootWeight = saturate( (lengthScale + FLT_EPSILON) / (mat.rootAlphaFalloff + FLT_EPSILON));
	float rootAlpha = lerp(0.0f, 1.0f, rootWeight);

	float lodAlpha = 1.0f - hairConstantBuffer.lodAlphaFactor;

	float alpha = rootAlpha * lodAlpha;

	return alpha;
}

/////////////////////////////////////////////////////////////////////////////////////////////
inline float GFSDK_HAIR_SoftDepthCmpGreater(float sampledDepth, float calcDepth)
{
	return max(0.0, sampledDepth - calcDepth);
}

inline float GFSDK_HAIR_SoftDepthCmpLess(float sampledDepth, float calcDepth)
{
	return max(0.0, calcDepth - sampledDepth);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Compute hair to hair shadow
//////////////////////////////////////////////////////////////////////////////////////////////
float GFSDK_Hair_ShadowPCF(
	float2 texcoord, 
	float calcDepth, 
	SamplerState texSampler, 
	Texture2D shadowTexture, 
	int shadowUseLeftHanded)
{
	float shadow = 0;
	float wsum = 0;

	float w, h;
	uint numMipLevels;
	shadowTexture.GetDimensions(0, w, h, numMipLevels);

	float invResolution = 1.0f / float(w);

	[unroll]
	for (int dx = - 1; dx <= 1; dx ++) {
		for (int dy = -1; dy <= 1; dy ++) {
			
			float w = 1.0f / (1.0f + dx * dx + dy * dy);
			float2 coords = texcoord + float2(float(dx) * invResolution, float(dy) * invResolution);

			float sampleDepth = SAMPLE_LEVEL(shadowTexture, texSampler, coords, 0).r;  
			float shadowDepth = 0;
			if (shadowUseLeftHanded == 0)
				shadowDepth = GFSDK_HAIR_SoftDepthCmpGreater(sampleDepth, calcDepth);
			else shadowDepth = GFSDK_HAIR_SoftDepthCmpLess(sampleDepth, calcDepth);

			shadow += w * shadowDepth;
			wsum += w;
		}
	}
	 
	float s = shadow / wsum;
	return s;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Use gain = 1.0 for R.H.S light camera (depth is greater for closer object)
// Use gain = -1.0f for L.H.S light camera (depth is greater for far object)
/////////////////////////////////////////////////////////////////////////////////////////////
inline float GFSDK_Hair_ShadowPenetrationDepth(float sampledDepth, float calcDepth, float gain = -1.0f)
{
	return max(0.0f, gain * (sampledDepth - calcDepth));
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Given a shadow texture that stores linear depth, we filter depth by comparing against stored depth of neigbhors of the point.
// Use gain = 1.0 for R.H.S light camera (depth is greater for closer object)
// Use gain = -1.0f for L.H.S light camera (depth is greater for far object)
/////////////////////////////////////////////////////////////////////////////////////////////
inline float GFSDK_Hair_ShadowFilterDepth(Texture2D shadowTexture, SamplerState texSampler, float2 texcoord, float depth, float gain = -1.0f )
{
	float filteredDepth = 0;
	float n = 0;

	[unroll]
	for (int dx = - 1; dx <= 1; dx += 2) {
		for (int dy = -1; dy <= 1; dy += 2) {

		    float4 S = shadowTexture.Gather(texSampler, texcoord, int2(dx, dy));

			filteredDepth += GFSDK_Hair_ShadowPenetrationDepth(S.x, depth, gain);
			filteredDepth += GFSDK_Hair_ShadowPenetrationDepth(S.y, depth, gain);
			filteredDepth += GFSDK_Hair_ShadowPenetrationDepth(S.z, depth, gain);
			filteredDepth += GFSDK_Hair_ShadowPenetrationDepth(S.w, depth, gain);

			n += 4;
		}
	}
	 
	return filteredDepth / n;
}

/////////////////////////////////////////////////////////////////////////////////////////////
inline float GFSDK_Hair_ShadowLitFactor(
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer,
	GFSDK_Hair_Material			mat, 
	float						filteredDepth)
{
	if (!hairConstantBuffer.receiveShadows)
		return 1.0f;

	return exp( -filteredDepth * mat.shadowSigma);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Compute some visualization color option
//////////////////////////////////////////////////////////////////////////////////////////////
bool GFSDK_Hair_VisualizeColor(
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer,
	GFSDK_Hair_Material			hairMaterial,
	GFSDK_Hair_ShaderAttributes attr,
	inout float3				outColor
	)
{
	switch (hairConstantBuffer.colorizeMode)
	{
	case 1: // LOD
		{
			float3 zeroColor = float3(0.0, 0.0f, 1.0f);
			float3 distanceColor	= float3(1.0f, 0.0f, 0.0f);
			float3 detailColor	= float3(0.0f, 1.0f, 0.0f);
			float3 alphaColor	= float3(1.0f, 1.0f, 0.0f);

			float distanceFactor = hairConstantBuffer.lodDistanceFactor;
			float detailFactor = hairConstantBuffer.lodDetailFactor;
			float alphaFactor = hairConstantBuffer.lodAlphaFactor;

			outColor.rgb = zeroColor;

			if (distanceFactor > 0.0f)
				outColor.rgb = lerp(zeroColor, distanceColor, distanceFactor);

			if (alphaFactor > 0.0f)
				outColor.rgb = lerp(zeroColor, alphaColor, alphaFactor);

			if (detailFactor > 0.0f)
				outColor.rgb = lerp(zeroColor, detailColor, detailFactor);

			break;
		}
	case 2: // tangent
		{
			outColor.rgb = 0.5f + 0.5f * attr.T.xyz; // colorize hair with its tangnet vector
			break;
		}
	case 3: // normal
		{
			outColor.rgb = 0.5f + 0.5f * attr.N.xyz; // colorize hair with its normal vector
			break;
		}
	default:
		return false;
	}

	return true; // color computed 
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Convert screen space position (SV_Position) to clip space
//////////////////////////////////////////////////////////////////////////////////////////////
inline float4 GFSDK_Hair_ScreenToClip(float4 input, GFSDK_Hair_ConstantBuffer hairConstantBuffer)
{
	float4 sp;

	// convert to ndc
	sp.xy = mul( float4(input.x, input.y, 0.0f, 1.0f), hairConstantBuffer.inverseViewport).xy;
	sp.zw = input.zw;

	// undo perspective division to get clip
	sp.xyz *= input.w; 

	return sp;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Convert screen space position (SV_Position) to view space position
//////////////////////////////////////////////////////////////////////////////////////////////
inline float4 GFSDK_Hair_ScreenToView(float4 pixelPosition, GFSDK_Hair_ConstantBuffer hairConstantBuffer)
{
	float4 ndc = GFSDK_Hair_ScreenToClip(pixelPosition, hairConstantBuffer);
	return mul( ndc, hairConstantBuffer.inverseProjection);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Convert screen space position (SV_Position) to world space position
//////////////////////////////////////////////////////////////////////////////////////////////
inline float3 GFSDK_Hair_ScreenToWorld(float4 pixelPosition, GFSDK_Hair_ConstantBuffer	hairConstantBuffer)
{
	float4 wp = mul(float4(pixelPosition.xyz, 1.0f), hairConstantBuffer.inverseViewProjectionViewport);
	wp.xyz /= wp.w;
	return wp.xyz;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// DO NOT MODIFY FUNCTIONS BELOW
// THESE ARE RESERVED FOR HAIRWORKS INTERNAL USE AND SUBJECT TO CHANGE FREQUENTLY WITHOUT NOTICE
/////////////////////////////////////////////////////////////////////////////////////////////
inline float GFSDK_Hair_PackFloat2(float2 v)
{
	const float base = 2048;

	float basey = floor(base * v.y);
	float packed = basey + v.x;

	return packed;
}

inline float2 GFSDK_Hair_UnpackFloat2(float packedVal)
{
	const float inv_base = 1.0f / 2048.0f;

	float ubase = floor(packedVal);
	float unpackedy = ubase * inv_base;
	float unpackedx = packedVal - ubase;

	return float2(unpackedx, unpackedy);
}

inline float GFSDK_Hair_PackSignedFloat(float x)
{
	return 0.5f + 0.5f * clamp(x, -1.0, 1.0);
}

inline float GFSDK_Hair_PackSignedFloat2(float2 v)
{
	float sx = GFSDK_Hair_PackSignedFloat(v.x);
	float sy = GFSDK_Hair_PackSignedFloat(v.y);

	return GFSDK_Hair_PackFloat2(float2(sx,sy));
}



inline float GFSDK_Hair_UnpackSignedFloat(float x)
{
	return clamp(2.0f * (x - 0.5f), -1.0f, 1.0f);
}

inline float2 GFSDK_Hair_UnpackSignedFloat2(float x)
{
	float2 unpacked = GFSDK_Hair_UnpackFloat2(x);
	float sx = GFSDK_Hair_UnpackSignedFloat(unpacked.x);
	float sy = GFSDK_Hair_UnpackSignedFloat(unpacked.y);

	return float2(sx, sy);
}


//////////////////////////////////////////////////////////////////////////////
// input to this pixel shader (output from eariler geometry/vertex stages)
// These values are packed/compressed to minimize shader overhead
// Do NOT use these values directly from your shader. Use GFSDK_HairGetShaderAttributes() below.
//////////////////////////////////////////////////////////////////////////////
struct GFSDK_Hair_PixelShaderInput
{
	float4						position		: SYS_POSITION;

	float						hairtex			: HAIR_TEX; 

	NOINTERPOLATION	float		compTexcoord	: COMP_TEXCOORD;

	NOINTERPOLATION		uint	primitiveID		: C;
	NOINTERPOLATION		float	coords			: COORDS;
};

//////////////////////////////////////////////////////////////////////////////
// MACRO - DO NOT USE.
//////////////////////////////////////////////////////////////////////////////
inline GFSDK_Hair_ShaderAttributes
__GFSDK_HAIR_GET_SHADER_ATTRIBUTES__(
	GFSDK_Hair_PixelShaderInput input, 
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer
	)
{
	GFSDK_Hair_ShaderAttributes attr;

	attr.P		= GFSDK_Hair_ScreenToWorld(input.position, hairConstantBuffer);

	attr.texcoords.xy = GFSDK_Hair_UnpackFloat2(input.compTexcoord.x);
	attr.texcoords.z = input.hairtex;
	attr.texcoords.w = 0.5f;

	attr.T = 0;
	attr.N = 0;

	attr.V = normalize(hairConstantBuffer.camPosition.xyz - attr.P);

	attr.hairID = floor(attr.texcoords.x * 2048 * 2048 + 2048 * attr.texcoords.y);

	return attr;
}

//////////////////////////////////////////////////////////////////////////////
// MACRO - DO NOT USE.
//////////////////////////////////////////////////////////////////////////////
#define GFSDK_HAIR_INDICES_BUFFER_TYPE	Buffer<float3>
#define GFSDK_HAIR_TANGENT_BUFFER_TYPE	Buffer<float4>
#define GFSDK_HAIR_NORMAL_BUFFER_TYPE	Buffer<float4>
#define DECLARE_INTERPOLATED_VAR(BUFFER, VAR, INDEX) \
	float3 VAR; \
	{ \
		float3 v0 = BUFFER.Load( INDEX[0] ).xyz; \
		float3 v1 = BUFFER.Load( INDEX[1] ).xyz; \
		float3 v2 = BUFFER.Load( INDEX[2] ).xyz; \
		VAR = coords.x * v0 + coords.y * v1 + (1.0f - coords.x - coords.y) * v2; \
	} 

/*
	This macro derived function fills all the attributes needed for hair shading.
	To get attributes for shaders, use this function defined as

	GFSDK_Hair_ShaderAttributes
	GFSDK_Hair_GetShaderAttributes(
		GFSDK_Hair_PixelShaderInput input, 
		GFSDK_Hair_ConstantBuffer	hairConstantBuffer
	);

	, where input is the pixel shader input and hairConstantBuffer is constant buffer defined for HairWorks.
	The output (GFSDK_Hair_ShaderAttributes) contains all the attributes needed for hair shading such as
		world position (P), 
		tangent (T), 
		surface normal (N), 
		view vector (V), 
		texture coordinates (texcoords)
		__GFSDK_HAIR_GET_SHADER_ATTRIBUTES__(INPUT, CBUFFER, GFSDK_HAIR_RESOUCES_VAR)

	*/
#define DECLARE_DEFAULT_SHADER_ATTRIBUTE_FUNC \
	inline GFSDK_Hair_ShaderAttributes \
	GFSDK_Hair_GetShaderAttributes( \
		const GFSDK_Hair_PixelShaderInput	input, \
		const GFSDK_Hair_ConstantBuffer		hairConstantBuffer) \
	{ \
		GFSDK_Hair_ShaderAttributes attr = __GFSDK_HAIR_GET_SHADER_ATTRIBUTES__(input, hairConstantBuffer); \
		\
		float		hairtex		= attr.texcoords.z; \
		const int	numPoints	= hairConstantBuffer.strandPointCount; \
		float		hairCoord	= hairtex * float(numPoints); \
		int			vertexID0	= floor(hairCoord); \
		int			vertexID1	= min(vertexID0 + 1, numPoints-1); \
		float		hairFrac	= hairCoord - float(vertexID0); \
		\
		int3 hairIndices = floor(GFSDK_HAIR_RESOURCE_FACE_HAIR_INDICES.Load(input.primitiveID)); \
		int3 rootIndices =  hairIndices * numPoints; \
		\
		int3	vertexIndices0 = rootIndices + int3(vertexID0, vertexID0, vertexID0); \
		int3	vertexIndices1 = rootIndices + int3(vertexID1, vertexID1, vertexID1); \
		\
		float2	coords = GFSDK_Hair_UnpackFloat2(input.coords); \
		\
		DECLARE_INTERPOLATED_VAR(GFSDK_HAIR_RESOURCE_TANGENTS, T0, vertexIndices0); \
		DECLARE_INTERPOLATED_VAR(GFSDK_HAIR_RESOURCE_TANGENTS, T1, vertexIndices1); \
		\
		DECLARE_INTERPOLATED_VAR(GFSDK_HAIR_RESOURCE_NORMALS, N0, vertexIndices0); \
		DECLARE_INTERPOLATED_VAR(GFSDK_HAIR_RESOURCE_NORMALS, N1, vertexIndices1); \
		\
		attr.T = normalize(lerp(T0, T1, hairFrac)); \
		attr.N = normalize(lerp(N0, N1, hairFrac)); \
		\
		return attr; \
	} 

//////////////////////////////////////////////////////////////////////////////////////////////
// Shader Resources need to be declared in first section of the shader. See sampler shader codes.
#define GFSDK_HAIR_DECLARE_SHADER_RESOURCES(SLOT0, SLOT1, SLOT2) \
	GFSDK_HAIR_INDICES_BUFFER_TYPE	GFSDK_HAIR_RESOURCE_FACE_HAIR_INDICES	: register(SLOT0); \
	GFSDK_HAIR_TANGENT_BUFFER_TYPE	GFSDK_HAIR_RESOURCE_TANGENTS			: register(SLOT1); \
	GFSDK_HAIR_NORMAL_BUFFER_TYPE	GFSDK_HAIR_RESOURCE_NORMALS				: register(SLOT2); \
	DECLARE_DEFAULT_SHADER_ATTRIBUTE_FUNC;

#endif // ifndef _CPP

#endif // end header
