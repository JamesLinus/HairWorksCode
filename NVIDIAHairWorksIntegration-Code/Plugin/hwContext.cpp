﻿#include "pch.h"
#include "HairWorksIntegration.h"
#include "hwContext.h"
#include<D3Dcompiler.h>



bool operator==(const hwConversionSettings &a, const hwConversionSettings &b)
{
#define cmp(V) a.V==b.V
    return cmp(m_targetUpAxisHint) && cmp(m_targetHandednessHint) && cmp(m_pConversionMatrix) && cmp(m_targetSceneUnit);
#undef cmp
}

bool hwFileToString(std::string &o_buf, const char *path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) { return false; }
    f.seekg(0, std::ios::end);
    o_buf.resize(f.tellg());
    f.seekg(0, std::ios::beg);
    f.read(&o_buf[0], o_buf.size());
    return true;
}



hwContext::hwContext()
    : m_d3ddev(nullptr)
    , m_d3dctx(nullptr)
    , m_sdk(nullptr)
    , m_rs_enable_depth(nullptr)
    , m_rs_constant_buffer(nullptr)
{
}

hwContext::~hwContext()
{
    finalize();
}

bool hwContext::valid() const
{
    return m_d3ddev!=nullptr && m_d3dctx!=nullptr && m_sdk!=nullptr;
}

bool hwContext::initialize(const char *path_to_dll, hwDevice *d3d_device)
{
    if (m_sdk != nullptr) { return true; }
    if (path_to_dll == nullptr || d3d_device == nullptr) { return false; }

    m_sdk = GFSDK_LoadHairSDK(path_to_dll, GFSDK_HAIRWORKS_VERSION);
    if (m_sdk != nullptr) {
        hwDebugLog("GFSDK_LoadHairSDK(\"%s\") succeeded.\n", path_to_dll);
    }
    else {
        hwDebugLog("GFSDK_LoadHairSDK(\"%s\") failed.\n", path_to_dll);
        return false;
    }

    if (m_sdk->InitRenderResources((ID3D11Device*)d3d_device) == GFSDK_HAIR_RETURN_OK) {
        hwDebugLog("GFSDK_HairSDK::InitRenderResources() succeeded.\n");
    }
    else {
        hwDebugLog("GFSDK_HairSDK::InitRenderResources() failed.\n");
        finalize();
        return false;
    }

    m_d3ddev = (ID3D11Device*)d3d_device;
    m_d3ddev->GetImmediateContext(&m_d3dctx);
    if (m_sdk->SetCurrentContext(m_d3dctx) == GFSDK_HAIR_RETURN_OK) {
        hwDebugLog("GFSDK_HairSDK::SetCurrentContext() succeeded.\n");
    }
    else {
        hwDebugLog("GFSDK_HairSDK::SetCurrentContext() failed.\n");
        finalize();
        return false;
    }

    {
        CD3D11_DEPTH_STENCIL_DESC desc;
        m_d3ddev->CreateDepthStencilState(&desc, &m_rs_enable_depth);
    }
    {
        // create constant buffer for hair rendering pixel shader
        D3D11_BUFFER_DESC desc;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = sizeof(hwConstantBuffer);
        desc.StructureByteStride = 0;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.MiscFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        m_d3ddev->CreateBuffer(&desc, 0, &m_rs_constant_buffer);

		
		

		std::fill_n(m_ShadowTextures, 8, (ID3D11ShaderResourceView*)NULL);
    }

    return true;
}

