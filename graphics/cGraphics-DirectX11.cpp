// cGraphics-DirectX11.cpp - directX11 backend for imGui
//{{{  includes
#include "cGraphics.h"

#include <stdio.h>
#include <tchar.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <imgui.h>

#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

namespace {
  //{{{
  struct sVertexConstantBuffer {
    float mMatrix[4][4];
    };
  //}}}
  //{{{
  struct sViewportData {
    IDXGISwapChain* mSwapChain;
    ID3D11RenderTargetView* mRTView;

    sViewportData() {
      mSwapChain = NULL;
      mRTView = NULL;
      }

    ~sViewportData() {
      IM_ASSERT(mSwapChain == NULL && mRTView == NULL);
      }
    };
  //}}}
  //{{{
  struct sBackendData {
    ID3D11Device*             mD3dDevice;
    ID3D11DeviceContext*      mD3dDeviceContext;
    IDXGIFactory*             mDxgiFactory;
    ID3D11Buffer*             mVB;
    ID3D11Buffer*             mIB;
    ID3D11VertexShader*       mVertexShader;
    ID3D11InputLayout*        mInputLayout;
    ID3D11Buffer*             mVertexConstantBuffer;
    ID3D11PixelShader*        mPixelShader;
    ID3D11SamplerState*       mFontSampler;
    ID3D11ShaderResourceView* mFontTextureView;
    ID3D11RasterizerState*    mRasterizerState;
    ID3D11BlendState*         mBlendState;
    ID3D11DepthStencilState*  mDepthStencilState;
    int                       mVertexBufferSize;
    int                       mIndexBufferSize;

    IDXGISwapChain*           mDxgiSwapChain;
    ID3D11RenderTargetView*   mMainRenderTargetView;

    sBackendData() {
      memset (this, 0, sizeof(*this));
      mVertexBufferSize = 5000;
      mIndexBufferSize = 10000;
      }
    };
  //}}}
  //{{{
  // Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
  // It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
  sBackendData* getBackendData() {
    return ImGui::GetCurrentContext() ? (sBackendData*)ImGui::GetIO().BackendRendererUserData : NULL;
    }
  //}}}

