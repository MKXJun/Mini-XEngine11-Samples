#include <Graphics/Shader.h>
#include <Graphics/RenderStates.h>
#include <wrl/client.h>
#include <memory>
#include <functional>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fstream>
#include <json.hpp>


//
// 枚举与类声明
//


enum ShaderFlag {
	PixelShader = 0x1,
	VertexShader = 0x2,
	GeometryShader = 0x4,
	HullShader = 0x8,
	DomainShader = 0x10,
	ComputeShader = 0x20,
};



// 着色器资源
struct ShaderResourceInfo
{
	std::string name;
	uint32_t startSlot = 0;
	D3D11_SRV_DIMENSION dim = D3D11_SRV_DIMENSION_UNKNOWN;
};

// 可读写资源
struct RWResourceInfo
{
	std::string name;
	uint32_t startSlot = 0;
	D3D11_UAV_DIMENSION dim = D3D11_UAV_DIMENSION_UNKNOWN;
};

// 采样器状态
struct SamplerStateInfo
{
	std::string name;
	uint32_t startSlot = 0;
};

struct CBufferBase
{
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	CBufferBase() : isDirty() {}

	BOOL isDirty;
	ComPtr<ID3D11Buffer> cBuffer;

	virtual HRESULT CreateBuffer(ID3D11Device* device) = 0;
	virtual void UpdateBuffer(ID3D11DeviceContext* deviceContext) = 0;
	virtual void BindVS(ID3D11DeviceContext* deviceContext) = 0;
	virtual void BindHS(ID3D11DeviceContext* deviceContext) = 0;
	virtual void BindDS(ID3D11DeviceContext* deviceContext) = 0;
	virtual void BindGS(ID3D11DeviceContext* deviceContext) = 0;
	virtual void BindCS(ID3D11DeviceContext* deviceContext) = 0;
	virtual void BindPS(ID3D11DeviceContext* deviceContext) = 0;
};


// 内部使用的常量缓冲区数据
struct CBufferData : CBufferBase
{
	std::vector<uint8_t> data;
	std::string cbufferName;
	uint32_t startSlot;

	CBufferData() : CBufferBase(), startSlot() {}
	CBufferData(std::string_view name, uint32_t startSlot, uint32_t byteWidth, void* initData = nullptr) :
		CBufferBase(), cbufferName(name), data(byteWidth), startSlot(startSlot)
	{
		if (initData)
			memcpy_s(data.data(), byteWidth, initData, byteWidth);
	}

	HRESULT CreateBuffer(ID3D11Device* device) override
	{
		if (cBuffer != nullptr)
			return S_OK;
		D3D11_BUFFER_DESC cbd;
		ZeroMemory(&cbd, sizeof(cbd));
		cbd.Usage = D3D11_USAGE_DYNAMIC;
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbd.ByteWidth = (uint32_t)data.size();
		return device->CreateBuffer(&cbd, nullptr, cBuffer.GetAddressOf());
	}

	void UpdateBuffer(ID3D11DeviceContext* deviceContext) override
	{
		if (isDirty)
		{
			isDirty = false;
			D3D11_MAPPED_SUBRESOURCE mappedData;
			deviceContext->Map(cBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
			memcpy_s(mappedData.pData, data.size(), data.data(), data.size());
			deviceContext->Unmap(cBuffer.Get(), 0);
		}
	}

	void BindVS(ID3D11DeviceContext* deviceContext) override
	{
		deviceContext->VSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
	}

	void BindHS(ID3D11DeviceContext* deviceContext) override
	{
		deviceContext->HSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
	}

	void BindDS(ID3D11DeviceContext* deviceContext) override
	{
		deviceContext->DSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
	}

	void BindGS(ID3D11DeviceContext* deviceContext) override
	{
		deviceContext->GSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
	}

	void BindCS(ID3D11DeviceContext* deviceContext) override
	{
		deviceContext->CSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
	}

	void BindPS(ID3D11DeviceContext* deviceContext) override
	{
		deviceContext->PSSetConstantBuffers(startSlot, 1, cBuffer.GetAddressOf());
	}
};

struct Property
{
	uint32_t startByteOffset = 0;
	uint32_t byteWidth = 0;
	CBufferData* pCBufferData = nullptr;
};

// 常量缓冲区
struct ConstantBufferInfo
{
	std::string name;
	uint32_t startSlot;
	uint32_t byteWidth;
	std::unordered_map<size_t, Property> properties;
};




struct VertexShaderInfo
{
	using PropertyID = size_t;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> pVS;

	std::map<uint32_t, ConstantBufferInfo> cBuffers;
	std::unordered_map<PropertyID, ShaderResourceInfo> shaderResources;
	std::unordered_map<PropertyID, SamplerStateInfo> samplers;

	std::vector<D3D11_INPUT_ELEMENT_DESC> signatureParams;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> pDefaultInputLayout;
};

struct DomainShaderInfo
{
	using PropertyID = size_t;

	Microsoft::WRL::ComPtr<ID3D11DomainShader> pDS;

