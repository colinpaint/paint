// cGraphics-DirectX11.cpp
//{{{  includes
#include "cGraphics.h"

#include <stdio.h>
#include <tchar.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <imgui.h>

#include "../log/cLog.h"

#pragma comment(lib, "d3dcompiler")

using namespace std;
using namespace fmt;
//}}}

namespace {
  //{{{
  // Helper structure we store in the void* RenderUserData field of each ImGuiViewport to easily retrieve our backend data.
  struct sViewportData {
    IDXGISwapChain* SwapChain;
    ID3D11RenderTargetView* RTView;

    sViewportData() {
      SwapChain = NULL;
      RTView = NULL;
      }

    ~sViewportData() {
      IM_ASSERT(SwapChain == NULL && RTView == NULL);
      }
    };
  //}}}
  //{{{
  struct sBackendData {
    ID3D11Device*             pd3dDevice;
    ID3D11DeviceContext*      pd3dDeviceContext;
    IDXGIFactory*             pFactory;
    ID3D11Buffer*             pVB;
    ID3D11Buffer*             pIB;
    ID3D11VertexShader*       pVertexShader;
    ID3D11InputLayout*        pInputLayout;
    ID3D11Buffer*             pVertexConstantBuffer;
    ID3D11PixelShader*        pPixelShader;
    ID3D11SamplerState*       pFontSampler;
    ID3D11ShaderResourceView* pFontTextureView;
    ID3D11RasterizerState*    pRasterizerState;
    ID3D11BlendState*         pBlendState;
    ID3D11DepthStencilState*  pDepthStencilState;
    int                       VertexBufferSize;
    int                       IndexBufferSize;