HRESULT hwContext::initShadowsImpl(int resolution, ID3D11Texture2D* tex){
	
	if (ShadowTexture){
		ShadowTexture->Release();
		ShadowTexture = NULL;
	}
	if (m_pShadowSRV){
		m_pShadowSRV->Release();
		m_pShadowSRV = NULL;
	}
	if (m_pShadowRTV){
		m_pShadowRTV->Release();
		m_pShadowRTV = NULL;
	}
	if (m_pDepthTexture){
		m_pDepthTexture->Release();
		m_pDepthTexture = NULL;
	}
	if (m_pDepthDSV){
		m_pDepthDSV->Release();
		m_pDepthDSV = NULL;
	}
	// create shadow render target
	{
		/*D3D11_TEXTURE2D_DESC texDesc;
		tex->GetDesc(&texDesc);
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		ZeroMemory(&SRVDesc, sizeof(SRVDesc));
		SRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.MipLevels = 1;*/
		//m_d3dctx->CopyResource(m_UnityShadow, tex);
		//if (FAILED(m_d3ddev->CreateShaderResourceView(m_UnityShadow, &SRVDesc, &m_UnityShadowSRV)))
			//return S_FALSE;
		D3D11_TEXTURE2D_DESC ShadowDesc;
		ZeroMemory(&ShadowDesc, sizeof(ShadowDesc));
		ShadowDesc.Format = DXGI_FORMAT_R32_FLOAT;
		ShadowDesc.Width = UINT(resolution);
		ShadowDesc.Height = UINT(resolution);
		ShadowDesc.ArraySize = 1;
		ShadowDesc.MiscFlags = 0;
		ShadowDesc.MipLevels = 1;
		ShadowDesc.SampleDesc.Count = 1;
		ShadowDesc.SampleDesc.Quality = 0;
		ShadowDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		ShadowDesc.Usage = D3D11_USAGE_DEFAULT;
		ShadowDesc.CPUAccessFlags = 0;
		if (FAILED(m_d3ddev->CreateTexture2D(&ShadowDesc, NULL, &ShadowTexture)))
			return S_FALSE;
		if (FAILED(m_d3ddev->CreateShaderResourceView(ShadowTexture, NULL, &m_pShadowSRV)))
			return S_FALSE;
		if (FAILED(m_d3ddev->CreateRenderTargetView(ShadowTexture, NULL, &m_pShadowRTV)))
			return S_FALSE;

	}

	if (tex){
		tex->Release();
		tex = NULL;
	}
	// create shadow depth stencil
	{
		D3D11_TEXTURE2D_DESC desc;
		desc.Format = DXGI_FORMAT_R32_TYPELESS;
		desc.Width = UINT(resolution);
		desc.Height = UINT(resolution);
		desc.ArraySize = 1;
		desc.MiscFlags = 0;
		desc.MipLevels = 1;

		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.CPUAccessFlags = 0;

		if (FAILED(m_d3ddev->CreateTexture2D(&desc, NULL, &m_pDepthTexture)))
			return S_FALSE;

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = 0;
		dsvDesc.Texture2D.MipSlice = 0;

		if (FAILED(m_d3ddev->CreateDepthStencilView(m_pDepthTexture, &dsvDesc, &m_pDepthDSV)))
			return S_FALSE;
	}
	m_d3dctx->OMGetRenderTargets(1, &m_BackBuffer, &m_DepthStencil);
	return S_OK;
}

void hwContext::prepareShadowTarget(){
	m_d3dctx->OMSetRenderTargets(1, &m_pShadowRTV, m_pDepthDSV);
	float clearDepth = FLT_MAX;
	float ClearColor[4] = { clearDepth, clearDepth, clearDepth, clearDepth };
	m_d3dctx->ClearRenderTargetView(m_pShadowRTV, ClearColor);
	m_d3dctx->ClearDepthStencilView(m_pDepthDSV, D3D11_CLEAR_DEPTH, 1.0, 0);
}

void hwContext::prepareRenderTarget(){
	m_d3dctx->OMSetRenderTargets(1, &m_BackBuffer, m_DepthStencil);
}

void hwContext::finalize()
{
    for (auto &i : m_instances) { instanceRelease(i.handle); }
    m_instances.clear();

    for (auto &i : m_assets) { assetRelease(i.handle); }
    m_assets.clear();

    for (auto &i : m_shaders) { shaderRelease(i.handle); }
    m_shaders.clear();

    for (auto &i : m_srvtable) { i.second->Release(); }
    m_srvtable.clear();

    for (auto &i : m_rtvtable) { i.second->Release(); }
    m_rtvtable.clear();

    if (m_rs_enable_depth) {
        m_rs_enable_depth->Release();
        m_rs_enable_depth = nullptr;
    }

    if (m_sdk) {
        m_sdk->Release();
        m_sdk = nullptr;
    }
    if (m_d3dctx)
    {
        m_d3dctx->Release();
        m_d3dctx = nullptr;
    }

}

void hwContext::move(hwContext &from)
{
#define mov(V) V=from.V; from.V=decltype(V)();

    mov(m_sdk);
    mov(m_d3dctx);
    mov(m_d3ddev);

    mov(m_shaders);
    mov(m_assets);
    mov(m_instances);
    mov(m_srvtable);
    mov(m_rtvtable);
    //mov(m_commands);

    mov(m_rs_enable_depth);
    mov(m_rs_constant_buffer);
    //mov(m_cb);

#undef mov
}


hwShaderData& hwContext::newShaderData()
{
    auto i = std::find_if(m_shaders.begin(), m_shaders.end(), [](const hwShaderData &v) { return !v; });
    if (i != m_shaders.end()) { return *i; }

    hwShaderData tmp;
    tmp.handle = m_shaders.size();
    m_shaders.push_back(tmp);
    return m_shaders.back();
}

