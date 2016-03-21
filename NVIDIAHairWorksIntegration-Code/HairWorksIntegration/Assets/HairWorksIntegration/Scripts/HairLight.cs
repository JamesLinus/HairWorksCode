using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Reflection;
using UnityEngine;
using UnityEngine.Rendering;
#if UNITY_EDITOR
using UnityEditor;
#endif


[AddComponentMenu("Hair Works Integration/Hair Light")]
[RequireComponent(typeof(Light))]
[ExecuteInEditMode]
public class HairLight : MonoBehaviour
{

	[System.Serializable] public class FocusSetup {
		public Transform target;
		public Vector3 offset;
		public float radius = 1f;
		public float depthBias = 0.0005f;
		public float sceneCaptureDistance = 50f;
	}

	public enum Dimension {
		x256	= 256,
		x512	= 512,
		x1024	= 1024,
		x2048	= 2048,
		x4096	= 4096,
		x8192	= 8192,
	}

    #region static
    static HashSet<HairLight> s_instances;
    static hwLightData[] s_light_data;
    static IntPtr s_light_data_ptr;

    static public HashSet<HairLight> GetInstances()
    {
        if (s_instances == null)
        {
            s_instances = new HashSet<HairLight>();
        }
        return s_instances;
    }

    static public void AssignLightData()
    {
        if(s_light_data == null)
        {
            s_light_data = new hwLightData[hwLightData.MaxLights];
            s_light_data_ptr = Marshal.UnsafeAddrOfPinnedArrayElement(s_light_data, 0);
        }

        var instances = GetInstances();
        int n = Mathf.Min(instances.Count, hwLightData.MaxLights);
        int i = 0;
        foreach (var l in instances)
        {
            s_light_data[i] = l.GetLightData();
            if(++i == n) { break; }
        }
        HairWorksIntegration.hwSetLights(n, s_light_data_ptr);
    }
    #endregion


    public enum Type
    {
        Directional,
        Point,
    }

    hwLightData m_data;
	[HideInInspector] public RenderTexture shadowTexture;
	Camera m_shadowCamera;
	public Matrix4x4 shadowMatrix = Matrix4x4.identity;
	public Matrix4x4 shadowViewMat = Matrix4x4.identity;
	public Matrix4x4 shadowProjMat = Matrix4x4.identity;
	public bool castsShadows;
	public bool visualizeShadowMap = false;
	public Dimension shadowMapSize = Dimension.x2048;
	public LayerMask inclusionMask;
	public Matrix4x4 m_shadowSpaceMatrix;
	public FocusSetup focus;
	public int FOV = 60;
	public bool useSceneCapture;
	public float cullingDistance;
    public bool m_copy_light_params = false;
    public LightType m_type = LightType.Directional;
    public float m_range = 10.0f;
    public Color m_color = Color.white;
    public float m_intensity = 1.0f;
	public Vector3 m_location = new Vector3(0, 0, 0);
	public Vector3 m_direction = new Vector3(0, 0, 0);
	public int m_angle = 180;
    CommandBuffer m_cb;

    public CommandBuffer GetCommandBuffer()
    {
        if(m_cb == null)
        {
            m_cb = new CommandBuffer();
            m_cb.name = "Hair Shadow";
			m_cb.SetGlobalTexture ("MyShadowMap,", new RenderTargetIdentifier (BuiltinRenderTextureType.CurrentActive));
            GetComponent<Light>().AddCommandBuffer(LightEvent.AfterShadowMap, m_cb);
        }
        return m_cb;
    }

    public hwLightData GetLightData()
    {
        var t = GetComponent<Transform>();
		m_data.lightView = shadowViewMat;
		m_data.lightWorldToTex = shadowMatrix;
        m_data.type = (int)m_type;
        m_data.range = m_range;
        m_data.color = new Color(m_color.r * m_intensity, m_color.g * m_intensity, m_color.b * m_intensity, 0.0f);
        m_data.position = t.position;
        m_data.direction = t.forward;
		m_data.angle = m_angle;
        return m_data;

    }

	void SetFocus() {


		if (GetComponent<Light> ().type == LightType.Directional)
			m_shadowCamera.orthographicSize = focus.radius;
		else
			m_shadowCamera.fieldOfView = FOV;
		m_shadowCamera.nearClipPlane = Camera.main.nearClipPlane;
		m_shadowCamera.farClipPlane = Camera.main.farClipPlane * 100;
		if (GetComponent<Light>().type == LightType.Directional)
		m_shadowCamera.projectionMatrix = GL.GetGPUProjectionMatrix(Matrix4x4.Ortho(-focus.radius, focus.radius, -focus.radius, focus.radius, 0f, focus.radius * 2f), false);

		float db = -focus.depthBias;
		m_shadowSpaceMatrix.SetRow(0, new Vector4(0.5f, 0.0f, 0.0f, 0.5f));
		m_shadowSpaceMatrix.SetRow(1, new Vector4(0.0f, 0.5f, 0.0f, 0.5f));
		m_shadowSpaceMatrix.SetRow(2, new Vector4(0.0f, 0.0f,   1f,   db));
		m_shadowSpaceMatrix.SetRow(3, new Vector4(0.0f, 0.0f, 0.0f, 1.0f));
	}



