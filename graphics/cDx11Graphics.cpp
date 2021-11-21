// cDx11Graphics.cpp - !!! need to finsh quad, frameBuffer and shader !!!
#ifdef WIN32 // stop linux compile, simpler cmake
//{{{  includes
#define NOMINMAX

#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

#include <stdio.h>
#include <tchar.h>
#include <d3d11.h>
#include <d3dcompiler.h>

// imGui
#include <imgui.h>
#include <backends/imgui_impl_dx11.h>

#include "cGraphics.h"
#include "../platform/cPlatform.h"
#include "../utils/cLog.h"

using namespace std;
//}}}
//#define USE_IMPL // use released imgui backend impl untouched, !!! framework only, not finished !!!

constexpr bool kDebug = false;
namespace {
  //{{{
  class cDx11Texture : public cTexture {
  public:
    cDx11Texture (eTextureType textureType, cPoint size, uint8_t* pixels)
      : cTexture(textureType, size) {(void)pixels;}

    virtual void setPixels (uint8_t* pixels) final { (void)pixels; }
    virtual void setSource() final {}
    };
  //}}}
  //{{{
  class cDx11Quad : public cQuad {
  public:
    //{{{
    cDx11Quad (cPoint size) : cQuad(mSize) {

      // vertices
      //glGenBuffers (1, &mVertexBufferObject);
      //glBindBuffer (GL_ARRAY_BUFFER, mVertexBufferObject);

      const float widthF = static_cast<float>(size.x);
      const float heightF = static_cast<float>(size.y);
      const float kVertices[] = {
        0.f,   heightF,  0.f,1.f, // tl vertex
        widthF,0.f,      1.f,0.f, // br vertex
        0.f,   0.f,      0.f,0.f, // bl vertex
        widthF,heightF,  1.f,1.f, // tr vertex
        };

      // indices
      //mNumIndices = sizeof(kIndices);
      }
    //}}}
    //{{{
    cDx11Quad (cPoint size, const cRect& rect) : cQuad(size) {

      // vertexArray

      const float subLeftF = static_cast<float>(rect.left);
      const float subRightF = static_cast<float>(rect.right);
      const float subBottomF = static_cast<float>(rect.bottom);
      const float subTopF = static_cast<float>(rect.top);

      const float subLeftTexF = subLeftF / size.x;
      const float subRightTexF = subRightF / size.x;
      const float subBottomTexF = subBottomF / size.y;
      const float subTopTexF = subTopF / size.y;

      const float kVertices[] = {
        subLeftF, subTopF,     subLeftTexF, subTopTexF,    // tl
        subRightF,subBottomF,  subRightTexF,subBottomTexF, // br
        subLeftF, subBottomF,  subLeftTexF, subBottomTexF, // bl
        subRightF,subTopF,     subRightTexF,subTopTexF     // tr
        };


      // indices
      //mNumIndices = sizeof(kIndices);
      }
    //}}}
    virtual ~cDx11Quad() = default;

    //{{{
    void draw() {
      }
    //}}}
    };
  //}}}
  //{{{
  class cDx11FrameBuffer : public cFrameBuffer {
  public:
    cDx11FrameBuffer() : cFrameBuffer({0,0}) {
      mImageFormat = 0;
      mInternalFormat = 0;
      }

    cDx11FrameBuffer (cPoint size, eFormat format) : cFrameBuffer(size) {
      (void)format;
      mImageFormat = 0;
      mInternalFormat = 0;
      }

    cDx11FrameBuffer (uint8_t* pixels, cPoint size, eFormat format) : cFrameBuffer(size) {
      (void)format;
      (void)pixels;
      mImageFormat = 0;
      mInternalFormat = 0;
      }

    virtual ~cDx11FrameBuffer() {
      free (mPixels);
      }