	std::map<uint32_t, ConstantBufferInfo> cBuffers;
	std::unordered_map<PropertyID, ShaderResourceInfo> shaderResources;
	std::unordered_map<PropertyID, SamplerStateInfo> samplers;
};

struct HullShaderInfo
{
	using PropertyID = size_t;

	Microsoft::WRL::ComPtr<ID3D11HullShader> pHS;

	std::map<uint32_t, ConstantBufferInfo> cBuffers;
	std::unordered_map<PropertyID, ShaderResourceInfo> shaderResources;
	std::unordered_map<PropertyID, SamplerStateInfo> samplers;
};

struct GeometryShaderInfo
{
	using PropertyID = size_t;

	Microsoft::WRL::ComPtr<ID3D11GeometryShader> pGS;

	std::map<uint32_t, ConstantBufferInfo> cBuffers;
	std::unordered_map<PropertyID, ShaderResourceInfo> shaderResources;
	std::unordered_map<PropertyID, SamplerStateInfo> samplers;
};

struct PixelShaderInfo
{
	using PropertyID = size_t;

	Microsoft::WRL::ComPtr<ID3D11PixelShader> pPS;

	std::map<uint32_t, ConstantBufferInfo> cBuffers;
	std::unordered_map<PropertyID, ShaderResourceInfo> shaderResources;
	std::unordered_map<PropertyID, SamplerStateInfo> samplers;
	std::unordered_map<PropertyID, RWResourceInfo> rwResources;
};

struct ComputeShaderInfo
{
	using PropertyID = size_t;

	Microsoft::WRL::ComPtr<ID3D11ComputeShader> pCS;

	std::map<uint32_t, ConstantBufferInfo> cBuffers;
	std::unordered_map<PropertyID, ShaderResourceInfo> shaderResources;
	std::unordered_map<PropertyID, SamplerStateInfo> samplers;
	std::unordered_map<PropertyID, RWResourceInfo> rwResources;
};

struct ShaderPass
{
	using PropertyID = size_t;

	ShaderPass(std::map<uint32_t, CBufferData>& _cBuffers)
		: cBuffers(_cBuffers)
	{
	}
	ShaderPass(ShaderPass&&) noexcept = default;

	void SetRasterizerState(ID3D11RasterizerState* pRS);
	void SetBlendState(ID3D11BlendState* pBS, const FLOAT blendFactor[4], UINT sampleMask);
	void SetDepthStencilState(ID3D11DepthStencilState* pDSS, UINT stencilRef);

	void SetSamplerState(std::string_view name, ID3D11SamplerState* samplerState);
	void SetShaderResource(std::string_view name, ID3D11ShaderResourceView* srv);
	void SetUnorderedAccess(std::string_view name, ID3D11UnorderedAccessView* uav, UINT* initialCount);


	const std::vector<D3D11_INPUT_ELEMENT_DESC>& GetInputSignatures();
	ID3D11InputLayout* GetInputLayout();
	void Apply(ID3D11DeviceContext* deviceContext);

	// 渲染状态
	Microsoft::WRL::ComPtr<ID3D11BlendState> pBlendState = nullptr;
	std::array<float, 4> blendFactor = {};
	UINT sampleMask = 0xFFFFFFFF;

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> pRasterizerState = nullptr;

	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> pDepthStencilState = nullptr;
	UINT stencilRef = 0;


	// 着色器相关信息
	VertexShaderInfo* pVSInfo = nullptr;
	HullShaderInfo* pHSInfo = nullptr;
	DomainShaderInfo* pDSInfo = nullptr;
	GeometryShaderInfo* pGSInfo = nullptr;
	PixelShaderInfo* pPSInfo = nullptr;
	ComputeShaderInfo* pCSInfo = nullptr;

	std::unordered_map<PropertyID, std::pair<uint32_t, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>> shaderResources;
	std::unordered_map<PropertyID, std::pair<uint32_t, Microsoft::WRL::ComPtr<ID3D11SamplerState>>> samplers;
	std::unordered_map<PropertyID, std::pair<uint32_t, std::pair<Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>, std::unique_ptr<uint32_t>>>> rwResources;

	std::map<uint32_t, CBufferData>& cBuffers;
};

class Shader::Impl
{
public:
	Impl()
	{
	}
	~Impl() = default;

	struct PassDesc
	{
		std::string vsName;
		std::string hsName;
		std::string dsName;
		std::string gsName;
		std::string psName;
		std::string csName;
	};

	static bool InitAll(ID3D11Device* pDevice);
	static bool Exists(std::string_view name);

	// name: ShaderFileName/EntryPoint
	static HRESULT AddFromBlob(std::string_view name, ID3D11Device* device, ID3DBlob* blob);
	static HRESULT UpdateShaderReflection(std::string_view name, ID3D11Device* device, ID3D11ShaderReflection* pShaderReflection, UINT shaderFlag);

	void SetGlobalRaw(size_t propertyID, const void* data, uint32_t byteOffset = 0, uint32_t byteCount = 0xFFFFFFFF);
	ShaderPass& AddPass(const PassDesc& desc);

	std::string m_Name;

	std::vector<ShaderPass> m_Passes;

	std::unordered_map<size_t, Property> m_Properties;
	std::map<uint32_t, CBufferData> m_CBuffers;
};