#include "EffectManager.h"
#include "../ThirdParty/json/include/json.hpp"
#include "d3dUtil.h"
#include "DXTrace.h"
#include <fstream>
#include <vector>
#include <string>

using namespace Microsoft::WRL;

void EffectManager::CreateFromJson(std::string_view jsonPath)
{
	std::ifstream fin(jsonPath.data());
	nlohmann::json json;
	fin >> json;
	if (!json.is_array())
		throw std::exception("Error: effect json file must start with array!");
	size_t effectCount = json.size();
	for (size_t i = 0; i < effectCount; ++i)
	{
		auto& effectObject = json[i];
		if (!effectObject.is_object())
			throw std::exception("Error: effect object must be json object!");
		
		auto effectName = effectObject["effectName"].get<std::string>();
		auto effectPath = effectObject["path"].get<std::string>();
		auto& passArray = effectObject["passes"];
		auto passCount = passArray.size();

		auto pEffect = std::make_unique<Effect>();

		std::map<std::string, std::map<std::string, ComPtr<ID3DBlob>>> shaderBlobs;
		std::vector<std::string> shaderTypes = {
			"vs", "ds", "hs", "gs", "ps", "cs"
		};

		for (size_t j = 0; j < passCount; ++j)
		{
			shaderBlobs.try_emplace(shaderTypes[j]);

			auto& passObject = passArray[j];
			auto passName = passObject["passName"].get<std::string>();
			EffectPassDesc passDesc;
			size_t shaderTypeCount = shaderTypes.size();
			std::vector<std::string> shaderNames(shaderTypeCount);
			for (size_t k = 0; k < shaderTypeCount; ++k)
			{
				auto it = passObject.find(shaderTypes[k]);
				if (it != passObject.end() && !it->is_null())
				{
					shaderNames[k] = it->get<std::string>();
					LPCSTR* strs = reinterpret_cast<LPCSTR*>(&passDesc);
					strs[k] = shaderNames[k].c_str();
					
					auto it = shaderBlobs[shaderTypes[k]].find(strs[k]);
					if (it == shaderBlobs[shaderTypes[k]].end())
					{
						it = shaderBlobs[shaderTypes[k]].emplace(strs[k], nullptr).first;
						std::wstring hlslPath = UTF8ToUCS2(effectPath);
						if (hlslPath.back() != L'/' && hlslPath.back() != L'\\')
							hlslPath.push_back(L'/');
						hlslPath += UTF8ToUCS2(effectName);
						std::wstring csoPath = hlslPath + L'_';
						csoPath += UTF8ToUCS2(shaderNames[k]);
						std::string sm = shaderTypes[k] + "_5_0";
						hlslPath += L".hlsl";
						csoPath += L".cso";
						ThrowIfFailed(CreateShaderFromFile(csoPath.c_str(), hlslPath.c_str(), strs[k], sm.c_str(), it->second.GetAddressOf()));
						pEffect->AddShader(strs[k], m_pDevice.Get(), it->second.Get());
					}
				}
			}
			
			pEffect->AddEffectPass(passName.c_str(), m_pDevice.Get(), &passDesc);

		}
		m_pEffects[effectName].swap(pEffect);

	}
}