hwHShader hwContext::shaderLoadFromFile(const std::string &path)
{
	{
		auto i = std::find_if(m_shaders.begin(), m_shaders.end(), [&](const hwShaderData &v) { return v.path == path; });
		if (i != m_shaders.end() && i->ref_count > 0) {
			++i->ref_count;
			return i->handle;
		}
	}

	std::string bin;
	if (!hwFileToString(bin, path.c_str())) {
		hwDebugLog("failed to load shader (%s)\n", path.c_str());
		return hwNullHandle;
	}

	hwShaderData& v = newShaderData();
	v.path = path;
	if (SUCCEEDED(m_d3ddev->CreatePixelShader(&bin[0], bin.size(), nullptr, &v.shader))) {
		v.ref_count = 1;
		hwDebugLog("CreatePixelShader(%s) : %d succeeded.\n", path.c_str(), v.handle);
		return v.handle;
	}
	else {
		hwDebugLog("CreatePixelShader(%s) failed.\n", path.c_str());
	}
	return hwNullHandle;
}


void hwContext::shaderRelease(hwHShader hs)
{
    if (hs >= m_shaders.size()) { return; }

    auto &v = m_shaders[hs];
    if (v.ref_count > 0 && --v.ref_count == 0) {
        v.shader->Release();
        v.invalidate();
        hwDebugLog("shaderRelease(%d)\n", hs);
    }
}

void hwContext::shaderReload(hwHShader hs)
{
    if (hs >= m_shaders.size()) { return; }

    auto &v = m_shaders[hs];
    // release existing shader
    if (v.shader) {
        v.shader->Release();
        v.shader = nullptr;
    }

    // reload
    std::string bin;
    if (!hwFileToString(bin, v.path.c_str())) {
        hwDebugLog("failed to reload shader (%s)\n", v.path.c_str());
        return;
    }
    if (SUCCEEDED(m_d3ddev->CreatePixelShader(&bin[0], bin.size(), nullptr, &v.shader))) {
        hwDebugLog("CreatePixelShader(%s) : %d reloaded.\n", v.path.c_str(), v.handle);
    }
    else {
        hwDebugLog("CreatePixelShader(%s) failed to reload.\n", v.path.c_str());
    }
}


hwAssetData& hwContext::newAssetData()
{
    auto i = std::find_if(m_assets.begin(), m_assets.end(), [](const hwAssetData &v) { return !v; });
    if (i != m_assets.end()) { return *i; }

    hwAssetData tmp;
    tmp.handle = m_assets.size();
    m_assets.push_back(tmp);
    return m_assets.back();
}

hwHAsset hwContext::assetLoadFromFile(const std::string &path, const hwConversionSettings *_settings)
{
    hwConversionSettings settings;
    if (_settings != nullptr) { settings = *_settings; }

    {
        auto i = std::find_if(m_assets.begin(), m_assets.end(),
            [&](const hwAssetData &v) { return v.path == path && v.settings==settings; });
        if (i != m_assets.end() && i->ref_count > 0) {
            ++i->ref_count;
            return i->aid;
        }
    }

    hwAssetData& v = newAssetData();
    v.settings = settings;
    v.path = path;
    if (m_sdk->LoadHairAssetFromFile(path.c_str(), &v.aid, nullptr, &settings) == GFSDK_HAIR_RETURN_OK) {
        v.ref_count = 1;

        hwDebugLog("GFSDK_HairSDK::LoadHairAssetFromFile(\"%s\") : %d succeeded.\n", path.c_str(), v.handle);
        return v.handle;
    }
    else {
        hwDebugLog("GFSDK_HairSDK::LoadHairAssetFromFile(\"%s\") failed.\n", path.c_str());
    }
    return hwNullHandle;
}

void hwContext::assetRelease(hwHAsset ha)
{
    if (ha >= m_assets.size()) { return; }

    auto &v = m_assets[ha];
    if (v.ref_count > 0 && --v.ref_count==0) {
        if (m_sdk->FreeHairAsset(v.aid) == GFSDK_HAIR_RETURN_OK) {
            hwDebugLog("GFSDK_HairSDK::FreeHairAsset(%d) succeeded.\n", ha);
        }
        else {
            hwDebugLog("GFSDK_HairSDK::FreeHairAsset(%d) failed.\n", ha);
        }
        v.invalidate();
    }
}

