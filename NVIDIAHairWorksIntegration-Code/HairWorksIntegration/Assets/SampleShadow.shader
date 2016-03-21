Shader "Hidden/SampleShadow"
{
	Properties
	{
		_MainTex ("Texture", 2D) = "white" {}
	}
	SubShader
	{
		// No culling or depth
		Cull Off ZWrite Off ZTest Always

		Pass
		{
			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag
			
			#include "UnityCG.cginc"
			#include "AutoLight.cginc"

			struct appdata
			{
				float4 vertex : POSITION;
				float2 uv : TEXCOORD0;
			};

			struct v2f
			{
				float2 uv : TEXCOORD0;
				float4 vertex : SV_POSITION;
			};

			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = mul(UNITY_MATRIX_MVP, v.vertex);
				o.uv = v.uv;
				return o;
			}
			
			sampler2D _MainTex;
			sampler2D MyShadowMap;

			fixed4 frag (v2f i) : SV_Target
			{

			float3 currentPos = worldPos.xyz;

			float4 viewZ = -viewPos.z; 
			float4 zNear = float4( viewZ >= _LightSplitsNear ); 
			float4 zFar = float4( viewZ < _LightSplitsFar ); 
			float4 weights = zNear * zFar; 

				//calculate shadow at this sample position
			float3 shadowCoord0 = mul(unity_World2Shadow[0], float4(currentPos,1)).xyz; 
			float3 shadowCoord1 = mul(unity_World2Shadow[1], float4(currentPos,1)).xyz; 
			float3 shadowCoord2 = mul(unity_World2Shadow[2], float4(currentPos,1)).xyz; 
			float3 shadowCoord3 = mul(unity_World2Shadow[3], float4(currentPos,1)).xyz;
			
			float4 shadowCoord = float4(shadowCoord0 * weights[0] + shadowCoord1 * weights[1] + shadowCoord2 * weights[2] + shadowCoord3 * weights[3],1); 
			
			//do shadow test and store the result				
			float shadowTerm = UNITY_SAMPLE_SHADOW(MyShadowMap, shadowCoord);
					
				float4 col = float4 (shadowTerm, shadowTerm, shadowTerm, 1);

				return col;
			}
			ENDCG
		}
	}
}
