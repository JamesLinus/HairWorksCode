Shader "Custom/Shadow"
{
	Properties
	{
		_MainTex ("Texture", 2D) = "white" {}
		_ShadowTex("Shadow Depth", 2D) = "white"{}
		_ShadowCam("Shadow Camera Texture", 2D) = "white"{}
		_Blend("Blend Strenght", float) = 1
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
			sampler2D _ShadowTex;
			sampler2D _ShadowCam;
			float _Blend;

			fixed4 frag (v2f i) : SV_Target
			{
				float col = tex2D(_ShadowTex, float2 ((i.uv.x/2), (i.uv.y/2))).r;
				fixed4 main = tex2D(_ShadowCam, i.uv);
				float3 newCol = lerp(main.rgb, float3(col, 0, 0), _Blend);
				return float4(newCol, 1);
			}
			ENDCG
		}
	}
}
