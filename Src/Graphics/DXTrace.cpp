#include "DXTrace.h"
#include <cstdio>

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber)
	: ErrorCode(hr), FunctionName(functionName), Filename(filename), LineNumber(lineNumber)
{
}

std::wstring DxException::ToString() const
{
	WCHAR strBufferError[300];
	WCHAR strBufferHR[40];
	
	// Windows SDK 8.0起DirectX的错误信息已经集成进错误码中，可以通过FormatMessageW获取错误信息字符串
	// 不需要分配字符串内存
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, ErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		strBufferError, 256, nullptr);

	WCHAR* errorStr = wcsrchr(strBufferError, L'\r');
	if (errorStr)
	{
		errorStr[0] = L'\0';	// 擦除FormatMessageW带来的换行符(把\r\n的\r置换为\0即可)
	}

	swprintf_s(strBufferHR, 40, L" (0x%0.8x)", ErrorCode);
	wcscat_s(strBufferError, strBufferHR);


	std::wstring message = L"文件名：" + Filename + 
		L"\n行号：" + std::to_wstring(LineNumber) + 
		L"\n错误码含义：" + strBufferError +
		L"\n当前调用：" + FunctionName;

	return message;
}
