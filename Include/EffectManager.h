
#pragma once

#include <string_view>
#include <wrl/client.h>
#include <map>
#include "Effect.h"

class EffectManager
{
public:

	static EffectManager& Get();

	void CreateFromJson(std::string_view jsonPath);
	Effect* GetEffect(std::string_view effectName);

	EffectManager(ID3D11Device* device) : m_pDevice(device) {}
	~EffectManager() = default;

private:

	Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;

	std::map<std::string, std::unique_ptr<Effect>> m_pEffects;
};