	void UpdateFocus() {
		
		var targetPos = focus.target.position + focus.target.right * focus.offset.x
			+ focus.target.up * focus.offset.y + focus.target.forward * focus.offset.z;
		var lightDir = -GetComponent<Light>().transform.forward;
		var lightOri = GetComponent<Light>().transform.rotation;
		if (GetComponent<Light> ().type != LightType.Directional) 
			m_shadowCamera.transform.position = GetComponent<Light> ().transform.position;
		else
			m_shadowCamera.transform.position = targetPos - lightDir * focus.radius;
		if (GetComponent<Light>().type != LightType.Point)
			m_shadowCamera.transform.rotation = lightOri;
		 else 
			m_shadowCamera.transform.LookAt (targetPos);
		
		float db = -focus.depthBias;
		m_shadowSpaceMatrix.SetRow(0, new Vector4(0.5f, 0.0f, 0.0f, 0.5f));
		m_shadowSpaceMatrix.SetRow(1, new Vector4(0.0f, 0.5f, 0.0f, 0.5f));
		m_shadowSpaceMatrix.SetRow(2, new Vector4(0.0f, 0.0f,   1f,   db));
		m_shadowSpaceMatrix.SetRow(3, new Vector4(0.0f, 0.0f, 0.0f, 1.0f));

		shadowViewMat = m_shadowCamera.worldToCameraMatrix;
		shadowProjMat = GL.GetGPUProjectionMatrix(m_shadowCamera.projectionMatrix, true);
		shadowMatrix = m_shadowSpaceMatrix * shadowProjMat * shadowViewMat;
	}

	bool CheckVisibility(Camera cam) {
		//UpdateAutoFocus(focus);

		var targetPos = focus.target.position + focus.target.right * focus.offset.x
			+ focus.target.up * focus.offset.y + focus.target.forward * focus.offset.z;
		var bounds = new Bounds(targetPos, Vector3.one);

		return (targetPos - cam.transform.position).sqrMagnitude < (cullingDistance * cullingDistance)
			&& GeometryUtility.TestPlanesAABB(GeometryUtility.CalculateFrustumPlanes(cam), bounds);
	}

	void AllocateTarget() {
		shadowTexture = new RenderTexture((int)shadowMapSize, (int)shadowMapSize, 16, RenderTextureFormat.RFloat, RenderTextureReadWrite.Linear);
		shadowTexture.filterMode = FilterMode.Point;
		shadowTexture.useMipMap = false;
		shadowTexture.generateMips = false;
		shadowTexture.wrapMode = TextureWrapMode.Clamp;
		shadowTexture.Create ();
		m_shadowCamera.targetTexture =  shadowTexture;

	}

	void ReleaseTarget() {
		m_shadowCamera.targetTexture = null;
		DestroyImmediate(shadowTexture);
		shadowTexture = null;
	}

	void OnDestroy() {
		if(m_shadowCamera)
			DestroyImmediate(m_shadowCamera.gameObject);
	}

	void OnValidate() {
		if(!Application.isPlaying || !m_shadowCamera)
			return;

		ReleaseTarget();
		AllocateTarget();
		m_shadowCamera.cullingMask = useSceneCapture ? (LayerMask)~0 : inclusionMask;
		SetFocus ();
	}

	void Awake(){ 
		shadowMatrix = Matrix4x4.identity;
		var shadowCameraGO = new GameObject("#> _Shadow Camera < " + this.name);
		shadowCameraGO.hideFlags = HideFlags.DontSave;
		m_shadowCamera = shadowCameraGO.AddComponent<Camera>();
		m_shadowCamera.renderingPath = RenderingPath.Forward;
		m_shadowCamera.clearFlags = CameraClearFlags.Depth;
		m_shadowCamera.depthTextureMode = DepthTextureMode.None;
		m_shadowCamera.useOcclusionCulling = false;
		if (GetComponent<Light> ().type == LightType.Directional)
			m_shadowCamera.orthographic = true;
		else
			m_shadowCamera.orthographic = false;
		m_shadowCamera.cullingMask = useSceneCapture ? (LayerMask)~0 : inclusionMask;
		m_shadowCamera.depth = -100;
		m_shadowCamera.aspect = 1f;
		m_shadowCamera.SetReplacementShader (Shader.Find ("Hidden/ShadowDepth"), "RenderType");
		m_shadowCamera.tag = "ShadowCamera";
		m_shadowCamera.enabled = false;
		SetFocus ();
	}

	public void RenderShadowDepth(){
		UpdateFocus ();
		m_shadowCamera.Render ();
	}

    void OnEnable()
    {
		AllocateTarget();
        GetInstances().Add(this);
        if(GetInstances().Count > hwLightData.MaxLights)
        {
            Debug.LogWarning("Max HairLight is " + hwLightData.MaxLights + ". Current active HairLight is " + GetInstances().Count);
        }

    }

    void OnDisable()
	{
		ReleaseTarget();
		GetInstances ().Remove (this);

    }

    void Update()
    {
        if(m_copy_light_params)
        {
			
            var l = GetComponent<Light>();
            m_type = l.type;
            m_range = l.range;
            m_color = l.color;
            m_intensity = l.intensity;
			if (l.type == LightType.Point || l.type == LightType.Spot)
			m_location = l.transform.position;
			if (l.type == LightType.Directional || l.type == LightType.Spot)
			m_direction = l.transform.forward;
			if (l.type == LightType.Spot)
				m_angle = (int)l.spotAngle;
        }
		UpdateFocus ();
    }

}
