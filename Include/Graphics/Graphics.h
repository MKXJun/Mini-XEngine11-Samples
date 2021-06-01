#pragma once

#include <wrl/client.h>
#include <Graphics/ResourceManager.h>
#include <Graphics/Shader.h>
#include <Graphics/RenderPipeline.h>
#include <Graphics/RenderContext.h>
#include <Graphics/CommandBuffer.h>
#include <Math/XMath.h>


class Graphics
{
private:
	friend class MainWindow;
	friend class RenderContext;
	friend class CommandBuffer;
	class Impl;
};

//
// Settings
// Exe
// DrawMesh Command
// 