void hwContext::assetReload(hwHAsset ha)
{
    if (ha >= m_assets.size()) { return; }

    auto &v = m_assets[ha];
    // release existing asset
    if (m_sdk->FreeHairAsset(v.aid)) {
        v.aid = hwNullAssetID;
    }

    // reload
    if (m_sdk->LoadHairAssetFromFile(v.path.c_str(), &v.aid, nullptr, &v.settings) == GFSDK_HAIR_RETURN_OK) {
        hwDebugLog("GFSDK_HairSDK::LoadHairAssetFromFile(\"%s\") : %d reloaded.\n", v.path.c_str(), v.handle);
    }
    else {
        hwDebugLog("GFSDK_HairSDK::LoadHairAssetFromFile(\"%s\") failed to reload.\n", v.path.c_str());
    }
}

int hwContext::assetGetNumBones(hwHAsset ha) const
{
    uint32_t r = 0;
    if (ha >= m_assets.size()) { return r; }

    if (m_sdk->GetNumBones(m_assets[ha].aid, &r) != GFSDK_HAIR_RETURN_OK) {
        hwDebugLog("GFSDK_HairSDK::GetNumBones(%d) failed.\n", ha);
    }
    return r;
}

const char* hwContext::assetGetBoneName(hwHAsset ha, int nth) const
{
    static char tmp[256];
    if (ha >= m_assets.size()) { tmp[0] = '\0'; return tmp; }

    if (m_sdk->GetBoneName(m_assets[ha].aid, nth, tmp) != GFSDK_HAIR_RETURN_OK) {
        hwDebugLog("GFSDK_HairSDK::GetBoneName(%d) failed.\n", ha);
    }
    return tmp;
}

void hwContext::assetGetBoneIndices(hwHAsset ha, hwFloat4 &o_indices) const
{
    if (ha >= m_assets.size()) { return; }

    if (m_sdk->GetBoneIndices(m_assets[ha].aid, &o_indices) != GFSDK_HAIR_RETURN_OK) {
        hwDebugLog("GFSDK_HairSDK::GetBoneIndices(%d) failed.\n", ha);
    }
}

void hwContext::assetGetBoneWeights(hwHAsset ha, hwFloat4 &o_weight) const
{
    if (ha >= m_assets.size()) { return; }

    if (m_sdk->GetBoneWeights(m_assets[ha].aid, &o_weight) != GFSDK_HAIR_RETURN_OK) {
        hwDebugLog("GFSDK_HairSDK::GetBoneWeights(%d) failed.\n", ha);
    }
}

void hwContext::assetGetBindPose(hwHAsset ha, int nth, hwMatrix &o_mat)
{
    if (ha >= m_assets.size()) { return; }

    if (m_sdk->GetBindPose(m_assets[ha].aid, nth, &o_mat) != GFSDK_HAIR_RETURN_OK) {
        hwDebugLog("GFSDK_HairSDK::GetBindPose(%d, %d) failed.\n", ha, nth);
    }
}

void hwContext::assetGetDefaultDescriptor(hwHAsset ha, hwHairDescriptor &o_desc) const
{
    if (ha >= m_assets.size()) { return; }

    if (m_sdk->CopyInstanceDescriptorFromAsset(m_assets[ha].aid, o_desc) != GFSDK_HAIR_RETURN_OK) {
        hwDebugLog("GFSDK_HairSDK::CopyInstanceDescriptorFromAsset(%d) failed.\n", ha);
    }
}



hwInstanceData& hwContext::newInstanceData()
{
    auto i = std::find_if(m_instances.begin(), m_instances.end(), [](const hwInstanceData &v) { return !v; });
    if (i != m_instances.end()) { return *i; }

    hwInstanceData tmp;
    tmp.handle = m_instances.size();
    m_instances.push_back(tmp);
    return m_instances.back();
}

hwHInstance hwContext::instanceCreate(hwHAsset ha)
{
    if (ha >= m_assets.size()) { return hwNullHandle; }

    hwInstanceData& v = newInstanceData();
    v.hasset = ha;
    if (m_sdk->CreateHairInstance(m_assets[ha].aid, &v.iid) == GFSDK_HAIR_RETURN_OK) {
        hwDebugLog("GFSDK_HairSDK::CreateHairInstance(%d) : %d succeeded.\n", ha, v.handle);
    }
    else
    {
        hwDebugLog("GFSDK_HairSDK::CreateHairInstance(%d) failed.\n", ha);
    }
    return v.handle;
}

