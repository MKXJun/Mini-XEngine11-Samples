#include "ShaderImpl.h"
#include "DXTrace.h"
#include "d3dUtil.h"


using namespace Microsoft::WRL;

# pragma warning(disable: 26812)

namespace
{
	struct ShaderDestroyer
	{
		void operator()(Shader* pShader) {
			pShader->Destroy();
		}
	};

	Microsoft::WRL::ComPtr<ID3D11Device> s_pDevice;

	std::unordered_map<size_t, std::unique_ptr<Shader, ShaderDestroyer>> s_ShaderMap;
	
	std::unordered_set<std::string> s_SemanticNames;

	std::unordered_set<size_t> s_ShaderSet;		// 记录下面的shader
	std::unordered_map<size_t, VertexShaderInfo> s_VertexShaders;
	std::unordered_map<size_t, HullShaderInfo> s_HullShaders;
	std::unordered_map<size_t, DomainShaderInfo> s_DomainShaders;
	std::unordered_map<size_t, GeometryShaderInfo> s_GeometryShaders;
	std::unordered_map<size_t, PixelShaderInfo> s_PixelShaders;
	std::unordered_map<size_t, ComputeShaderInfo> s_ComputeShaders;
}

//
// 代码宏
//

#define CREATE_SHADER(FullShaderType, ShaderType, ShaderID)\
{\
	s_ShaderSet.insert(ShaderID);\
	s_##FullShaderType##s.try_emplace(ShaderID);\
	hr = device->Create##FullShaderType##(blob->GetBufferPointer(), blob->GetBufferSize(),\
		nullptr, s_##FullShaderType##s[ShaderID].p##ShaderType##.GetAddressOf());\
	break;\
}

#define PASS_ADD_CBUFFERS_AND_PROPERTIES(ShaderType)\
{\
	std::map<uint32_t, CBufferData>::iterator cbufferIt;\
	bool emplaced;\
	for (auto& _it : it->second.cBuffers)\
	{\
		std::tie(cbufferIt, emplaced) = m_CBuffers.try_emplace(_it.second.startSlot, CBufferData(_it.second.name, _it.second.startSlot, _it.second.byteWidth));\
		if (emplaced)\
		{\
			cbufferIt->second.CreateBuffer(s_pDevice.Get());\
			for (auto& p : it->second.cBuffers[_it.second.startSlot].properties)\
			{\
				std::unordered_map<size_t, Property>::iterator propertyIt;\
				std::tie(propertyIt, emplaced) = m_Properties.try_emplace(p.first, p.second);\
				if (emplaced)\
				{\
					propertyIt->second.pCBufferData = &cbufferIt->second;\
				}\
			}\
		}\
	}\
}

#define PASS_ADD_SAMPLERS \
{\
	for (auto& _it : it->second.samplers)\
		pass.samplers.try_emplace(_it.first, std::make_pair(_it.second.startSlot, Microsoft::WRL::ComPtr<ID3D11SamplerState>()));\
}

#define PASS_ADD_SHADER_RESOURCES \
{\
	for (auto& _it : it->second.shaderResources)\
		pass.shaderResources.try_emplace(_it.first, std::make_pair(_it.second.startSlot, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>()));\
}

#define PASS_ADD_RWRESOURCES \
{\
	for (auto& _it : it->second.rwResources)\
		pass.rwResources.try_emplace(_it.first, std::make_pair(_it.second.startSlot, std::make_pair(Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>(), std::unique_ptr<uint32_t>())));\
}

#define PASS_SET_SHADER(ShaderType) \
{\
	deviceContext->##ShaderType##SetShader(p##ShaderType##Info->p##ShaderType##.Get(), nullptr, 0);\
}

#define PASS_SET_CBUFFER(ShaderType) \
{\
	for (auto& it : cBuffers)\
	{\
		it.second.UpdateBuffer(deviceContext);\
		deviceContext->##ShaderType##SetConstantBuffers(it.first, 1, it.second.cBuffer.GetAddressOf());\
	}\
}

#define PASS_SET_SAMPLER(ShaderType) \
{\
	for (auto& it : this->samplers)\
		deviceContext->##ShaderType##SetSamplers(it.second.first, 1, it.second.second.GetAddressOf());\
}

#define PASS_SET_SHADERRESOURCE(ShaderType) \
{\
	for (auto& it : shaderResources)\
		deviceContext->##ShaderType##SetShaderResources(it.second.first, 1, it.second.second.GetAddressOf());\
}

//
// 函数
//
static std::string SystemTimeToString(const SYSTEMTIME& st)
{
	char str[40];
	sprintf_s(str, "%d/%d/%d %d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour,
		st.wMinute, st.wSecond);
	return std::string(str);
}

static SYSTEMTIME StringToSystemTime(const std::string& str)
{
	SYSTEMTIME st;
	sscanf_s(str.c_str(), "%hd/%hd/%hd %hd:%hd:%hd", &st.wYear, &st.wMonth, &st.wDay,
		&st.wHour, &st.wMinute, &st.wSecond);
	return st;
}

template<class Type,
	typename std::enable_if_t<std::is_enum_v<Type>>* = nullptr>
	static bool Parse(const nlohmann::json& json, std::string_view name, const std::map<std::string, uint32_t> enumMap, Type& out)
{
	if (auto it = json.find(name); it != json.end())
	{
		if (!it->is_string())
			return false;
		auto str = it->get<std::string>();
		std::transform(str.begin(), str.end(), str.begin(), ::toupper);
		if (auto _it = enumMap.find(str); _it != enumMap.end())
			out = static_cast<Type>(_it->second);
	}
	return true;
}

template<class Type,
	typename std::enable_if_t<std::is_signed_v<Type>&& std::is_integral_v<Type>>* = nullptr>
	static bool Parse(const nlohmann::json& json, std::string_view name, Type& out)
{
	if (auto it = json.find(name); it != json.end())
	{
		if (!it->is_number_integer())
			return false;
		out = it->get<Type>();
	}
	return true;
}

template<class Type,
	typename std::enable_if_t<std::is_unsigned_v<Type>&& std::is_integral_v<Type>>* = nullptr>
	static bool Parse(const nlohmann::json& json, std::string_view name, Type& out)
{
	if (auto it = json.find(name); it != json.end())
	{
		if (!it->is_number_unsigned())
			return false;
		out = it->get<Type>();
	}
	return true;
}

template<class Type,
	typename std::enable_if_t<std::is_floating_point_v<Type>>* = nullptr>
	static bool Parse(const nlohmann::json& json, std::string_view name, Type& out)
{
	if (auto it = json.find(name); it != json.end())
	{
		if (!it->is_number_float())
			return false;
		out = it->get<Type>();
	}
	return true;
}

