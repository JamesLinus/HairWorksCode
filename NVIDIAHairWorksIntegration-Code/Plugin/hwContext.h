﻿#ifndef hwContext_h
#define hwContext_h

struct hwShaderData
{
    hwHShader handle;
    int ref_count;
    ID3D11PixelShader *shader;
    std::string path;

    hwShaderData() : handle(hwNullHandle), ref_count(0), shader(nullptr) {}
    void invalidate() { ref_count = 0; shader = nullptr; path.clear(); }
    operator bool() const { return shader != nullptr; }
};

struct hwAssetData
{
    hwHAsset handle;
    int ref_count;
    hwAssetID aid;
    std::string path;
    hwConversionSettings settings;

    hwAssetData() : handle(hwNullHandle), aid(hwNullAssetID), ref_count(0) {}
    void invalidate() { ref_count = 0; aid = hwNullAssetID; path.clear(); }
    operator bool() const { return aid != hwNullAssetID; }
};

struct hwInstanceData
{
    hwHInstance handle;
    hwInstanceID iid;
    hwHAsset hasset;
    bool cast_shadow;
    bool receive_shadow;

    hwInstanceData() : handle(hwNullHandle), iid(hwNullInstanceID), hasset(hwNullHandle), cast_shadow(false), receive_shadow(false) {}
    void invalidate() { iid = hwNullInstanceID; hasset = hwNullAssetID; cast_shadow = false; receive_shadow = false; }
    operator bool() const { return iid != hwNullInstanceID; }
};

enum hwELightType
{
    hwELightType_Directional,
    hwELightType_Point,
};

struct hwLightData
{
	gfsdk_float4x4 lightView;
	gfsdk_float4x4 lightWorldToTex;
    hwELightType type; int pad[3];
    hwFloat4 position; // w: range
    hwFloat4 direction;
    hwFloat4 color;
	int angle; int pad2[3];

    hwLightData()
		: lightView({1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f})
		, lightWorldToTex({ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f})
		, type(hwELightType_Directional)
        , position({ 0.0f, 0.0f, 0.0f, 0.0f })
        , direction({ 0.0f, 0.0f, 0.0f, 0.0f })
        , color({ 1.0f, 1.0f, 1.0f, 1.0 })
		, angle(180)
    {}
};


struct hwConstantBuffer
{
	hwFloat4 shAr;
	hwFloat4 shAg;
	hwFloat4 shAb;
	hwFloat4 shBr;
	hwFloat4 shBg;
	hwFloat4 shBb;
	hwFloat4 shC;
    int num_lights; int pad0[3];
    hwLightData lights[hwMaxLights];
    GFSDK_HairShaderConstantBuffer hw;
    hwConstantBuffer() : num_lights(0) {}
};



class hwContext
{
public:
    // メインスレッドと Unity のレンダリングスレッドは別である可能性があるため、
    // 外に見せる描画系関数はコマンドを積むだけにして、UnityRenderEvent() でそれを flush するという形式をとる。
    enum CommandID
    {
        CID_StepSimulation,
        CID_SetViewProjection,
        CID_SetRenderTarget,
        CID_SetShader,
        CID_SetLights,
        CID_Render,
        CID_RenderShadow,
		CID_SetSphericalHarmonics,
		CID_InitShadows
    };
    struct DrawCommandVP
    {
        CommandID command;
        float fov;
        hwMatrix view;
        hwMatrix proj;
    };
    struct DrawCommandRT
    {
        CommandID command;
        hwTexture *framebuffer;
        hwTexture *depthbuffer;
    };
    struct DrawCommandL
    {
        CommandID command;
        int num_lights;
        hwLightData lights[hwMaxLights];
    };
    struct DrawCommandI
    {
        CommandID command;
        int arg;
    };
    struct DrawCommandF
    {
        CommandID command;
        float arg;
    };

	struct DrawCommandSH
	{
		CommandID command;
		hwFloat4 shAr;
		hwFloat4 shAg;
		hwFloat4 shAb;
		hwFloat4 shBr;
		hwFloat4 shBg;
		hwFloat4 shBb;
		hwFloat4 shC;
		
	};

	struct DrawCommandShadow
	{
		CommandID command;
		int arg;
		ID3D11Texture2D* shadowMap;
		//int numShadowCasters;
	};

	struct DrawCommandInitShadows
	{
		CommandID command;
		int rersolution;
		ID3D11Texture2D *shadowMap;
	};



public:
    hwContext();
    ~hwContext();
    bool valid() const;

    bool initialize(const char *path_to_dll, hwDevice *d3d_device);
    void finalize();
    void move(hwContext &from);
	void prepareShadowTarget();

    hwHShader       shaderLoadFromFile(const std::string &path);
    void            shaderRelease(hwHShader hs);
    void            shaderReload(hwHShader hs);