void hwContext::instanceRelease(hwHInstance hi)
{
    if (hi >= m_instances.size()) { return; }
    auto &v = m_instances[hi];

    if (m_sdk->FreeHairInstance(v.iid) == GFSDK_HAIR_RETURN_OK) {
        hwDebugLog("GFSDK_HairSDK::FreeHairInstance(%d) succeeded.\n", hi);
    }
    else {
        hwDebugLog("GFSDK_HairSDK::FreeHairInstance(%d) failed.\n", hi);
    }
    v.invalidate();
}

void hwContext::instanceGetBounds(hwHInstance hi, hwFloat3 &o_min, hwFloat3 &o_max) const
{
    if (hi >= m_instances.size()) { return; }
    auto &v = m_instances[hi];

    if (m_sdk->GetBounds(v.iid, &o_min, &o_max) != GFSDK_HAIR_RETURN_OK)
    {
        hwDebugLog("GFSDK_HairSDK::GetBounds(%d) failed.\n", hi);
    }
}

void hwContext::instanceGetDescriptor(hwHInstance hi, hwHairDescriptor &desc) const
{
    if (hi >= m_instances.size()) { return; }
    auto &v = m_instances[hi];

    if (m_sdk->CopyCurrentInstanceDescriptor(v.iid, desc) != GFSDK_HAIR_RETURN_OK)
    {
        hwDebugLog("GFSDK_HairSDK::CopyCurrentInstanceDescriptor(%d) failed.\n", hi);
    }
}

void hwContext::instanceSetDescriptor(hwHInstance hi, const hwHairDescriptor &desc)
{
    if (hi >= m_instances.size()) { return; }
    auto &v = m_instances[hi];

    if (m_sdk->UpdateInstanceDescriptor(v.iid, desc) != GFSDK_HAIR_RETURN_OK)
    {
        hwDebugLog("GFSDK_HairSDK::UpdateInstanceDescriptor(%d) failed.\n", hi);
    }
}

void hwContext::instanceSetTexture(hwHInstance hi, hwTextureType type, ID3D11Texture2D* tex)
{
    if (hi >= m_instances.size()) { return; }
    auto &v = m_instances[hi];
	D3D11_TEXTURE2D_DESC texDesc;
	tex->GetDesc(&texDesc);
	ID3D11ShaderResourceView* srv = 0;
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	ZeroMemory(&SRVDesc, sizeof(SRVDesc));
	SRVDesc.Format = texDesc.Format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.MipLevels = texDesc.MipLevels;
	m_d3ddev->CreateShaderResourceView(tex, &SRVDesc, &srv);
	if (tex) {
		tex->Release(); tex = NULL;
	}
   if (!srv || m_sdk->SetTextureSRV(v.iid, type, srv) != GFSDK_HAIR_RETURN_OK)
 {
   hwDebugLog("GFSDK_HairSDK::SetTextureSRV(%d, %d) failed.\n", hi, type);
   }
   if (srv) {
	   srv->Release(); srv = NULL;
   }
}

void hwContext::instanceUpdateSkinningMatrices(hwHInstance hi, int num_bones, hwMatrix *matrices)
{
    if (matrices == nullptr) { return; }
    if (hi >= m_instances.size()) { return; }
    auto &v = m_instances[hi];

    if (m_sdk->UpdateSkinningMatrices(v.iid, num_bones, matrices) != GFSDK_HAIR_RETURN_OK)
    {
        hwDebugLog("GFSDK_HairSDK::UpdateSkinningMatrices(%d) failed.\n", hi);
    }
}

void hwContext::instanceUpdateSkinningDQs(hwHInstance hi, int num_bones, hwDQuaternion *dqs)
{
    if (dqs == nullptr) { return; }
    if (hi >= m_instances.size()) { return; }
    auto &v = m_instances[hi];

    if (m_sdk->UpdateSkinningDQs(v.iid, num_bones, dqs) != GFSDK_HAIR_RETURN_OK)
    {
        hwDebugLog("GFSDK_HairSDK::UpdateSkinningDQs(%d) failed.\n", hi);
    }
}

void hwContext::beginScene()
{
    m_mutex.lock();
}

void hwContext::endScene()
{
    m_mutex.unlock();
}

template<class T>
void hwContext::pushDrawCommand(const T &c)
{
    const char *begin = (const char*)&c;
    m_commands.insert(m_commands.end(), begin, begin+sizeof(T));
}

void hwContext::setRenderTarget(hwTexture *framebuffer, hwTexture *depthbuffer)
{
    DrawCommandRT c = { CID_SetRenderTarget, framebuffer, depthbuffer };
    pushDrawCommand(c);
}