template<class Type,
	typename std::enable_if_t<std::is_array_v<Type>&& std::is_arithmetic_v<std::remove_extent_t<Type>>>* = nullptr>
	static bool Parse(const nlohmann::json& json, std::string_view name, Type& out)
{
	if (auto it = json.find(name); it != json.end())
	{
		if (!it->is_array())
			return false;
		size_t size = it->size();
		for (size_t i = 0; i < size; ++i)
		{
			out[i] = it[i].get<std::remove_extent_t<Type>>();
		}
	}
	return true;
}


// BOOL是int类型
static bool ParseBool(const nlohmann::json& json, std::string_view name, BOOL& out)
{
	if (auto it = json.find(name); it != json.end())
	{
		if (!it->is_boolean())
			return false;
		out = it->get<bool>();
	}
	return true;
}


static HRESULT CreateRasterizerStateFromJson(ID3D11Device* device, const nlohmann::json& json, ID3D11RasterizerState** outRasterizerState)
{
	if (!json.is_object() || !device || !outRasterizerState)
		return E_INVALIDARG;
	CD3D11_RASTERIZER_DESC rsDesc{ CD3D11_DEFAULT{} };
	static std::map<std::string, uint32_t> fillMode{
		{"WIREFRAME", D3D11_FILL_WIREFRAME},
		{"SOLID", D3D11_FILL_SOLID}
	};
	static std::map<std::string, uint32_t> cullMode{
		{"NONE", D3D11_CULL_NONE},
		{"FRONT", D3D11_CULL_FRONT},
		{"BACK", D3D11_CULL_BACK}
	};

	if (!Parse(json, "FillMode", fillMode, rsDesc.FillMode)) return E_INVALIDARG;
	if (!Parse(json, "CullMode", cullMode, rsDesc.CullMode)) return E_INVALIDARG;
	if (!ParseBool(json, "FrontCounterClockwise", rsDesc.FrontCounterClockwise)) return E_INVALIDARG;
	if (!Parse(json, "DepthBias", rsDesc.DepthBias)) return E_INVALIDARG;
	if (!Parse(json, "DepthBiasClamp", rsDesc.DepthBiasClamp)) return E_INVALIDARG;
	if (!Parse(json, "SlopeScaledDepthBias", rsDesc.SlopeScaledDepthBias)) return E_INVALIDARG;
	if (!ParseBool(json, "DepthClipEnable", rsDesc.DepthClipEnable)) return E_INVALIDARG;
	if (!ParseBool(json, "ScissorEnable", rsDesc.ScissorEnable)) return E_INVALIDARG;
	if (!ParseBool(json, "MultisampleEnable", rsDesc.MultisampleEnable)) return E_INVALIDARG;
	if (!ParseBool(json, "AntialiasedLineEnable", rsDesc.AntialiasedLineEnable)) return E_INVALIDARG;

	return device->CreateRasterizerState(&rsDesc, outRasterizerState);
}

static HRESULT CreateDepthStencilStateFromJson(ID3D11Device* device, const nlohmann::json& json, ID3D11DepthStencilState** outDepthStencilState)
{
	if (!json.is_object() || !device || !outDepthStencilState)
		return E_INVALIDARG;
	CD3D11_DEPTH_STENCIL_DESC dsDesc{ CD3D11_DEFAULT{} };
	static std::map<std::string, uint32_t> depthWriteMask{
		{"zero", D3D11_DEPTH_WRITE_MASK_ZERO},
		{"all", D3D11_DEPTH_WRITE_MASK_ALL}
	};
	static std::map<std::string, uint32_t> compFunc{
		{"NEVER", D3D11_COMPARISON_NEVER},
		{"LESS", D3D11_COMPARISON_LESS},
		{"EQUAL", D3D11_COMPARISON_EQUAL},
		{"LESS_EQUAL", D3D11_COMPARISON_LESS_EQUAL},
		{"GREATER", D3D11_COMPARISON_GREATER},
		{"NOT_EQUAL", D3D11_COMPARISON_NOT_EQUAL},
		{"GREATER_EQUAL", D3D11_COMPARISON_GREATER_EQUAL},
		{"ALWAYS", D3D11_COMPARISON_ALWAYS}
	};

	static std::map<std::string, uint32_t> stencilFunc{
		{"KEEP", D3D11_STENCIL_OP_KEEP},
		{"ZERO", D3D11_STENCIL_OP_ZERO},
		{"REPLACE", D3D11_STENCIL_OP_REPLACE},
		{"INCR_SAT", D3D11_STENCIL_OP_INCR_SAT},
		{"DECR_SAT", D3D11_STENCIL_OP_DECR_SAT},
		{"INVERT", D3D11_STENCIL_OP_INVERT},
		{"INCR", D3D11_STENCIL_OP_INCR},
		{"DECR", D3D11_STENCIL_OP_DECR}
	};

	if (!ParseBool(json, "DepthEnable", dsDesc.DepthEnable)) return E_INVALIDARG;
	if (!Parse(json, "DepthWriteMask", depthWriteMask, dsDesc.DepthWriteMask)) return E_INVALIDARG;
	if (!Parse(json, "DepthFunc", compFunc, dsDesc.DepthFunc)) return E_INVALIDARG;
	if (!ParseBool(json, "StencilEnable", dsDesc.StencilEnable)) return E_INVALIDARG;
	if (!Parse(json, "StencilReadMask", dsDesc.StencilReadMask)) return E_INVALIDARG;
	if (!Parse(json, "StencilWriteMask", dsDesc.StencilWriteMask)) return E_INVALIDARG;
	if (!Parse(json, "FrontFace.StencilFailOp", stencilFunc, dsDesc.FrontFace.StencilFailOp)) return E_INVALIDARG;
	if (!Parse(json, "FrontFace.StencilDepthFailOp", stencilFunc, dsDesc.FrontFace.StencilDepthFailOp)) return E_INVALIDARG;
	if (!Parse(json, "FrontFace.StencilPassOp", stencilFunc, dsDesc.FrontFace.StencilPassOp)) return E_INVALIDARG;
	if (!Parse(json, "FrontFace.StencilFunc", compFunc, dsDesc.FrontFace.StencilFunc)) return E_INVALIDARG;
	if (!Parse(json, "BackFace.StencilFailOp", stencilFunc, dsDesc.BackFace.StencilFailOp)) return E_INVALIDARG;
	if (!Parse(json, "BackFace.StencilDepthFailOp", stencilFunc, dsDesc.BackFace.StencilDepthFailOp)) return E_INVALIDARG;
	if (!Parse(json, "BackFace.StencilPassOp", stencilFunc, dsDesc.BackFace.StencilPassOp)) return E_INVALIDARG;
	if (!Parse(json, "BackFace.StencilFunc", compFunc, dsDesc.BackFace.StencilFunc)) return E_INVALIDARG;

	return device->CreateDepthStencilState(&dsDesc, outDepthStencilState);
}

