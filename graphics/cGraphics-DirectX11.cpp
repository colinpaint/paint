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
  struct VERTEX_CONSTANT_BUFFER {
    float   mvp[4][4];
    };

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
  //void createFontsTexture() {
  //// build texture atlas

    //sBackendData* backendData = getBackendData();

    //unsigned char* pixels;
    //int width;
    //int height;
    //ImGui::GetIO().Fonts->GetTexDataAsRGBA32 (&pixels, &width, &height);

    //// upload texture to graphics system
    //D3D11_TEXTURE2D_DESC texture2dDesc;
    //ZeroMemory (&texture2dDesc, sizeof(texture2dDesc));
    //texture2dDesc.Width = width;
    //texture2dDesc.Height = height;
    //texture2dDesc.MipLevels = 1;
    //texture2dDesc.ArraySize = 1;
    //texture2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //texture2dDesc.SampleDesc.Count = 1;
    //texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
    //texture2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    //texture2dDesc.CPUAccessFlags = 0;

    //ID3D11Texture2D* texture2d = NULL;
    //D3D11_SUBRESOURCE_DATA subResource;
    //subResource.pSysMem = pixels;
    //subResource.SysMemPitch = texture2dDesc.Width * 4;
    //subResource.SysMemSlicePitch = 0;

    //backendData->pd3dDevice->CreateTexture2D (&texture2dDesc, &subResource, &texture2d);
    //IM_ASSERT (texture2d != NULL);

    //// create texture view
    //D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
    //ZeroMemory (&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
    //shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    //shaderResourceViewDesc.Texture2D.MipLevels = texture2dDesc.MipLevels;
    //shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    //backendData->pd3dDevice->CreateShaderResourceView (texture2d, &shaderResourceViewDesc, &backendData->pFontTextureView);
    //texture2d->Release();

    //// store our fonts texture view
    //ImGui::GetIO().Fonts->SetTexID ((ImTextureID)backendData->pFontTextureView);

    //// create texture sampler
    //D3D11_SAMPLER_DESC samplerDesc;
    //ZeroMemory (&samplerDesc, sizeof(samplerDesc));
    //samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    //samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    //samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    //samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    //samplerDesc.MipLODBias = 0.f;
    //samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    //samplerDesc.MinLOD = 0.f;
    //samplerDesc.MaxLOD = 0.f;
    //backendData->pd3dDevice->CreateSamplerState (&samplerDesc, &backendData->pFontSampler);
    //}
  //}}}
  //{{{
  //bool createDeviceObjects() {

    //sBackendData* backendData = getBackendData();

    ////  create vertexShader
    //{{{
    //static const char* kVertexShaderStr =
        //"cbuffer vertexBuffer : register(b0) \
        //{\
          //float4x4 ProjectionMatrix; \
        //};\
        //struct VS_INPUT\
        //{\
          //float2 pos : POSITION;\
          //float4 col : COLOR0;\
          //float2 uv  : TEXCOORD0;\
        //};\
        //\
        //struct PS_INPUT\
        //{\
          //float4 pos : SV_POSITION;\
          //float4 col : COLOR0;\
          //float2 uv  : TEXCOORD0;\
        //};\
        //\
        //PS_INPUT main(VS_INPUT input)\
        //{\
          //PS_INPUT output;\
          //output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
          //output.col = input.col;\
          //output.uv  = input.uv;\
          //return output;\
        //}";
    //}}}
    //ID3DBlob* vertexShaderBlob;
    //if (FAILED (D3DCompile (kVertexShaderStr, strlen(kVertexShaderStr),
                            //NULL, NULL, NULL,
                            //"main", "vs_4_0", 0, 0, &vertexShaderBlob, NULL))) {
      //{{{  error, return false
      //cLog::log (LOGERROR, "vertexShader compile failed");
      //return false;
      //}
      //}}}
    //if (backendData->pd3dDevice->CreateVertexShader (vertexShaderBlob->GetBufferPointer(),
                                                     //vertexShaderBlob->GetBufferSize(),
                                                     //NULL, &backendData->pVertexShader) != S_OK) {
      //{{{  release, return false
      //cLog::log (LOGERROR, "vertexShader create failed");
      //vertexShaderBlob->Release();
      //return false;
      //}
      //}}}
    //{{{  create the input layout
    //D3D11_INPUT_ELEMENT_DESC localLayout[] = {
      //{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
      //{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
      //{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
      //};

    //if (backendData->pd3dDevice->CreateInputLayout (localLayout, 3,
                                                    //vertexShaderBlob->GetBufferPointer(),
                                                    //vertexShaderBlob->GetBufferSize(),
                                                    //&backendData->pInputLayout) != S_OK) {
      //// release, return false
      //cLog::log (LOGERROR, "pixelShader constants create failed");
      //vertexShaderBlob->Release();
      //return false;
      //}
    //}}}
    //vertexShaderBlob->Release();

    //{{{  create the constant buffer
    //D3D11_BUFFER_DESC bufferDesc;
    //bufferDesc.ByteWidth = sizeof(sViewportData);
    //bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    //bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    //bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    //bufferDesc.MiscFlags = 0;
    //backendData->pd3dDevice->CreateBuffer (&bufferDesc, NULL, &backendData->pVertexConstantBuffer);
    //}}}

    //// create pixelShader
    //{{{
    //static const char* kPixelShaderStr =
        //"struct PS_INPUT\
        //{\
        //float4 pos : SV_POSITION;\
        //float4 col : COLOR0;\
        //float2 uv  : TEXCOORD0;\
        //};\
        //sampler sampler0;\
        //Texture2D texture0;\
        //\
        //float4 main(PS_INPUT input) : SV_Target\
        //{\
        //float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
        //return out_col; \
        //}";
    //}}}
    //ID3DBlob* pixelShaderBlob;
    //if (FAILED (D3DCompile (kPixelShaderStr, strlen(kPixelShaderStr),
                            //NULL, NULL, NULL,
                            //"main", "ps_4_0", 0, 0, &pixelShaderBlob, NULL))) {
      //{{{  error, return false
      //cLog::log (LOGERROR, "pixelShader compile failed");
      //return false;
      //}
      //}}}
    //if (backendData->pd3dDevice->CreatePixelShader (pixelShaderBlob->GetBufferPointer(),
                                                    //pixelShaderBlob->GetBufferSize(),
                                                    //NULL, &backendData->pPixelShader) != S_OK) {
      //{{{  release, return false
      //cLog::log (LOGERROR, "create pixelShader failed");
      //pixelShaderBlob->Release();
      //return false;
      //}
      //}}}
    //pixelShaderBlob->Release();

    //// create the blendDesc
    //D3D11_BLEND_DESC blendDesc;
    //ZeroMemory (&blendDesc, sizeof(blendDesc));
    //blendDesc.AlphaToCoverageEnable = false;
    //blendDesc.RenderTarget[0].BlendEnable = true;
    //blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    //blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    //blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    //blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    //blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    //blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    //blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    //backendData->pd3dDevice->CreateBlendState (&blendDesc, &backendData->pBlendState);

    //// create the rasterizerDesc
    //D3D11_RASTERIZER_DESC rasterizerDesc;
    //ZeroMemory (&rasterizerDesc, sizeof(rasterizerDesc));
    //rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    //rasterizerDesc.CullMode = D3D11_CULL_NONE;
    //rasterizerDesc.ScissorEnable = true;
    //rasterizerDesc.DepthClipEnable = true;
    //backendData->pd3dDevice->CreateRasterizerState (&rasterizerDesc, &backendData->pRasterizerState);

    //// create depthStencilDesc
    //D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    //ZeroMemory (&depthStencilDesc, sizeof(depthStencilDesc));
    //depthStencilDesc.DepthEnable = false;
    //depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    //depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    //depthStencilDesc.StencilEnable = false;
    //depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    //depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    //depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    //depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    //depthStencilDesc.BackFace = depthStencilDesc.FrontFace;
    //backendData->pd3dDevice->CreateDepthStencilState (&depthStencilDesc, &backendData->pDepthStencilState);

    //createFontsTexture();

    //return true;
    //}
  //}}}
  //{{{
  //void setupRenderState (ImDrawData* drawData, ID3D11DeviceContext* deviceContext) {

    //// set viewport
    //D3D11_VIEWPORT viewport;
    //memset (&viewport, 0, sizeof(D3D11_VIEWPORT));
    //viewport.Width = drawData->DisplaySize.x;
    //viewport.Height = drawData->DisplaySize.y;
    //viewport.MinDepth = 0.0f;
    //viewport.MaxDepth = 1.0f;
    //viewport.TopLeftX = viewport.TopLeftY = 0;
    //deviceContext->RSSetViewports (1, &viewport);

    //// set shader, vertex buffers
    //sBackendData* backendData = getBackendData();
    //unsigned int stride = sizeof(ImDrawVert);
    //unsigned int offset = 0;
    //deviceContext->IASetInputLayout (backendData->pInputLayout);
    //deviceContext->IASetVertexBuffers (0, 1, &backendData->pVB, &stride, &offset);
    //deviceContext->IASetIndexBuffer (backendData->pIB, sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
    //deviceContext->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //deviceContext->VSSetShader (backendData->pVertexShader, NULL, 0);
    //deviceContext->VSSetConstantBuffers (0, 1, &backendData->pVertexConstantBuffer);
    //deviceContext->PSSetShader (backendData->pPixelShader, NULL, 0);
    //deviceContext->PSSetSamplers (0, 1, &backendData->pFontSampler);
    //deviceContext->GSSetShader (NULL, NULL, 0);
    //deviceContext->HSSetShader (NULL, NULL, 0);
    //deviceContext->DSSetShader (NULL, NULL, 0);
    //deviceContext->CSSetShader (NULL, NULL, 0);

    //// set blend state
    //const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
    //deviceContext->OMSetBlendState (backendData->pBlendState, blend_factor, 0xffffffff);
    //deviceContext->OMSetDepthStencilState (backendData->pDepthStencilState, 0);
    //deviceContext->RSSetState (backendData->pRasterizerState);
    //}
  //}}}
  //{{{
  //void renderDrawData (ImDrawData* drawData) {

    //// avoid rendering when minimized
    //if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
      //return;

    //sBackendData* backendData = getBackendData();
    //ID3D11DeviceContext* deviceContext = backendData->pd3dDeviceContext;

    //// copy drawList vertices,indices to continuous GPU buffers
    //int numVertices = 0;
    //int numIndices = 0;
    //{{{  manage vertex GPU buffer
    //if (!backendData->pVB || (backendData->VertexBufferSize < drawData->TotalVtxCount)) {
      //// need new vertexBuffer
      //if (backendData->pVB) {
        //// release old vertexBuffer
        //backendData->pVB->Release();
        //backendData->pVB = NULL;
        //}
      //backendData->VertexBufferSize = drawData->TotalVtxCount + 5000;

      //// get new vertexBuffer
      //D3D11_BUFFER_DESC desc;
      //memset (&desc, 0, sizeof(D3D11_BUFFER_DESC));
      //desc.Usage = D3D11_USAGE_DYNAMIC;
      //desc.ByteWidth = backendData->VertexBufferSize * sizeof(ImDrawVert);
      //desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
      //desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      //desc.MiscFlags = 0;
      //if (backendData->pd3dDevice->CreateBuffer (&desc, NULL, &backendData->pVB) < 0) {
        //cLog::log (LOGERROR, "vertex CreateBuffer failed");
        //return;
        //}
      //}

    //// map gpu vertexBuffer
    //D3D11_MAPPED_SUBRESOURCE vertexSubResource;
    //if (deviceContext->Map (backendData->pVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &vertexSubResource) != S_OK) {
      //cLog::log (LOGERROR, "vertex Map failed");
      //return;
      //}

    //ImDrawVert* vertexDest = (ImDrawVert*)vertexSubResource.pData;
    //}}}
    //{{{  manage index GPU buffer
    //if (!backendData->pIB || (backendData->IndexBufferSize < drawData->TotalIdxCount)) {
      //// need new indexBuffer
      //if (backendData->pIB) {
        //// release old indexBuffer
        //backendData->pIB->Release();
        //backendData->pIB = NULL;
        //}
      //backendData->IndexBufferSize = drawData->TotalIdxCount + 10000;

      //// get new indexBuffer
      //D3D11_BUFFER_DESC desc;
      //memset (&desc, 0, sizeof(D3D11_BUFFER_DESC));
      //desc.Usage = D3D11_USAGE_DYNAMIC;
      //desc.ByteWidth = backendData->IndexBufferSize * sizeof(ImDrawIdx);
      //desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
      //desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      //if (backendData->pd3dDevice->CreateBuffer (&desc, NULL, &backendData->pIB) < 0) {
        //cLog::log (LOGERROR, "index CreateBuffer failed");
        //return;
        //}
      //}

    //// map gpu indexBuffer
    //D3D11_MAPPED_SUBRESOURCE indexSubResource;
    //if (deviceContext->Map (backendData->pIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &indexSubResource) != S_OK) {
      //cLog::log (LOGERROR, "index Map failed");
      //return;
      //}
    //ImDrawIdx* indexDest = (ImDrawIdx*)indexSubResource.pData;
    //}}}
    //for (int n = 0; n < drawData->CmdListsCount; n++) {
      //const ImDrawList* cmdList = drawData->CmdLists[n];
      //memcpy (vertexDest, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
      //memcpy (indexDest, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
      //vertexDest += cmdList->VtxBuffer.Size;
      //indexDest += cmdList->IdxBuffer.Size;
      //numVertices += cmdList->VtxBuffer.Size;
      //numIndices += cmdList->IdxBuffer.Size;
      //}
    //deviceContext->Unmap (backendData->pVB, 0);
    //deviceContext->Unmap (backendData->pIB, 0);
    ////cLog::log (LOGINFO, format ("added {} {}", numVertices, numIndices));

    //{{{  set orthographic projection matrix in GPU constant
    //{{{
    //struct sMatrix {
      //float matrix[4][4];
      //};
    //}}}
    //// visible imgui space lies
    //// - from draw_data->DisplayPos (top left)
    //// - to draw_data->DisplayPos+data_data->DisplaySize (bottom right)
    //// - DisplayPos is (0,0) for single viewport apps.

    //const float L = drawData->DisplayPos.x;
    //const float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
    //const float T = drawData->DisplayPos.y;
    //const float B = drawData->DisplayPos.y + drawData->DisplaySize.y;

    //const float kMatrix[4][4] = {
      //{ 2.0f/(R-L),  0.0f,        0.0f, 0.0f },
      //{ 0.0f,        2.0f/(T-B),  0.0f, 0.0f },
      //{ 0.0f,        0.0f,        0.5f, 0.0f },
      //{ (R+L)/(L-R), (T+B)/(B-T), 0.5f, 1.0f },
      //};

    //// map and copy vertex matrix
    //D3D11_MAPPED_SUBRESOURCE mappedSubResource;
    //if (deviceContext->Map (backendData->pVertexConstantBuffer,
                            //0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource) != S_OK) {
      //cLog::log (LOGERROR, "vertex constant Map failed");
      //return;
      //}

    //sMatrix* matrix = (sMatrix*)mappedSubResource.pData;
    //memcpy (&matrix->matrix, kMatrix, sizeof(kMatrix));
    //deviceContext->Unmap (backendData->pVertexConstantBuffer, 0);
    //}}}

    //setupRenderState (drawData, deviceContext);

    //// render command lists
    //int indexOffset = 0;
    //int vertexOffset = 0;
    //ImVec2 clipOffset = drawData->DisplayPos;
    //for (int cmdListIndex = 0; cmdListIndex < drawData->CmdListsCount; cmdListIndex++) {
      //const ImDrawList* cmdList = drawData->CmdLists[cmdListIndex];
      //for (int cmdIndex = 0; cmdIndex < cmdList->CmdBuffer.Size; cmdIndex++) {
        //const ImDrawCmd* drawCmd = &cmdList->CmdBuffer[cmdIndex];
        //if (drawCmd->UserCallback != NULL) {
          //// User callback, registered via ImDrawList::AddCallback()
          //// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
          //if (drawCmd->UserCallback == ImDrawCallback_ResetRenderState)
            //setupRenderState (drawData, deviceContext);
          //else
            //drawCmd->UserCallback (cmdList, drawCmd);
          //}
        //else {
          //// Apply scissor/clipping rectangle
          //const D3D11_RECT r = {
            //(LONG)(drawCmd->ClipRect.x - clipOffset.x), (LONG)(drawCmd->ClipRect.y - clipOffset.y),
            //(LONG)(drawCmd->ClipRect.z - clipOffset.x), (LONG)(drawCmd->ClipRect.w - clipOffset.y) };
          //deviceContext->RSSetScissorRects (1, &r);

          //// bind texture and draw
          //ID3D11ShaderResourceView* shaderResourceView = (ID3D11ShaderResourceView*)drawCmd->GetTexID();
          //deviceContext->PSSetShaderResources (0, 1, &shaderResourceView);
          //deviceContext->DrawIndexed (drawCmd->ElemCount, drawCmd->IdxOffset + indexOffset, drawCmd->VtxOffset + vertexOffset);
          //}
        //}

      //indexOffset += cmdList->IdxBuffer.Size;
      //vertexOffset += cmdList->VtxBuffer.Size;
      //}
    //}
  //}}}
  //{{{
  void invalidateDeviceObjects() {

    sBackendData* backendData = getBackendData();

    if (backendData->pFontSampler)
      backendData->pFontSampler->Release();

    if (backendData->pFontTextureView)
      backendData->pFontTextureView->Release();

    if (backendData->pIB)
      backendData->pIB->Release();

    if (backendData->pVB)
      backendData->pVB->Release();

    if (backendData->pBlendState)
      backendData->pBlendState->Release();

    if (backendData->pDepthStencilState)
      backendData->pDepthStencilState->Release();

    if (backendData->pRasterizerState)
      backendData->pRasterizerState->Release();

    if (backendData->pPixelShader)
      backendData->pPixelShader->Release();

    if (backendData->pVertexConstantBuffer)
      backendData->pVertexConstantBuffer->Release();

    if (backendData->pInputLayout)
      backendData->pInputLayout->Release();

    if (backendData->pVertexShader)
      backendData->pVertexShader->Release();
    }
  //}}}

  //{{{
  void createFontsTexture()
  {
      // Build texture atlas
      ImGuiIO& io = ImGui::GetIO();
      sBackendData* bd = getBackendData();
      unsigned char* pixels;
      int width, height;
      io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

      // Upload texture to graphics system
      {
          D3D11_TEXTURE2D_DESC desc;
          ZeroMemory(&desc, sizeof(desc));
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
          bd->pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
          IM_ASSERT(pTexture != NULL);

          // Create texture view
          D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
          ZeroMemory(&srvDesc, sizeof(srvDesc));
          srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
          srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
          srvDesc.Texture2D.MipLevels = desc.MipLevels;
          srvDesc.Texture2D.MostDetailedMip = 0;
          bd->pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &bd->pFontTextureView);
          pTexture->Release();
      }

      // Store our identifier
      io.Fonts->SetTexID((ImTextureID)bd->pFontTextureView);

      // Create texture sampler
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
          bd->pd3dDevice->CreateSamplerState(&desc, &bd->pFontSampler);
      }
  }
  //}}}
  //{{{
  bool createDeviceObjects()
  {
    sBackendData* bd = getBackendData();
      if (!bd->pd3dDevice)
          return false;
      if (bd->pFontSampler)
          invalidateDeviceObjects();

      // By using D3DCompile() from <d3dcompiler.h> / d3dcompiler.lib, we introduce a dependency to a given version of d3dcompiler_XX.dll (see D3DCOMPILER_DLL_A)
      // If you would like to use this DX11 sample code but remove this dependency you can:
      //  1) compile once, save the compiled shader blobs into a file or source code and pass them to CreateVertexShader()/CreatePixelShader() [preferred solution]
      //  2) use code to detect any version of the DLL and grab a pointer to D3DCompile from the DLL.
      // See https://github.com/ocornut/imgui/pull/638 for sources and details.

      // Create the vertex shader
      {
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

          ID3DBlob* vertexShaderBlob;
          if (FAILED(D3DCompile(vertexShader, strlen(vertexShader), NULL, NULL, NULL, "main", "vs_4_0", 0, 0, &vertexShaderBlob, NULL)))
              return false; // NB: Pass ID3DBlob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
          if (bd->pd3dDevice->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), NULL, &bd->pVertexShader) != S_OK)
          {
              vertexShaderBlob->Release();
              return false;
          }

          // Create the input layout
          D3D11_INPUT_ELEMENT_DESC local_layout[] =
          {
              { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
              { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
              { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
          };
          if (bd->pd3dDevice->CreateInputLayout(local_layout, 3, vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &bd->pInputLayout) != S_OK)
          {
              vertexShaderBlob->Release();
              return false;
          }
          vertexShaderBlob->Release();

          // Create the constant buffer
          {
              D3D11_BUFFER_DESC desc;
              desc.ByteWidth = sizeof(VERTEX_CONSTANT_BUFFER);
              desc.Usage = D3D11_USAGE_DYNAMIC;
              desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
              desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
              desc.MiscFlags = 0;
              bd->pd3dDevice->CreateBuffer(&desc, NULL, &bd->pVertexConstantBuffer);
          }
      }

      // Create the pixel shader
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
          if (bd->pd3dDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), NULL, &bd->pPixelShader) != S_OK)
          {
              pixelShaderBlob->Release();
              return false;
          }
          pixelShaderBlob->Release();
      }

      // Create the blending setup
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
          bd->pd3dDevice->CreateBlendState(&desc, &bd->pBlendState);
      }

      // Create the rasterizer state
      {
          D3D11_RASTERIZER_DESC desc;
          ZeroMemory(&desc, sizeof(desc));
          desc.FillMode = D3D11_FILL_SOLID;
          desc.CullMode = D3D11_CULL_NONE;
          desc.ScissorEnable = true;
          desc.DepthClipEnable = true;
          bd->pd3dDevice->CreateRasterizerState(&desc, &bd->pRasterizerState);
      }

      // Create depth-stencil State
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
          bd->pd3dDevice->CreateDepthStencilState(&desc, &bd->pDepthStencilState);
      }

      createFontsTexture();

      return true;
  }
  //}}}
  //{{{
  void setupRenderState (ImDrawData* draw_data, ID3D11DeviceContext* ctx)
  {
    sBackendData* bd = getBackendData();

      // Setup viewport
      D3D11_VIEWPORT vp;
      memset(&vp, 0, sizeof(D3D11_VIEWPORT));
      vp.Width = draw_data->DisplaySize.x;
      vp.Height = draw_data->DisplaySize.y;
      vp.MinDepth = 0.0f;
      vp.MaxDepth = 1.0f;
      vp.TopLeftX = vp.TopLeftY = 0;
      ctx->RSSetViewports(1, &vp);

      // Setup shader and vertex buffers
      unsigned int stride = sizeof(ImDrawVert);
      unsigned int offset = 0;
      ctx->IASetInputLayout(bd->pInputLayout);
      ctx->IASetVertexBuffers(0, 1, &bd->pVB, &stride, &offset);
      ctx->IASetIndexBuffer(bd->pIB, sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
      ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      ctx->VSSetShader(bd->pVertexShader, NULL, 0);
      ctx->VSSetConstantBuffers(0, 1, &bd->pVertexConstantBuffer);
      ctx->PSSetShader(bd->pPixelShader, NULL, 0);
      ctx->PSSetSamplers(0, 1, &bd->pFontSampler);
      ctx->GSSetShader(NULL, NULL, 0);
      ctx->HSSetShader(NULL, NULL, 0); // In theory we should backup and restore this as well.. very infrequently used..
      ctx->DSSetShader(NULL, NULL, 0); // In theory we should backup and restore this as well.. very infrequently used..
      ctx->CSSetShader(NULL, NULL, 0); // In theory we should backup and restore this as well.. very infrequently used..

      // Setup blend state
      const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
      ctx->OMSetBlendState(bd->pBlendState, blend_factor, 0xffffffff);
      ctx->OMSetDepthStencilState(bd->pDepthStencilState, 0);
      ctx->RSSetState(bd->pRasterizerState);
  }
  //}}}
  //{{{
  void renderDrawData (ImDrawData* draw_data)
  {
      // Avoid rendering when minimized
      if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
          return;

      sBackendData* bd = getBackendData();
      ID3D11DeviceContext* ctx = bd->pd3dDeviceContext;

      // Create and grow vertex/index buffers if needed
      if (!bd->pVB || bd->VertexBufferSize < draw_data->TotalVtxCount)
      {
          if (bd->pVB) { bd->pVB->Release(); bd->pVB = NULL; }
          bd->VertexBufferSize = draw_data->TotalVtxCount + 5000;
          D3D11_BUFFER_DESC desc;
          memset(&desc, 0, sizeof(D3D11_BUFFER_DESC));
          desc.Usage = D3D11_USAGE_DYNAMIC;
          desc.ByteWidth = bd->VertexBufferSize * sizeof(ImDrawVert);
          desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
          desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
          desc.MiscFlags = 0;
          if (bd->pd3dDevice->CreateBuffer(&desc, NULL, &bd->pVB) < 0)
              return;
      }
      if (!bd->pIB || bd->IndexBufferSize < draw_data->TotalIdxCount)
      {
          if (bd->pIB) { bd->pIB->Release(); bd->pIB = NULL; }
          bd->IndexBufferSize = draw_data->TotalIdxCount + 10000;
          D3D11_BUFFER_DESC desc;
          memset(&desc, 0, sizeof(D3D11_BUFFER_DESC));
          desc.Usage = D3D11_USAGE_DYNAMIC;
          desc.ByteWidth = bd->IndexBufferSize * sizeof(ImDrawIdx);
          desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
          desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
          if (bd->pd3dDevice->CreateBuffer(&desc, NULL, &bd->pIB) < 0)
              return;
      }

      // Upload vertex/index data into a single contiguous GPU buffer
      D3D11_MAPPED_SUBRESOURCE vtx_resource, idx_resource;
      if (ctx->Map(bd->pVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &vtx_resource) != S_OK)
          return;
      if (ctx->Map(bd->pIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &idx_resource) != S_OK)
          return;
      ImDrawVert* vtx_dst = (ImDrawVert*)vtx_resource.pData;
      ImDrawIdx* idx_dst = (ImDrawIdx*)idx_resource.pData;
      for (int n = 0; n < draw_data->CmdListsCount; n++)
      {
          const ImDrawList* cmd_list = draw_data->CmdLists[n];
          memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
          memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
          vtx_dst += cmd_list->VtxBuffer.Size;
          idx_dst += cmd_list->IdxBuffer.Size;
      }
      ctx->Unmap(bd->pVB, 0);
      ctx->Unmap(bd->pIB, 0);

      // Setup orthographic projection matrix into our constant buffer
      // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
      {
          D3D11_MAPPED_SUBRESOURCE mapped_resource;
          if (ctx->Map(bd->pVertexConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK)
              return;
          VERTEX_CONSTANT_BUFFER* constant_buffer = (VERTEX_CONSTANT_BUFFER*)mapped_resource.pData;
          float L = draw_data->DisplayPos.x;
          float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
          float T = draw_data->DisplayPos.y;
          float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
          float mvp[4][4] =
          {
              { 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
              { 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
              { 0.0f,         0.0f,           0.5f,       0.0f },
              { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
          };
          memcpy(&constant_buffer->mvp, mvp, sizeof(mvp));
          ctx->Unmap(bd->pVertexConstantBuffer, 0);
      }

      // Backup DX state that will be modified to restore it afterwards (unfortunately this is very ugly looking and verbose. Close your eyes!)
      struct BACKUP_DX11_STATE
      {
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
      BACKUP_DX11_STATE old = {};
      old.ScissorRectsCount = old.ViewportsCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
      ctx->RSGetScissorRects(&old.ScissorRectsCount, old.ScissorRects);
      ctx->RSGetViewports(&old.ViewportsCount, old.Viewports);
      ctx->RSGetState(&old.RS);
      ctx->OMGetBlendState(&old.BlendState, old.BlendFactor, &old.SampleMask);
      ctx->OMGetDepthStencilState(&old.DepthStencilState, &old.StencilRef);
      ctx->PSGetShaderResources(0, 1, &old.PSShaderResource);
      ctx->PSGetSamplers(0, 1, &old.PSSampler);
      old.PSInstancesCount = old.VSInstancesCount = old.GSInstancesCount = 256;
      ctx->PSGetShader(&old.PS, old.PSInstances, &old.PSInstancesCount);
      ctx->VSGetShader(&old.VS, old.VSInstances, &old.VSInstancesCount);
      ctx->VSGetConstantBuffers(0, 1, &old.VSConstantBuffer);
      ctx->GSGetShader(&old.GS, old.GSInstances, &old.GSInstancesCount);

      ctx->IAGetPrimitiveTopology(&old.PrimitiveTopology);
      ctx->IAGetIndexBuffer(&old.IndexBuffer, &old.IndexBufferFormat, &old.IndexBufferOffset);
      ctx->IAGetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset);
      ctx->IAGetInputLayout(&old.InputLayout);

      // Setup desired DX state
      setupRenderState(draw_data, ctx);

      // Render command lists
      // (Because we merged all buffers into a single one, we maintain our own offset into them)
      int global_idx_offset = 0;
      int global_vtx_offset = 0;
      ImVec2 clip_off = draw_data->DisplayPos;
      for (int n = 0; n < draw_data->CmdListsCount; n++)
      {
          const ImDrawList* cmd_list = draw_data->CmdLists[n];
          for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
          {
              const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
              if (pcmd->UserCallback != NULL)
              {
                  // User callback, registered via ImDrawList::AddCallback()
                  // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                  if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                      setupRenderState(draw_data, ctx);
                  else
                      pcmd->UserCallback(cmd_list, pcmd);
              }
              else
              {
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

      // Restore modified DX state
      ctx->RSSetScissorRects(old.ScissorRectsCount, old.ScissorRects);
      ctx->RSSetViewports(old.ViewportsCount, old.Viewports);
      ctx->RSSetState(old.RS); if (old.RS) old.RS->Release();
      ctx->OMSetBlendState(old.BlendState, old.BlendFactor, old.SampleMask); if (old.BlendState) old.BlendState->Release();
      ctx->OMSetDepthStencilState(old.DepthStencilState, old.StencilRef); if (old.DepthStencilState) old.DepthStencilState->Release();
      ctx->PSSetShaderResources(0, 1, &old.PSShaderResource); if (old.PSShaderResource) old.PSShaderResource->Release();
      ctx->PSSetSamplers(0, 1, &old.PSSampler); if (old.PSSampler) old.PSSampler->Release();
      ctx->PSSetShader(old.PS, old.PSInstances, old.PSInstancesCount); if (old.PS) old.PS->Release();
      for (UINT i = 0; i < old.PSInstancesCount; i++) if (old.PSInstances[i]) old.PSInstances[i]->Release();
      ctx->VSSetShader(old.VS, old.VSInstances, old.VSInstancesCount); if (old.VS) old.VS->Release();
      ctx->VSSetConstantBuffers(0, 1, &old.VSConstantBuffer); if (old.VSConstantBuffer) old.VSConstantBuffer->Release();
      ctx->GSSetShader(old.GS, old.GSInstances, old.GSInstancesCount); if (old.GS) old.GS->Release();
      for (UINT i = 0; i < old.VSInstancesCount; i++) if (old.VSInstances[i]) old.VSInstances[i]->Release();
      ctx->IASetPrimitiveTopology(old.PrimitiveTopology);
      ctx->IASetIndexBuffer(old.IndexBuffer, old.IndexBufferFormat, old.IndexBufferOffset); if (old.IndexBuffer) old.IndexBuffer->Release();
      ctx->IASetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset); if (old.VertexBuffer) old.VertexBuffer->Release();
      ctx->IASetInputLayout(old.InputLayout); if (old.InputLayout) old.InputLayout->Release();
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

    cLog::log (LOGINFO, "destroyWindow");

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

    cLog::log (LOGINFO, "setWindowSize");

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

    cLog::log (LOGINFO, "renderWindow");

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

    cLog::log (LOGINFO, "swapBuffers");
    sViewportData* viewportData = (sViewportData*)viewport->RendererUserData;
    viewportData->SwapChain->Present (0,0); // Present without vsync
    }
  //}}}
  }

//{{{
bool cGraphics::init (void* device, void* deviceContext) {

  bool ok = false;

  // allocate backendData and set backend capabilities
  sBackendData* backendData = IM_NEW (sBackendData)();
  ImGui::GetIO().BackendRendererUserData = (void*)backendData;
  ImGui::GetIO().BackendRendererName = "imgui_impl_dx11";
  ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
  // We can create multi-viewports on the Renderer side (optional)
  //ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

  // Get factory from device adpater
  ID3D11Device* d3dDevice = (ID3D11Device*)device;
  ID3D11DeviceContext* d3dDeviceContext = (ID3D11DeviceContext*)deviceContext;

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

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
          // init platFormInterface
          ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
          platform_io.Renderer_CreateWindow = createWindow;
          platform_io.Renderer_DestroyWindow = destroyWindow;
          platform_io.Renderer_SetWindowSize = setWindowSize;
          platform_io.Renderer_RenderWindow = renderWindow;
          platform_io.Renderer_SwapBuffers = swapBuffers;
          }

        ok = createDeviceObjects();
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
  invalidateDeviceObjects();

  sBackendData* backendData = getBackendData();
  if (backendData->pFactory)
    backendData->pFactory->Release();
  if (backendData->pd3dDevice)
    backendData->pd3dDevice->Release();
  if (backendData->pd3dDeviceContext)
    backendData->pd3dDeviceContext->Release();

  ImGui::GetIO().BackendRendererName = NULL;
  ImGui::GetIO().BackendRendererUserData = NULL;

  IM_DELETE (backendData);
  }
//}}}

//{{{
void cGraphics::draw() {
  renderDrawData (ImGui::GetDrawData());
  }
//}}}