void hwContext::setViewProjection(const hwMatrix &view, const hwMatrix &proj, float fov)
{
    DrawCommandVP c = { CID_SetViewProjection, fov, view, proj };
    pushDrawCommand(c);
}



void hwContext::setShader(hwHShader hs)
{
    DrawCommandI c = { CID_SetShader, hs };
    pushDrawCommand(c);
}

void hwContext::setLights(int num_lights, const hwLightData *lights)
{
    num_lights = std::min<int>(num_lights, hwMaxLights);
    DrawCommandL c = { CID_SetLights, num_lights };
    std::copy(lights, lights + num_lights, c.lights);
    pushDrawCommand(c);
}

void hwContext::setSphericalHarmonics(const hwFloat4 &Ar, const hwFloat4 &Ag, const hwFloat4 &Ab, const hwFloat4 &Br, const hwFloat4 &Bg, const hwFloat4 &Bb, const hwFloat4 &C)
{
	DrawCommandSH c = { CID_SetSphericalHarmonics, Ar, Ag, Ab, Br, Bg, Bb, C};
	pushDrawCommand(c);
}

void hwContext::initShadows(int resolution, ID3D11Texture2D* shadowMap){
	DrawCommandInitShadows c = { CID_InitShadows, resolution, shadowMap };
	pushDrawCommand(c);
}

void hwContext::render(hwHInstance hi)
{
    DrawCommandI c = { CID_Render, hi };
    pushDrawCommand(c);
}

void hwContext::renderShadow(hwHInstance hi, ID3D11Texture2D* shadowTexture/*, int numShadowCasters*/)
{
    DrawCommandShadow c = { CID_RenderShadow, hi, shadowTexture};
    pushDrawCommand(c);

}

void hwContext::stepSimulation(float dt)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    DrawCommandF c = { CID_StepSimulation, dt };
    pushDrawCommand(c);
}


hwSRV* hwContext::getSRV(hwTexture *tex)
{
    {
        auto i = m_srvtable.find(tex);
        if (i != m_srvtable.end()) {
            return i->second;
        }
    }

	ID3D11ShaderResourceView *ret;
    if (SUCCEEDED(m_d3ddev->CreateShaderResourceView(tex, NULL, &ret))) {
        m_srvtable[tex] = ret;
    }
    return ret;
}

hwRTV* hwContext::getRTV(hwTexture *tex)
{
    {
        auto i = m_rtvtable.find(tex);
        if (i != m_rtvtable.end()) {
            return i->second;
        }
    }

    hwRTV *ret = nullptr;
    if (SUCCEEDED(m_d3ddev->CreateRenderTargetView(tex, nullptr, &ret))) {
        m_rtvtable[tex] = ret;
    }
    return ret;
}


void hwContext::setRenderTargetImpl(hwTexture *framebuffer, hwTexture *depthbuffer)
{
	ID3D11RenderTargetView *rtv = NULL;
	ID3D11DepthStencilView *dsv = NULL;
	if (framebuffer)
		if (FAILED(m_d3ddev->CreateRenderTargetView(framebuffer, NULL, &rtv)))
			exit(-1);
	if (depthbuffer)
		if (FAILED(m_d3ddev->CreateDepthStencilView(depthbuffer, NULL, &dsv)))
			exit(-1);

	m_d3dctx->OMSetRenderTargets(rtv ? 1 : 0, &rtv, dsv);

	if (dsv)
		dsv->Release();
	if (rtv)
		rtv->Release();
}

void hwContext::setViewProjectionImpl(const hwMatrix &view, const hwMatrix &proj, float fov)
{
    if (m_sdk->SetViewProjection((const gfsdk_float4x4*)&view, (const gfsdk_float4x4*)&proj, GFSDK_HAIR_RIGHT_HANDED, fov) != GFSDK_HAIR_RETURN_OK)
    {
        hwDebugLog("GFSDK_HairSDK::SetViewProjection() failed.\n");
    }
}


void hwContext::setShaderImpl(hwHShader hs)
{
    if (hs >= m_shaders.size()) { return; }

    auto &v = m_shaders[hs];
    if (v.shader) {
        m_d3dctx->PSSetShader(v.shader, nullptr, 0);
    }
}

void hwContext::setLightsImpl(int num_lights, const hwLightData *lights)
{
    m_cb.num_lights = num_lights;
    std::copy(lights, lights + num_lights, m_cb.lights);
}