static HRESULT CreateBlendStateFromJson(ID3D11Device* device, const nlohmann::json& json, ID3D11BlendState** outBlendState)
{
	if (!json.is_object() || !device || !outBlendState)
		return E_INVALIDARG;
	CD3D11_BLEND_DESC bsDesc{ CD3D11_DEFAULT{} };
	static std::map<std::string, uint32_t> blend{
		{"ZERO", D3D11_BLEND_ZERO},
		{"ONE", D3D11_BLEND_ONE},
		{"SRC_COLOR", D3D11_BLEND_SRC_COLOR},
		{"INV_SRC_COLOR", D3D11_BLEND_INV_SRC_COLOR},
		{"SRC_ALPHA", D3D11_BLEND_SRC_ALPHA},
		{"INV_SRC_ALPHA", D3D11_BLEND_INV_SRC_ALPHA},
		{"DEST_ALPHA", D3D11_BLEND_DEST_ALPHA},
		{"INV_DEST_ALPHA", D3D11_BLEND_INV_DEST_ALPHA},
		{"DEST_COLOR", D3D11_BLEND_DEST_COLOR},
		{"INV_DEST_COLOR", D3D11_BLEND_INV_DEST_COLOR},
		{"SRC_ALPHA_SAT", D3D11_BLEND_SRC_ALPHA_SAT},
		{"BLEND_FACTOR", D3D11_BLEND_BLEND_FACTOR},
		{"INV_BLEND_FACTOR", D3D11_BLEND_INV_BLEND_FACTOR},
		{"SRC1_COLOR", D3D11_BLEND_SRC1_COLOR},
		{"INV_SRC1_COLOR", D3D11_BLEND_INV_SRC1_COLOR},
		{"SRC1_ALPHA", D3D11_BLEND_SRC1_ALPHA},
		{"INV_SRC1_ALPHA", D3D11_BLEND_INV_SRC1_ALPHA}
	};

	static std::map<std::string, uint32_t> blendOp{
		{"ADD", D3D11_BLEND_OP_ADD},
		{"SUBTRACT", D3D11_BLEND_OP_SUBTRACT},
		{"REV_SUBTRACT", D3D11_BLEND_OP_REV_SUBTRACT},
		{"MIN", D3D11_BLEND_OP_MIN},
		{"MAX", D3D11_BLEND_OP_MAX},
	};

	static std::map<std::string, uint32_t> stencilFunc{
		{"KEEP", D3D11_STENCIL_OP_KEEP},
		{"ZERO", D3D11_STENCIL_OP_ZERO},
		{"REPLACE", D3D11_STENCIL_OP_REPLACE},
		{"INCR_SAT", D3D11_STENCIL_OP_INCR_SAT},
		{"DECR_SAT", D3D11_STENCIL_OP_DECR_SAT},
		{"INVERT", D3D11_STENCIL_OP_INVERT},
		{"INCR", D3D11_STENCIL_OP_INCR},
		{"DECR", D3D11_STENCIL_OP_DECR}
	};

	if (!ParseBool(json, "AlphaToCoverageEnable", bsDesc.AlphaToCoverageEnable)) return E_INVALIDARG;
	if (!ParseBool(json, "IndependentBlendEnable", bsDesc.IndependentBlendEnable)) return E_INVALIDARG;
	if (!ParseBool(json, "BlendEnable", bsDesc.RenderTarget[0].BlendEnable)) return E_INVALIDARG;
	if (!Parse(json, "SrcBlend", blend, bsDesc.RenderTarget[0].SrcBlend)) return E_INVALIDARG;
	if (!Parse(json, "DestBlend", blend, bsDesc.RenderTarget[0].DestBlend)) return E_INVALIDARG;
	if (!Parse(json, "BlendOp", blendOp, bsDesc.RenderTarget[0].BlendOp)) return E_INVALIDARG;
	if (!Parse(json, "SrcBlendAlpha", blend, bsDesc.RenderTarget[0].SrcBlendAlpha)) return E_INVALIDARG;
	if (!Parse(json, "DestBlendAlpha", blend, bsDesc.RenderTarget[0].DestBlendAlpha)) return E_INVALIDARG;
	if (!Parse(json, "BlendOpAlpha", blendOp, bsDesc.RenderTarget[0].BlendOpAlpha)) return E_INVALIDARG;
	if (!Parse(json, "SrcBlend", blend, bsDesc.RenderTarget[0].SrcBlend)) return E_INVALIDARG;
	if (!Parse(json, "RenderTargetWriteMask", bsDesc.RenderTarget[0].RenderTargetWriteMask)) return E_INVALIDARG;

	if (bsDesc.IndependentBlendEnable)
	{
		for (int i = 0; i < 8; ++i)
		{
			if (!ParseBool(json, "RenderTarget[" + std::to_string(i) + "].BlendEnable", bsDesc.RenderTarget[i].BlendEnable)) return E_INVALIDARG;
			if (!Parse(json, "RenderTarget[" + std::to_string(i) + "].SrcBlend", blend, bsDesc.RenderTarget[i].SrcBlend)) return E_INVALIDARG;
			if (!Parse(json, "RenderTarget[" + std::to_string(i) + "].DestBlend", blend, bsDesc.RenderTarget[i].DestBlend)) return E_INVALIDARG;
			if (!Parse(json, "RenderTarget[" + std::to_string(i) + "].BlendOp", blendOp, bsDesc.RenderTarget[i].BlendOp)) return E_INVALIDARG;
			if (!Parse(json, "RenderTarget[" + std::to_string(i) + "].SrcBlendAlpha", blend, bsDesc.RenderTarget[i].SrcBlendAlpha)) return E_INVALIDARG;
			if (!Parse(json, "RenderTarget[" + std::to_string(i) + "].DestBlendAlpha", blend, bsDesc.RenderTarget[i].DestBlendAlpha)) return E_INVALIDARG;
			if (!Parse(json, "RenderTarget[" + std::to_string(i) + "].BlendOpAlpha", blendOp, bsDesc.RenderTarget[i].BlendOpAlpha)) return E_INVALIDARG;
			if (!Parse(json, "RenderTarget[" + std::to_string(i) + "].SrcBlend", blend, bsDesc.RenderTarget[i].SrcBlend)) return E_INVALIDARG;
			if (!Parse(json, "RenderTarget[" + std::to_string(i) + "].RenderTargetWriteMask", bsDesc.RenderTarget[i].RenderTargetWriteMask)) return E_INVALIDARG;
		}
	}

	return device->CreateBlendState(&bsDesc, outBlendState);
}

//
// ShaderPass
//
void ShaderPass::SetRasterizerState(ID3D11RasterizerState* pRS)
{
	this->pRasterizerState = pRS;
}

