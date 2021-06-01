#pragma once

#include <wrl/client.h>
#include <Graphics/ResourceManager.h>
#include <Graphics/Shader.h>
#include <Graphics/RenderPipeline.h>
// TODO LIST:
// [ ] Mouse mode switching problem.
// [ ] ImGui support.
// [ ] Template Project.
// [ ] GAMES202 HW0.
// [*] Effects.json need to support RenderStates.

RenderPipeline* GetRenderPipeline();

#define CREATE_CUSTOM_RENDERPIPELINE(ClassName) \
RenderPipeline* GetRenderPipeline() \
{\
	static std::unique_ptr<ClassName> s_pRenderPipeline;\
	if (!s_pRenderPipeline)\
		s_pRenderPipeline = std::make_unique<ClassName>();\
	return s_pRenderPipeline.get();\
}

#define USE_WINMAIN \
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd);

#define USE_MAIN \
int main(int argc, char* argv[]);