    //{{{
    uint8_t* getPixels() {

      if (!mPixels) {
        // create mPixels, texture pixels shadow buffer
        if (kDebug)
          cLog::log (LOGINFO, fmt::format ("getPixels malloc {}", getNumPixelBytes()));
        mPixels = static_cast<uint8_t*>(malloc (getNumPixelBytes()));
        }

      else if (!mDirtyPixelsRect.isEmpty()) {
        if (kDebug)
          cLog::log (LOGINFO, fmt::format ("getPixels get {},{} {},{}",
                                      mDirtyPixelsRect.left, mDirtyPixelsRect.top,
                                      mDirtyPixelsRect.getWidth(), mDirtyPixelsRect.getHeight()));

        mDirtyPixelsRect = cRect(0,0,0,0);
        }

      return mPixels;
      }
    //}}}
    //{{{
    void setSize (cPoint size) {

      if (mFrameBufferObject == 0)
        mSize = size;
      else
        cLog::log (LOGERROR, "unimplmented setSize of non screen framebuffer");
      };
    //}}}
    //{{{
    void setTarget (const cRect& rect) {
    // set us as target, set viewport to our size, invalidate contents (probably a clear)

      // texture could be changed, add to dirtyPixelsRect
      mDirtyPixelsRect += rect;
      }
    //}}}
    //{{{
    void setBlend() {
      }
    //}}}
    //{{{
    void setSource() {

      if (mFrameBufferObject) {
        }
      else
        cLog::log (LOGERROR, "windowFrameBuffer cannot be src");
      }
    //}}}

    //{{{
    void invalidate() {
      clear (cColor (0.f,0.f,0.f, 0.f));
      }
    //}}}
    //{{{
    void pixelsChanged (const cRect& rect) {
    // pixels in rect changed, write back to texture

      if (mPixels) {
        if (kDebug)
          cLog::log (LOGINFO, fmt::format ("pixelsChanged {},{} {},{} - dirty {},{} {},{}",
                                      rect.left, rect.top, rect.getWidth(), rect.getHeight()));
        }
      }
    //}}}

    void clear (const cColor& color) { (void)color; }
    //{{{
    void blit (cFrameBuffer& src, cPoint srcPoint, const cRect& dstRect) {

      (void)src;
      mDirtyPixelsRect += dstRect;

      if (kDebug)
        cLog::log (LOGINFO, fmt::format ("blit src:{},{} dst:{},{} {},{} dirty:{},{} {},{}",
                                    srcPoint.x, srcPoint.y,
                                    dstRect.left, dstRect.top, dstRect.getWidth(), dstRect.getHeight(),
                                    mDirtyPixelsRect.left, mDirtyPixelsRect.top,
                                    mDirtyPixelsRect.getWidth(), mDirtyPixelsRect.getHeight()));
      }
    //}}}

    bool cFrameBuffer::checkStatus() { return true; }
    //{{{
    void reportInfo() {
      cLog::log (LOGINFO, fmt::format ("frameBuffer reportInfo {},{}", mSize.x, mSize.y));
      }
    //}}}
    };
  //}}}

  //{{{
  uint32_t compileShader (const string& vertShaderString, const string& fragShaderString) {
    (void)vertShaderString;
    (void)fragShaderString;
    return 0;
    }
  //}}}
  //{{{
  class cDx11LayerShader : public cLayerShader {
  public:
    cDx11LayerShader() : cLayerShader() {
      }
    virtual ~cDx11LayerShader() = default;

    // sets
    virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      (void)model;
      (void)projection;
      }
    virtual void setHueSatVal (float hue, float sat, float val) final {
      (void)hue;
      (void)sat;
      (void)val;
      }