void ShaderPass::SetBlendState(ID3D11BlendState* pBS, const FLOAT blendFactor[4], UINT sampleMask)
{
	this->pBlendState = pBS;
	if (blendFactor)
		memcpy_s(this->blendFactor.data(), sizeof blendFactor, blendFactor, sizeof blendFactor);
	this->sampleMask = sampleMask;
}

void ShaderPass::SetDepthStencilState(ID3D11DepthStencilState* pDSS, UINT stencilRef)
{
	this->pDepthStencilState = pDSS;
	this->stencilRef = stencilRef;
}

void ShaderPass::SetSamplerState(std::string_view name, ID3D11SamplerState* samplerState)
{
	if (auto it = samplers.find(Shader::StringToID(name)); it != samplers.end())
		it->second.second = samplerState;
}

void ShaderPass::SetShaderResource(std::string_view name, ID3D11ShaderResourceView* srv)
{
	if (auto it = shaderResources.find(Shader::StringToID(name)); it != shaderResources.end())
		it->second.second = srv;
}

void ShaderPass::SetUnorderedAccess(std::string_view name, ID3D11UnorderedAccessView* uav, UINT* initialCount)
{
	if (auto it = rwResources.find(Shader::StringToID(name)); it != rwResources.end())
	{
		it->second.second.first = uav;
		it->second.second.second = initialCount ? std::make_unique<uint32_t>(*initialCount) : nullptr;
	}
}

const std::vector<D3D11_INPUT_ELEMENT_DESC>& ShaderPass::GetInputSignatures()
{
	return this->pVSInfo->signatureParams;
}

ID3D11InputLayout* ShaderPass::GetInputLayout()
{
	return this->pVSInfo->pDefaultInputLayout.Get();
}

void ShaderPass::Apply(ID3D11DeviceContext* deviceContext)
{
	//
	// 设置着色器、常量缓冲区、形参常量缓冲区、采样器、着色器资源、可读写资源
	//
	if (pVSInfo)
	{
		PASS_SET_SHADER(VS);
		PASS_SET_CBUFFER(VS);
		PASS_SET_SAMPLER(VS);
		PASS_SET_SHADERRESOURCE(VS);
	}
	else
	{
		deviceContext->VSSetShader(nullptr, nullptr, 0);
	}

	if (pDSInfo)
	{
		PASS_SET_SHADER(DS);
		PASS_SET_CBUFFER(DS);
		PASS_SET_SAMPLER(DS);
		PASS_SET_SHADERRESOURCE(DS);
	}
	else
	{
		deviceContext->DSSetShader(nullptr, nullptr, 0);
	}

	if (pHSInfo)
	{
		PASS_SET_SHADER(HS);
		PASS_SET_CBUFFER(HS);
		PASS_SET_SAMPLER(HS);
		PASS_SET_SHADERRESOURCE(HS);
	}
	else
	{
		deviceContext->HSSetShader(nullptr, nullptr, 0);
	}

	if (pGSInfo)
	{
		PASS_SET_SHADER(GS);
		PASS_SET_CBUFFER(GS);
		PASS_SET_SAMPLER(GS);
		PASS_SET_SHADERRESOURCE(GS);
	}
	else
	{
		deviceContext->GSSetShader(nullptr, nullptr, 0);
	}

	if (pPSInfo)
	{
		PASS_SET_SHADER(PS);
		PASS_SET_CBUFFER(PS);
		PASS_SET_SAMPLER(PS);
		PASS_SET_SHADERRESOURCE(PS);
		for (auto& p : rwResources)
			deviceContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL,
				nullptr, nullptr, p.second.first, 1, p.second.second.first.GetAddressOf(),
				(p.second.second.second ? p.second.second.second.get() : nullptr));
	}
	else
	{
		deviceContext->PSSetShader(nullptr, nullptr, 0);
	}

	if (pCSInfo)
	{
		PASS_SET_SHADER(CS);
		PASS_SET_CBUFFER(CS);
		PASS_SET_SAMPLER(CS);
		PASS_SET_SHADERRESOURCE(CS);
		for (auto& p : rwResources)
			deviceContext->CSSetUnorderedAccessViews(p.second.first, 1, p.second.second.first.GetAddressOf(),
				(p.second.second.second ? p.second.second.second.get() : nullptr));
	}
	else
	{
		deviceContext->CSSetShader(nullptr, nullptr, 0);
	}

	// 设置渲染状态
	deviceContext->RSSetState(pRasterizerState.Get());
	deviceContext->OMSetBlendState(pBlendState.Get(), blendFactor.data(), sampleMask);
	deviceContext->OMSetDepthStencilState(pDepthStencilState.Get(), stencilRef);
}


//
// Shader::Impl
//
void Shader::Impl::SetGlobalRaw(size_t propertyID, const void* data, uint32_t byteOffset, uint32_t byteCount)
{
	auto it = m_Properties.find(propertyID);
	if (it == m_Properties.end())
		return;
	if (byteOffset > it->second.byteWidth)
		return;
	if (byteOffset + byteCount > it->second.byteWidth)
		byteCount = it->second.byteWidth - byteOffset;

	// 仅当值不同时更新
	if (memcmp(it->second.pCBufferData->data.data() + it->second.startByteOffset + byteOffset, data, byteCount))
	{
		memcpy_s(it->second.pCBufferData->data.data() + it->second.startByteOffset + byteOffset, byteCount, data, byteCount);
		it->second.pCBufferData->isDirty = true;
	}
}

