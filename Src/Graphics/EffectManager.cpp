
#include <Graphics/EffectManager.h>
#include <Graphics/RenderStates.h>

#include <fstream>
#include <vector>
#include <wrl/client.h>

#include <HLSL/XDefines.h>

#include "d3dUtil.h"
#include "DXTrace.h"

#include <json.hpp>


using namespace Microsoft::WRL;

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
	typename std::enable_if_t<std::is_signed_v<Type> && std::is_integral_v<Type>>* = nullptr>
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
	typename std::enable_if_t<std::is_unsigned_v<Type> && std::is_integral_v<Type>>* = nullptr>
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
	typename std::enable_if_t<std::is_array_v<Type> && std::is_arithmetic_v<std::remove_extent_t<Type>>>* = nullptr>
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


namespace
{
	EffectManager* s_pSingleton = nullptr;
}

EffectManager::EffectManager(ID3D11Device* device)
	: m_pDevice(device)
{
	if (s_pSingleton)
		throw std::exception("EffectManager is a singleton!");
	s_pSingleton = this;
}

EffectManager::~EffectManager()
{
}

EffectManager& EffectManager::Get()
{
	return *s_pSingleton;
}

void EffectManager::CreateEffectsFromJson(std::string_view jsonPath, std::string_view modifiedRecordJsonPath)

