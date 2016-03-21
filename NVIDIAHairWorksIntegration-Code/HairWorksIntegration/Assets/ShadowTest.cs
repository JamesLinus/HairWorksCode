using UnityEngine;
using UnityEngine.Rendering;

[RequireComponent(typeof(Camera))]
public class ShadowTest : MonoBehaviour {

	public bool visualizeShadowMap;
	public Light hairLight;
	HairLight Hlight;
	public GameObject obj;
	HairInstance instance;
	void Start()
	{
		if (hairLight == null)
			Debug.LogError ("No light specified to access shadow map.");
		else
		Hlight = hairLight.GetComponent<HairLight> ();
		instance = obj.GetComponent<HairInstance> ();
	}

	void OnRenderImage(RenderTexture src, RenderTexture dest)
	{
		if (visualizeShadowMap) {
			if (Hlight == null)
				Debug.LogError ("No Hair Light script detected on light.");
			else
			Graphics.Blit (Hlight.shadowTexture, dest);
		} else
			Graphics.Blit (instance.HairWorksShadowMap, dest);
	}
}