ShaderPass& Shader::Impl::AddPass(const PassDesc& desc)
{
	m_Passes.push_back(ShaderPass(m_CBuffers));
	auto& pass = m_Passes.back();
	if (std::unordered_map<size_t, VertexShaderInfo>::iterator it; !desc.vsName.empty() && (it = s_VertexShaders.find(StringToID(desc.vsName))) != s_VertexShaders.end())
	{
		pass.pVSInfo = &it->second;
		PASS_ADD_CBUFFERS_AND_PROPERTIES(VS);
		PASS_ADD_SAMPLERS;
		PASS_ADD_SHADER_RESOURCES;
	}
	if (std::unordered_map<size_t, HullShaderInfo>::iterator it; !desc.hsName.empty() && (it = s_HullShaders.find(StringToID(desc.hsName))) != s_HullShaders.end())
	{
		pass.pHSInfo = &it->second;
		PASS_ADD_CBUFFERS_AND_PROPERTIES(HS);
		PASS_ADD_SAMPLERS;
		PASS_ADD_SHADER_RESOURCES;
	}
	if (std::unordered_map<size_t, DomainShaderInfo>::iterator it; !desc.dsName.empty() && (it = s_DomainShaders.find(StringToID(desc.dsName))) != s_DomainShaders.end())
	{
		pass.pDSInfo = &it->second;
		PASS_ADD_CBUFFERS_AND_PROPERTIES(DS);
		PASS_ADD_SAMPLERS;
		PASS_ADD_SHADER_RESOURCES;
	}
	if (std::unordered_map<size_t, GeometryShaderInfo>::iterator it; !desc.gsName.empty() && (it = s_GeometryShaders.find(StringToID(desc.gsName))) != s_GeometryShaders.end())
	{
		pass.pGSInfo = &it->second;
		PASS_ADD_CBUFFERS_AND_PROPERTIES(GS);
		PASS_ADD_SAMPLERS;
		PASS_ADD_SHADER_RESOURCES;
	}
	if (std::unordered_map<size_t, PixelShaderInfo>::iterator it; !desc.psName.empty() && (it = s_PixelShaders.find(StringToID(desc.psName))) != s_PixelShaders.end())
	{
		pass.pPSInfo = &it->second;
		PASS_ADD_CBUFFERS_AND_PROPERTIES(PS);
		PASS_ADD_SAMPLERS;
		PASS_ADD_SHADER_RESOURCES;
		PASS_ADD_RWRESOURCES;
	}
	if (std::unordered_map<size_t, ComputeShaderInfo>::iterator it; !desc.csName.empty() && (it = s_ComputeShaders.find(StringToID(desc.csName))) != s_ComputeShaders.end())
	{
		pass.pCSInfo = &it->second;
		PASS_ADD_CBUFFERS_AND_PROPERTIES(CS);
		PASS_ADD_SAMPLERS;
		PASS_ADD_SHADER_RESOURCES;
		PASS_ADD_RWRESOURCES;
	}

	return m_Passes.back();
}

bool Shader::Impl::InitAll(ID3D11Device* pDevice)
{
	s_pDevice = pDevice;
	return true;
}

bool Shader::Impl::Exists(std::string_view name)
{
	return s_ShaderSet.find(StringToID(name)) != s_ShaderSet.end();
}

HRESULT Shader::Impl::AddFromBlob(std::string_view name, ID3D11Device* device, ID3DBlob* blob)
{
	if (name.empty())
		return E_INVALIDARG;

	size_t shaderID = StringToID(name);
	if (auto it = s_ShaderSet.find(shaderID); it != s_ShaderSet.end())
		return S_OK;

	HRESULT hr;
	// 着色器反射
	ComPtr<ID3D11ShaderReflection> pShaderReflection;
	hr = D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(), __uuidof(ID3D11ShaderReflection),
		reinterpret_cast<void**>(pShaderReflection.GetAddressOf()));
	if (FAILED(hr))
		return hr;

	// 获取着色器类型
	D3D11_SHADER_DESC sd;
	pShaderReflection->GetDesc(&sd);
	ShaderFlag shaderFlag = static_cast<ShaderFlag>(1 << D3D11_SHVER_GET_TYPE(sd.Version));
	
	// 创建着色器信息并建立反射
	switch (shaderFlag)
	{
	case PixelShader:    CREATE_SHADER(PixelShader, PS, shaderID);
	case VertexShader:   CREATE_SHADER(VertexShader, VS, shaderID);
	case GeometryShader: CREATE_SHADER(GeometryShader, GS, shaderID);
	case HullShader:     CREATE_SHADER(HullShader, HS, shaderID);
	case DomainShader:   CREATE_SHADER(DomainShader, DS, shaderID);
	case ComputeShader:  CREATE_SHADER(ComputeShader, CS, shaderID);
	}
	if (FAILED(hr))
		return hr;

	// 建立着色器反射
	hr = UpdateShaderReflection(name, device, pShaderReflection.Get(), shaderFlag);

	// 创建输入布局
	if (shaderFlag == ShaderFlag::VertexShader)
	{
		hr = device->CreateInputLayout(s_VertexShaders[shaderID].signatureParams.data(), (uint32_t)s_VertexShaders[shaderID].signatureParams.size(),
			blob->GetBufferPointer(), blob->GetBufferSize(), s_VertexShaders[shaderID].pDefaultInputLayout.GetAddressOf());
	}

	return hr;
}