    virtual void use() final {
      }
    };
  //}}}
  //{{{
  class cDx11PaintShader : public cPaintShader {
  public:
    cDx11PaintShader() : cPaintShader() {
      }
    virtual ~cDx11PaintShader() = default;

    // sets
    virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      (void)model;
      (void)projection;
      }
    virtual void setStroke (cVec2 pos, cVec2 prevPos, float radius, const cColor& color) final {
      (void)pos;
      (void)prevPos;
      (void)radius;
      (void)color;
      }

    virtual void use() final {
      }
    };
  //}}}

  #ifndef USE_IMPL
    // local implementation of backend impl
    //{{{
    struct sBackendData {
      ID3D11Device*             mD3dDevice;
      ID3D11DeviceContext*      mD3dDeviceContext;
      IDXGIFactory*             mDxgiFactory;

      IDXGISwapChain*           mDxgiSwapChain;
      ID3D11RenderTargetView*   mMainRenderTargetView;

      ID3D11Buffer*             mVertexBuffer;
      ID3D11Buffer*             mIndexBuffer;
      int                       mVertexBufferSize;
      int                       mIndexBufferSize;

      ID3D11VertexShader*       mVertexShader;
      ID3D11InputLayout*        mInputLayout;
      ID3D11Buffer*             mVertexConstantBuffer;
      ID3D11PixelShader*        mPixelShader;

      bool                      mFontLoaded;
      ID3D11SamplerState*       mFontSampler;
      ID3D11ShaderResourceView* mFontTextureView;

      ID3D11RasterizerState*    mRasterizerState;
      ID3D11BlendState*         mBlendState;
      ID3D11DepthStencilState*  mDepthStencilState;

      sBackendData() {
        memset (this, 0, sizeof(*this));
        mVertexBufferSize = 5000;
        mIndexBufferSize = 10000;
        mFontLoaded = false;
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
    class cDrawListShader : public cShader {
    public:
      cDrawListShader() : cShader() {
        }
      virtual ~cDrawListShader() = default;

      void setMatrix (float* matrix) {
        (void)matrix;
        }

      void use() final {
        }
      };
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
      if (backendData->mD3dDevice->CreateShaderResourceView (
        texture2d, &shaderResourceViewDesc, &backendData->mFontTextureView) != S_OK) {
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
    void setupRenderState (ImDrawData* drawData) {

      sBackendData* backendData = getBackendData();
      ID3D11DeviceContext* deviceContext = backendData->mD3dDeviceContext;

      // set shader, vertex buffers
      unsigned int stride = sizeof(ImDrawVert);
      unsigned int offset = 0;
      deviceContext->IASetInputLayout (backendData->mInputLayout);
      deviceContext->IASetVertexBuffers (0, 1, &backendData->mVertexBuffer, &stride, &offset);
      deviceContext->IASetIndexBuffer (backendData->mIndexBuffer, sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
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
        //{{{  manage,map vertex GPU buffer
        if (!backendData->mVertexBuffer || (backendData->mVertexBufferSize < drawData->TotalVtxCount)) {
          // need new vertexBuffer
          if (backendData->mVertexBuffer) {
            // release old vertexBuffer
            backendData->mVertexBuffer->Release();
            backendData->mVertexBuffer = NULL;
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
          if (backendData->mD3dDevice->CreateBuffer (&desc, NULL, &backendData->mVertexBuffer) < 0) {
            cLog::log (LOGERROR, "vertex CreateBuffer failed");
            return;
            }
          }

        // map gpu vertexBuffer
        D3D11_MAPPED_SUBRESOURCE vertexSubResource;
        if (deviceContext->Map (backendData->mVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &vertexSubResource) != S_OK) {
          cLog::log (LOGERROR, "vertexBuffer Map failed");
          return;
          }

        ImDrawVert* vertexDest = (ImDrawVert*)vertexSubResource.pData;
        //}}}
        //{{{  manage,map index GPU buffer
        if (!backendData->mIndexBuffer || (backendData->mIndexBufferSize < drawData->TotalIdxCount)) {
          // need new indexBuffer
          if (backendData->mIndexBuffer) {
            // release old indexBuffer
            backendData->mIndexBuffer->Release();
            backendData->mIndexBuffer = NULL;
            }
          backendData->mIndexBufferSize = drawData->TotalIdxCount + 10000;

          // get new indexBuffer
          D3D11_BUFFER_DESC desc;
          memset (&desc, 0, sizeof(D3D11_BUFFER_DESC));
          desc.Usage = D3D11_USAGE_DYNAMIC;
          desc.ByteWidth = backendData->mIndexBufferSize * sizeof(ImDrawIdx);
          desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
          desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
          if (backendData->mD3dDevice->CreateBuffer (&desc, NULL, &backendData->mIndexBuffer) < 0) {
            cLog::log (LOGERROR, "index CreateBuffer failed");
            return;
            }
          }

        // map gpu indexBuffer
        D3D11_MAPPED_SUBRESOURCE indexSubResource;
        if (deviceContext->Map (backendData->mIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &indexSubResource) != S_OK) {
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
        deviceContext->Unmap (backendData->mVertexBuffer, 0);
        deviceContext->Unmap (backendData->mIndexBuffer, 0);

        setupRenderState (drawData);
        //{{{  calc orthoProject matrix
        const float left = drawData->DisplayPos.x;
        const float right = drawData->DisplayPos.x + drawData->DisplaySize.x;
        const float top = drawData->DisplayPos.y;
        const float bottom = drawData->DisplayPos.y + drawData->DisplaySize.y;

        const float kOrthoMatrix[4][4] = {
           2.f/(right-left),          0.f,                       0.f,  0.f,
           0.f,                       2.f/(top-bottom),          0.f,  0.f,
           0.f,                       0.f,                       0.5f, 0.f,
           (right+left)/(left-right), (top+bottom)/(bottom-top), 0.5f, 1.f,
          };
        //}}}
        //{{{  copy kOrthoMatrix to mapped GPU vertexConstantBuffer
        D3D11_MAPPED_SUBRESOURCE mappedSubResource;

        if (deviceContext->Map (backendData->mVertexConstantBuffer,
                                0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource) != S_OK) {
          cLog::log (LOGERROR, "vertexConstant Map failed");
          return;
          }

        memcpy (mappedSubResource.pData, kOrthoMatrix, sizeof(kOrthoMatrix));
        deviceContext->Unmap (backendData->mVertexConstantBuffer, 0);
        //}}}

        // render command lists
        ImVec2 clipOffset = drawData->DisplayPos;

        int indexOffset = 0;
        int vertexOffset = 0;
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
            deviceContext->DrawIndexed (drawCmd->ElemCount,
                                        drawCmd->IdxOffset + indexOffset,
                                        drawCmd->VtxOffset + vertexOffset);
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
  #endif
  }

// cDx11Graphics class header, easy to extract header if static register not ok
//{{{
class cDx11Graphics : public cGraphics {
public:
  void shutdown() final;

  // create resources
  virtual cTexture* createTexture (cTexture::eTextureType textureType, cPoint size, uint8_t* pixels) final;

  virtual cQuad* createQuad (cPoint size) final;
  virtual cQuad* createQuad (cPoint size, const cRect& rect) final;

  virtual cFrameBuffer* createFrameBuffer() final;
  virtual cFrameBuffer* createFrameBuffer (cPoint size, cFrameBuffer::eFormat format) final;
  virtual cFrameBuffer* createFrameBuffer (uint8_t* pixels, cPoint size, cFrameBuffer::eFormat format) final;

  virtual cLayerShader* createLayerShader() final;
  virtual cPaintShader* createPaintShader() final;
  virtual cTextureShader* createTextureShader (cTexture::eTextureType textureType) final;

  virtual void background (const cPoint& size) final;

  // actions
  virtual void newFrame() final;
  virtual void drawUI (cPoint windowSize) final;
  virtual void windowResize (int width, int height) final;

protected:
  virtual bool init (cPlatform& platform) final;

private:
  // static register
  static cGraphics* create (const std::string& className);
  inline static const bool mRegistered = registerClass ("dx11", &create);
  };
//}}}

// public:
//{{{
void cDx11Graphics::shutdown() {

  #ifdef USE_IMPL
    // todo
  #else
    ImGui::DestroyPlatformWindows();

    sBackendData* backendData = getBackendData();
    if (backendData) {
      backendData->mFontSampler->Release();
      backendData->mFontTextureView->Release();
      backendData->mIndexBuffer->Release();
      backendData->mVertexBuffer->Release();
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
  #endif
  }
//}}}

// - resource creates
//{{{
cTexture* cDx11Graphics::createTexture (cTexture::eTextureType textureType, cPoint size, uint8_t* pixels) {
  return new cDx11Texture (textureType, size, pixels);
  }
//}}}

//{{{
cQuad* cDx11Graphics::createQuad (cPoint size) {
  return new cDx11Quad (size);
  }
//}}}
//{{{
cQuad* cDx11Graphics::createQuad (cPoint size, const cRect& rect) {
  return new cDx11Quad (size, rect);
  }
//}}}

//{{{
cFrameBuffer* cDx11Graphics::createFrameBuffer() {
  return new cDx11FrameBuffer();
  }
//}}}
//{{{
cFrameBuffer* cDx11Graphics::createFrameBuffer (cPoint size, cFrameBuffer::eFormat format) {
  return new cDx11FrameBuffer (size, format);
  }
//}}}
//{{{
cFrameBuffer* cDx11Graphics::createFrameBuffer (uint8_t* pixels, cPoint size, cFrameBuffer::eFormat format) {
  return new cDx11FrameBuffer (pixels, size, format);
  }
//}}}

//{{{
cTextureShader* cDx11Graphics::createTextureShader(cTexture::eTextureType textureType) {
  (void)textureType;
  return nullptr;
  }
//}}}
//{{{
cLayerShader* cDx11Graphics::createLayerShader() {
  return new cDx11LayerShader();
  }
//}}}
//{{{
cPaintShader* cDx11Graphics::createPaintShader() {
  return new cDx11PaintShader();
  }
//}}}

void cDx11Graphics::background (const cPoint& size) { (void)size; }

//{{{
void cDx11Graphics::windowResize (int width, int height) {

  //cLog::log (LOGINFO, fmt::format ("cGraphics::windowResized {}", width, height));
  sBackendData* backendData = getBackendData();
  if (backendData) {
    backendData->mMainRenderTargetView->Release();
    backendData->mDxgiSwapChain->ResizeBuffers (0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    createMainRenderTarget();
    }
  }
//}}}
//{{{
void cDx11Graphics::newFrame() {

  #ifdef USE_IMPL
    // todo
  #else
    sBackendData* backendData = getBackendData();
    if (!backendData->mFontLoaded) {
      createFontTexture();
      backendData->mFontLoaded = true;
      }
  #endif

  ImGui::NewFrame();
  }
//}}}
//{{{
void cDx11Graphics::drawUI (cPoint windowSize) {

  (void)windowSize;
  ImGui::Render();

  #ifdef USE_IMPL
    // todo
  #else
    sBackendData* backendData = getBackendData();

    // set mainRenderTarget
    backendData->mD3dDeviceContext->OMSetRenderTargets (1, &backendData->mMainRenderTargetView, NULL);

    // clear mainRenderTarget
    const float kClearColorWithAlpha[4] = { 0.25f,0.25f,0.25f, 1.f };
    backendData->mD3dDeviceContext->ClearRenderTargetView (backendData->mMainRenderTargetView, kClearColorWithAlpha);

    // really draw imGui drawList
    renderDrawData (ImGui::GetDrawData());
  #endif
  }
//}}}

// protected:
//{{{
bool cDx11Graphics::init (cPlatform& platform) {

  bool ok = false;

  #ifdef USE_IMPL
    // todo
  #else
    // allocate backendData
    sBackendData* backendData = IM_NEW (sBackendData)();

    // set backend capabilities
    ImGui::GetIO().BackendRendererUserData = (void*)backendData;
    ImGui::GetIO().BackendRendererName = "imgui_impl_dx11";
    ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    // We can create multi-viewports on the Renderer side (optional)
    ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

    // Get factory from device adpater
    ID3D11Device* d3dDevice = (ID3D11Device*)platform.getDevice();
    ID3D11DeviceContext* d3dDeviceContext = (ID3D11DeviceContext*)platform.getDeviceContext();
    IDXGISwapChain* dxgiSwapChain = (IDXGISwapChain*)platform.getSwapChain();

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
          ok &= createMainRenderTarget();
          cLog::log (LOGINFO, fmt::format ("graphics Dx11 init ok {}", ok));
          }
        dxgiAdapter->Release();
        }
      dxgiDevice->Release();
      }
  #endif

  return ok;
  }
//}}}

// private:
//{{{
cGraphics* cDx11Graphics::create (const std::string& className) {

  (void)className;
  return new cDx11Graphics();
  }
//}}}
#endif