  //{{{
  bool createResources() {

    sBackendData* backendData = getBackendData();

    //{{{  create vertex shader
    static const char* kVertexShaderStr =
      "cbuffer vertexBuffer : register(b0) {"
      "  float4x4 ProjectionMatrix;"
      "  };"

      "struct VS_INPUT {"
      "  float2 pos : POSITION;"
      "  float4 col : COLOR0;"
      "  float2 uv  : TEXCOORD0;"
      "  };"

      "struct PS_INPUT {"
      "  float4 pos : SV_POSITION;"
      "  float4 col : COLOR0;"
      "  float2 uv  : TEXCOORD0;"
      "  };"

      "PS_INPUT main (VS_INPUT input) {"
      "  PS_INPUT output;"
      "  output.pos = mul (ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));"
      "  output.col = input.col;"
      "  output.uv  = input.uv;"
      "  return output;"
      "  }";

    ID3DBlob* vertexShaderBlob;
    if (FAILED (D3DCompile (kVertexShaderStr, strlen(kVertexShaderStr),
                            NULL, NULL, NULL, "main", "vs_4_0", 0, 0, &vertexShaderBlob, NULL))) {
      //{{{  error, return false
      cLog::log (LOGERROR, "vertex shader compile failed");
      return false;
      }
      //}}}

    if (backendData->mD3dDevice->CreateVertexShader (vertexShaderBlob->GetBufferPointer(),
                                                     vertexShaderBlob->GetBufferSize(),
                                                     NULL, &backendData->mVertexShader) != S_OK) {
      //{{{  error, return false
      cLog::log (LOGERROR, "vertext shader create failed");
      vertexShaderBlob->Release();
      return false;
      }
      //}}}

    // create input layout
    D3D11_INPUT_ELEMENT_DESC kInputLayout[] = {
      { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
      };

    if (backendData->mD3dDevice->CreateInputLayout (kInputLayout, 3,
                                                    vertexShaderBlob->GetBufferPointer(),
                                                    vertexShaderBlob->GetBufferSize(),
                                                    &backendData->mInputLayout) != S_OK) {
      //{{{  error, return false
      cLog::log (LOGERROR, "inputLayout create failed");
      vertexShaderBlob->Release();
      return false;
      }
      //}}}

    vertexShaderBlob->Release();
    //}}}

    // create vertexConstant buffer
    D3D11_BUFFER_DESC bufferDesc;
    bufferDesc.ByteWidth = sizeof(sVertexConstantBuffer);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = 0;
    backendData->mD3dDevice->CreateBuffer (&bufferDesc, NULL, &backendData->mVertexConstantBuffer);

    //{{{  create pixel shader
    static const char* kPixelShaderStr =
      "struct PS_INPUT {"
      "  float4 pos : SV_POSITION;"
      "  float4 col : COLOR0;"
      "  float2 uv  : TEXCOORD0;"
      "  };"

      "sampler sampler0;"
      "texture2D texture0;"

      "float4 main (PS_INPUT input) : SV_Target {"
      "  float4 out_col = input.col * texture0.Sample (sampler0, input.uv);"
      "  return out_col;"
      "  }";

    ID3DBlob* pixelShaderBlob;
    if (FAILED (D3DCompile (kPixelShaderStr, strlen(kPixelShaderStr),
                            NULL, NULL, NULL, "main", "ps_4_0", 0, 0, &pixelShaderBlob, NULL))) {
      //{{{  error, return false
      cLog::log (LOGERROR, "pixel shader compile failed");
      return false;
      }
      //}}}

    if (backendData->mD3dDevice->CreatePixelShader (pixelShaderBlob->GetBufferPointer(),
                                                    pixelShaderBlob->GetBufferSize(),
                                                    NULL, &backendData->mPixelShader) != S_OK) {
      //{{{  error, return false
      cLog::log (LOGERROR, "pixel shader create failed");
      pixelShaderBlob->Release();
      return false;
      }
      //}}}

    pixelShaderBlob->Release();
    //}}}

    // create blendDesc
    D3D11_BLEND_DESC blendDesc;
    ZeroMemory (&blendDesc, sizeof(blendDesc));
    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.RenderTarget[0].BlendEnable = true;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    backendData->mD3dDevice->CreateBlendState (&blendDesc, &backendData->mBlendState);

    // create rasterizerDesc
    D3D11_RASTERIZER_DESC rasterizerDesc;
    ZeroMemory (&rasterizerDesc, sizeof(rasterizerDesc));
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.ScissorEnable = true;
    rasterizerDesc.DepthClipEnable = true;
    backendData->mD3dDevice->CreateRasterizerState (&rasterizerDesc, &backendData->mRasterizerState);

    // create depthStencilDesc
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    ZeroMemory (&depthStencilDesc, sizeof(depthStencilDesc));
    depthStencilDesc.DepthEnable = false;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    depthStencilDesc.StencilEnable = false;
    depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    depthStencilDesc.BackFace = depthStencilDesc.FrontFace;
    backendData->mD3dDevice->CreateDepthStencilState (&depthStencilDesc, &backendData->mDepthStencilState);

    return true;
    }
  //}}}
  //{{{
  bool createFontTexture() {

    unsigned char* pixels;
    int width;
    int height;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32 (&pixels, &width, &height);

    sBackendData* backendData = getBackendData();

    // create and upload texture to gpu
    D3D11_TEXTURE2D_DESC texture2dDesc;
    ZeroMemory (&texture2dDesc, sizeof(texture2dDesc));
    texture2dDesc.Width = width;
    texture2dDesc.Height = height;
    texture2dDesc.MipLevels = 1;
    texture2dDesc.ArraySize = 1;
    texture2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture2dDesc.SampleDesc.Count = 1;
    texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
    texture2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texture2dDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = pixels;
    subResource.SysMemPitch = texture2dDesc.Width * 4;
    subResource.SysMemSlicePitch = 0;

    ID3D11Texture2D* texture2d = NULL;
    if (backendData->mD3dDevice->CreateTexture2D (&texture2dDesc, &subResource, &texture2d) != S_OK) {
      //{{{  error, returm false
      cLog::log (LOGERROR, "createFontTexture CreateTexture2D failed");
      return false;
      }
      //}}}

    // create textureView
    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
    ZeroMemory (&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
    shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MipLevels = texture2dDesc.MipLevels;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    if (backendData->mD3dDevice->CreateShaderResourceView (texture2d, &shaderResourceViewDesc,
                                                           &backendData->mFontTextureView) != S_OK) {
      //{{{  error, return false
      cLog::log (LOGERROR, "createFontTexture CreateShaderResourceView failed");
      return false;
      }
      //}}}
    texture2d->Release();

    // store our fonts texture view
    ImGui::GetIO().Fonts->SetTexID ((ImTextureID)backendData->mFontTextureView);

    // create texture sampler
    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory (&samplerDesc, sizeof(samplerDesc));
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MipLODBias = 0.f;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.MinLOD = 0.f;
    samplerDesc.MaxLOD = 0.f;
    if (backendData->mD3dDevice->CreateSamplerState (&samplerDesc, &backendData->mFontSampler) != S_OK) {
      //{{{  error, return false
      cLog::log (LOGERROR, "createFontTexture CreateSamplerState failed");
      return false;
      }
      //}}}

    return true;
    }
  //}}}
  //{{{
  bool createMainRenderTarget() {

    bool ok = true;

    sBackendData* backendData = getBackendData();
    ID3D11Texture2D* backBuffer;
    backendData->mDxgiSwapChain->GetBuffer (0, IID_PPV_ARGS (&backBuffer));

    if (backendData->mD3dDevice->CreateRenderTargetView (backBuffer, NULL, &backendData->mMainRenderTargetView) != S_OK) {
      cLog::log (LOGERROR, "createMainRenderTarget failed");
      ok = false;
      }

    backBuffer->Release();

    return ok;
    }
  //}}}

  //{{{
  void setupRenderState (ImDrawData* drawData) {

    sBackendData* backendData = getBackendData();
    ID3D11DeviceContext* deviceContext = backendData->mD3dDeviceContext;

    // set shader, vertex buffers
    unsigned int stride = sizeof(ImDrawVert);
    unsigned int offset = 0;
    deviceContext->IASetInputLayout (backendData->mInputLayout);
    deviceContext->IASetVertexBuffers (0, 1, &backendData->mVB, &stride, &offset);
    deviceContext->IASetIndexBuffer (backendData->mIB, sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
    deviceContext->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext->VSSetShader (backendData->mVertexShader, NULL, 0);
    deviceContext->VSSetConstantBuffers (0, 1, &backendData->mVertexConstantBuffer);
    deviceContext->PSSetShader (backendData->mPixelShader, NULL, 0);
    deviceContext->PSSetSamplers (0, 1, &backendData->mFontSampler);
    deviceContext->GSSetShader (NULL, NULL, 0);
    deviceContext->HSSetShader (NULL, NULL, 0);
    deviceContext->DSSetShader (NULL, NULL, 0);
    deviceContext->CSSetShader (NULL, NULL, 0);

    // set blend state
    const float blend_factor[4] = { 0.f,0.f, 0.f,0.f };
    deviceContext->OMSetBlendState (backendData->mBlendState, blend_factor, 0xffffffff);
    deviceContext->OMSetDepthStencilState (backendData->mDepthStencilState, 0);
    deviceContext->RSSetState (backendData->mRasterizerState);

    // set viewport
    D3D11_VIEWPORT viewport;
    memset (&viewport, 0, sizeof(D3D11_VIEWPORT));
    viewport.Width = drawData->DisplaySize.x;
    viewport.Height = drawData->DisplaySize.y;
    viewport.MinDepth = 0.f;
    viewport.MaxDepth = 1.f;
    viewport.TopLeftX = viewport.TopLeftY = 0;
    deviceContext->RSSetViewports (1, &viewport);
    }
  //}}}
  //{{{
  void renderDrawData (ImDrawData* drawData) {

    if ((drawData->DisplaySize.x > 0.f) && (drawData->DisplaySize.y > 0.f)) {
      sBackendData* backendData = getBackendData();
      ID3D11DeviceContext* deviceContext = backendData->mD3dDeviceContext;

      // copy drawList vertices,indices to continuous GPU buffers
      //{{{  manage vertex GPU buffer
      if (!backendData->mVB || (backendData->mVertexBufferSize < drawData->TotalVtxCount)) {
        // need new vertexBuffer
        if (backendData->mVB) {
          // release old vertexBuffer
          backendData->mVB->Release();
          backendData->mVB = NULL;
          }
        backendData->mVertexBufferSize = drawData->TotalVtxCount + 5000;

        // get new vertexBuffer
        D3D11_BUFFER_DESC desc;
        memset (&desc, 0, sizeof(D3D11_BUFFER_DESC));
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = backendData->mVertexBufferSize * sizeof(ImDrawVert);
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        if (backendData->mD3dDevice->CreateBuffer (&desc, NULL, &backendData->mVB) < 0) {
          cLog::log (LOGERROR, "vertex CreateBuffer failed");
          return;
          }
        }

      // map gpu vertexBuffer
      D3D11_MAPPED_SUBRESOURCE vertexSubResource;
      if (deviceContext->Map (backendData->mVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &vertexSubResource) != S_OK) {
        cLog::log (LOGERROR, "vertexBuffer Map failed");
        return;
        }

      ImDrawVert* vertexDest = (ImDrawVert*)vertexSubResource.pData;
      //}}}
      //{{{  manage index GPU buffer
      if (!backendData->mIB || (backendData->mIndexBufferSize < drawData->TotalIdxCount)) {
        // need new indexBuffer
        if (backendData->mIB) {
          // release old indexBuffer
          backendData->mIB->Release();
          backendData->mIB = NULL;
          }
        backendData->mIndexBufferSize = drawData->TotalIdxCount + 10000;

        // get new indexBuffer
        D3D11_BUFFER_DESC desc;
        memset (&desc, 0, sizeof(D3D11_BUFFER_DESC));
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = backendData->mIndexBufferSize * sizeof(ImDrawIdx);
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (backendData->mD3dDevice->CreateBuffer (&desc, NULL, &backendData->mIB) < 0) {
          cLog::log (LOGERROR, "index CreateBuffer failed");
          return;
          }
        }

      // map gpu indexBuffer
      D3D11_MAPPED_SUBRESOURCE indexSubResource;
      if (deviceContext->Map (backendData->mIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &indexSubResource) != S_OK) {
        cLog::log (LOGERROR, "indexBuffer Map failed");
        return;
        }
      ImDrawIdx* indexDest = (ImDrawIdx*)indexSubResource.pData;
      //}}}
      for (int n = 0; n < drawData->CmdListsCount; n++) {
        const ImDrawList* cmdList = drawData->CmdLists[n];
        memcpy (vertexDest, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy (indexDest, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
        vertexDest += cmdList->VtxBuffer.Size;
        indexDest += cmdList->IdxBuffer.Size;
        }
      deviceContext->Unmap (backendData->mVB, 0);
      deviceContext->Unmap (backendData->mIB, 0);

      setupRenderState (drawData);
      //{{{  set orthoProject matrix in GPU vertexConstantBuffer
      const float L = drawData->DisplayPos.x;
      const float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
      const float T = drawData->DisplayPos.y;
      const float B = drawData->DisplayPos.y + drawData->DisplaySize.y;

      const float kMatrix[4][4] = {
         2.0f/(R-L),  0.0f,        0.0f, 0.0f ,
         0.0f,        2.0f/(T-B),  0.0f, 0.0f ,
         0.0f,        0.0f,        0.5f, 0.0f ,
         (R+L)/(L-R), (T+B)/(B-T), 0.5f, 1.0f ,
        };

      // copy kMatrix to mapped matrix vertexConstantBuffer
      D3D11_MAPPED_SUBRESOURCE mappedSubResource;
      if (deviceContext->Map (backendData->mVertexConstantBuffer,
                              0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource) != S_OK) {
        cLog::log (LOGERROR, "vertexConstant Map failed");
        return;
        }
      memcpy ((float*)mappedSubResource.pData, kMatrix, sizeof(kMatrix));
      deviceContext->Unmap (backendData->mVertexConstantBuffer, 0);
      //}}}

      // render command lists
      int indexOffset = 0;
      int vertexOffset = 0;
      ImVec2 clipOffset = drawData->DisplayPos;

      for (int cmdListIndex = 0; cmdListIndex < drawData->CmdListsCount; cmdListIndex++) {
        const ImDrawList* cmdList = drawData->CmdLists[cmdListIndex];
        for (int cmdIndex = 0; cmdIndex < cmdList->CmdBuffer.Size; cmdIndex++) {
          const ImDrawCmd* drawCmd = &cmdList->CmdBuffer[cmdIndex];

          // set scissor clip rect
          const D3D11_RECT r = {
            (LONG)(drawCmd->ClipRect.x - clipOffset.x), (LONG)(drawCmd->ClipRect.y - clipOffset.y),
            (LONG)(drawCmd->ClipRect.z - clipOffset.x), (LONG)(drawCmd->ClipRect.w - clipOffset.y) };
          deviceContext->RSSetScissorRects (1, &r);

          // bind texture
          ID3D11ShaderResourceView* shaderResourceView = (ID3D11ShaderResourceView*)drawCmd->GetTexID();
          deviceContext->PSSetShaderResources (0, 1, &shaderResourceView);

          // draw
          deviceContext->DrawIndexed (drawCmd->ElemCount, drawCmd->IdxOffset + indexOffset, drawCmd->VtxOffset + vertexOffset);
          }

        indexOffset += cmdList->IdxBuffer.Size;
        vertexOffset += cmdList->VtxBuffer.Size;
        }
      }
    }
  //}}}

  // platform interface
  //{{{
  void createWindow (ImGuiViewport* viewport) {

    cLog::log (LOGINFO, "createWindow");

    sBackendData* backendData = getBackendData();
    sViewportData* viewportData = IM_NEW (sViewportData)();
    viewport->RendererUserData = viewportData;

    // platformHandleRaw should always be a HWND, whereas PlatformHandle might be a higher-level handle (e.g. GLFWWindow*, SDL_Window*).
    // Some backend will leave PlatformHandleRaw NULL, in which case we assume PlatformHandle will contain the HWND.
    HWND hWnd = viewport->PlatformHandleRaw ? (HWND)viewport->PlatformHandleRaw : (HWND)viewport->PlatformHandle;
    IM_ASSERT(hWnd != 0);

    // create swap chain
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory (&swapChainDesc, sizeof(swapChainDesc));
    swapChainDesc.BufferDesc.Width = (UINT)viewport->Size.x;
    swapChainDesc.BufferDesc.Height = (UINT)viewport->Size.y;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 1;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.Flags = 0;
    IM_ASSERT ((viewportData->mSwapChain == NULL) && (viewportData->mRTView == NULL));
    backendData->mDxgiFactory->CreateSwapChain (backendData->mD3dDevice, &swapChainDesc, &viewportData->mSwapChain);

    // create the render target
    if (viewportData->mSwapChain) {
      ID3D11Texture2D* backBuffer;
      viewportData->mSwapChain->GetBuffer (0, IID_PPV_ARGS (&backBuffer));
      backendData->mD3dDevice->CreateRenderTargetView (backBuffer, NULL, &viewportData->mRTView);
      backBuffer->Release();
      }
    }
  //}}}
  //{{{
  void destroyWindow (ImGuiViewport* viewport) {
  // The main viewport (owned by the application) will always have RendererUserData == NULL since we didn't create the data for it.

    cLog::log (LOGINFO, "destroyWindow");

    if (sViewportData* viewportData = (sViewportData*)viewport->RendererUserData) {
      if (viewportData->mSwapChain)
        viewportData->mSwapChain->Release();
      viewportData->mSwapChain = NULL;

      if (viewportData->mRTView)
        viewportData->mRTView->Release();
      viewportData->mRTView = NULL;

      IM_DELETE(viewportData);
      }

    viewport->RendererUserData = NULL;
    }
  //}}}
  //{{{
  void setWindowSize (ImGuiViewport* viewport, ImVec2 size) {

    cLog::log (LOGINFO, "setWindowSize");

    sViewportData* viewportData = (sViewportData*)viewport->RendererUserData;
    if (viewportData->mRTView) {
      viewportData->mRTView->Release();
      viewportData->mRTView = NULL;
      }

    if (viewportData->mSwapChain) {
      ID3D11Texture2D* backBuffer = NULL;
      viewportData->mSwapChain->ResizeBuffers (0, (UINT)size.x, (UINT)size.y, DXGI_FORMAT_UNKNOWN, 0);
      viewportData->mSwapChain->GetBuffer (0, IID_PPV_ARGS(&backBuffer));
      if (backBuffer == NULL) {
        fprintf (stderr, "SetWindowSize() failed creating buffers.\n");
        return;
        }

      sBackendData* backendData = getBackendData();
      backendData->mD3dDevice->CreateRenderTargetView (backBuffer, NULL, &viewportData->mRTView);
      backBuffer->Release();
      }
    }
  //}}}
  //{{{
  void renderWindow (ImGuiViewport* viewport, void*) {

    cLog::log (LOGINFO, "renderWindow");

    sBackendData* backendData = getBackendData();
    sViewportData* viewportData = (sViewportData*)viewport->RendererUserData;

    ImVec4 clearColor = ImVec4 (0.0f, 0.0f, 0.0f, 1.0f);
    backendData->mD3dDeviceContext->OMSetRenderTargets (1, &viewportData->mRTView, NULL);
    if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear))
      backendData->mD3dDeviceContext->ClearRenderTargetView (viewportData->mRTView, (float*)&clearColor);

    renderDrawData (viewport->DrawData);
    }
  //}}}
  //{{{
  void swapBuffers (ImGuiViewport* viewport, void*) {

    cLog::log (LOGINFO, "swapBuffers");

    sViewportData* viewportData = (sViewportData*)viewport->RendererUserData;
    viewportData->mSwapChain->Present (0,0); // Present without vsync
    }
  //}}}
  }

//{{{
bool cGraphics::init (void* device, void* deviceContext, void* swapChain) {

  bool ok = false;

  // allocate backendData
  sBackendData* backendData = IM_NEW (sBackendData)();

  // set backend capabilities
  ImGui::GetIO().BackendRendererUserData = (void*)backendData;
  ImGui::GetIO().BackendRendererName = "imgui_impl_dx11";
  ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
  // We can create multi-viewports on the Renderer side (optional)
  ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

  // Get factory from device adpater
  ID3D11Device* d3dDevice = (ID3D11Device*)device;
  ID3D11DeviceContext* d3dDeviceContext = (ID3D11DeviceContext*)deviceContext;
  IDXGISwapChain* dxgiSwapChain = (IDXGISwapChain*)swapChain;

  IDXGIDevice* dxgiDevice = NULL;
  if (d3dDevice->QueryInterface (IID_PPV_ARGS (&dxgiDevice)) == S_OK) {
    IDXGIAdapter* dxgiAdapter = NULL;
    if (dxgiDevice->GetParent (IID_PPV_ARGS (&dxgiAdapter)) == S_OK) {
      IDXGIFactory* dxgiFactory = NULL;
      if (dxgiAdapter->GetParent (IID_PPV_ARGS (&dxgiFactory)) == S_OK) {
        backendData->mD3dDevice = d3dDevice;
        backendData->mD3dDeviceContext = d3dDeviceContext;
        backendData->mDxgiSwapChain = dxgiSwapChain;
        backendData->mDxgiFactory = dxgiFactory;
        backendData->mD3dDevice->AddRef();
        backendData->mD3dDeviceContext->AddRef();

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
          // init platFormInterface
          ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
          platform_io.Renderer_CreateWindow = createWindow;
          platform_io.Renderer_DestroyWindow = destroyWindow;
          platform_io.Renderer_SetWindowSize = setWindowSize;
          platform_io.Renderer_RenderWindow = renderWindow;
          platform_io.Renderer_SwapBuffers = swapBuffers;
          }

        // create resources
        ok = createResources();
        ok &= createFontTexture();
        ok &= createMainRenderTarget();
        cLog::log (LOGINFO, format ("graphics DirectX11 init ok {}", ok));
        }
      dxgiAdapter->Release();
      }
    dxgiDevice->Release();
    }

  return ok;
  }
//}}}
//{{{
void cGraphics::shutdown() {

  ImGui::DestroyPlatformWindows();

  sBackendData* backendData = getBackendData();
  if (backendData) {
    backendData->mFontSampler->Release();
    backendData->mFontTextureView->Release();
    backendData->mIB->Release();
    backendData->mVB->Release();
    backendData->mBlendState->Release();
    backendData->mDepthStencilState->Release();
    backendData->mRasterizerState->Release();
    backendData->mPixelShader->Release();
    backendData->mVertexConstantBuffer->Release();
    backendData->mInputLayout->Release();
    backendData->mVertexShader->Release();
    backendData->mDxgiFactory->Release();
    backendData->mD3dDevice->Release();
    backendData->mDxgiSwapChain->Release();
    backendData->mMainRenderTargetView->Release();
    }

  ImGui::GetIO().BackendRendererName = NULL;
  ImGui::GetIO().BackendRendererUserData = NULL;

  IM_DELETE (backendData);
  }
//}}}

//{{{
void cGraphics::draw() {

  sBackendData* backendData = getBackendData();

  // set mainRenderTarget
  backendData->mD3dDeviceContext->OMSetRenderTargets (1, &backendData->mMainRenderTargetView, NULL);

  // clear mainRenderTarget
  const float kClearColorWithAlpha[4] = { 0.25f,0.25f,0.25f, 1.f };
  backendData->mD3dDeviceContext->ClearRenderTargetView (backendData->mMainRenderTargetView, kClearColorWithAlpha);

  // really draw imGui drawList
  renderDrawData (ImGui::GetDrawData());
  }
//}}}
//{{{
void cGraphics::windowResized (int width, int height) {

  //cLog::log (LOGINFO, format ("cGraphics::windowResized {}", width, height));
  sBackendData* backendData = getBackendData();
  if (backendData) {
    backendData->mMainRenderTargetView->Release();
    backendData->mDxgiSwapChain->ResizeBuffers (0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    createMainRenderTarget();
    }
  }
//}}}