    hwHAsset        assetLoadFromFile(const std::string &path, const hwConversionSettings *conv);
    void            assetRelease(hwHAsset ha);
    void            assetReload(hwHAsset ha);
    int             assetGetNumBones(hwHAsset ha) const;
    const char*     assetGetBoneName(hwHAsset ha, int nth) const;
    void            assetGetBoneIndices(hwHAsset ha, hwFloat4 &o_indices) const;
    void            assetGetBoneWeights(hwHAsset ha, hwFloat4 &o_weight) const;
    void            assetGetBindPose(hwHAsset ha, int nth, hwMatrix &o_mat);
    void            assetGetDefaultDescriptor(hwHAsset ha, hwHairDescriptor &o_desc) const;

    hwHInstance     instanceCreate(hwHAsset ha);
    void            instanceRelease(hwHInstance hi);
    void            instanceGetBounds(hwHInstance hi, hwFloat3 &o_min, hwFloat3 &o_max) const;
    void            instanceGetDescriptor(hwHInstance hi, hwHairDescriptor &desc) const;
    void            instanceSetDescriptor(hwHInstance hi, const hwHairDescriptor &desc);
    void            instanceSetTexture(hwHInstance hi, hwTextureType type, ID3D11Texture2D* tex);
    void            instanceUpdateSkinningMatrices(hwHInstance hi, int num_bones, hwMatrix *matrices);
    void            instanceUpdateSkinningDQs(hwHInstance hi, int num_bones, hwDQuaternion *dqs);

    void beginScene();
    void endScene();
    void setViewProjection(const hwMatrix &view, const hwMatrix &proj, float fov);
	void setSphericalHarmonics(const hwFloat4 &Ar, const hwFloat4 &Ag, const hwFloat4 &Ab, const hwFloat4 &Br, const hwFloat4 &Bg, const hwFloat4 &Bb, const hwFloat4 &C);
	void prepareRenderTarget();
	void initShadows(int resolution, ID3D11Texture2D *shadowMap);
    void setRenderTarget(hwTexture *framebuffer, hwTexture *depthbuffer);
    void setShader(hwHShader hs);
    void setLights(int num_lights, const hwLightData *lights);
    void render(hwHInstance hi);
	void renderShadow(hwHInstance hi, ID3D11Texture2D *shadowMap/*, int numShadowCasters*/);
    void stepSimulation(float dt);
    void flush();

private:
    hwShaderData&   newShaderData();
    hwAssetData&    newAssetData();
    hwInstanceData& newInstanceData();

    template<class T> void pushDrawCommand(const T &c);
    void setViewProjectionImpl(const hwMatrix &view, const hwMatrix &proj, float fov);
    void setRenderTargetImpl(hwTexture *framebuffer, hwTexture *depthbuffer);
    void setShaderImpl(hwHShader hs);
    void setLightsImpl(int num_lights, const hwLightData *lights);
    void renderImpl(hwHInstance hi);
    void renderShadowImpl(hwHInstance hi, ID3D11Texture2D *shadowMap/*, int numShadowCasters*/);
    void stepSimulationImpl(float dt);
	void setSphericalHarmonicsImpl(const hwFloat4 &Ar, const hwFloat4 &Ag, const hwFloat4 &Ab, const hwFloat4 &Br, const hwFloat4 &Bg, const hwFloat4 &Bb, const hwFloat4 &C);
	HRESULT initShadowsImpl(int resolution, ID3D11Texture2D *shadowMap);
    hwSRV* getSRV(hwTexture *tex);
    hwRTV* getRTV(hwTexture *tex);

private:
    typedef std::vector<hwShaderData>       ShaderCont;
    typedef std::vector<hwAssetData>        AssetCont;
    typedef std::vector<hwInstanceData>     InstanceCont;
    typedef std::map<hwTexture*, hwSRV*>    SRVTable;
    typedef std::map<hwTexture*, hwRTV*>    RTVTable;
    typedef std::vector<char>               DrawCommands;

    std::mutex              m_mutex;

    ID3D11Device            *m_d3ddev;
    ID3D11DeviceContext     *m_d3dctx;
    hwSDK                   *m_sdk;
    ShaderCont              m_shaders;
    AssetCont               m_assets;
    InstanceCont            m_instances;
    SRVTable                m_srvtable;
    RTVTable                m_rtvtable;
    DrawCommands            m_commands;
    DrawCommands            m_commands_back;

    ID3D11DepthStencilState *m_rs_enable_depth;
    ID3D11Buffer            *m_rs_constant_buffer;

	ID3D11Texture2D*			ShadowTexture;
	ID3D11RenderTargetView*		m_pShadowRTV;
	ID3D11ShaderResourceView*	m_pShadowSRV;

	ID3D11Texture2D*			m_pDepthTexture;
	ID3D11DepthStencilView*		m_pDepthDSV;

	ID3D11Texture2D*            m_UnityShadow;
	ID3D11ShaderResourceView*	m_UnityShadowSRV;

	D3D11_VIEWPORT				m_viewport;

	ID3D11ShaderResourceView*  m_ShadowTextures[8];

	ID3D11RenderTargetView*    m_BackBuffer;
	ID3D11DepthStencilView*    m_DepthStencil;

    hwConstantBuffer        m_cb;
};

#endif // hwContext_h