HRESULT Shader::Impl::UpdateShaderReflection(std::string_view name, ID3D11Device* device, ID3D11ShaderReflection* pShaderReflection, UINT shaderFlag)
{
	size_t shaderID = StringToID(name);

	HRESULT hr;
	// 输入布局
	if (shaderFlag == VertexShader)
	{
		LPCSTR lastSemanticName = nullptr;
		for (UINT i = 0;; ++i)
		{
			D3D11_SIGNATURE_PARAMETER_DESC spDesc;
			hr = pShaderReflection->GetInputParameterDesc(i, &spDesc);
			if (FAILED(hr))
				break;

			D3D11_INPUT_ELEMENT_DESC ieDesc{};
			auto it = s_SemanticNames.find(spDesc.SemanticName);
			if (it == s_SemanticNames.end())
			{
				auto p = s_SemanticNames.insert(spDesc.SemanticName);
				it = p.first;
			}
			ieDesc.SemanticName = it->c_str();
			ieDesc.SemanticIndex = 0;
			ieDesc.InputSlot = 0;
			ieDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			auto& vs = s_VertexShaders[StringToID(name)];
			if (i > 0 && vs.signatureParams[i - 1].SemanticName == ieDesc.SemanticName)
			{
				ieDesc.SemanticIndex = vs.signatureParams[i - 1].SemanticIndex + 1;
				ieDesc.InputSlot = vs.signatureParams[i - 1].InputSlot;
			}
			else if (i > 0)
			{
				ieDesc.InputSlot = vs.signatureParams[i - 1].InputSlot + 1;
			}

			int compCount = (int)round(log2(spDesc.Mask + 1));
			if (spDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
			{
				switch (compCount)
				{
				case 1: ieDesc.Format = DXGI_FORMAT_R32_FLOAT; break;
				case 2: ieDesc.Format = DXGI_FORMAT_R32G32_FLOAT; break;
				case 3: ieDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT; break;
				case 4: ieDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
				}
			}
			else if (spDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
			{
				switch (compCount)
				{
				case 1: ieDesc.Format = DXGI_FORMAT_R32_SINT; break;
				case 2: ieDesc.Format = DXGI_FORMAT_R32G32_SINT; break;
				case 3: ieDesc.Format = DXGI_FORMAT_R32G32B32_SINT; break;
				case 4: ieDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT; break;
				}
			}
			else if (spDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
			{
				switch (compCount)
				{
				case 1: ieDesc.Format = DXGI_FORMAT_R32_UINT; break;
				case 2: ieDesc.Format = DXGI_FORMAT_R32G32_UINT; break;
				case 3: ieDesc.Format = DXGI_FORMAT_R32G32B32_UINT; break;
				case 4: ieDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT; break;
				}
			}
			ieDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			ieDesc.InstanceDataStepRate = 0;

			vs.signatureParams.push_back(ieDesc);
		}
	}

	D3D11_SHADER_DESC sd;
	hr = pShaderReflection->GetDesc(&sd);
	if (FAILED(hr))
		return hr;

	for (UINT i = 0;; ++i)
	{
		D3D11_SHADER_INPUT_BIND_DESC sibDesc;
		hr = pShaderReflection->GetResourceBindingDesc(i, &sibDesc);
		// 读取完变量后会失败，但这并不是失败的调用
		if (FAILED(hr))
			break;

		// 常量缓冲区
		if (sibDesc.Type == D3D_SIT_CBUFFER)
		{
			ID3D11ShaderReflectionConstantBuffer* pSRCBuffer = pShaderReflection->GetConstantBufferByName(sibDesc.Name);
			// 获取cbuffer内的变量信息并建立映射
			D3D11_SHADER_BUFFER_DESC cbDesc{};
			hr = pSRCBuffer->GetDesc(&cbDesc);
			if (FAILED(hr))
				return hr;
			

			// 确定常量缓冲区的创建位置
			static std::string_view params = "$Params";
			bool isParam = (params == sibDesc.Name);
			if (!isParam)
			{
				switch (shaderFlag)
				{
				case VertexShader: s_VertexShaders[shaderID].cBuffers.try_emplace(sibDesc.BindPoint, ConstantBufferInfo{ sibDesc.Name, sibDesc.BindPoint, cbDesc.Size }); break;
				case DomainShader: s_DomainShaders[shaderID].cBuffers.try_emplace(sibDesc.BindPoint, ConstantBufferInfo{ sibDesc.Name, sibDesc.BindPoint, cbDesc.Size }); break;
				case HullShader: s_HullShaders[shaderID].cBuffers.try_emplace(sibDesc.BindPoint, ConstantBufferInfo{ sibDesc.Name, sibDesc.BindPoint, cbDesc.Size }); break;
				case GeometryShader: s_GeometryShaders[shaderID].cBuffers.try_emplace(sibDesc.BindPoint, ConstantBufferInfo{ sibDesc.Name, sibDesc.BindPoint, cbDesc.Size }); break;
				case PixelShader: s_PixelShaders[shaderID].cBuffers.try_emplace(sibDesc.BindPoint, ConstantBufferInfo{ sibDesc.Name, sibDesc.BindPoint, cbDesc.Size }); break;
				case ComputeShader: s_ComputeShaders[shaderID].cBuffers.try_emplace(sibDesc.BindPoint, ConstantBufferInfo{ sibDesc.Name, sibDesc.BindPoint, cbDesc.Size }); break;
				}
			}

			// 记录内部变量
			for (UINT j = 0; j < cbDesc.Variables; ++j)
			{
				ID3D11ShaderReflectionVariable* pSRVar = pSRCBuffer->GetVariableByIndex(j);
				D3D11_SHADER_VARIABLE_DESC svDesc;
				hr = pSRVar->GetDesc(&svDesc);
				if (FAILED(hr))
					return hr;

				// 常量缓冲区的成员
				if (!isParam)
				{
					std::unordered_map<size_t, Property>::iterator it;
					bool emplaced;
					switch (shaderFlag)
					{
					case VertexShader:   std::tie(it, emplaced) = s_VertexShaders[shaderID].cBuffers[sibDesc.BindPoint].properties.try_emplace(StringToID(svDesc.Name), Property{}); break;
					case DomainShader:   std::tie(it, emplaced) = s_DomainShaders[shaderID].cBuffers[sibDesc.BindPoint].properties.try_emplace(StringToID(svDesc.Name), Property{}); break;
					case HullShader:     std::tie(it, emplaced) = s_HullShaders[shaderID].cBuffers[sibDesc.BindPoint].properties.try_emplace(StringToID(svDesc.Name), Property{}); break;
					case GeometryShader: std::tie(it, emplaced) = s_GeometryShaders[shaderID].cBuffers[sibDesc.BindPoint].properties.try_emplace(StringToID(svDesc.Name), Property{}); break;
					case PixelShader:    std::tie(it, emplaced) = s_PixelShaders[shaderID].cBuffers[sibDesc.BindPoint].properties.try_emplace(StringToID(svDesc.Name), Property{}); break;
					case ComputeShader:  std::tie(it, emplaced) = s_ComputeShaders[shaderID].cBuffers[sibDesc.BindPoint].properties.try_emplace(StringToID(svDesc.Name), Property{}); break;
					}	
					it->second.startByteOffset = svDesc.StartOffset;
					it->second.byteWidth = svDesc.Size;
				}
				
			}
		}
		// 着色器资源
		else if (sibDesc.Type == D3D_SIT_TEXTURE || sibDesc.Type == D3D_SIT_STRUCTURED || sibDesc.Type == D3D_SIT_BYTEADDRESS ||
			sibDesc.Type == D3D_SIT_TBUFFER)
		{
			std::unordered_map<size_t, ShaderResourceInfo>::iterator it;
			bool emplaced;
			switch (shaderFlag)
			{
			case VertexShader:   std::tie(it, emplaced) = s_VertexShaders[shaderID].shaderResources.try_emplace(StringToID(sibDesc.Name)); break;
			case DomainShader:   std::tie(it, emplaced) = s_DomainShaders[shaderID].shaderResources.try_emplace(StringToID(sibDesc.Name)); break;
			case HullShader:     std::tie(it, emplaced) = s_HullShaders[shaderID].shaderResources.try_emplace(StringToID(sibDesc.Name)); break;
			case GeometryShader: std::tie(it, emplaced) = s_GeometryShaders[shaderID].shaderResources.try_emplace(StringToID(sibDesc.Name)); break;
			case PixelShader:    std::tie(it, emplaced) = s_PixelShaders[shaderID].shaderResources.try_emplace(StringToID(sibDesc.Name)); break;
			case ComputeShader:  std::tie(it, emplaced) = s_ComputeShaders[shaderID].shaderResources.try_emplace(StringToID(sibDesc.Name)); break;
			}

			it->second.name = sibDesc.Name;
			it->second.dim = sibDesc.Dimension;
			it->second.startSlot = sibDesc.BindPoint;
		}
		// 采样器
		else if (sibDesc.Type == D3D_SIT_SAMPLER)
		{
			std::unordered_map<size_t, SamplerStateInfo>::iterator it;
			bool emplaced;
			switch (shaderFlag)
			{
			case VertexShader:   std::tie(it, emplaced) = s_VertexShaders[shaderID].samplers.try_emplace(StringToID(sibDesc.Name)); break;
			case DomainShader:   std::tie(it, emplaced) = s_DomainShaders[shaderID].samplers.try_emplace(StringToID(sibDesc.Name)); break;
			case HullShader:     std::tie(it, emplaced) = s_HullShaders[shaderID].samplers.try_emplace(StringToID(sibDesc.Name)); break;
			case GeometryShader: std::tie(it, emplaced) = s_GeometryShaders[shaderID].samplers.try_emplace(StringToID(sibDesc.Name)); break;
			case PixelShader:    std::tie(it, emplaced) = s_PixelShaders[shaderID].samplers.try_emplace(StringToID(sibDesc.Name)); break;
			case ComputeShader:  std::tie(it, emplaced) = s_ComputeShaders[shaderID].samplers.try_emplace(StringToID(sibDesc.Name)); break;
			}
			it->second.name = sibDesc.Name;
			it->second.startSlot = sibDesc.BindPoint;
		}
		// 可读写资源
		else if (sibDesc.Type == D3D_SIT_UAV_RWTYPED || sibDesc.Type == D3D_SIT_UAV_RWSTRUCTURED ||
			sibDesc.Type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER || sibDesc.Type == D3D_SIT_UAV_APPEND_STRUCTURED ||
			sibDesc.Type == D3D_SIT_UAV_CONSUME_STRUCTURED || sibDesc.Type == D3D_SIT_UAV_RWBYTEADDRESS)
		{
			std::unordered_map<size_t, RWResourceInfo>::iterator it;
			bool emplaced;
			switch (shaderFlag)
			{
			case PixelShader:    std::tie(it, emplaced) = s_PixelShaders[shaderID].rwResources.try_emplace(StringToID(sibDesc.Name)); break;
			case ComputeShader:  std::tie(it, emplaced) = s_ComputeShaders[shaderID].rwResources.try_emplace(StringToID(sibDesc.Name)); break;
			}
			it->second.name = sibDesc.Name;
			it->second.dim = static_cast<D3D11_UAV_DIMENSION>(sibDesc.Dimension);
			it->second.startSlot = sibDesc.BindPoint;
		}
	}

	return S_OK;
}

//
// Shader
//
Shader::Shader(std::string_view name)
	: pImpl(std::make_unique<Shader::Impl>())
{
	pImpl->m_Name = name;
}

Shader::~Shader()
{
}

void Shader::Destroy()
{
	delete this;
}

void Shader::InitFromJson(std::string_view jsonFile, std::string_view recordFile)
{
	std::ifstream fin(jsonFile.data());
	nlohmann::json json, jsonRecord;
	bool noRecord = false;
	if (!fin.is_open())
		throw std::exception("Error: shader json file is not found!");
	fin >> json;
	fin.close();
	if (!json.is_object())
		throw std::exception("Error: shader json file must start with json object!");

	if (!recordFile.empty())
	{
		fin.open(recordFile.data());
		if (!fin.is_open())
		{
			noRecord = true;
			jsonRecord = nlohmann::json::object();
		}
		else
		{
			fin >> jsonRecord;
			fin.close();
			if (!json.is_object())
				throw std::exception("Error: modified record json file must start with json object!");
		}
	}

	size_t shaderCount = json.size();
	// 读取所有Shader类
	for (auto shaderIter = json.begin(); shaderIter != json.end(); ++shaderIter)
	{
		auto& shaderName = shaderIter.key();
		auto& shaderObject = shaderIter.value();
		if (!shaderObject.is_object())
			throw std::exception("Error: Shader object must be a json object!");
		
		auto shaderPath = shaderObject["Path"].get<std::string>();
		auto& passes = shaderObject["Passes"];
		if (!passes.is_array())
			throw std::exception("Error: Passes object must be a json array!");

		std::unordered_map<size_t, std::unique_ptr<Shader, ShaderDestroyer>>::iterator iter;
		bool emplaced;
		std::tie(iter, emplaced) = s_ShaderMap.try_emplace(StringToID(shaderName));
		if (!emplaced)
			throw std::exception("Error: Duplicate Shader object name!");
		auto& pShader = iter->second = std::unique_ptr<Shader, ShaderDestroyer>(new Shader(shaderName));
		
		std::wstring wShaderPath = UTF8ToUCS2(shaderPath);
		std::wstring wShaderName = UTF8ToUCS2(shaderName);
		bool isModified = noRecord;
		if (!recordFile.empty())
		{
			// 读取本地修改时间进行比对
			HANDLE hFile = CreateFile(wShaderPath.c_str(), 0, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				std::string str = "Loading HLSL file failed, error code: " + std::to_string(GetLastError());
				throw std::exception(str.c_str());
			}
			FILETIME ft, lft;
			SYSTEMTIME st;
			GetFileTime(hFile, nullptr, nullptr, &ft);
			FileTimeToLocalFileTime(&ft, &lft);
			FileTimeToSystemTime(&lft, &st);

			auto mod_it = jsonRecord.find(shaderPath);
			if (mod_it == jsonRecord.end())
			{
				isModified = true;
				jsonRecord[shaderPath] = SystemTimeToString(st);
			}
			else
			{
				SYSTEMTIME prev = StringToSystemTime(mod_it->get<std::string>());
				if (prev.wYear != st.wYear || prev.wMonth != st.wMonth || prev.wDay != st.wDay ||
					prev.wHour != st.wHour || prev.wMinute != st.wMinute || prev.wSecond != st.wSecond)
				{
					isModified = true;
					jsonRecord[shaderPath] = SystemTimeToString(st);
				}
			}
		}

		std::map<std::string, std::map<std::string, ComPtr<ID3DBlob>>> shaderBlobs{
			{"vs", {}}, {"ds", {}}, {"hs", {}}, {"gs", {}}, {"ps", {}}, {"cs", {}}
		};

		// 读取所有pass
		for (auto passIter = passes.begin(); passIter != passes.end(); ++passIter)
		{
			auto& passObject = passIter.value();
			if (!passObject.is_object())
				throw std::exception("Error: pass object must be json object!");

			Impl::PassDesc passDesc;
			ComPtr<ID3D11RasterizerState> pRS;
			ComPtr<ID3D11DepthStencilState> pDSS;
			ComPtr<ID3D11BlendState> pBS;
			ComPtr<ID3DBlob> pErrorMsg;
			UINT stencilValue = 0;
			bool hasBlendFactor = false;
			float blendFactor[4]{};
			UINT sampleMask = 0xFFFFFFFF;
			// 读取所有shader、状态
			for (auto it = passObject.begin(); it != passObject.end(); ++it)
			{
				auto& type = it.key();

				std::string* strs = reinterpret_cast<std::string*>(&passDesc);
				static std::map<std::string, int> offsets{
					{"vs", 0}, {"ds", 1}, {"hs", 2}, {"gs", 3}, {"ps", 4}, {"cs", 5}, {"rs", -1}, {"dss", -2}, {"bs", -3}
				};

				switch (offsets[type])
				{
				case -1:
					ThrowIfFailed(CreateRasterizerStateFromJson(s_pDevice.Get(), it.value(), pRS.GetAddressOf()));
					break;
				case -2:
					ThrowIfFailed(CreateDepthStencilStateFromJson(s_pDevice.Get(), it.value(), pDSS.GetAddressOf()));
					Parse(it.value(), "StencilValue", stencilValue);
					break;
				case -3:
					ThrowIfFailed(CreateBlendStateFromJson(s_pDevice.Get(), it.value(), pBS.GetAddressOf()));
					hasBlendFactor = Parse(it.value(), "BlendFactor", blendFactor);
					Parse(it.value(), "SampleMask", sampleMask);
					break;
				default:
				{
					bool emplaced;
					std::map<std::string, ComPtr<ID3DBlob>>::iterator _iter;
					auto entryPoint = it.value().get<std::string>();
					std::tie(_iter, emplaced) = shaderBlobs[type].try_emplace(entryPoint);
					if (emplaced)
					{
						std::wstring wCsoPath = wShaderName + L'/';
						wCsoPath += UTF8ToUCS2(entryPoint);
						std::string shaderModel = type + "_5_0";
						wCsoPath += L".cso";

						CreateDirectory(wShaderName.c_str(), nullptr);
						std::string localPath = shaderPath;
						if (size_t pos; (pos = localPath.find_last_of('/')) != std::string::npos || (pos = localPath.find_last_of('\\')) != std::string::npos)
							localPath.erase(pos + 1);
						else
							localPath.clear();

						//
						// 读取 或 编译shader
						// 
						HRESULT hr = E_FAIL;
						if (!isModified)
						{
							hr = D3DReadFileToBlob(wCsoPath.c_str(), _iter->second.GetAddressOf());
						}
						if (FAILED(hr))
						{
							XShaderInclude includeHandler({"../../Include", "../../../Include"}, localPath);

							DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
							// 设置 D3DCOMPILE_DEBUG 标志用于获取着色器调试信息。该标志可以提升调试体验，
							// 但仍然允许着色器进行优化操作
							dwShaderFlags |= D3DCOMPILE_DEBUG;

							// 在Debug环境下禁用优化以避免出现一些不合理的情况
							dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
							hr = D3DCompileFromFile(wShaderPath.c_str(), nullptr, &includeHandler, entryPoint.c_str(), shaderModel.c_str(),
								dwShaderFlags, 0, _iter->second.GetAddressOf(), pErrorMsg.GetAddressOf());
							if (pErrorMsg)
							{
								OutputDebugStringA(reinterpret_cast<char*>(pErrorMsg->GetBufferPointer()));
								throw std::exception(reinterpret_cast<char*>(pErrorMsg->GetBufferPointer()));
							}
								
							ThrowIfFailed(hr);
							ThrowIfFailed(D3DWriteBlobToFile(_iter->second.Get(), wCsoPath.c_str(), isModified));
						}
							
						std::string name = shaderPath + "/" + entryPoint;
						ThrowIfFailed(Impl::AddFromBlob(name, s_pDevice.Get(), _iter->second.Get()));
						strs[offsets[type]] = name;

					}
					break;
				}
				}
			}

			auto& pass = pShader->pImpl->AddPass(passDesc);
			pass.SetRasterizerState(pRS.Get());
			pass.SetDepthStencilState(pDSS.Get(), stencilValue);
			pass.SetBlendState(pBS.Get(), hasBlendFactor ? blendFactor : nullptr, sampleMask);
			// TODO: Sampler State binding with texture
			/*pass.SetSamplerState(X_SAMPLER_LINEAR_WARP, RenderStates::SSLinearWrap.Get());
			pass.SetSamplerState(X_SAMPLER_ANISTROPIC_WRAP, RenderStates::SSAnistropicWrap.Get());
			pass.SetSamplerState(X_SAMPLER_POINT_CLAMP, RenderStates::SSPointClamp.Get());
			pass.SetSamplerState(X_SAMPLER_SHADOW, RenderStates::SSShadow.Get());*/
		}
	}

	if (!recordFile.empty())
	{
		std::ofstream fout(recordFile.data());
		fout << jsonRecord;
		fout.close();
	}
}

Shader* Shader::Find(std::string_view name)
{
	if (auto it = s_ShaderMap.find(StringToID(name)); it != s_ShaderMap.end())
		return it->second.get();
	return nullptr;
}

void Shader::SetGlobalInt(std::string_view name, int value)
{
	SetGlobalInt(StringToID(name), value);
}

void Shader::SetGlobalInt(size_t propertyId, int value)
{
	pImpl->SetGlobalRaw(propertyId, &value, 0, sizeof(int));
}

void Shader::SetGlobalFloat(std::string_view name, float value)
{
	SetGlobalFloat(StringToID(name), value);
}

void Shader::SetGlobalFloat(size_t propertyId, float value)
{
	pImpl->SetGlobalRaw(propertyId, &value, 0, sizeof(float));
}

void Shader::SetGlobalVector(std::string_view name, const XMath::Vector4& value)
{
	SetGlobalVector(StringToID(name), value);
}

void Shader::SetGlobalVector(size_t propertyId, const XMath::Vector4& value)
{
	pImpl->SetGlobalRaw(propertyId, value.data(), 0, (uint32_t)value.size());
}

void Shader::SetGlobalColor(std::string_view name, const Color& value)
{
	SetGlobalColor(StringToID(name), value);
}

void Shader::SetGlobalColor(size_t propertyId, const Color& value)
{
	pImpl->SetGlobalRaw(propertyId, value, 0, (uint32_t)sizeof value);
}

void Shader::SetGlobalMatrix(std::string_view name, const XMath::Matrix4x4& value)
{
	SetGlobalMatrix(StringToID(name), value);
}

void Shader::SetGlobalMatrix(size_t propertyId, const XMath::Matrix4x4& value)
{
	pImpl->SetGlobalRaw(propertyId, value.data(), 0, (uint32_t)sizeof value);
}

void Shader::SetGlobalVectorArray(std::string_view name, const std::vector<XMath::Vector4>& value)
{
	SetGlobalVectorArray(StringToID(name), value);
}

void Shader::SetGlobalVectorArray(size_t propertyId, const std::vector<XMath::Vector4>& value)
{
	pImpl->SetGlobalRaw(propertyId, value.data(), 0, (uint32_t)(value.size() * sizeof(XMath::Vector4)));
}

void Shader::SetGlobalMatrixArray(std::string_view name, const std::vector<XMath::Matrix4x4>& value)
{
	SetGlobalMatrixArray(StringToID(name), value);
}

void Shader::SetGlobalMatrixArray(size_t propertyId, const std::vector<XMath::Matrix4x4>& value)
{
	pImpl->SetGlobalRaw(propertyId, value.data(), 0, (uint32_t)(value.size() * sizeof(XMath::Matrix4x4)));
}

size_t Shader::GetPassCount() const
{
	return pImpl->m_Passes.size();
}