{
	std::ifstream fin(jsonPath.data());
	nlohmann::json json, jsonRecord;
	bool noRecord = false;
	if (!fin.is_open())
		throw std::exception("Error: effect json file is not found!");
	fin >> json;
	fin.close();
	if (!json.is_object())
		throw std::exception("Error: effect json file must start with json object!");

	if (!modifiedRecordJsonPath.empty())
	{
		fin.open(modifiedRecordJsonPath.data());
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

	size_t effectCount = json.size();
	// 读取所有effect
	for (auto effectIter = json.begin(); effectIter != json.end(); ++effectIter)
	{
		auto& effectName = effectIter.key();
		auto& effectObject = effectIter.value();
		if (!effectObject.is_object())
			throw std::exception("Error: effect object must be json object!");
		auto effectPath = effectObject["Path"].get<std::string>();
		
		auto& passes = effectObject["Passes"];
		auto pEffect = std::make_unique<Effect>();

		std::wstring prefix = UTF8ToUCS2(effectPath);
		if (prefix.back() != L'/' && prefix.back() != L'\\')
			prefix.push_back(L'/');
		prefix += UTF8ToUCS2(effectName);
		std::wstring hlslPath = prefix + L".hlsl";
		bool isModified = noRecord;
		if (!modifiedRecordJsonPath.empty())
		{
			// 读取本地修改时间进行比对
			HANDLE hFile = CreateFile(hlslPath.c_str(), 0, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
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

			auto mod_it = jsonRecord.find(effectName);
			if (mod_it == jsonRecord.end())
			{
				isModified = true;
				jsonRecord[effectName] = SystemTimeToString(st);
			}
			else
			{
				SYSTEMTIME prev = StringToSystemTime(mod_it->get<std::string>());
				if (prev.wYear != st.wYear || prev.wMonth != st.wMonth || prev.wDay != st.wDay ||
					prev.wHour != st.wHour || prev.wMinute != st.wMinute || prev.wSecond != st.wSecond)
				{
					isModified = true;
					jsonRecord[effectName] = SystemTimeToString(st);
				}
			}
		}

		std::map<std::string, std::map<std::string, ComPtr<ID3DBlob>>> shaderBlobs{
			{"vs", {}}, {"ds", {}}, {"hs", {}}, {"gs", {}}, {"ps", {}}, {"cs", {}}
		};

		// 读取所有pass
		for (auto passIter = passes.begin(); passIter != passes.end(); ++passIter)
		{
			auto& passName = passIter.key();
			auto& passObject = passIter.value();
			if (!passObject.is_object())
				throw std::exception("Error: pass object must be json object!");
			
			EffectPassDesc passDesc;
			ComPtr<ID3D11RasterizerState> pRS;
			ComPtr<ID3D11DepthStencilState> pDSS;
			ComPtr<ID3D11BlendState> pBS;
			UINT stencilValue = 0;
			bool hasBlendFactor = false;
			float blendFactor[4]{};
			UINT sampleMask = 0xFFFFFFFF;
			// 读取所有shader、状态
			for (auto it = passObject.begin(); it != passObject.end(); ++it)
			{
				auto& type = it.key();

				LPCSTR* strs = reinterpret_cast<LPCSTR*>(&passDesc);
				static std::map<std::string, int> offsets {
					{"vs", 0}, {"ds", 1}, {"hs", 2}, {"gs", 3}, {"ps", 4}, {"cs", 5}, {"rs", -1}, {"dss", -2}, {"bs", -3}
				};
				
				switch (offsets[type])
				{
				case -1:
					ThrowIfFailed(CreateRasterizerStateFromJson(m_pDevice.Get(), it.value(), pRS.GetAddressOf()));
					break;
				case -2:
					ThrowIfFailed(CreateDepthStencilStateFromJson(m_pDevice.Get(), it.value(), pDSS.GetAddressOf()));
					Parse(it.value(), "StencilValue", stencilValue);
					break;
				case -3:
					ThrowIfFailed(CreateBlendStateFromJson(m_pDevice.Get(), it.value(), pBS.GetAddressOf()));
					hasBlendFactor = Parse(it.value(), "BlendFactor", blendFactor);
					Parse(it.value(), "SampleMask", sampleMask);
					break;
				default:
				{
					bool emplaced;
					std::map<std::string, ComPtr<ID3DBlob>>::iterator iter;
					auto entryPoint = it.value().get<std::string>();
					std::tie(iter, emplaced) = shaderBlobs[type].try_emplace(entryPoint);
					if (emplaced)
					{
						strs[offsets[type]] = iter->first.c_str();

						std::wstring csoPath = prefix + L'_';
						csoPath += UTF8ToUCS2(entryPoint);
						std::string shaderModel = type + "_5_0";
						csoPath += L".cso";

						if (isModified)
							DeleteFile(csoPath.c_str());

						ThrowIfFailed(CreateShaderFromFile(csoPath.c_str(), hlslPath.c_str(), entryPoint.c_str(), shaderModel.c_str(),
							iter->second.GetAddressOf()));
						ThrowIfFailed(pEffect->AddShader(entryPoint.c_str(), m_pDevice.Get(), iter->second.Get()));
					}
					break;
				}
				}
			}

			ThrowIfFailed(pEffect->AddEffectPass(passName.c_str(), m_pDevice.Get(), &passDesc));
			auto pPass = pEffect->GetEffectPass(passName.c_str());
			pPass->SetRasterizerState(pRS.Get());
			pPass->SetDepthStencilState(pDSS.Get(), stencilValue);
			pPass->SetBlendState(pBS.Get(), hasBlendFactor ? blendFactor : nullptr, sampleMask);
		}

		int slot = pEffect->GetSamplerStateByName(X_SAMPLER_LINEAR_WARP);
		if (slot != -1) pEffect->SetSamplerStateBySlot(slot, RenderStates::SSLinearWrap.Get());
		slot = pEffect->GetSamplerStateByName(X_SAMPLER_ANISTROPIC_WRAP);
		if (slot != -1) pEffect->SetSamplerStateBySlot(slot, RenderStates::SSAnistropicWrap.Get());
		slot = pEffect->GetSamplerStateByName(X_SAMPLER_POINT_CLAMP);
		if (slot != -1) pEffect->SetSamplerStateBySlot(slot, RenderStates::SSPointClamp.Get());
		slot = pEffect->GetSamplerStateByName(X_SAMPLER_SHADOW);
		if (slot != -1) pEffect->SetSamplerStateBySlot(slot, RenderStates::SSShadow.Get());



		m_pEffects[effectName].swap(pEffect);

	}

	if (!modifiedRecordJsonPath.empty())
	{
		std::ofstream fout(modifiedRecordJsonPath.data());
		fout << jsonRecord;
		fout.close();
	}
}

bool EffectManager::RegisterEffect(std::string_view effectName, std::unique_ptr<Effect>&& pEffect)
{
	return m_pEffects.try_emplace(effectName.data(), std::move(pEffect)).second;
}

Effect* EffectManager::FindEffect(std::string_view effectName)
{
	auto it = m_pEffects.find(effectName.data());
	if (it != m_pEffects.end())
		return it->second.get();
	return nullptr;
}


