
#pragma once

#include <string_view>
#include <wrl/client.h>
#include <d3d11_1.h>
#include <map>

#include <Graphics/Effect.h>

class EffectManager
{
public:
	EffectManager(ID3D11Device* device);
	~EffectManager();

	static EffectManager& Get();

	void CreateEffectsFromJson(std::string_view jsonPath, std::string_view modifiedRecordJsonPath = "");
	bool RegisterEffect(std::string_view effectName, std::unique_ptr<Effect>&& pEffect);
	Effect* FindEffect(std::string_view effectName);

private:
	std::map<std::string, std::unique_ptr<Effect>> m_pEffects;

	Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;
};
