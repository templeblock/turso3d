// For conditions of distribution and use, see copyright notice in License.txt

#include "../../Debug/Log.h"
#include "../../Debug/Profiler.h"
#include "D3D11Graphics.h"
#include "D3D11RasterizerState.h"

#include <d3d11.h>

#include "../../Debug/DebugNew.h"

namespace Turso3D
{

RasterizerState::RasterizerState() :
    stateObject(nullptr)
{
}

RasterizerState::~RasterizerState()
{
    Release();
}

void RasterizerState::Release()
{
    if (graphics)
    {
        if (graphics->GetRasterizerState() == this)
            graphics->SetRasterizerState(nullptr);
    }
    
    if (stateObject)
    {
        ID3D11RasterizerState* d3dRasterizerState = (ID3D11RasterizerState*)stateObject;
        d3dRasterizerState->Release();
        stateObject = nullptr;
    }
}

bool RasterizerState::Define(FillMode fillMode_, CullMode cullMode_, int depthBias_, float depthBiasClamp_,
    float slopeScaledDepthBias_, bool depthClipEnable_, bool scissorEnable_)

{
    PROFILE(DefineRasterizerState);

    Release();

    fillMode = fillMode_;
    cullMode = cullMode_;
    depthBias = depthBias_;
    depthBiasClamp = depthBiasClamp_;
    slopeScaledDepthBias = slopeScaledDepthBias_;
    depthClipEnable = depthClipEnable_;
    scissorEnable = scissorEnable_;

    if (graphics && graphics->IsInitialized())
    {
        D3D11_RASTERIZER_DESC stateDesc;
        memset(&stateDesc, 0, sizeof stateDesc);

        stateDesc.FillMode = (D3D11_FILL_MODE)fillMode;
        stateDesc.CullMode = (D3D11_CULL_MODE)cullMode;
        stateDesc.FrontCounterClockwise = FALSE;
        stateDesc.DepthBias = depthBias;
        stateDesc.DepthBiasClamp = depthBiasClamp;
        stateDesc.SlopeScaledDepthBias = slopeScaledDepthBias;
        stateDesc.DepthClipEnable = depthClipEnable;
        stateDesc.ScissorEnable = scissorEnable;
        stateDesc.MultisampleEnable = TRUE;
        stateDesc.AntialiasedLineEnable = FALSE;

        ID3D11Device* d3dDevice = (ID3D11Device*)graphics->D3DDevice();
        d3dDevice->CreateRasterizerState(&stateDesc, (ID3D11RasterizerState**)&stateObject);

        if (!stateObject)
        {
            LOGERROR("Failed to create rasterizer state");
            return false;
        }
        else
            LOGDEBUG("Created rasterizer state");
    }

    return true;
}

}
