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
  struct sBackendData {
    ID3D11Device*               pd3dDevice;
    ID3D11DeviceContext*        pd3dDeviceContext;
    IDXGIFactory*               pFactory;
    ID3D11Buffer*               pVB;
    ID3D11Buffer*               pIB;
    ID3D11VertexShader*         pVertexShader;
    ID3D11InputLayout*          pInputLayout;
    ID3D11Buffer*               pVertexConstantBuffer;
    ID3D11PixelShader*          pPixelShader;
    ID3D11SamplerState*         pFontSampler;
    ID3D11ShaderResourceView*   pFontTextureView;
    ID3D11RasterizerState*      pRasterizerState;
    ID3D11BlendState*           pBlendState;
    ID3D11DepthStencilState*    pDepthStencilState;
    int                         VertexBufferSize;
    int                         IndexBufferSize;

    sBackendData()       { memset(this, 0, sizeof(*this)); VertexBufferSize = 5000; IndexBufferSize = 10000; }
    };
  //}}}
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
  struct sVertexConstantBuffer {
    float mvp[4][4];
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

    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    sBackendData* backendData = getBackendData();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32 (&pixels, &width, &height);

    //{{{  Upload texture to graphics system
    {
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory (&desc, sizeof(desc));

    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = pixels;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    backendData->pd3dDevice->CreateTexture2D (&desc, &subResource, &pTexture);
    IM_ASSERT(pTexture != NULL);

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory (&srvDesc, sizeof(srvDesc));

    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    backendData->pd3dDevice->CreateShaderResourceView (pTexture, &srvDesc, &backendData->pFontTextureView);
    pTexture->Release();
    }
    //}}}

    // Store our identifier
    io.Fonts->SetTexID ((ImTextureID)backendData->pFontTextureView);

    //{{{  Create texture sampler
    {
    D3D11_SAMPLER_DESC desc;
    ZeroMemory(&desc, sizeof(desc));

    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.MipLODBias = 0.f;
    desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    desc.MinLOD = 0.f;
    desc.MaxLOD = 0.f;

    backendData->pd3dDevice->CreateSamplerState(&desc, &backendData->pFontSampler);
    }
    //}}}
    }
  //}}}
  //{{{
  bool createDeviceObjects() {
  // By using D3DCompile() from <d3dcompiler.h> / d3dcompiler.lib, we introduce a dependency to a given version of d3dcompiler_XX.dll (see D3DCOMPILER_DLL_A)
  // If you would like to use this DX11 sample code but remove this dependency you can:
  //  1) compile once, save the compiled shader blobs into a file or source code and pass them to CreateVertexShader()/CreatePixelShader() [preferred solution]
  //  2) use code to detect any version of the DLL and grab a pointer to D3DCompile from the DLL.
  // See https://github.com/ocornut/imgui/pull/638 for sources and details.

    sBackendData* backendData = getBackendData();
    if (!backendData->pd3dDevice)
      return false;

    {
    //  Create the vertex shader
    //{{{
    static const char* vertexShader =
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

    // NB: Pass ID3DBlob* pErrorBlob to D3DCompile() to get error showing in
    // (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
    ID3DBlob* vertexShaderBlob;
    if (FAILED(D3DCompile(vertexShader, strlen(vertexShader), NULL, NULL, NULL, "main", "vs_4_0", 0, 0, &vertexShaderBlob, NULL)))
        return false;

    if (backendData->pd3dDevice->CreateVertexShader (
        vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(),
        NULL, &backendData->pVertexShader) != S_OK) {
      vertexShaderBlob->Release();
      return false;
      }

    // Create the input layout
    D3D11_INPUT_ELEMENT_DESC local_layout[] = {
      { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
      };

    if (backendData->pd3dDevice->CreateInputLayout(local_layout, 3, vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &backendData->pInputLayout) != S_OK) {
      vertexShaderBlob->Release();
      return false;
      }
    vertexShaderBlob->Release();

      {
      // Create the constant buffer
      D3D11_BUFFER_DESC desc;
      desc.ByteWidth = sizeof(sViewportData);
      desc.Usage = D3D11_USAGE_DYNAMIC;
      desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      desc.MiscFlags = 0;
      backendData->pd3dDevice->CreateBuffer(&desc, NULL, &backendData->pVertexConstantBuffer);
      }

    }
    //{{{  Create the pixel shader
    {
    static const char* pixelShader =
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

    ID3DBlob* pixelShaderBlob;
    if (FAILED(D3DCompile(pixelShader, strlen(pixelShader), NULL, NULL, NULL, "main", "ps_4_0", 0, 0, &pixelShaderBlob, NULL)))
      return false; // NB: Pass ID3DBlob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!

    if (backendData->pd3dDevice->CreatePixelShader (pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(),
                                           NULL, &backendData->pPixelShader) != S_OK) {
      pixelShaderBlob->Release();
      return false;
      }

    pixelShaderBlob->Release();
    }
    //}}}
    //{{{  Create the blending setup
    {
    D3D11_BLEND_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.AlphaToCoverageEnable = false;
    desc.RenderTarget[0].BlendEnable = true;
    desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    backendData->pd3dDevice->CreateBlendState(&desc, &backendData->pBlendState);
    }
    //}}}
    //{{{  Create the rasterizer state
    {
    D3D11_RASTERIZER_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = D3D11_CULL_NONE;
    desc.ScissorEnable = true;
    desc.DepthClipEnable = true;
    backendData->pd3dDevice->CreateRasterizerState(&desc, &backendData->pRasterizerState);
    }
    //}}}
    //{{{  Create depth-stencil State
    {
    D3D11_DEPTH_STENCIL_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.DepthEnable = false;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    desc.StencilEnable = false;
    desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    desc.BackFace = desc.FrontFace;
    backendData->pd3dDevice->CreateDepthStencilState(&desc, &backendData->pDepthStencilState);
    }
    //}}}

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

    sBackendData* backendData = getBackendData();

    // Setup viewport
    D3D11_VIEWPORT vp;
    memset (&vp, 0, sizeof(D3D11_VIEWPORT));
    vp.Width = drawData->DisplaySize.x;
    vp.Height = drawData->DisplaySize.y;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = vp.TopLeftY = 0;
    deviceContext->RSSetViewports(1, &vp);

    // Setup shader and vertex buffers
    unsigned int stride = sizeof(ImDrawVert);
    unsigned int offset = 0;
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

    // Avoid rendering when minimized
    if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
      return;

    sBackendData* backendData = getBackendData();
    ID3D11DeviceContext* ctx = backendData->pd3dDeviceContext;

    // Create and grow vertex/index buffers if needed
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
      if (backendData->pd3dDevice->CreateBuffer(&desc, NULL, &backendData->pVB) < 0)
          return;
      }

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
      if (backendData->pd3dDevice->CreateBuffer(&desc, NULL, &backendData->pIB) < 0)
          return;
      }

    // Upload vertex/index data into a single contiguous GPU buffer
    D3D11_MAPPED_SUBRESOURCE vtx_resource, idx_resource;
    if (ctx->Map (backendData->pVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &vtx_resource) != S_OK)
      return;
    if (ctx->Map (backendData->pIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &idx_resource) != S_OK)
      return;

    ImDrawVert* vtx_dst = (ImDrawVert*)vtx_resource.pData;
    ImDrawIdx* idx_dst = (ImDrawIdx*)idx_resource.pData;
    for (int n = 0; n < drawData->CmdListsCount; n++) {
      const ImDrawList* cmdList = drawData->CmdLists[n];
      memcpy (vtx_dst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
      memcpy (idx_dst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
      vtx_dst += cmdList->VtxBuffer.Size;
      idx_dst += cmdList->IdxBuffer.Size;
      }
    ctx->Unmap(backendData->pVB, 0);
    ctx->Unmap(backendData->pIB, 0);

    //{{{  Setup orthographic projection matrix into our constant buffer
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    {
    D3D11_MAPPED_SUBRESOURCE mapped_resource;
    if (ctx->Map(backendData->pVertexConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK)
        return;
    sVertexConstantBuffer* constant_buffer = (sVertexConstantBuffer*)mapped_resource.pData;
    float L = drawData->DisplayPos.x;
    float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
    float T = drawData->DisplayPos.y;
    float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
    float mvp[4][4] = {
        { 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
        { 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
        { 0.0f,         0.0f,           0.5f,       0.0f },
        { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
      };
    memcpy(&constant_buffer->mvp, mvp, sizeof(mvp));
    ctx->Unmap(backendData->pVertexConstantBuffer, 0);
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
    ctx->RSGetScissorRects (&old.ScissorRectsCount, old.ScissorRects);
    ctx->RSGetViewports (&old.ViewportsCount, old.Viewports);
    ctx->RSGetState (&old.RS);
    ctx->OMGetBlendState (&old.BlendState, old.BlendFactor, &old.SampleMask);
    ctx->OMGetDepthStencilState (&old.DepthStencilState, &old.StencilRef);
    ctx->PSGetShaderResources (0, 1, &old.PSShaderResource);
    ctx->PSGetSamplers (0, 1, &old.PSSampler);
    old.PSInstancesCount = old.VSInstancesCount = old.GSInstancesCount = 256;
    ctx->PSGetShader (&old.PS, old.PSInstances, &old.PSInstancesCount);
    ctx->VSGetShader (&old.VS, old.VSInstances, &old.VSInstancesCount);
    ctx->VSGetConstantBuffers (0, 1, &old.VSConstantBuffer);
    ctx->GSGetShader (&old.GS, old.GSInstances, &old.GSInstancesCount);
    ctx->IAGetPrimitiveTopology (&old.PrimitiveTopology);
    ctx->IAGetIndexBuffer (&old.IndexBuffer, &old.IndexBufferFormat, &old.IndexBufferOffset);
    ctx->IAGetVertexBuffers (0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset);
    ctx->IAGetInputLayout (&old.InputLayout);

    // Setup desired DX state
    setupRenderState (drawData, ctx);

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_idx_offset = 0;
    int global_vtx_offset = 0;
    ImVec2 clip_off = drawData->DisplayPos;
    for (int n = 0; n < drawData->CmdListsCount; n++) {
      const ImDrawList* cmd_list = drawData->CmdLists[n];
      for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
        const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
        if (pcmd->UserCallback != NULL) {
          // User callback, registered via ImDrawList::AddCallback()
          // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
          if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
            setupRenderState (drawData, ctx);
          else
            pcmd->UserCallback (cmd_list, pcmd);
          }
        else {
          // Apply scissor/clipping rectangle
          const D3D11_RECT r = { (LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y) };
          ctx->RSSetScissorRects(1, &r);

          // Bind texture, Draw
          ID3D11ShaderResourceView* texture_srv = (ID3D11ShaderResourceView*)pcmd->GetTexID();
          ctx->PSSetShaderResources(0, 1, &texture_srv);
          ctx->DrawIndexed(pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset);
          }
        }
      global_idx_offset += cmd_list->IdxBuffer.Size;
      global_vtx_offset += cmd_list->VtxBuffer.Size;
      }

    //{{{  Restore modified DX state
    ctx->RSSetScissorRects(old.ScissorRectsCount, old.ScissorRects);
    ctx->RSSetViewports(old.ViewportsCount, old.Viewports);
    ctx->RSSetState(old.RS); if (old.RS) old.RS->Release();

    ctx->OMSetBlendState(old.BlendState, old.BlendFactor, old.SampleMask);
    if (old.BlendState)
      old.BlendState->Release();

    ctx->OMSetDepthStencilState(old.DepthStencilState, old.StencilRef);
    if (old.DepthStencilState)
      old.DepthStencilState->Release();

    ctx->PSSetShaderResources(0, 1, &old.PSShaderResource);
    if (old.PSShaderResource)
      old.PSShaderResource->Release();
    ctx->PSSetSamplers(0, 1, &old.PSSampler);
    if (old.PSSampler)
      old.PSSampler->Release();
    ctx->PSSetShader(old.PS, old.PSInstances, old.PSInstancesCount);
    if (old.PS)
      old.PS->Release();
    for (UINT i = 0; i < old.PSInstancesCount; i++)
      if (old.PSInstances[i])
        old.PSInstances[i]->Release();

    ctx->VSSetShader(old.VS, old.VSInstances, old.VSInstancesCount);
    if (old.VS) old.VS->Release();
      ctx->VSSetConstantBuffers(0, 1, &old.VSConstantBuffer);
    if (old.VSConstantBuffer) old.VSConstantBuffer->Release();
      ctx->GSSetShader(old.GS, old.GSInstances, old.GSInstancesCount);
    if (old.GS)
      old.GS->Release();
    for (UINT i = 0; i < old.VSInstancesCount; i++)
      if (old.VSInstances[i])
        old.VSInstances[i]->Release();

    ctx->IASetPrimitiveTopology(old.PrimitiveTopology);
    ctx->IASetIndexBuffer(old.IndexBuffer, old.IndexBufferFormat, old.IndexBufferOffset);

    if (old.IndexBuffer)
      old.IndexBuffer->Release();

    ctx->IASetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset);
    if (old.VertexBuffer)
      old.VertexBuffer->Release();

    ctx->IASetInputLayout(old.InputLayout);
    if (old.InputLayout)
      old.InputLayout->Release();
    //}}}
    }
  //}}}

  //{{{
  void createWindow (ImGuiViewport* viewport) {

    sBackendData* backendData = getBackendData();
    sViewportData* viewportData = IM_NEW(sViewportData)();
    viewport->RendererUserData = viewportData;

    // PlatformHandleRaw should always be a HWND, whereas PlatformHandle might be a higher-level handle (e.g. GLFWWindow*, SDL_Window*).
    // Some backend will leave PlatformHandleRaw NULL, in which case we assume PlatformHandle will contain the HWND.
    HWND hWnd = viewport->PlatformHandleRaw ? (HWND)viewport->PlatformHandleRaw : (HWND)viewport->PlatformHandle;
    IM_ASSERT(hWnd != 0);

    // Create swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory (&sd, sizeof(sd));
    sd.BufferDesc.Width = (UINT)viewport->Size.x;
    sd.BufferDesc.Height = (UINT)viewport->Size.y;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 1;
    sd.OutputWindow = hWnd;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = 0;

    IM_ASSERT (viewportData->SwapChain == NULL && viewportData->RTView == NULL);
    backendData->pFactory->CreateSwapChain (backendData->pd3dDevice, &sd, &viewportData->SwapChain);

    // Create the render target
    if (viewportData->SwapChain) {
      ID3D11Texture2D* pBackBuffer;
      viewportData->SwapChain->GetBuffer (0, IID_PPV_ARGS (&pBackBuffer));
      backendData->pd3dDevice->CreateRenderTargetView (pBackBuffer, NULL, &viewportData->RTView);
      pBackBuffer->Release();
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

    sBackendData* backendData = getBackendData();

    sViewportData* viewportData = (sViewportData*)viewport->RendererUserData;
    if (viewportData->RTView) {
      viewportData->RTView->Release();
      viewportData->RTView = NULL;
      }

    if (viewportData->SwapChain) {
      ID3D11Texture2D* pBackBuffer = NULL;
      viewportData->SwapChain->ResizeBuffers(0, (UINT)size.x, (UINT)size.y, DXGI_FORMAT_UNKNOWN, 0);

      viewportData->SwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
      if (pBackBuffer == NULL) {
        fprintf(stderr, "SetWindowSize() failed creating buffers.\n");
        return;
        }

      backendData->pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &viewportData->RTView);
      pBackBuffer->Release();
      }
    }
  //}}}
  //{{{
  void renderWindow (ImGuiViewport* viewport, void*) {

    sBackendData* backendData = getBackendData();
    sViewportData* viewportData = (sViewportData*)viewport->RendererUserData;
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

    backendData->pd3dDeviceContext->OMSetRenderTargets(1, &viewportData->RTView, NULL);
    if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear))
      backendData->pd3dDeviceContext->ClearRenderTargetView (viewportData->RTView, (float*)&clear_color);

    renderDrawData (viewport->DrawData);
    }
  //}}}

  //{{{
  void swapBuffers (ImGuiViewport* viewport, void*) {

    sViewportData* viewportData = (sViewportData*)viewport->RendererUserData;
    viewportData->SwapChain->Present(0, 0); // Present without vsync
    }
  //}}}
  //{{{
  void initPlatformInterface() {

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    platform_io.Renderer_CreateWindow = createWindow;
    platform_io.Renderer_DestroyWindow = destroyWindow;
    platform_io.Renderer_SetWindowSize = setWindowSize;
    platform_io.Renderer_RenderWindow = renderWindow;
    platform_io.Renderer_SwapBuffers = swapBuffers;
    }
  //}}}
  //{{{
  void shutdownPlatformInterface() {
    ImGui::DestroyPlatformWindows();
    }
  //}}}
  }

//{{{
bool cGraphics::init (ID3D11Device* device, ID3D11DeviceContext* deviceContext) {

  ImGuiIO& io = ImGui::GetIO();
  IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");

  // Setup backend capabilities flags
  sBackendData* backendData = IM_NEW (sBackendData)();
  io.BackendRendererUserData = (void*)backendData;
  io.BackendRendererName = "imgui_impl_dx11";
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
  io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;  // We can create multi-viewports on the Renderer side (optional)

  // Get factory from device
  IDXGIDevice* pDXGIDevice = NULL;
  IDXGIAdapter* pDXGIAdapter = NULL;
  IDXGIFactory* pFactory = NULL;

  if (device->QueryInterface (IID_PPV_ARGS (&pDXGIDevice)) == S_OK)
    if (pDXGIDevice->GetParent (IID_PPV_ARGS (&pDXGIAdapter)) == S_OK)
      if (pDXGIAdapter->GetParent (IID_PPV_ARGS (&pFactory)) == S_OK) {
        backendData->pd3dDevice = device;
        backendData->pd3dDeviceContext = deviceContext;
        backendData->pFactory = pFactory;
        }

  if (pDXGIDevice)
    pDXGIDevice->Release();
  if (pDXGIAdapter)
    pDXGIAdapter->Release();

  backendData->pd3dDevice->AddRef();
  backendData->pd3dDeviceContext->AddRef();

  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    initPlatformInterface();

  createDeviceObjects();

  return true;
  }
//}}}
//{{{
void cGraphics::shutdown() {

  ImGuiIO& io = ImGui::GetIO();
  sBackendData* backendData = getBackendData();

  shutdownPlatformInterface();
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
