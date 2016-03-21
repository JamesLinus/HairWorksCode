using UnityEngine;
using System.Collections;
[ExecuteInEditMode]
public class HairWorksShadowManager : MonoBehaviour {

	// Use this for initialization
	RenderTexture dummy;
	void Start () {
		dummy = new RenderTexture (2048, 2048, 16, RenderTextureFormat.Depth);
		dummy.filterMode = FilterMode.Bilinear;
		dummy.useMipMap = false;
		dummy.generateMips = false;
		dummy.wrapMode = TextureWrapMode.Clamp;
		RenderTexture.active = dummy;
		GL.Begin (GL.TRIANGLES);
		GL.Clear (true, true, new Color (0.5f, 0.5f, 0.5f, 1));
		GL.End ();
		dummy.Create ();
		RenderTexture.active = null;
		//HairWorksIntegration.hwInitShadows (2048, dummy.GetNativeTexturePtr ());
		print ("Shadows Initialized");
	}
	
	// Update is called once per frame
	void Update () {
		
	}
}
