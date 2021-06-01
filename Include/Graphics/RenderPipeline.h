#pragma once

#include "RenderContext.h"
#include <Component/Camera.h>

class RenderPipeline
{
public:
    virtual void Render(RenderContext* pContext, std::vector<Camera*>& pCameras) = 0;
};
