//***************************************************************************************
// DXTrace.h by X_Jun(MKXJun) (C) 2018-2020 All Rights Reserved.
// Licensed under the MIT License.
//
// DirectX错误追踪 
// DirectX Error Tracing. 
//***************************************************************************************

#pragma once

#include <Windows.h>
#include <string>

class DxException
{
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

	std::wstring ToString() const;

	HRESULT ErrorCode = S_OK;
	std::wstring FunctionName;
	std::wstring Filename;
	int LineNumber = -1;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)												\
{																		\
    HRESULT hr__ = (x);													\
    std::wstring wfn = __FILEW__;										\
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); }	\
}
#endif