void hwContext::renderImpl(hwHInstance hi)
{
    if (hi >= m_instances.size()) { return; }
    auto &v = m_instances[hi];

	if (m_pShadowSRV){
		m_pShadowSRV->Release();
		m_pShadowSRV = NULL;
	}
    // update constant buffer
    {
        m_sdk->PrepareShaderConstantBuffer(v.iid, &m_cb.hw);

        D3D11_MAPPED_SUBRESOURCE MappedResource;
        m_d3dctx->Map(m_rs_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
        *((hwConstantBuffer*)MappedResource.pData) = m_cb;
        m_d3dctx->Unmap(m_rs_constant_buffer, 0);

        m_d3dctx->PSSetConstantBuffers(0, 1, &m_rs_constant_buffer);
    }
	// set sampler state
	{
		D3D11_SAMPLER_DESC samplerDesc;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.BorderColor[0] = 0;
		samplerDesc.BorderColor[1] = 0;
		samplerDesc.BorderColor[2] = 0;
		samplerDesc.BorderColor[3] = 0;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
		samplerDesc.MipLODBias = 0;
		ID3D11SamplerState* sampler = 0;
		m_d3ddev->CreateSamplerState(&samplerDesc, &sampler);
		m_d3dctx->PSSetSamplers(0, 1, &sampler);
		if (sampler){
			sampler->Release(); sampler = NULL;
		}

		D3D11_SAMPLER_DESC samplerDescSh;

		samplerDescSh.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDescSh.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDescSh.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDescSh.BorderColor[0] = 0;
		samplerDescSh.BorderColor[1] = 0;
		samplerDescSh.BorderColor[2] = 0;
		samplerDescSh.BorderColor[3] = 0;
		samplerDescSh.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDescSh.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		samplerDescSh.MaxAnisotropy = 0;
		samplerDescSh.MinLOD = 0;
		samplerDescSh.MaxLOD = D3D11_FLOAT32_MAX;
		samplerDescSh.MipLODBias = 0;
		ID3D11SamplerState* samplerSh = 0;
		m_d3ddev->CreateSamplerState(&samplerDescSh, &samplerSh);
		m_d3dctx->PSSetSamplers(1, 1, &samplerSh);
		if (samplerSh){
			samplerSh->Release(); samplerSh = NULL;
		}

	}
    // set shader resource views
    {
        ID3D11ShaderResourceView* SRVs[GFSDK_HAIR_NUM_SHADER_RESOUCES];
        m_sdk->GetShaderResources(v.iid, SRVs);
        m_d3dctx->PSSetShaderResources(0, GFSDK_HAIR_NUM_SHADER_RESOUCES, SRVs);
    }
	ID3D11ShaderResourceView* ppTextureSRVs[3];
	if (m_sdk->GetTextureSRV(v.iid, GFSDK_HAIR_TEXTURE_ROOT_COLOR, &ppTextureSRVs[0]) == GFSDK_HAIR_RETURN_OK){
		if (ppTextureSRVs[0])
			m_d3dctx->PSSetShaderResources(3, 1, &ppTextureSRVs[0]);
	}
	if (m_sdk->GetTextureSRV(v.iid, GFSDK_HAIR_TEXTURE_TIP_COLOR, &ppTextureSRVs[1]) == GFSDK_HAIR_RETURN_OK){
		if (ppTextureSRVs[1])
			m_d3dctx->PSSetShaderResources(4, 1, &ppTextureSRVs[1]);
	}
	if (m_sdk->GetTextureSRV(v.iid, GFSDK_HAIR_TEXTURE_SPECULAR, &ppTextureSRVs[2]) == GFSDK_HAIR_RETURN_OK){
		if (ppTextureSRVs[2])
			m_d3dctx->PSSetShaderResources(5, 1, &ppTextureSRVs[2]);
	}
	if (m_UnityShadow && m_UnityShadow != NULL){
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		D3D11_TEXTURE2D_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(texDesc));
		m_UnityShadow->GetDesc(&texDesc);
		srvDesc.Format = texDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		m_d3ddev->CreateShaderResourceView(m_UnityShadow, &srvDesc, &m_pShadowSRV);
		if (m_pShadowSRV && m_pShadowSRV != NULL)
		m_d3dctx->PSSetShaderResources(6, 1, &m_pShadowSRV);
	}

    // render
    auto settings = GFSDK_HairShaderSettings(true, false);
    if (m_sdk->RenderHairs(v.iid, &settings) != GFSDK_HAIR_RETURN_OK)
    {
        hwDebugLog("GFSDK_HairSDK::RenderHairs(%d) failed.\n", hi);
    }
	std::fill_n(m_ShadowTextures, 8, (ID3D11ShaderResourceView*)NULL);
    // render indicators
    m_sdk->RenderVisualization(v.iid);
}