    sBackendData() {
      memset (this, 0, sizeof(*this));
      VertexBufferSize = 5000;
      IndexBufferSize = 10000;
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
  void createFontsTexture() {
  // build texture atlas

    sBackendData* backendData = getBackendData();

    unsigned char* pixels;
    int width;
    int height;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32 (&pixels, &width, &height);

    // upload texture to graphics system
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

    ID3D11Texture2D* texture2d = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = pixels;
    subResource.SysMemPitch = texture2dDesc.Width * 4;
    subResource.SysMemSlicePitch = 0;

    backendData->pd3dDevice->CreateTexture2D (&texture2dDesc, &subResource, &texture2d);
    IM_ASSERT (texture2d != NULL);

    // create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
    ZeroMemory (&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
    shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MipLevels = texture2dDesc.MipLevels;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    backendData->pd3dDevice->CreateShaderResourceView (texture2d, &shaderResourceViewDesc, &backendData->pFontTextureView);
    texture2d->Release();

    // store our fonts texture view
    ImGui::GetIO().Fonts->SetTexID ((ImTextureID)backendData->pFontTextureView);

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
    backendData->pd3dDevice->CreateSamplerState (&samplerDesc, &backendData->pFontSampler);
    }
  //}}}
  //{{{
  bool createDeviceObjects() {

    sBackendData* backendData = getBackendData();
    if (!backendData->pd3dDevice)
      return false;

    //  create vertexShader
    //{{{
    static const char* kVertexShaderStr =
        "cbuffer vertexBuffer : register(b0) \
        {\
          float4x4 ProjectionMatrix; \
        };\
        struct VS_INPUT\
        {\
          float2 pos : POSITION;\
          float4 col : COLOR0;\
          float2 uv  : TEXCOORD0;\
        };\
        \
        struct PS_INPUT\
        {\
          float4 pos : SV_POSITION;\
          float4 col : COLOR0;\
          float2 uv  : TEXCOORD0;\
        };\
        \
        PS_INPUT main(VS_INPUT input)\
        {\
          PS_INPUT output;\
          output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
          output.col = input.col;\
          output.uv  = input.uv;\
          return output;\
        }";
    //}}}
    ID3DBlob* vertexShaderBlob;
    if (FAILED (D3DCompile (kVertexShaderStr, strlen(kVertexShaderStr),
                            NULL, NULL, NULL, "main", "vs_4_0", 0, 0, &vertexShaderBlob, NULL)))
      return false;
    if (backendData->pd3dDevice->CreateVertexShader (vertexShaderBlob->GetBufferPointer(),
                                                     vertexShaderBlob->GetBufferSize(),
                                                     NULL, &backendData->pVertexShader) != S_OK) {
      //{{{  release, return false
      vertexShaderBlob->Release();
      return false;
      }
      //}}}
    //{{{  create the input layout
    D3D11_INPUT_ELEMENT_DESC local_layout[] = {
      { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
      };

    if (backendData->pd3dDevice->CreateInputLayout (local_layout, 3,
                                                    vertexShaderBlob->GetBufferPointer(),
                                                    vertexShaderBlob->GetBufferSize(),
                                                    &backendData->pInputLayout) != S_OK) {
      //{{{  release, return false
      vertexShaderBlob->Release();
      return false;
      }
      //}}}
    //}}}
    vertexShaderBlob->Release();
    //{{{  create the constant buffer
    D3D11_BUFFER_DESC bufferDesc;
    bufferDesc.ByteWidth = sizeof(sViewportData);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = 0;
    backendData->pd3dDevice->CreateBuffer (&bufferDesc, NULL, &backendData->pVertexConstantBuffer);
    //}}}

    // create pixelShader
    //{{{
    static const char* kPixelShaderStr =
        "struct PS_INPUT\
        {\
        float4 pos : SV_POSITION;\
        float4 col : COLOR0;\
        float2 uv  : TEXCOORD0;\
        };\
        sampler sampler0;\
        Texture2D texture0;\
        \
        float4 main(PS_INPUT input) : SV_Target\
        {\
        float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
        return out_col; \
        }";
    //}}}
    ID3DBlob* pixelShaderBlob;
    if (FAILED (D3DCompile (kPixelShaderStr, strlen(kPixelShaderStr),
                            NULL, NULL, NULL, "main", "ps_4_0", 0, 0, &pixelShaderBlob, NULL)))
      return false;
    if (backendData->pd3dDevice->CreatePixelShader (pixelShaderBlob->GetBufferPointer(),
                                                    pixelShaderBlob->GetBufferSize(),
                                                    NULL, &backendData->pPixelShader) != S_OK) {
      //{{{  release, return false
      pixelShaderBlob->Release();
      return false;
      }
      //}}}
    pixelShaderBlob->Release();

    // create the blendDesc
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
    backendData->pd3dDevice->CreateBlendState (&blendDesc, &backendData->pBlendState);

    // create the rasterizerDesc
    D3D11_RASTERIZER_DESC rasterizerDesc;
    ZeroMemory (&rasterizerDesc, sizeof(rasterizerDesc));
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.ScissorEnable = true;
    rasterizerDesc.DepthClipEnable = true;
    backendData->pd3dDevice->CreateRasterizerState (&rasterizerDesc, &backendData->pRasterizerState);

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
    backendData->pd3dDevice->CreateDepthStencilState (&depthStencilDesc, &backendData->pDepthStencilState);

    createFontsTexture();

    return true;
    }
  //}}}
  //{{{
  void invalidateDeviceObjects() {

    sBackendData* backendData = getBackendData();
    if (!backendData->pd3dDevice)
        return;

    if (backendData->pFontSampler) {
      backendData->pFontSampler->Release();
      backendData->pFontSampler = NULL;
      }
    if (backendData->pFontTextureView) {
      backendData->pFontTextureView->Release();
      backendData->pFontTextureView = NULL; ImGui::GetIO().Fonts->SetTexID(NULL);
      } // We copied data->pFontTextureView to io.Fonts->TexID so let's clear that as well.

    if (backendData->pIB) {
      backendData->pIB->Release();
      backendData->pIB = NULL;
      }
    if (backendData->pVB) {
      backendData->pVB->Release();
      backendData->pVB = NULL;
      }

    if (backendData->pBlendState) {
      backendData->pBlendState->Release();
      backendData->pBlendState = NULL;
      }

    if (backendData->pDepthStencilState) {
      backendData->pDepthStencilState->Release();
      backendData->pDepthStencilState = NULL;
      }

    if (backendData->pRasterizerState) {
      backendData->pRasterizerState->Release();
      backendData->pRasterizerState = NULL;
      }

    if (backendData->pPixelShader) {
      backendData->pPixelShader->Release();
      backendData->pPixelShader = NULL;
      }

    if (backendData->pVertexConstantBuffer) {
      backendData->pVertexConstantBuffer->Release();
      backendData->pVertexConstantBuffer = NULL;
      }

    if (backendData->pInputLayout) {
      backendData->pInputLayout->Release();
      backendData->pInputLayout = NULL;
      }

    if (backendData->pVertexShader) {
      backendData->pVertexShader->Release();
      backendData->pVertexShader = NULL;
      }
    }
  //}}}

  //{{{
  // Functions
  void setupRenderState (ImDrawData* drawData, ID3D11DeviceContext* deviceContext) {

    // Setup viewport
    D3D11_VIEWPORT vp;
    memset (&vp, 0, sizeof(D3D11_VIEWPORT));
    vp.Width = drawData->DisplaySize.x;
    vp.Height = drawData->DisplaySize.y;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = vp.TopLeftY = 0;
    deviceContext->RSSetViewports (1, &vp);

    // Setup shader and vertex buffers
    unsigned int stride = sizeof(ImDrawVert);
    unsigned int offset = 0;

    sBackendData* backendData = getBackendData();
    deviceContext->IASetInputLayout (backendData->pInputLayout);
    deviceContext->IASetVertexBuffers (0, 1, &backendData->pVB, &stride, &offset);
    deviceContext->IASetIndexBuffer (backendData->pIB, sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
    deviceContext->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext->VSSetShader (backendData->pVertexShader, NULL, 0);
    deviceContext->VSSetConstantBuffers (0, 1, &backendData->pVertexConstantBuffer);
    deviceContext->PSSetShader (backendData->pPixelShader, NULL, 0);
    deviceContext->PSSetSamplers (0, 1, &backendData->pFontSampler);
    deviceContext->GSSetShader (NULL, NULL, 0);
    deviceContext->HSSetShader (NULL, NULL, 0); // In theory we should backup and restore this as well.. very infrequently used..
    deviceContext->DSSetShader (NULL, NULL, 0); // In theory we should backup and restore this as well.. very infrequently used..
    deviceContext->CSSetShader (NULL, NULL, 0); // In theory we should backup and restore this as well.. very infrequently used..

    // Setup blend state
    const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
    deviceContext->OMSetBlendState (backendData->pBlendState, blend_factor, 0xffffffff);
    deviceContext->OMSetDepthStencilState (backendData->pDepthStencilState, 0);
    deviceContext->RSSetState (backendData->pRasterizerState);
    }
  //}}}
  //{{{
  // Render function
  void renderDrawData (ImDrawData* drawData) {

    // avoid rendering when minimized
    if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
      return;

    sBackendData* backendData = getBackendData();
    ID3D11DeviceContext* deviceContext = backendData->pd3dDeviceContext;

    // copy drawList vertex,index to GPU
    //{{{  manage gpu vertex buffer
    if (!backendData->pVB || backendData->VertexBufferSize < drawData->TotalVtxCount) {
      if (backendData->pVB) {
        backendData->pVB->Release();
        backendData->pVB = NULL;
        }
      backendData->VertexBufferSize = drawData->TotalVtxCount + 5000;

      D3D11_BUFFER_DESC desc;
      memset (&desc, 0, sizeof(D3D11_BUFFER_DESC));
      desc.Usage = D3D11_USAGE_DYNAMIC;
      desc.ByteWidth = backendData->VertexBufferSize * sizeof(ImDrawVert);
      desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      desc.MiscFlags = 0;
      if (backendData->pd3dDevice->CreateBuffer (&desc, NULL, &backendData->pVB) < 0)
        return;
      }

    D3D11_MAPPED_SUBRESOURCE vtx_resource;
    if (deviceContext->Map (backendData->pVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &vtx_resource) != S_OK)
      return;
    ImDrawVert* vtx_dst = (ImDrawVert*)vtx_resource.pData;
    //}}}
    //{{{  manage gpu index buffer
    if (!backendData->pIB || backendData->IndexBufferSize < drawData->TotalIdxCount) {
      if (backendData->pIB) {
        backendData->pIB->Release();
        backendData->pIB = NULL;
        }
      backendData->IndexBufferSize = drawData->TotalIdxCount + 10000;

      D3D11_BUFFER_DESC desc;
      memset (&desc, 0, sizeof(D3D11_BUFFER_DESC));
      desc.Usage = D3D11_USAGE_DYNAMIC;
      desc.ByteWidth = backendData->IndexBufferSize * sizeof(ImDrawIdx);
      desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      if (backendData->pd3dDevice->CreateBuffer (&desc, NULL, &backendData->pIB) < 0)
        return;
      }

    D3D11_MAPPED_SUBRESOURCE idx_resource;
    if (deviceContext->Map (backendData->pIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &idx_resource) != S_OK)
      return;
    ImDrawIdx* idx_dst = (ImDrawIdx*)idx_resource.pData;
    //}}}
    for (int n = 0; n < drawData->CmdListsCount; n++) {
      const ImDrawList* cmdList = drawData->CmdLists[n];
      memcpy (vtx_dst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
      memcpy (idx_dst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
      vtx_dst += cmdList->VtxBuffer.Size;
      idx_dst += cmdList->IdxBuffer.Size;
      }
    deviceContext->Unmap (backendData->pVB, 0);
    deviceContext->Unmap (backendData->pIB, 0);

    //{{{  Setup orthographic projection matrix into our constant buffer
    //{{{
    struct sMatrix {
      float matrix[4][4];
      };
    //}}}
    {
    // visible imgui space lies
    // - from draw_data->DisplayPos (top left)
    // - to draw_data->DisplayPos+data_data->DisplaySize (bottom right)
    // - DisplayPos is (0,0) for single viewport apps.
    const float L = drawData->DisplayPos.x;
    const float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
    const float T = drawData->DisplayPos.y;
    const float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
    const float kMatrix[4][4] = {
      { 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
      { 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
      { 0.0f,         0.0f,           0.5f,       0.0f },
      { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
      };

    D3D11_MAPPED_SUBRESOURCE mappedSubResource;
    if (deviceContext->Map (backendData->pVertexConstantBuffer,
                            0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource) != S_OK)
      return;
    sMatrix* matrix = (sMatrix*)mappedSubResource.pData;
    memcpy (&matrix->matrix, kMatrix, sizeof(sMatrix));
    deviceContext->Unmap (backendData->pVertexConstantBuffer, 0);
    }
    //}}}

    //{{{
    // Backup DX state that will be modified to restore it afterwards (unfortunately this is very ugly looking and verbose. Close your eyes!)
    struct BACKUP_DX11_STATE {
      UINT                        ScissorRectsCount, ViewportsCount;
      D3D11_RECT                  ScissorRects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
      D3D11_VIEWPORT              Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
      ID3D11RasterizerState*      RS;
      ID3D11BlendState*           BlendState;
      FLOAT                       BlendFactor[4];
      UINT                        SampleMask;
      UINT                        StencilRef;
      ID3D11DepthStencilState*    DepthStencilState;
      ID3D11ShaderResourceView*   PSShaderResource;
      ID3D11SamplerState*         PSSampler;
      ID3D11PixelShader*          PS;
      ID3D11VertexShader*         VS;
      ID3D11GeometryShader*       GS;
      UINT                        PSInstancesCount, VSInstancesCount, GSInstancesCount;
      ID3D11ClassInstance         *PSInstances[256], *VSInstances[256], *GSInstances[256];   // 256 is max according to PSSetShader documentation
      D3D11_PRIMITIVE_TOPOLOGY    PrimitiveTopology;
      ID3D11Buffer*               IndexBuffer, *VertexBuffer, *VSConstantBuffer;
      UINT                        IndexBufferOffset, VertexBufferStride, VertexBufferOffset;
      DXGI_FORMAT                 IndexBufferFormat;
      ID3D11InputLayout*          InputLayout;
      };
    //}}}
    BACKUP_DX11_STATE old = {};
    old.ScissorRectsCount = old.ViewportsCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    deviceContext->RSGetScissorRects (&old.ScissorRectsCount, old.ScissorRects);
    deviceContext->RSGetViewports (&old.ViewportsCount, old.Viewports);
    deviceContext->RSGetState (&old.RS);
    deviceContext->OMGetBlendState (&old.BlendState, old.BlendFactor, &old.SampleMask);
    deviceContext->OMGetDepthStencilState (&old.DepthStencilState, &old.StencilRef);
    deviceContext->PSGetShaderResources (0, 1, &old.PSShaderResource);
    deviceContext->PSGetSamplers (0, 1, &old.PSSampler);
    old.PSInstancesCount = old.VSInstancesCount = old.GSInstancesCount = 256;
    deviceContext->PSGetShader (&old.PS, old.PSInstances, &old.PSInstancesCount);
    deviceContext->VSGetShader (&old.VS, old.VSInstances, &old.VSInstancesCount);
    deviceContext->VSGetConstantBuffers (0, 1, &old.VSConstantBuffer);
    deviceContext->GSGetShader (&old.GS, old.GSInstances, &old.GSInstancesCount);
    deviceContext->IAGetPrimitiveTopology (&old.PrimitiveTopology);
    deviceContext->IAGetIndexBuffer (&old.IndexBuffer, &old.IndexBufferFormat, &old.IndexBufferOffset);
    deviceContext->IAGetVertexBuffers (0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset);
    deviceContext->IAGetInputLayout (&old.InputLayout);

    // Setup desired DX state
    setupRenderState (drawData, deviceContext);

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int globalIndexOffset = 0;
    int globalVertexOffset = 0;
    ImVec2 clipOffset = drawData->DisplayPos;
    for (int cmdListIndex = 0; cmdListIndex < drawData->CmdListsCount; cmdListIndex++) {
      const ImDrawList* cmdList = drawData->CmdLists[cmdListIndex];
      for (int cmdIndex = 0; cmdIndex < cmdList->CmdBuffer.Size; cmdIndex++) {
        const ImDrawCmd* drawCmd = &cmdList->CmdBuffer[cmdIndex];
        if (drawCmd->UserCallback != NULL) {
          // User callback, registered via ImDrawList::AddCallback()
          // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
          if (drawCmd->UserCallback == ImDrawCallback_ResetRenderState)
            setupRenderState (drawData, deviceContext);
          else
            drawCmd->UserCallback (cmdList, drawCmd);
          }
        else {
          // Apply scissor/clipping rectangle
          const D3D11_RECT r = { (LONG)(drawCmd->ClipRect.x - clipOffset.x), (LONG)(drawCmd->ClipRect.y - clipOffset.y),
                                 (LONG)(drawCmd->ClipRect.z - clipOffset.x), (LONG)(drawCmd->ClipRect.w - clipOffset.y) };
          deviceContext->RSSetScissorRects (1, &r);

          // bind texture and draw
          ID3D11ShaderResourceView* shaderResourceView = (ID3D11ShaderResourceView*)drawCmd->GetTexID();
          deviceContext->PSSetShaderResources (0, 1, &shaderResourceView);
          deviceContext->DrawIndexed (
            drawCmd->ElemCount, drawCmd->IdxOffset + globalIndexOffset, drawCmd->VtxOffset + globalVertexOffset);
          }
        }
      globalIndexOffset += cmdList->IdxBuffer.Size;
      globalVertexOffset += cmdList->VtxBuffer.Size;
      }

    //{{{  restore modified DX state
    deviceContext->RSSetScissorRects (old.ScissorRectsCount, old.ScissorRects);
    deviceContext->RSSetViewports (old.ViewportsCount, old.Viewports);
    deviceContext->RSSetState (old.RS); if (old.RS) old.RS->Release();

    deviceContext->OMSetBlendState (old.BlendState, old.BlendFactor, old.SampleMask);
    if (old.BlendState)
      old.BlendState->Release();

    deviceContext->OMSetDepthStencilState (old.DepthStencilState, old.StencilRef);
    if (old.DepthStencilState)
      old.DepthStencilState->Release();

    deviceContext->PSSetShaderResources (0, 1, &old.PSShaderResource);
    if (old.PSShaderResource)
      old.PSShaderResource->Release();
    deviceContext->PSSetSamplers (0, 1, &old.PSSampler);
    if (old.PSSampler)
      old.PSSampler->Release();
    deviceContext->PSSetShader (old.PS, old.PSInstances, old.PSInstancesCount);
    if (old.PS)
      old.PS->Release();
    for (UINT i = 0; i < old.PSInstancesCount; i++)
      if (old.PSInstances[i])
        old.PSInstances[i]->Release();

    deviceContext->VSSetShader (old.VS, old.VSInstances, old.VSInstancesCount);
    if (old.VS) old.VS->Release();
      deviceContext->VSSetConstantBuffers (0, 1, &old.VSConstantBuffer);
    if (old.VSConstantBuffer) old.VSConstantBuffer->Release();
      deviceContext->GSSetShader(old.GS, old.GSInstances, old.GSInstancesCount);
    if (old.GS)
      old.GS->Release();
    for (UINT i = 0; i < old.VSInstancesCount; i++)
      if (old.VSInstances[i])
        old.VSInstances[i]->Release();

    deviceContext->IASetPrimitiveTopology (old.PrimitiveTopology);
    deviceContext->IASetIndexBuffer (old.IndexBuffer, old.IndexBufferFormat, old.IndexBufferOffset);

    if (old.IndexBuffer)
      old.IndexBuffer->Release();

    deviceContext->IASetVertexBuffers (0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset);
    if (old.VertexBuffer)
      old.VertexBuffer->Release();

    deviceContext->IASetInputLayout (old.InputLayout);
    if (old.InputLayout)
      old.InputLayout->Release();
    //}}}
    }
  //}}}

  // platform interface
  //{{{
  void createWindow (ImGuiViewport* viewport) {

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

    IM_ASSERT (viewportData->SwapChain == NULL && viewportData->RTView == NULL);
    backendData->pFactory->CreateSwapChain (backendData->pd3dDevice, &swapChainDesc, &viewportData->SwapChain);

    // create the render target
    if (viewportData->SwapChain) {
      ID3D11Texture2D* backBuffer;
      viewportData->SwapChain->GetBuffer (0, IID_PPV_ARGS (&backBuffer));
      backendData->pd3dDevice->CreateRenderTargetView (backBuffer, NULL, &viewportData->RTView);
      backBuffer->Release();
      }
    }
  //}}}
  //{{{
  void destroyWindow (ImGuiViewport* viewport) {
  // The main viewport (owned by the application) will always have RendererUserData == NULL since we didn't create the data for it.

    if (sViewportData* viewportData = (sViewportData*)viewport->RendererUserData) {
      if (viewportData->SwapChain)
        viewportData->SwapChain->Release();
      viewportData->SwapChain = NULL;

      if (viewportData->RTView)
        viewportData->RTView->Release();
      viewportData->RTView = NULL;

      IM_DELETE(viewportData);
      }

    viewport->RendererUserData = NULL;
    }
  //}}}
  //{{{
  void setWindowSize (ImGuiViewport* viewport, ImVec2 size) {

    sViewportData* viewportData = (sViewportData*)viewport->RendererUserData;

    if (viewportData->RTView) {
      viewportData->RTView->Release();
      viewportData->RTView = NULL;
      }

    if (viewportData->SwapChain) {
      ID3D11Texture2D* backBuffer = NULL;
      viewportData->SwapChain->ResizeBuffers (0, (UINT)size.x, (UINT)size.y, DXGI_FORMAT_UNKNOWN, 0);
      viewportData->SwapChain->GetBuffer (0, IID_PPV_ARGS(&backBuffer));
      if (backBuffer == NULL) {
        fprintf (stderr, "SetWindowSize() failed creating buffers.\n");
        return;
        }

      sBackendData* backendData = getBackendData();
      backendData->pd3dDevice->CreateRenderTargetView (backBuffer, NULL, &viewportData->RTView);
      backBuffer->Release();
      }
    }
  //}}}
  //{{{
  void renderWindow (ImGuiViewport* viewport, void*) {

    sBackendData* backendData = getBackendData();
    sViewportData* viewportData = (sViewportData*)viewport->RendererUserData;

    ImVec4 clearColor = ImVec4 (0.0f, 0.0f, 0.0f, 1.0f);
    backendData->pd3dDeviceContext->OMSetRenderTargets (1, &viewportData->RTView, NULL);
    if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear))
      backendData->pd3dDeviceContext->ClearRenderTargetView (viewportData->RTView, (float*)&clearColor);

    renderDrawData (viewport->DrawData);
    }
  //}}}
  //{{{
  void swapBuffers (ImGuiViewport* viewport, void*) {

    sViewportData* viewportData = (sViewportData*)viewport->RendererUserData;
    viewportData->SwapChain->Present (0,0); // Present without vsync
    }
  //}}}
  }

//{{{
bool cGraphics::init (void* device, void* deviceContext) {

  bool ok = false;

  ImGuiIO& io = ImGui::GetIO();
  IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");

  // Setup backend capabilities flags
  sBackendData* backendData = IM_NEW (sBackendData)();
  io.BackendRendererUserData = (void*)backendData;
  io.BackendRendererName = "imgui_impl_dx11";
  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
  // We can create multi-viewports on the Renderer side (optional)
  io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

  // typed device, deviceContext
  ID3D11Device* d3dDevice = (ID3D11Device*)device;
  ID3D11DeviceContext* d3dDeviceContext = (ID3D11DeviceContext*)deviceContext;

  // Get factory from device adpater
  IDXGIDevice* dxgiDevice = NULL;
  if (d3dDevice->QueryInterface (IID_PPV_ARGS (&dxgiDevice)) == S_OK) {
    IDXGIAdapter* dxgiAdapter = NULL;
    if (dxgiDevice->GetParent (IID_PPV_ARGS (&dxgiAdapter)) == S_OK) {
      IDXGIFactory* dxgiFactory = NULL;
      if (dxgiAdapter->GetParent (IID_PPV_ARGS (&dxgiFactory)) == S_OK) {
        backendData->pd3dDevice = d3dDevice;
        backendData->pd3dDeviceContext = d3dDeviceContext;
        backendData->pFactory = dxgiFactory;
        backendData->pd3dDevice->AddRef();
        backendData->pd3dDeviceContext->AddRef();

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
          // init platFormInterface
          ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
          platform_io.Renderer_CreateWindow = createWindow;
          platform_io.Renderer_DestroyWindow = destroyWindow;
          platform_io.Renderer_SetWindowSize = setWindowSize;
          platform_io.Renderer_RenderWindow = renderWindow;
          platform_io.Renderer_SwapBuffers = swapBuffers;
          }

        ok = createDeviceObjects();
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

  ImGuiIO& io = ImGui::GetIO();
  sBackendData* backendData = getBackendData();

  ImGui::DestroyPlatformWindows();
  invalidateDeviceObjects();

  if (backendData->pFactory)
    backendData->pFactory->Release();

  if (backendData->pd3dDevice)
    backendData->pd3dDevice->Release();

  if (backendData->pd3dDeviceContext)
    backendData->pd3dDeviceContext->Release();

  io.BackendRendererName = NULL;
  io.BackendRendererUserData = NULL;

  IM_DELETE(backendData);
  }
//}}}

//{{{
void cGraphics::draw() {
  renderDrawData (ImGui::GetDrawData());
  }
//}}}