void hwContext::renderShadowImpl(hwHInstance hi, ID3D11Texture2D* shadowMap/*, int numShadowCasters*/)
{
    if (hi >= m_instances.size()) { return; }
    auto &v = m_instances[hi];
	
	
	//D3D11_TEXTURE2D_DESC desc;
	//shadowMap->GetDesc(&desc);
	/*D3D11_SAMPLER_DESC samplerDesc;

	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	samplerDesc.MipLODBias = 0;
	ID3D11SamplerState* sampler = 0;
	m_d3ddev->CreateSamplerState(&samplerDesc, &sampler);
	m_d3dctx->PSSetSamplers(0, 1, &sampler);
	if (sampler){
		sampler->Release(); sampler = NULL;
	}*/
   
	//m_d3dctx->UpdateSubresource(m_UnityShadow, 0, NULL, shadowMap, desc.Width * 16, 0);
	//m_d3dctx->PSSetShaderResources(0, 1, &m_UnityShadowSRV);
    //auto settings = GFSDK_HairShaderSettings(false, true);
    //if (m_sdk->RenderHairs(v.iid, &settings) != GFSDK_HAIR_RETURN_OK)
    //{
        //hwDebugLog("GFSDK_HairSDK::RenderHairs(%d) failed.\n", hi);
    //}
	if (!shadowMap || shadowMap == NULL)
		return;
	
	m_d3dctx->CopyResource(m_UnityShadow, shadowMap);
}

void hwContext::stepSimulationImpl(float dt)
{
    if (m_sdk->StepSimulation(dt) != GFSDK_HAIR_RETURN_OK)
    {
        hwDebugLog("GFSDK_HairSDK::StepSimulation(%f) failed.\n", dt);
    }
}

void hwContext::setSphericalHarmonicsImpl(const hwFloat4 &Ar, const hwFloat4 &Ag, const hwFloat4 &Ab, const hwFloat4 &Br, const hwFloat4 &Bg, const hwFloat4 &Bb, const hwFloat4 &C)
{
	m_cb.shAr = Ar;
	m_cb.shAg = Ag;
	m_cb.shAb = Ab;
	m_cb.shBr = Br;
	m_cb.shBg = Bg;
	m_cb.shBb = Bb;
	m_cb.shC = C;

}

void hwContext::flush()
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_commands_back = m_commands;
        m_commands.clear();
    }

    m_d3dctx->OMSetDepthStencilState(m_rs_enable_depth, 0);
    auto &commands = m_commands_back;
    for (int i=0; i<commands.size(); ) {
        CommandID cid = (CommandID&)commands[i];
		switch (cid) {
		case CID_SetViewProjection:
		{
			const auto c = (DrawCommandVP&)commands[i];
			setViewProjectionImpl(c.view, c.proj, c.fov);
			i += sizeof(c);
			break;
		}
		case CID_SetRenderTarget:
		{
			const auto c = (DrawCommandRT&)commands[i];
			setRenderTargetImpl(c.framebuffer, c.depthbuffer);
			i += sizeof(c);
			break;
		}
		case CID_SetShader:
		{
			const auto c = (DrawCommandI&)commands[i];
			setShaderImpl(c.arg);
			i += sizeof(c);
			break;
		}
		case CID_SetLights:
		{
			const auto c = (DrawCommandL&)commands[i];
			setLightsImpl(c.num_lights, c.lights);
			i += sizeof(c);
			break;
		}
		case CID_Render:
		{
			const auto c = (DrawCommandI&)commands[i];
			renderImpl(c.arg);
			i += sizeof(c);
			break;
		}
		case CID_RenderShadow:
		{
			const auto c = (DrawCommandShadow&)commands[i];
			renderShadowImpl(c.arg, c.shadowMap/*, c.numShadowCasters*/);
			i += sizeof(c);
			break;
		}
		case CID_StepSimulation:
		{
			const auto c = (DrawCommandF&)commands[i];
			stepSimulationImpl(c.arg);
			i += sizeof(c);
			break;
		}
		case CID_SetSphericalHarmonics:
		{
			const auto c = (DrawCommandSH&)commands[i];
			setSphericalHarmonicsImpl(c.shAr, c.shAg, c.shAb, c.shBr, c.shBb, c.shBb, c.shC);
			i += sizeof(c);
			break;
		}
		case CID_InitShadows:
		{
			const auto c = (DrawCommandInitShadows&)commands[i];
			initShadowsImpl(c.rersolution, c.shadowMap);
			i += sizeof(c);
			break;
		}
		}
    }
}

