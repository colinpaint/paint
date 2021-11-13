// utils.cpp  - utils for libmfx test
//{{{
// Copyright (c) 2019 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//}}}
//#define D3D9
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include "utils.h"

#ifdef _WIN32
  #include <initguid.h>
  #include <intrin.h>

  #define D3DFMT_NV12 (D3DFORMAT)MAKEFOURCC('N','V','1','2')
  #define D3DFMT_YV12 (D3DFORMAT)MAKEFOURCC('Y','V','1','2')

  #ifdef D3D9
    #pragma warning(disable : 4201) // Disable annoying DX warning
    #include <d3d9.h>
    #include <dxva2api.h>
    #define DEVICE_MGR_TYPE MFX_HANDLE_DIRECT3D_DEVICE_MANAGER9
  #else
    #include <d3d11.h>
    #include <dxgi1_2.h>
    #define DEVICE_MGR_TYPE MFX_HANDLE_D3D11_DEVICE
  #endif
#else
  //{{{  vaapi headers
  #include <stdio.h>
  #include <string.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <math.h>
  #include <new>
  #include <stdlib.h>
  #include <assert.h>
  #include <sys/ioctl.h>
  #include <drm/drm.h>
  #include <drm/drm_fourcc.h>

  #include "va/va.h"
  #include "va/va_drm.h"
  //}}}
#endif

#include <string>
#include <map>
#include <array>
#include <vector>
#include <algorithm>

#include "../utils/format.h"
//}}}

#ifdef _WIN32
  //{{{  windows
  //{{{  implTypes
  const struct {
    mfxIMPL impl;       // actual implementation
    mfxU32  adapterID;  // device adapter number
    } implTypes[] = {
      {MFX_IMPL_HARDWARE, 0},
      {MFX_IMPL_HARDWARE2, 1},
      {MFX_IMPL_HARDWARE3, 2},
      {MFX_IMPL_HARDWARE4, 3}
      };
  //}}}

  std::map <mfxMemId*, mfxHDL> allocResponses;
  std::map <mfxHDL, mfxFrameAllocResponse> allocDecodeResponses;
  std::map <mfxHDL, int> allocDecodeRefCount;

  #ifdef D3D9
    //{{{  directx9
    IDirect3DDeviceManager9* pDeviceManager9 = NULL;
    IDirect3DDevice9Ex* pD3DD9 = NULL;
    IDirect3D9Ex* pD3D9 = NULL;
    HANDLE pDeviceHandle = NULL;
    IDirectXVideoAccelerationService* pDXVAServiceDec = NULL;
    IDirectXVideoAccelerationService* pDXVAServiceVPP = NULL;

    //{{{
    mfxU32 GetIntelDeviceAdapterNum (mfxSession session) {

      mfxIMPL impl;
      MFXQueryIMPL (session, &impl);
      mfxIMPL baseImpl = MFX_IMPL_BASETYPE(impl); // Extract Media SDK base implementation type

      // get corresponding adapter number
      mfxU32  adapterNum = 0;
      for (mfxU8 i = 0; i < sizeof(implTypes)/sizeof(implTypes[0]); i++) {
        if (implTypes[i].impl == baseImpl) {
          adapterNum = implTypes[i].adapterID;
          break;
          }
        }

      return adapterNum;
      }
    //}}}
    //{{{
    // Create HW device context
    mfxStatus CreateHWDevice (mfxSession session, mfxHDL* deviceHandle, HWND window) {

      // If window handle is not supplied, get window handle from coordinate 0,0
      if (window == NULL) {
        POINT point = {0, 0};
        window = WindowFromPoint(point);
        }

      HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &pD3D9);
      if (!pD3D9 || FAILED(hr))
        return MFX_ERR_DEVICE_FAILED;

      RECT rc;
      GetClientRect(window, &rc);

      D3DPRESENT_PARAMETERS D3DPP;
      memset (&D3DPP, 0, sizeof(D3DPP));
      D3DPP.Windowed = true;
      D3DPP.hDeviceWindow = window;
      D3DPP.Flags = D3DPRESENTFLAG_VIDEO;
      D3DPP.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
      D3DPP.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
      D3DPP.BackBufferCount = 1;
      D3DPP.BackBufferFormat = D3DFMT_A8R8G8B8;
      D3DPP.BackBufferWidth = rc.right - rc.left;
      D3DPP.BackBufferHeight = rc.bottom - rc.top;
      D3DPP.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
      D3DPP.SwapEffect = D3DSWAPEFFECT_DISCARD;
      hr = pD3D9->CreateDeviceEx (GetIntelDeviceAdapterNum(session),
                                  D3DDEVTYPE_HAL,
                                  window,
                                  D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
                                  &D3DPP,
                                  NULL,
                                  &pD3DD9);
      if (FAILED(hr))
        return MFX_ERR_NULL_PTR;

      hr = pD3DD9->ResetEx (&D3DPP, NULL);
      if (FAILED(hr))
        return MFX_ERR_UNDEFINED_BEHAVIOR;

      hr = pD3DD9->Clear (0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
      if (FAILED(hr))
        return MFX_ERR_UNDEFINED_BEHAVIOR;

      UINT resetToken = 0;

      hr = DXVA2CreateDirect3DDeviceManager9 (&resetToken, &pDeviceManager9);
      if (FAILED(hr))
        return MFX_ERR_NULL_PTR;

      hr = pDeviceManager9->ResetDevice (pD3DD9, resetToken);
      if (FAILED(hr))
        return MFX_ERR_UNDEFINED_BEHAVIOR;

      *deviceHandle = (mfxHDL)pDeviceManager9;

      return MFX_ERR_NONE;
      }
    //}}}
    IDirect3DDevice9Ex* GetDevice() { return pD3DD9; }
    //{{{
    // Free HW device context
    void CleanupHWDevice() {

      if (pDeviceManager9)
        pDeviceManager9->CloseDeviceHandle(pDeviceHandle);

      MSDK_SAFE_RELEASE(pDXVAServiceDec);
      MSDK_SAFE_RELEASE(pDXVAServiceVPP);
      MSDK_SAFE_RELEASE(pDeviceManager9);
      MSDK_SAFE_RELEASE(pD3DD9);
      MSDK_SAFE_RELEASE(pD3D9);
      }
    //}}}

    //{{{
    void ClearYUVSurfaceD3D (mfxMemId memId) {

      IDirect3DSurface9* surface;

      D3DSURFACE_DESC desc;
      D3DLOCKED_RECT locked;

      surface = (IDirect3DSurface9*)memId;
      surface->GetDesc(&desc);
      surface->LockRect(&locked, 0, D3DLOCK_NOSYSLOCK);

      memset((mfxU8*)locked.pBits, 100, desc.Height*locked.Pitch);   // Y plane
      memset((mfxU8*)locked.pBits + desc.Height * locked.Pitch, 50, (desc.Height*locked.Pitch)/2);   // UV plane

      surface->UnlockRect();
      }
    //}}}
    //{{{
    void ClearRGBSurfaceD3D (mfxMemId memId) {

      IDirect3DSurface9* surface;
      D3DSURFACE_DESC desc;
      D3DLOCKED_RECT locked;

      surface = (IDirect3DSurface9*)memId;
      surface->GetDesc(&desc);
      surface->LockRect(&locked, 0, D3DLOCK_NOSYSLOCK);

      memset((mfxU8*)locked.pBits, 100, desc.Height*locked.Pitch);   // RGBA
      surface->UnlockRect();
      }
    //}}}

    //{{{
    mfxStatus _simpleAlloc (mfxFrameAllocRequest* request, mfxFrameAllocResponse* response) {

      // Determine surface format (current simple implementation only supports NV12 and RGB4(32))
      D3DFORMAT format;
      if (MFX_FOURCC_NV12 == request->Info.FourCC)
        format = D3DFMT_NV12;
      else if (MFX_FOURCC_RGB4 == request->Info.FourCC)
        format = D3DFMT_A8R8G8B8;
      else if (MFX_FOURCC_YUY2 == request->Info.FourCC)
        format = D3DFMT_YUY2;
      else if (MFX_FOURCC_YV12 == request->Info.FourCC)
        format = D3DFMT_YV12;
      else
        format = (D3DFORMAT)request->Info.FourCC;

      // Determine render target
      DWORD DxvaType;
      if (MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET & request->Type)
        DxvaType = DXVA2_VideoProcessorRenderTarget;
      else
        DxvaType = DXVA2_VideoDecoderRenderTarget;

      // Force use of video processor if color conversion is required (with this simple set of samples we only illustrate RGB4 color converison via VPP)
      if (D3DFMT_A8R8G8B8 == format) // must use processor service
        DxvaType = DXVA2_VideoProcessorRenderTarget;

      mfxMemId* mids = NULL;
      mids = new mfxMemId[request->NumFrameSuggested];
      if (!mids)
        return MFX_ERR_MEMORY_ALLOC;

      HRESULT hr = S_OK;
      if (!pDeviceHandle) {
        hr = pDeviceManager9->OpenDeviceHandle(&pDeviceHandle);
        if (FAILED(hr))
          return MFX_ERR_MEMORY_ALLOC;
        }

      IDirectXVideoAccelerationService* pDXVAServiceTmp = NULL;

      if (DXVA2_VideoDecoderRenderTarget == DxvaType) { // for both decode and encode
        if (pDXVAServiceDec == NULL)
          hr = pDeviceManager9->GetVideoService (pDeviceHandle, IID_IDirectXVideoDecoderService, (void**)&pDXVAServiceDec);
        pDXVAServiceTmp = pDXVAServiceDec;
        }
      else { // DXVA2_VideoProcessorRenderTarget ; for VPP
        if (pDXVAServiceVPP == NULL)
          hr = pDeviceManager9->GetVideoService (pDeviceHandle, IID_IDirectXVideoProcessorService, (void**)&pDXVAServiceVPP);
        pDXVAServiceTmp = pDXVAServiceVPP;
        }
      if (FAILED(hr))
        return MFX_ERR_MEMORY_ALLOC;

      // Allocate surfaces
      hr = pDXVAServiceTmp->CreateSurface (request->Info.Width, request->Info.Height,
                                           request->NumFrameSuggested - 1,
                                           format,
                                           D3DPOOL_DEFAULT,
                                           0,
                                           DxvaType,
                                           (IDirect3DSurface9**)mids,
                                           NULL);
      if (FAILED(hr))
        return MFX_ERR_MEMORY_ALLOC;

      response->mids = mids;
      response->NumFrameActual = request->NumFrameSuggested;

      return MFX_ERR_NONE;
      }
    //}}}
    //{{{
    mfxStatus _simpleFree (mfxFrameAllocResponse* response) {

      if (response->mids) {
        for (mfxU32 i = 0; i < response->NumFrameActual; i++) {
          if (response->mids[i]) {
            IDirect3DSurface9* handle = (IDirect3DSurface9*)response->mids[i];
            handle->Release();
            }
          }
        }

      delete [] response->mids;
      response->mids = 0;

      return MFX_ERR_NONE;
      }
    //}}}

    //{{{
    mfxStatus simpleLock (mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr) {

      pthis; // To suppress warning for this unused parameter

      IDirect3DSurface9* surface = (IDirect3DSurface9*)mid;

      if (surface == 0)
        return MFX_ERR_INVALID_HANDLE;
      if (ptr == 0)
        return MFX_ERR_LOCK_MEMORY;

      D3DSURFACE_DESC desc;
      HRESULT hr = surface->GetDesc (&desc);
      if (FAILED(hr))
        return MFX_ERR_LOCK_MEMORY;

      D3DLOCKED_RECT locked;
      hr = surface->LockRect (&locked, 0, D3DLOCK_NOSYSLOCK);
      if (FAILED(hr))
        return MFX_ERR_LOCK_MEMORY;

      // In these simple set of samples we only illustrate usage of NV12 and RGB4(32)
      if (D3DFMT_NV12 == desc.Format) {
        ptr->Pitch  = (mfxU16)locked.Pitch;
        ptr->Y      = (mfxU8*)locked.pBits;
        ptr->U      = (mfxU8*)locked.pBits + desc.Height * locked.Pitch;
        ptr->V      = ptr->U + 1;
        }
      else if (D3DFMT_A8R8G8B8 == desc.Format) {
        ptr->Pitch = (mfxU16)locked.Pitch;
        ptr->B = (mfxU8*)locked.pBits;
        ptr->G = ptr->B + 1;
        ptr->R = ptr->B + 2;
        ptr->A = ptr->B + 3;
        }
      else if (D3DFMT_YUY2 == desc.Format) {
        ptr->Pitch = (mfxU16)locked.Pitch;
        ptr->Y = (mfxU8*)locked.pBits;
        ptr->U = ptr->Y + 1;
        ptr->V = ptr->Y + 3;
        }
      else if (D3DFMT_YV12 == desc.Format) {
        ptr->Pitch = (mfxU16)locked.Pitch;
        ptr->Y = (mfxU8*)locked.pBits;
        ptr->V = ptr->Y + desc.Height * locked.Pitch;
        ptr->U = ptr->V + (desc.Height * locked.Pitch) / 4;
        }
      else if (D3DFMT_P8 == desc.Format) {
        ptr->Pitch = (mfxU16)locked.Pitch;
        ptr->Y = (mfxU8*)locked.pBits;
        ptr->U = 0;
        ptr->V = 0;
        }

      return MFX_ERR_NONE;
      }
    //}}}
    //{{{
    mfxStatus simpleUnlock (mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr) {

      pthis; // To suppress warning for this unused parameter

      IDirect3DSurface9* surface = (IDirect3DSurface9*)mid;
      if (surface == 0)
        return MFX_ERR_INVALID_HANDLE;

      surface->UnlockRect();

      if (NULL != ptr) {
        ptr->Pitch = 0;
        ptr->R     = 0;
        ptr->G     = 0;
        ptr->B     = 0;
        ptr->A     = 0;
        }

      return MFX_ERR_NONE;
      }
    //}}}

    //{{{
    mfxStatus simpleGethdl (mfxHDL pthis, mfxMemId mid, mfxHDL* handle) {

      pthis; // To suppress warning for this unused parameter

      if (handle == 0)
        return MFX_ERR_INVALID_HANDLE;
      *handle = mid;

      return MFX_ERR_NONE;
      }
    //}}}
    //}}}
  #else
    //{{{  directx11
    //{{{  CustomMemId
    typedef struct {
      mfxMemId    memId;
      mfxMemId    memIdStage;
      mfxU16      rw;
      } CustomMemId;
    //}}}
    ID3D11Device* D3D11Device;
    ID3D11DeviceContext* D3D11Ctx;

    //{{{
    mfxStatus CreateHWDevice (mfxSession session, mfxHDL* deviceHandle, HWND /*hWnd*/) {

      // get adapter
      mfxIMPL impl;
      MFXQueryIMPL (session, &impl);
      mfxIMPL baseImpl = MFX_IMPL_BASETYPE (impl); // Extract Media SDK base implementation type

      // get corresponding adapter number
      mfxU32 adapterNum = 0;
      for (mfxU8 i = 0; i < sizeof(implTypes)/sizeof(implTypes[0]); i++) {
        if (implTypes[i].impl == baseImpl) {
          adapterNum = implTypes[i].adapterID;
          break;
          }
        }

      // get DXGI factory
      IDXGIFactory2* DXGIFactory = nullptr;
      if (FAILED (CreateDXGIFactory (__uuidof(IDXGIFactory2), (void**)(&DXGIFactory))))
        return MFX_ERR_DEVICE_FAILED;

      // get adapter
      IDXGIAdapter* adapter = nullptr;
      if (FAILED (DXGIFactory->EnumAdapters (adapterNum, &adapter)))
        return MFX_ERR_DEVICE_FAILED;
      DXGIFactory->Release();

      // Window handle not required by DX11 since we do not showcase rendering.
      UINT dxFlags = 0; // D3D11_CREATE_DEVICE_DEBUG;
      static D3D_FEATURE_LEVEL FeatureLevels[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
      D3D_FEATURE_LEVEL featureLevelsOut;
      if (FAILED (D3D11CreateDevice (adapter,
                                     D3D_DRIVER_TYPE_UNKNOWN, NULL, dxFlags,
                                     FeatureLevels, (sizeof(FeatureLevels) / sizeof(FeatureLevels[0])),
                                     D3D11_SDK_VERSION, &D3D11Device, &featureLevelsOut, &D3D11Ctx)))
        return MFX_ERR_DEVICE_FAILED;
      adapter->Release();

      // set multiThreaded
      ID3D10Multithread* multithread = nullptr;
      if (FAILED (D3D11Device->QueryInterface (IID_PPV_ARGS (&multithread))))
        return MFX_ERR_DEVICE_FAILED;
      multithread->SetMultithreadProtected (true);
      multithread->Release();

      *deviceHandle = (mfxHDL)D3D11Device;
      return MFX_ERR_NONE;
      }
    //}}}
    //{{{
    void CleanupHWDevice() {
      D3D11Device->Release();
      D3D11Ctx->Release();
      }
    //}}}
    ID3D11DeviceContext* GetHWDeviceContext() { return D3D11Ctx; }

    void ClearYUVSurfaceD3D (mfxMemId /*memId*/) {}// TBD
    void ClearRGBSurfaceD3D (mfxMemId /*memId*/) {} // TBD

    //{{{
    mfxStatus _simpleAlloc (mfxFrameAllocRequest* request, mfxFrameAllocResponse* response) {

      // Determine surface format
      DXGI_FORMAT format;
      if (MFX_FOURCC_NV12 == request->Info.FourCC)
        format = DXGI_FORMAT_NV12;
      else if (MFX_FOURCC_RGB4 == request->Info.FourCC)
        format = DXGI_FORMAT_B8G8R8A8_UNORM;
      else if (MFX_FOURCC_YUY2== request->Info.FourCC)
        format = DXGI_FORMAT_YUY2;
      else if (MFX_FOURCC_P8 == request->Info.FourCC ) //|| MFX_FOURCC_P8_TEXTURE == request->Info.FourCC
        format = DXGI_FORMAT_P8;
      else
        return MFX_ERR_UNSUPPORTED;

      // Allocate custom container to keep texture and stage buffers for each surface
      // Container also stores the intended read and/or write operation.
      CustomMemId** mids = (CustomMemId**)calloc (request->NumFrameSuggested, sizeof(CustomMemId*));
      if (!mids)
        return MFX_ERR_MEMORY_ALLOC;

      for (int i = 0; i < request->NumFrameSuggested; i++) {
        mids[i] = (CustomMemId*)calloc (1, sizeof(CustomMemId));
        if (!mids[i])
          return MFX_ERR_MEMORY_ALLOC;
        mids[i]->rw = request->Type & 0xF000; // Set intended read/write operation
        }
      request->Type = request->Type & 0x0FFF;

      // because P8 data (bitstream) for h264 encoder should be allocated by CreateBuffer()
      // but P8 data (MBData) for MPEG2 encoder should be allocated by CreateTexture2D()
      if (request->Info.FourCC == MFX_FOURCC_P8) {
        D3D11_BUFFER_DESC desc = {0};
        if (!request->NumFrameSuggested)
          return MFX_ERR_MEMORY_ALLOC;
        desc.ByteWidth           = request->Info.Width * request->Info.Height;
        desc.Usage               = D3D11_USAGE_STAGING;
        desc.BindFlags           = 0;
        desc.CPUAccessFlags      = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags           = 0;
        desc.StructureByteStride = 0;

        ID3D11Buffer* buffer = 0;
        if (FAILED (D3D11Device->CreateBuffer(&desc, 0, &buffer)))
          return MFX_ERR_MEMORY_ALLOC;
        mids[0]->memId = reinterpret_cast<ID3D11Texture2D*>(buffer);
        }
      else {
        D3D11_TEXTURE2D_DESC desc = {0};
        desc.Width            = request->Info.Width;
        desc.Height           = request->Info.Height;
        desc.MipLevels        = 1;
        desc.ArraySize        = 1; // number of subresources is 1 in this case
        desc.Format           = format;
        desc.SampleDesc.Count = 1;
        desc.Usage            = D3D11_USAGE_DEFAULT;
        desc.BindFlags        = D3D11_BIND_DECODER;
        desc.MiscFlags        = 0;
        //desc.MiscFlags        = D3D11_RESOURCE_MISC_SHARED;

        if ((MFX_MEMTYPE_FROM_VPPIN & request->Type) &&
            (DXGI_FORMAT_B8G8R8A8_UNORM == desc.Format)) {
          desc.BindFlags = D3D11_BIND_RENDER_TARGET;
          if (desc.ArraySize > 2)
            return MFX_ERR_MEMORY_ALLOC;
          }

        if ((MFX_MEMTYPE_FROM_VPPOUT & request->Type) ||
            (MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET & request->Type)) {
          desc.BindFlags = D3D11_BIND_RENDER_TARGET;
          if (desc.ArraySize > 2)
            return MFX_ERR_MEMORY_ALLOC;
          }

        if (DXGI_FORMAT_P8 == desc.Format)
          desc.BindFlags = 0;

        // Create surface textures
        ID3D11Texture2D* texture2D;
        for (size_t i = 0; i < request->NumFrameSuggested / desc.ArraySize; i++) {
          if (FAILED (D3D11Device->CreateTexture2D (&desc, NULL, &texture2D)))
            return MFX_ERR_MEMORY_ALLOC;
          mids[i]->memId = texture2D;
          }

        desc.ArraySize = 1;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ; // | D3D11_CPU_ACCESS_WRITE;
        desc.BindFlags = 0;
        desc.MiscFlags = 0;
        //desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

        // Create surface staging textures
        for (size_t i = 0; i < request->NumFrameSuggested; i++) {
          if (FAILED (D3D11Device->CreateTexture2D (&desc, NULL, &texture2D)))
            return MFX_ERR_MEMORY_ALLOC;
          mids[i]->memIdStage = texture2D;
          }
        }

      response->mids = (mfxMemId*)mids;
      response->NumFrameActual = request->NumFrameSuggested;
      return MFX_ERR_NONE;
      }
    //}}}
    //{{{
    mfxStatus _simpleFree (mfxFrameAllocResponse* response) {

      if (response->mids) {
        for (mfxU32 i = 0; i < response->NumFrameActual; i++) {
          if (response->mids[i]) {
            CustomMemId* mid = (CustomMemId*)response->mids[i];

            ID3D11Texture2D* surface = (ID3D11Texture2D*)mid->memId;
            if (surface)
              surface->Release();

            ID3D11Texture2D* stage = (ID3D11Texture2D*)mid->memIdStage;
            if (stage)
              stage->Release();
            free (mid);
            }
          }

        free (response->mids);
        response->mids = NULL;
        }

      return MFX_ERR_NONE;
      }
    //}}}
    //{{{
    mfxStatus simpleLock (mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr) {

      pthis; // To suppress warning for this unused parameter

      HRESULT hRes = S_OK;

      D3D11_TEXTURE2D_DESC desc = {0};
      D3D11_MAPPED_SUBRESOURCE lockedRect = {0};
      D3D11_MAP mapType  = D3D11_MAP_READ;
      UINT mapFlags = D3D11_MAP_FLAG_DO_NOT_WAIT;
      CustomMemId* memId = (CustomMemId*)mid;
      ID3D11Texture2D* surface = (ID3D11Texture2D*)memId->memId;
      ID3D11Texture2D* stage = (ID3D11Texture2D*)memId->memIdStage;
      if (NULL == stage) {
        hRes = D3D11Ctx->Map (surface, 0, mapType, mapFlags, &lockedRect);
        desc.Format = DXGI_FORMAT_P8;
        }
      else {
        surface->GetDesc (&desc);

       // copy data only in case of user wants to read from stored surface
        if (memId->rw & WILL_READ)
          D3D11Ctx->CopySubresourceRegion (stage, 0, 0, 0, 0, surface, 0, NULL);

        do {
          hRes = D3D11Ctx->Map (stage, 0, mapType, mapFlags, &lockedRect);
          if (S_OK != hRes && DXGI_ERROR_WAS_STILL_DRAWING != hRes)
            return MFX_ERR_LOCK_MEMORY;
          } while (DXGI_ERROR_WAS_STILL_DRAWING == hRes);
        }

      if (FAILED (hRes))
        return MFX_ERR_LOCK_MEMORY;

      switch (desc.Format) {
        //{{{
        case DXGI_FORMAT_NV12:
          ptr->Pitch = (mfxU16)lockedRect.RowPitch;
          ptr->Y = (mfxU8*)lockedRect.pData;
          ptr->U = (mfxU8*)lockedRect.pData + desc.Height * lockedRect.RowPitch;
          ptr->V = ptr->U + 1;
          break;
        //}}}
        //{{{
        case DXGI_FORMAT_B8G8R8A8_UNORM :
          ptr->Pitch = (mfxU16)lockedRect.RowPitch;
          ptr->B = (mfxU8*)lockedRect.pData;
          ptr->G = ptr->B + 1;
          ptr->R = ptr->B + 2;
          ptr->A = ptr->B + 3;
          break;
        //}}}
        //{{{
        case DXGI_FORMAT_YUY2:
          ptr->Pitch = (mfxU16)lockedRect.RowPitch;
          ptr->Y = (mfxU8*)lockedRect.pData;
          ptr->U = ptr->Y + 1;
          ptr->V = ptr->Y + 3;
          break;
        //}}}
        //{{{
        case DXGI_FORMAT_P8 :
          ptr->Pitch = (mfxU16)lockedRect.RowPitch;
          ptr->Y = (mfxU8*)lockedRect.pData;
          ptr->U = 0;
          ptr->V = 0;
          break;
        //}}}
        //{{{
        default:
          return MFX_ERR_LOCK_MEMORY;
        //}}}
        }

      return MFX_ERR_NONE;
      }
    //}}}
    //{{{
    mfxStatus simpleUnlock (mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr) {

      pthis; // To suppress warning for this unused parameter

      CustomMemId* memId = (CustomMemId*)mid;
      ID3D11Texture2D* surface = (ID3D11Texture2D*)memId->memId;
      ID3D11Texture2D* stage = (ID3D11Texture2D*)memId->memIdStage;
      if (NULL == stage)
        D3D11Ctx->Unmap (surface, 0);
      else {
        D3D11Ctx->Unmap (stage, 0);
        // copy data only in case of user wants to write to stored surface
        if (memId->rw & WILL_WRITE)
          D3D11Ctx->CopySubresourceRegion (surface, 0, 0, 0, 0, stage, 0, NULL);
        }

      if (ptr) {
        ptr->Pitch = 0;
        ptr->Y = 0;
        ptr->U = 0;
        ptr->V = 0;
        ptr->A = 0;
        ptr->R = 0;
        ptr->G = 0;
        ptr->B = 0;
        }

      return MFX_ERR_NONE;
      }
    //}}}
    //{{{
    mfxStatus simpleGethdl (mfxHDL pthis, mfxMemId mid, mfxHDL* handle) {

      pthis; // To suppress warning for this unused parameter

      if (NULL == handle)
        return MFX_ERR_INVALID_HANDLE;

      CustomMemId* memId = (CustomMemId*)mid;
      mfxHDLPair* pPair = (mfxHDLPair*)handle;
      pPair->first  = memId->memId; // surface texture
      pPair->second = 0;

      return MFX_ERR_NONE;
      }
    //}}}
    //}}}
  #endif

  //{{{
  mfxStatus simpleAlloc (mfxHDL pthis, mfxFrameAllocRequest* request, mfxFrameAllocResponse* response) {

    mfxStatus status = MFX_ERR_NONE;

    if (request->Type & MFX_MEMTYPE_SYSTEM_MEMORY)
      return MFX_ERR_UNSUPPORTED;

    if (allocDecodeResponses.find(pthis) != allocDecodeResponses.end() &&
        (MFX_MEMTYPE_EXTERNAL_FRAME & request->Type) && (MFX_MEMTYPE_FROM_DECODE & request->Type)) {
      // Memory for this request was already allocated during manual allocation stage. Return saved response
      // When decode acceleration device (DXVA) is created it requires a list of D3D surfaces to be passed.
      // Therefore Media SDK will ask for the surface info/mids again at Init() stage, thus requiring us to return the saved response
      // (No such restriction applies to Encode or VPP)
      *response = allocDecodeResponses[pthis];
      allocDecodeRefCount[pthis]++;
      }
    else {
      status = _simpleAlloc (request, response);

      if (MFX_ERR_NONE == status) {
        if ((MFX_MEMTYPE_EXTERNAL_FRAME & request->Type) && (MFX_MEMTYPE_FROM_DECODE & request->Type)) {
          // Decode alloc response handling
          allocDecodeResponses[pthis] = *response;
          allocDecodeRefCount[pthis]++;
          }
        else
          // Encode and VPP alloc response handling
          allocResponses[response->mids] = pthis;
        }
      }

    return status;
    }
  //}}}
  //{{{
  mfxStatus simpleFree (mfxHDL pthis, mfxFrameAllocResponse* response) {

    if (!response)
      return MFX_ERR_NULL_PTR;

    if (allocResponses.find(response->mids) == allocResponses.end()) {
      // Decode free response handling
      if (--allocDecodeRefCount[pthis] == 0) {
        _simpleFree (response);
        allocDecodeResponses.erase (pthis);
        allocDecodeRefCount.erase (pthis);
        }
      }
    else {
      // Encode and VPP free response handling
      allocResponses.erase (response->mids);
      _simpleFree (response);
      }

    return MFX_ERR_NONE;
    }
  //}}}

  //{{{
  mfxStatus Initialize (MFXVideoSession* session, mfxFrameAllocator* mfxAllocator) {

    mfxStatus status = MFX_ERR_NONE;

    // create DirectX device context
    mfxHDL deviceHandle;
    status = CreateHWDevice (*session, &deviceHandle, NULL);
    MSDK_CHECK_RESULT (status, MFX_ERR_NONE, status);

    // Provide device manager to Media SDK
    status = session->SetHandle (DEVICE_MGR_TYPE, deviceHandle);
    MSDK_CHECK_RESULT (status, MFX_ERR_NONE, status);

    // If mfxFrameAllocator is provided it means we need to setup DirectX device and memory allocator
    if (mfxAllocator) {
      mfxAllocator->pthis = *session; // We use Media SDK session ID as the allocation identifier
      mfxAllocator->Alloc = simpleAlloc;
      mfxAllocator->Free = simpleFree;
      mfxAllocator->Lock = simpleLock;
      mfxAllocator->Unlock = simpleUnlock;
      mfxAllocator->GetHDL = simpleGethdl;

      // Since we are using video memory we must provide Media SDK with an external allocator
      status = session->SetFrameAllocator (mfxAllocator);
      MSDK_CHECK_RESULT (status, MFX_ERR_NONE, status);
      }

    return status;
    }
  //}}}
  void Release() { CleanupHWDevice(); }
  void ClearYUVSurfaceVMem (mfxMemId memId) { ClearYUVSurfaceD3D (memId); }
  void ClearRGBSurfaceVMem (mfxMemId memId) { ClearRGBSurfaceD3D (memId); }
  //}}}
#else
  //{{{  vaapi
  //{{{
  struct sharedResponse {
    mfxFrameAllocResponse mfxResponse;
    int refCount;
    };
  //}}}
  //{{{
  // VAAPI Allocator internal Mem ID
  struct vaapiMemId {
    VASurfaceID* m_surface;
    VAImage m_image;

    // variables for VAAPI Allocator inernal color convertion
    unsigned int m_fourcc;
    mfxU8* m_sys_buffer;
    mfxU8* m_va_buffer;
    };
  //}}}

  std::map<mfxMemId*, mfxHDL> allocResponses;
  std::map<mfxHDL, sharedResponse> allocDecodeResponses;

  // VAAPI display handle
  VADisplay m_va_dpy = NULL;

  // gfx card file descriptor
  int m_fd = -1;

  constexpr uint32_t DRI_MAX_NODES_NUM = 16;
  constexpr uint32_t DRI_RENDER_START_INDEX = 128;
  constexpr  uint32_t DRM_DRIVER_NAME_LEN = 4;
  const char* DRM_INTEL_DRIVER_NAME = "i915";
  const char* DRI_PATH = "/dev/dri/";
  const char* DRI_NODE_RENDER = "renderD";

  //{{{
  mfxStatus va_to_mfx_status (VAStatus va_res) {

    mfxStatus mfxRes = MFX_ERR_NONE;

    switch (va_res) {
      case VA_STATUS_SUCCESS:
        mfxRes = MFX_ERR_NONE;
        break;

      case VA_STATUS_ERROR_ALLOCATION_FAILED:
        mfxRes = MFX_ERR_MEMORY_ALLOC;
        break;

      case VA_STATUS_ERROR_ATTR_NOT_SUPPORTED:
      case VA_STATUS_ERROR_UNSUPPORTED_PROFILE:
      case VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT:
      case VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT:
      case VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE:
      case VA_STATUS_ERROR_FLAG_NOT_SUPPORTED:
      case VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED:
        mfxRes = MFX_ERR_UNSUPPORTED;
        break;

      case VA_STATUS_ERROR_INVALID_DISPLAY:
      case VA_STATUS_ERROR_INVALID_CONFIG:
      case VA_STATUS_ERROR_INVALID_CONTEXT:
      case VA_STATUS_ERROR_INVALID_SURFACE:
      case VA_STATUS_ERROR_INVALID_BUFFER:
      case VA_STATUS_ERROR_INVALID_IMAGE:
      case VA_STATUS_ERROR_INVALID_SUBPICTURE:
        mfxRes = MFX_ERR_NOT_INITIALIZED;
        break;

      case VA_STATUS_ERROR_INVALID_PARAMETER:
        mfxRes = MFX_ERR_INVALID_VIDEO_PARAM;
        break; // ???? fell though ????

      default:
        mfxRes = MFX_ERR_UNKNOWN;
        break;
      }

    return mfxRes;
    }
  //}}}
  //{{{
  //utiility function to convert MFX fourcc to VA format
  unsigned int ConvertMfxFourccToVAFormat (mfxU32 fourcc) {

    switch (fourcc) {
      case MFX_FOURCC_NV12:
        return VA_FOURCC_NV12;
      case MFX_FOURCC_YUY2:
        return VA_FOURCC_YUY2;
      case MFX_FOURCC_YV12:
        return VA_FOURCC_YV12;
      case MFX_FOURCC_RGB4:
        return VA_FOURCC_ARGB;
      case MFX_FOURCC_P8:
        return VA_FOURCC_P208;

      default:
        assert(!"unsupported fourcc");
        return 0;
      }
    }
  //}}}

  //{{{
  int get_drm_driver_name (int fd, char* name, int name_size) {

    drm_version_t version = {};
    version.name_len = name_size;
    version.name = name;
    return ioctl (fd, DRM_IOWR(0, drm_version), &version);
    }
  //}}}
  //{{{
  int open_intel_adapter() {

    std::string adapterPath = DRI_PATH;
    adapterPath += DRI_NODE_RENDER;

    char driverName[DRM_DRIVER_NAME_LEN + 1] = {};
    mfxU32 nodeIndex = DRI_RENDER_START_INDEX;

    for (mfxU32 i = 0; i < DRI_MAX_NODES_NUM; ++i) {
      std::string curAdapterPath = adapterPath + std::to_string(nodeIndex + i);

      int fd = open (curAdapterPath.c_str(), O_RDWR);
      if (fd < 0) continue;
      if (!get_drm_driver_name (fd, driverName, DRM_DRIVER_NAME_LEN) &&
          !strcmp (driverName, DRM_INTEL_DRIVER_NAME)) {
        return fd;
        }
      close(fd);
      }

    return -1;
    }
  //}}}
  //{{{
  mfxStatus CreateVAEnvDRM (mfxHDL* displayHandle) {

    VAStatus va_res = VA_STATUS_SUCCESS;
    mfxStatus status = MFX_ERR_NONE;
    int major_version = 0, minor_version = 0;

    m_fd = open_intel_adapter();
    if (m_fd < 0)
      status = MFX_ERR_NOT_INITIALIZED;

    if (MFX_ERR_NONE == status) {
      m_va_dpy = vaGetDisplayDRM(m_fd);

      *displayHandle = m_va_dpy;

      if (!m_va_dpy) {
        close(m_fd);
        status = MFX_ERR_NULL_PTR;
        }
      }

    if (MFX_ERR_NONE == status) {
      va_res = vaInitialize (m_va_dpy, &major_version, &minor_version);
      status = va_to_mfx_status (va_res);
      if (MFX_ERR_NONE != status) {
        close (m_fd);
        m_fd = -1;
        }
      }

    if (MFX_ERR_NONE != status)
      throw std::bad_alloc();

    return MFX_ERR_NONE;
    }
  //}}}
  //{{{
  void CleanupVAEnvDRM() {

    if (m_va_dpy) {
      vaTerminate(m_va_dpy);
      }

    if (m_fd >= 0) {
      close(m_fd);
      }
    }
  //}}}

  //{{{
  mfxStatus _simpleAlloc (mfxFrameAllocRequest* request, mfxFrameAllocResponse* response) {

    mfxStatus mfx_res = MFX_ERR_NONE;
    VAStatus va_res = VA_STATUS_SUCCESS;
    unsigned int va_fourcc = 0;
    VASurfaceID* surfaces = NULL;
    VASurfaceAttrib attrib;
    vaapiMemId* vaapi_mids = NULL, *vaapi_mid = NULL;
    mfxMemId* mids = NULL;
    mfxU32 fourcc = request->Info.FourCC;
    mfxU16 surfaces_num = request->NumFrameSuggested, numAllocated = 0, i = 0;
    bool bCreateSrfSucceeded = false;

    memset (response, 0, sizeof(mfxFrameAllocResponse));

    va_fourcc = ConvertMfxFourccToVAFormat(fourcc);
    if (!va_fourcc || ((VA_FOURCC_NV12 != va_fourcc) &&
                       (VA_FOURCC_YV12 != va_fourcc) &&
                       (VA_FOURCC_YUY2 != va_fourcc) &&
                       (VA_FOURCC_ARGB != va_fourcc) &&
                       (VA_FOURCC_P208 != va_fourcc))) {
      return MFX_ERR_MEMORY_ALLOC;
      }
    if (!surfaces_num) {
      return MFX_ERR_MEMORY_ALLOC;
      }

    if (MFX_ERR_NONE == mfx_res) {
      surfaces = (VASurfaceID*)calloc (surfaces_num, sizeof(VASurfaceID));
      vaapi_mids = (vaapiMemId*)calloc (surfaces_num, sizeof(vaapiMemId));
      mids = (mfxMemId*)calloc (surfaces_num, sizeof(mfxMemId));
      if ((NULL == surfaces) || (NULL == vaapi_mids) || (NULL == mids))
        mfx_res = MFX_ERR_MEMORY_ALLOC;
      }

    if (MFX_ERR_NONE == mfx_res) {
      if (VA_FOURCC_P208 != va_fourcc) {
        attrib.type = VASurfaceAttribPixelFormat;
        attrib.value.type = VAGenericValueTypeInteger;
        attrib.value.value.i = va_fourcc;
        attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;

        va_res = vaCreateSurfaces (m_va_dpy,
                                   VA_RT_FORMAT_YUV420,
                                   request->Info.Width,
                                   request->Info.Height,
                                   surfaces, surfaces_num,
                                   &attrib, 1);
        mfx_res = va_to_mfx_status (va_res);
        bCreateSrfSucceeded = (MFX_ERR_NONE == mfx_res);
        }
      else {
        VAContextID context_id = request->reserved[0];
        int codedbuf_size = (request->Info.Width * request->Info.Height) * 400 / (16 * 16);     //from libva spec

        for (numAllocated = 0; numAllocated < surfaces_num; numAllocated++) {
          VABufferID coded_buf;
          va_res = vaCreateBuffer (m_va_dpy, context_id, VAEncCodedBufferType,
                                   codedbuf_size, 1, NULL, &coded_buf);
          mfx_res = va_to_mfx_status (va_res);
          if (MFX_ERR_NONE != mfx_res)
            break;
          surfaces[numAllocated] = coded_buf;
          }
        }
      }

    if (MFX_ERR_NONE == mfx_res) {
      for (i = 0; i < surfaces_num; ++i) {
        vaapi_mid = &(vaapi_mids[i]);
        vaapi_mid->m_fourcc = fourcc;
        vaapi_mid->m_surface = &(surfaces[i]);
        mids[i] = vaapi_mid;
        }
      }

    if (MFX_ERR_NONE == mfx_res) {
      response->mids = mids;
      response->NumFrameActual = surfaces_num;
      }
    else {                // i.e. MFX_ERR_NONE != mfx_res
      response->mids = NULL;
      response->NumFrameActual = 0;
      if (VA_FOURCC_P208 != va_fourcc) {
        if (bCreateSrfSucceeded)
          vaDestroySurfaces(m_va_dpy, surfaces, surfaces_num);
        }
      else {
        for (i = 0; i < numAllocated; i++)
          vaDestroyBuffer(m_va_dpy, surfaces[i]);
        }
      if (mids) {
        free(mids);
        mids = NULL;
        }
      if (vaapi_mids) {
        free(vaapi_mids);
        vaapi_mids = NULL;
        }
      if (surfaces) {
        free(surfaces);
        surfaces = NULL;
        }
      }
    return mfx_res;
    }
  //}}}
  //{{{
  mfxStatus _simpleFree (mfxHDL pthis, mfxFrameAllocResponse* response) {

    vaapiMemId* vaapi_mids = NULL;
    VASurfaceID* surfaces = NULL;
    mfxU32 i = 0;

    bool isBitstreamMemory = false;
    bool actualFreeMemory = false;

    if (0 == memcmp(response, &(allocDecodeResponses[pthis].mfxResponse), sizeof(*response))) {
      // Decode free response handling
      allocDecodeResponses[pthis].refCount--;
      if (0 == allocDecodeResponses[pthis].refCount)
        actualFreeMemory = true;
      }
    else {
      // Encode and VPP free response handling
      actualFreeMemory = true;
      }

    if (actualFreeMemory) {
      if (response->mids) {
        vaapi_mids = (vaapiMemId*) (response->mids[0]);

        isBitstreamMemory = (MFX_FOURCC_P8 == vaapi_mids->m_fourcc) ? true : false;
        surfaces = vaapi_mids->m_surface;
        for (i = 0; i < response->NumFrameActual; ++i) {
          if (MFX_FOURCC_P8 == vaapi_mids[i].m_fourcc)
            vaDestroyBuffer(m_va_dpy, surfaces[i]);
          else if (vaapi_mids[i].m_sys_buffer)
            free(vaapi_mids[i].m_sys_buffer);
          }

        free(vaapi_mids);
        free(response->mids);
        response->mids = NULL;
        if (!isBitstreamMemory)
          vaDestroySurfaces(m_va_dpy, surfaces, response->NumFrameActual);
        free(surfaces);
        }
      response->NumFrameActual = 0;
      }

    return MFX_ERR_NONE;
    }
  //}}}

  //{{{
  mfxStatus simpleAlloc (mfxHDL pthis, mfxFrameAllocRequest* request, mfxFrameAllocResponse* response) {

    mfxStatus status = MFX_ERR_NONE;

    if (0 == request || 0 == response || 0 == request->NumFrameSuggested)
      return MFX_ERR_MEMORY_ALLOC;

    if ((request->Type & (MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET |
                          MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET)) == 0)
      return MFX_ERR_UNSUPPORTED;

    if (request->NumFrameSuggested <= allocDecodeResponses[pthis].mfxResponse.NumFrameActual
        && MFX_MEMTYPE_EXTERNAL_FRAME & request->Type
        && MFX_MEMTYPE_FROM_DECODE & request->Type
        &&allocDecodeResponses[pthis].mfxResponse.NumFrameActual != 0) {
      // Memory for this request was already allocated during manual allocation stage. Return saved response
      //   When decode acceleration device (VAAPI) is created it requires a list of VAAPI surfaces to be passed.
      //   Therefore Media SDK will ask for the surface info/mids again at Init() stage, thus requiring us to return the saved response
      //   (No such restriction applies to Encode or VPP)
      *response = allocDecodeResponses[pthis].mfxResponse;
      allocDecodeResponses[pthis].refCount++;
      }
    else {
      status = _simpleAlloc (request, response);
      if (MFX_ERR_NONE == status) {
        if (MFX_MEMTYPE_EXTERNAL_FRAME & request->Type &&
            MFX_MEMTYPE_FROM_DECODE & request->Type) {
          // Decode alloc response handling
          allocDecodeResponses[pthis].mfxResponse = *response;
          allocDecodeResponses[pthis].refCount++;
          //allocDecodeRefCount[pthis]++;
          }
        else {
          // Encode and VPP alloc response handling
          allocResponses[response->mids] = pthis;
          }
        }
      }

    return status;
    }
  //}}}
  //{{{
  mfxStatus simpleFree (mfxHDL pthis, mfxFrameAllocResponse* response) {

    if (!response)
      return MFX_ERR_NULL_PTR;

    if (allocResponses.find (response->mids) == allocResponses.end()) {
      // Decode free response handling
      if (--allocDecodeResponses[pthis].refCount == 0) {
        _simpleFree (pthis,response);
        allocDecodeResponses.erase (pthis);
        }
      }
    else {
      // Encode and VPP free response handling
      allocResponses.erase (response->mids);
      _simpleFree (pthis, response);
      }

    return MFX_ERR_NONE;
    }
  //}}}

  //{{{
  mfxStatus simpleLock (mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr) {

    (void) pthis;
    mfxStatus mfx_res = MFX_ERR_NONE;
    VAStatus va_res = VA_STATUS_SUCCESS;
    vaapiMemId* vaapi_mid = (vaapiMemId*) mid;
    mfxU8* pBuffer = 0;

    if (!vaapi_mid || !(vaapi_mid->m_surface))
      return MFX_ERR_INVALID_HANDLE;

    if (MFX_FOURCC_P8 == vaapi_mid->m_fourcc) {
      // bitstream processing
      VACodedBufferSegment* coded_buffer_segment;
      va_res = vaMapBuffer(m_va_dpy, *(vaapi_mid->m_surface), (void**)(&coded_buffer_segment));
      mfx_res = va_to_mfx_status(va_res);
      ptr->Y = (mfxU8*) coded_buffer_segment->buf;
      }

    else {
      // Image processing
      va_res = vaSyncSurface(m_va_dpy, *(vaapi_mid->m_surface));
      mfx_res = va_to_mfx_status(va_res);

      if (MFX_ERR_NONE == mfx_res) {
        va_res = vaDeriveImage(m_va_dpy, *(vaapi_mid->m_surface), &(vaapi_mid->m_image));
        mfx_res = va_to_mfx_status(va_res);
        }
      if (MFX_ERR_NONE == mfx_res) {
        va_res = vaMapBuffer(m_va_dpy, vaapi_mid->m_image.buf, (void**)&pBuffer);
        mfx_res = va_to_mfx_status(va_res);
        }

      if (MFX_ERR_NONE == mfx_res) {
        switch (vaapi_mid->m_image.format.fourcc) {
          //{{{
          case VA_FOURCC_NV12:
            if (vaapi_mid->m_fourcc == MFX_FOURCC_NV12) {
              ptr->Pitch = (mfxU16) vaapi_mid->m_image.pitches[0];
              ptr->Y = pBuffer + vaapi_mid->m_image.offsets[0];
              ptr->U = pBuffer + vaapi_mid->m_image.offsets[1];
              ptr->V = ptr->U + 1;
              }
            else
              mfx_res = MFX_ERR_LOCK_MEMORY;
             break;
          //}}}
          //{{{
          case VA_FOURCC_YV12:
            if (vaapi_mid->m_fourcc == MFX_FOURCC_YV12) {
              ptr->Pitch = (mfxU16) vaapi_mid->m_image.pitches[0];
              ptr->Y = pBuffer + vaapi_mid->m_image.offsets[0];
              ptr->V = pBuffer + vaapi_mid->m_image.offsets[1];
              ptr->U = pBuffer + vaapi_mid->m_image.offsets[2];
              }
            else
               mfx_res = MFX_ERR_LOCK_MEMORY;
            break;
          //}}}
          //{{{
          case VA_FOURCC_YUY2:
            if (vaapi_mid->m_fourcc == MFX_FOURCC_YUY2) {
              ptr->Pitch = (mfxU16) vaapi_mid->m_image.pitches[0];
              ptr->Y = pBuffer + vaapi_mid->m_image.offsets[0];
              ptr->U = ptr->Y + 1;
              ptr->V = ptr->Y + 3;
              }
            else
               mfx_res = MFX_ERR_LOCK_MEMORY;
            break;
          //}}}
          //{{{
          case VA_FOURCC_ARGB:
            if (vaapi_mid->m_fourcc == MFX_FOURCC_RGB4) {
              ptr->Pitch = (mfxU16) vaapi_mid->m_image.pitches[0];
              ptr->B = pBuffer + vaapi_mid->m_image.offsets[0];
              ptr->G = ptr->B + 1;
              ptr->R = ptr->B + 2;
              ptr->A = ptr->B + 3;
              }
            else
              mfx_res = MFX_ERR_LOCK_MEMORY;
            break;
          //}}}
          //{{{
          default:
            mfx_res = MFX_ERR_LOCK_MEMORY;
            break;
          //}}}
          }
        }
      }

    return mfx_res;
    }
  //}}}
  //{{{
  mfxStatus simpleUnlock (mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr) {

    (void) pthis;
    vaapiMemId* vaapi_mid = (vaapiMemId*) mid;

    if (!vaapi_mid || !(vaapi_mid->m_surface))
      return MFX_ERR_INVALID_HANDLE;

    if (MFX_FOURCC_P8 == vaapi_mid->m_fourcc) {     // bitstream processing
      vaUnmapBuffer(m_va_dpy, *(vaapi_mid->m_surface));
      }
    else {                // Image processing
      vaUnmapBuffer(m_va_dpy, vaapi_mid->m_image.buf);
      vaDestroyImage(m_va_dpy, vaapi_mid->m_image.image_id);

      if (NULL != ptr) {
        ptr->Pitch = 0;
        ptr->Y = NULL;
        ptr->U = NULL;
        ptr->V = NULL;
        ptr->A = NULL;
        }
      }

    return MFX_ERR_NONE;
    }
  //}}}

  //{{{
  mfxStatus simpleGethdl (mfxHDL pthis, mfxMemId mid, mfxHDL* handle) {

    (void) pthis;
    vaapiMemId* vaapi_mid = (vaapiMemId*) mid;

    if (!handle || !vaapi_mid || !(vaapi_mid->m_surface))
      return MFX_ERR_INVALID_HANDLE;

    *handle = vaapi_mid->m_surface; //VASurfaceID* <-> mfxHDL
    return MFX_ERR_NONE;
    }
  //}}}

  void ClearYUVSurfaceVAAPI (mfxMemId memId) { (void)memId; } // todo: clear VAAPI surface
  void ClearRGBSurfaceVAAPI (mfxMemId memId) { (void)memId; } // todo: clear VAAPI surface

  //{{{
  mfxStatus Initialize (MFXVideoSession* session, mfxFrameAllocator* mfxAllocator) {

    mfxStatus status = MFX_ERR_NONE;

    // Create VA display
    mfxHDL displayHandle = { 0 };
    status = CreateVAEnvDRM (&displayHandle);
    MSDK_CHECK_RESULT (status, MFX_ERR_NONE, status);

    // Provide VA display handle to Media SDK
    status = session->SetHandle (static_cast <mfxHandleType>(MFX_HANDLE_VA_DISPLAY), displayHandle);
    MSDK_CHECK_RESULT (status, MFX_ERR_NONE, status);

    // If mfxFrameAllocator is provided it means we need to setup  memory allocator
    if (mfxAllocator) {
      mfxAllocator->pthis = *session; // We use Media SDK session ID as the allocation identifier
      mfxAllocator->Alloc = simpleAlloc;
      mfxAllocator->Free = simpleFree;
      mfxAllocator->Lock = simpleLock;
      mfxAllocator->Unlock = simpleUnlock;
      mfxAllocator->GetHDL = simpleGethdl;

      // Since we are using video memory we must provide Media SDK with an external allocator
      status = session->SetFrameAllocator (mfxAllocator);
      MSDK_CHECK_RESULT (status, MFX_ERR_NONE, status);
      }

    return status;
    }
  //}}}
  void Release() { CleanupVAEnvDRM(); }
  void ClearYUVSurfaceVMem (mfxMemId memId) { ClearYUVSurfaceVAAPI (memId); }
  void ClearRGBSurfaceVMem (mfxMemId memId) { ClearRGBSurfaceVAAPI (memId); }
  //}}}
#endif

// utils
//{{{
FILE* OpenFile (const char* fileName, const char* mode) {
  return fopen (fileName, mode);
  }
//}}}
//{{{
void CloseFile (FILE* file) {

  if (file)
    fclose (file);
  }
//}}}

//{{{
std::string getMfxStatusString (mfxStatus status) {

  std::string statusString;
  switch (status) {
    case   0: statusString = "No error"; break;

    case  -1: statusString = "Unknown error"; break;
    case  -2: statusString = "Null pointer"; break;
    case  -3: statusString = "Unsupported feature/library load error"; break;
    case  -4: statusString = "Could not allocate memory"; break;
    case  -5: statusString = "Insufficient IO buffers"; break;
    case  -6: statusString = "Invalid handle"; break;
    case  -7: statusString = "Memory lock failure"; break;
    case  -8: statusString = "Function called before initialization"; break;
    case  -9: statusString = "Specified object not found"; break;
    case -10: statusString = "More input data expected"; break;
    case -11: statusString = "More output surfaces expected"; break;
    case -12: statusString = "Operation aborted";  break;
    case -13: statusString = "HW device lost";  break;
    case -14: statusString = "Incompatible video parameters" ; break;
    case -15: statusString = "Invalid video parameters";  break;
    case -16: statusString = "Undefined behavior"; break;
    case -17: statusString = "Device operation failure";  break;
    case -18: statusString = "More bitstream data expected";  break;
    case -19: statusString = "Incompatible audio parameters"; break;
    case -20: statusString = "Invalid audio parameters"; break;

    case   1: statusString = "the previous asynchronous operation is in execution"; break; //MFX_WRN_IN_EXECUTION
    case   2: statusString = "the HW acceleration device is busy"; break;                  //MFX_WRN_DEVICE_BUSY
    case   3: statusString = "the video parameters are changed during decoding"; break;    //MFX_WRN_VIDEO_PARAM_CHANGED
    case   4: statusString = "SW is used"; break;                                          //MFX_WRN_PARTIAL_ACCELERATION
    case   5: statusString = "incompatible video parameters"; break;                       //MFX_WRN_INCOMPATIBLE_VIDEO_PARAM
    case   6: statusString = "the value is saturated based on its valid range"; break;     //MFX_WRN_VALUE_NOT_CHANGED
    case   7: statusString = "the value is out of valid range"; break;                     //MFX_WRN_OUT_OF_RANGE
    case  10: statusString = "one of requested filters has been skipped"; break;           //MFX_WRN_FILTER_SKIPPED
    case  11: statusString = "incompatible audio parameters"; break;                       //MFX_WRN_INCOMPATIBLE_AUDIO_PARAM

    default: statusString = "Error code";
    }

  return fmt::format ("status {} {}", status, statusString);
  }
//}}}
//{{{
std::string getMfxInfoString (mfxIMPL mfxImpl, mfxVersion mfxVersion) {

  return fmt::format ("mfxImpl:{:x}{}{}{}{}{}{}{} verMajor:{} verMinor:{}",
                      mfxImpl,
                      ((mfxImpl & 0x0007) == MFX_IMPL_HARDWARE) ? " hw":"",
                      ((mfxImpl & 0x0007) == MFX_IMPL_SOFTWARE) ? " sw":"",
                      ((mfxImpl & 0x0007) == MFX_IMPL_AUTO_ANY) ? " autoAny":"",
                      ((mfxImpl & 0x0700) == MFX_IMPL_VIA_ANY) ? " any":"",
                      ((mfxImpl & 0x0700) == MFX_IMPL_VIA_D3D9) ? " d3d9":"",
                      ((mfxImpl & 0x0700) == MFX_IMPL_VIA_D3D11) ? " d3d11":"",
                      ((mfxImpl & 0x0700) == MFX_IMPL_VIA_VAAPI) ? " vaapi":"",
                      mfxVersion.Major, mfxVersion.Minor);
  }
//}}}

//{{{
void PrintErrString (int err,const char* filestr,int line) {

  switch (err) {
    case   0:
        printf("\n No error.\n");
        break;
    case  -1:
        printf("\n Unknown error: %s %d\n",filestr,line);
        break;
    case  -2:
        printf("\n Null pointer.  Check filename/path + permissions? %s %d\n",filestr,line);
        break;
    case  -3:
        printf("\n Unsupported feature/library load error. %s %d\n",filestr,line);
        break;
    case  -4:
        printf("\n Could not allocate memory. %s %d\n",filestr,line);
        break;
    case  -5:
        printf("\n Insufficient IO buffers. %s %d\n",filestr,line);
        break;
    case  -6:
        printf("\n Invalid handle. %s %d\n",filestr,line);
        break;
    case  -7:
        printf("\n Memory lock failure. %s %d\n",filestr,line);
        break;
    case  -8:
        printf("\n Function called before initialization. %s %d\n",filestr,line);
        break;
    case  -9:
        printf("\n Specified object not found. %s %d\n",filestr,line);
        break;
    case -10:
        printf("\n More input data expected. %s %d\n",filestr,line);
        break;
    case -11:
        printf("\n More output surfaces expected. %s %d\n",filestr,line);
        break;
    case -12:
        printf("\n Operation aborted. %s %d\n",filestr,line);
        break;
    case -13:
        printf("\n HW device lost. %s %d\n",filestr,line);
        break;
    case -14:
        printf("\n Incompatible video parameters. %s %d\n",filestr,line);
        break;
    case -15:
        printf("\n Invalid video parameters. %s %d\n",filestr,line);
        break;
    case -16:
        printf("\n Undefined behavior. %s %d\n",filestr,line);
        break;
    case -17:
        printf("\n Device operation failure. %s %d\n",filestr,line);
        break;
    case -18:
        printf("\n More bitstream data expected. %s %d\n",filestr,line);
        break;
    case -19:
        printf("\n Incompatible audio parameters. %s %d\n",filestr,line);
        break;
    case -20:
        printf("\n Invalid audio parameters. %s %d\n",filestr,line);
        break;
    default:
        printf("\nError code %d,\t%s\t%d\n\n", err, filestr, line);
    }
  }
//}}}
//{{{
char mfxFrameTypeString (mfxU16 FrameType) {

  mfxU8 FrameTmp = FrameType & 0xF;

  char FrameTypeOut;
  switch (FrameTmp) {
    case MFX_FRAMETYPE_I:
      FrameTypeOut = 'I';
      break;
    case MFX_FRAMETYPE_P:
      FrameTypeOut = 'P';
      break;
    case MFX_FRAMETYPE_B:
      FrameTypeOut = 'B';
      break;
    default:
      FrameTypeOut = '*';
    }

  return FrameTypeOut;
  }
//}}}

//{{{
void ClearYUVSurfaceSysMem (mfxFrameSurface1* sfc, mfxU16 width, mfxU16 height) {

   // In case simulating direct access to frames we initialize the allocated surfaces with default pattern
  memset (sfc->Data.Y, 100, width * height);  // Y plane
  memset (sfc->Data.U, 50, (width * height)/2);  // UV plane
  }
//}}}
//{{{
// Get free raw frame surface
int GetFreeSurfaceIndex (mfxFrameSurface1** surfacesPool, mfxU16 nPoolSize) {

  if (surfacesPool)
    for (mfxU16 i = 0; i < nPoolSize; i++)
      if (0 == surfacesPool[i]->Data.Locked)
        return i;

  return MFX_ERR_NOT_FOUND;
  }
//}}}
//{{{
int GetFreeSurfaceIndex (const std::vector<mfxFrameSurface1>& surfacesPool) {

  auto it = std::find_if (surfacesPool.begin(), surfacesPool.end(),
                          [](const mfxFrameSurface1& surface) {
                            return 0 == surface.Data.Locked;
                            });

  if (it == surfacesPool.end())
    return MFX_ERR_NOT_FOUND;
  else
    return (int)(it - surfacesPool.begin());
  }
//}}}

//{{{
mfxStatus ReadBitStreamData (mfxBitstream* pBS, FILE* fSource) {

  memmove (pBS->Data, pBS->Data + pBS->DataOffset, pBS->DataLength);
  pBS->DataOffset = 0;

  mfxU32 nBytesRead = (mfxU32)fread (pBS->Data + pBS->DataLength, 1, pBS->MaxLength - pBS->DataLength,
                                     fSource);

  if (0 == nBytesRead)
    return MFX_ERR_MORE_DATA;

  pBS->DataLength += nBytesRead;

  return MFX_ERR_NONE;
  }
//}}}

//{{{
mfxStatus ReadPlaneData (mfxU16 w, mfxU16 h, mfxU8* buf, mfxU8* ptr,
                        mfxU16 pitch, mfxU16 offset, FILE* fSource) {

  mfxU32 nBytesRead;
  for (mfxU16 i = 0; i < h; i++) {
    nBytesRead = (mfxU32) fread(buf, 1, w, fSource);
    if (w != nBytesRead)
      return MFX_ERR_MORE_DATA;
    for (mfxU16 j = 0; j < w; j++)
      ptr[i * pitch + j * 2 + offset] = buf[j];
    }

  return MFX_ERR_NONE;
  }
//}}}
//{{{
mfxStatus ReadPlaneData10Bit (mfxU16 w, mfxU16 h, mfxU16* buf, mfxU8* ptr,
    mfxU16 pitch, mfxU16 shift, FILE* fSource) {

  mfxU32 nBytesRead;
  mfxU16* shortPtr;

  for (mfxU16 i = 0; i < h; i++) {
    nBytesRead = (mfxU32)fread(buf, 2, w, fSource); //Reading in 16bits per pixel.
    if (w != nBytesRead)
      return MFX_ERR_MORE_DATA;

    // Read data with P010 and convert it to MS-P010
    //Shifting left the data in a 16bits boundary
    //Because each 10bit pixel channel takes 2 bytes with the LSB on the right side of the 16bits
    //See this web page for the description of MS-P010 format
    //https://msdn.microsoft.com/en-us/library/windows/desktop/bb970578(v=vs.85).aspx#overview
    if (shift > 0) {
      shortPtr = (mfxU16 *)(ptr + i * pitch);
      for (mfxU16 j = 0; j < w; j++)
        shortPtr[j] = buf[j] << 6;
      }
    }

  return MFX_ERR_NONE;
  }
//}}}

//{{{
mfxStatus LoadRawFrame (mfxFrameSurface1* surface, FILE* fSource)
{
    if (!fSource) {
        // Simulate instantaneous access to 1000 "empty" frames.
        static int frameCount = 0;
        if (1000 == frameCount++)
            return MFX_ERR_MORE_DATA;
        else
            return MFX_ERR_NONE;
    }

    mfxStatus status = MFX_ERR_NONE;
    mfxU32 nBytesRead;
    mfxU16 w, h, i, pitch;
    mfxU8* ptr;
    mfxFrameInfo* pInfo = &surface->Info;
    mfxFrameData* pData = &surface->Data;

    if (pInfo->CropH > 0 && pInfo->CropW > 0) {
        w = pInfo->CropW;
        h = pInfo->CropH;
    } else {
        w = pInfo->Width;
        h = pInfo->Height;
    }

    pitch = pData->Pitch;
    ptr = pData->Y + pInfo->CropX + pInfo->CropY * pData->Pitch;

    // read luminance plane
    for (i = 0; i < h; i++) {
        nBytesRead = (mfxU32) fread(ptr + i * pitch, 1, w, fSource);
        if (w != nBytesRead)
            return MFX_ERR_MORE_DATA;
    }

    mfxU8 buf[2048];        // maximum supported chroma width for nv12
    w /= 2;
    h /= 2;
    ptr = pData->UV + pInfo->CropX + (pInfo->CropY / 2) * pitch;
    if (w > 2048)
        return MFX_ERR_UNSUPPORTED;

    // load V
    status = ReadPlaneData(w, h, buf, ptr, pitch, 1, fSource);
    if (MFX_ERR_NONE != status)
        return status;
    // load U
    ReadPlaneData(w, h, buf, ptr, pitch, 0, fSource);
    if (MFX_ERR_NONE != status)
        return status;

    return MFX_ERR_NONE;
}
//}}}
//{{{
mfxStatus LoadRaw10BitFrame (mfxFrameSurface1* surface, FILE* fSource) {

  if (!fSource) {
    // Simulate instantaneous access to 1000 "empty" frames.
    static int frameCount = 0;
    if (1000 == frameCount++)
      return MFX_ERR_MORE_DATA;
    else
      return MFX_ERR_NONE;
    }

  mfxStatus status = MFX_ERR_NONE;
  mfxU16 w, h,pitch;
  mfxU8* ptr;
  mfxFrameInfo* pInfo = &surface->Info;
  mfxFrameData* pData = &surface->Data;

  if (pInfo->CropH > 0 && pInfo->CropW > 0) {
    w = pInfo->CropW;
    h = pInfo->CropH;
    }
  else {
    w = pInfo->Width;
    h = pInfo->Height;
    }

  pitch = pData->Pitch;
  mfxU16 buf[4096];        // maximum supported chroma width for nv12

  // read luminance plane
  ptr = pData->Y + pInfo->CropX + pInfo->CropY * pData->Pitch;
  status = ReadPlaneData10Bit(w, h, buf, ptr, pitch, pInfo->Shift, fSource);
  if (MFX_ERR_NONE != status)
    return status;

  // Load UV plan
  h /= 2;
  ptr = pData->UV + pInfo->CropX + (pInfo->CropY / 2) * pitch;

  // load U
  status = ReadPlaneData10Bit(w, h, buf, ptr, pitch, pInfo->Shift, fSource);
  if (MFX_ERR_NONE != status)
    return status;

  return MFX_ERR_NONE;
  }
//}}}
//{{{
mfxStatus LoadRawRGBFrame (mfxFrameSurface1* surface, FILE* fSource) {

  if (!fSource) {
    // Simulate instantaneous access to 1000 "empty" frames.
    static int frameCount = 0;
    if (1000 == frameCount++)
      return MFX_ERR_MORE_DATA;
    else
      return MFX_ERR_NONE;
    }

  mfxU32 nBytesRead;
  mfxU16 w, h;
  mfxFrameInfo* pInfo = &surface->Info;

  if (pInfo->CropH > 0 && pInfo->CropW > 0) {
    w = pInfo->CropW;
    h = pInfo->CropH;
    }
  else {
    w = pInfo->Width;
    h = pInfo->Height;
    }

  for (mfxU16 i = 0; i < h; i++) {
    nBytesRead = (mfxU32)fread(surface->Data.B + i * surface->Data.Pitch,
                           1, w * 4, fSource);
    if ((mfxU32)(w * 4) != nBytesRead)
      return MFX_ERR_MORE_DATA;
    }

  return MFX_ERR_NONE;
  }
//}}}

//{{{
mfxStatus WriteBitStreamFrame (mfxBitstream* pMfxBitstream, FILE* fSink) {

  if (!pMfxBitstream)
    return MFX_ERR_NULL_PTR;

  if (fSink) {
    mfxU32 nBytesWritten = (mfxU32) fwrite(pMfxBitstream->Data + pMfxBitstream->DataOffset, 1,
                            pMfxBitstream->DataLength, fSink);

    if (nBytesWritten != pMfxBitstream->DataLength)
      return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

  pMfxBitstream->DataLength = 0;

  return MFX_ERR_NONE;
  }
//}}}
//{{{
mfxStatus WriteSection (mfxU8* plane, mfxU16 factor, mfxU16 chunksize,
                       mfxFrameInfo* pInfo, mfxFrameData* pData, mfxU32 i,
                       mfxU32 j, FILE* fSink) {

  if (chunksize != fwrite (plane + (pInfo->CropY * pData->Pitch / factor + pInfo->CropX) + i * pData->Pitch + j,
                           1, chunksize, fSink))
    return MFX_ERR_UNDEFINED_BEHAVIOR;

  return MFX_ERR_NONE;
  }
//}}}
//{{{
mfxStatus WriteRawFrame (mfxFrameSurface1* surface, FILE* fSink) {

  mfxFrameInfo* pInfo = &surface->Info;
  mfxU16 h, w;
  if (pInfo->CropH > 0 && pInfo->CropW > 0) {
    w = pInfo->CropW;
    h = pInfo->CropH;
    }
  else {
    w = pInfo->Width;
    h = pInfo->Height;
    }

  mfxStatus status = MFX_ERR_NONE;
  mfxFrameData* pData = &surface->Data;
  if ((pInfo->FourCC == MFX_FOURCC_RGB4) || (pInfo->FourCC == MFX_FOURCC_A2RGB10)) {
    mfxU16 pitch = pData->Pitch;
    mfxU8* ptr = std::min (std::min(pData->R, pData->G), pData->B);

    for (mfxU16 i = 0; i < h; i++) {
      mfxU32 nByteWrite = (mfxU32)fwrite (ptr + i * pitch, 1, w * 4, fSink);
      if ((mfxU32)(w * 4) != nByteWrite)
        return MFX_ERR_MORE_DATA;
      }
    }
  else {
    for (mfxU16 i = 0; i < pInfo->CropH; i++)
      status = WriteSection (pData->Y, 1, pInfo->CropW, pInfo, pData, i, 0, fSink);

    h = pInfo->CropH / 2;
    w = pInfo->CropW;
    for (mfxU16 i = 0; i < h; i++)
      for (mfxU16 j = 0; j < w; j += 2)
        status = WriteSection (pData->UV, 2, 1, pInfo, pData, i, j, fSink);
    for (mfxU16 i = 0; i < h; i++)
      for (mfxU16 j = 1; j < w; j += 2)
        status = WriteSection (pData->UV, 2, 1, pInfo, pData, i, j, fSink);
    }

  return status;
  }
//}}}
//{{{
mfxStatus WriteSection10Bit (mfxU8* plane, mfxU16 factor, mfxU16 chunksize,
    mfxFrameInfo* pInfo, mfxFrameData* pData, mfxU32 i,
    /*mfxU32 j,*/ FILE* fSink) {

    // Temporary buffer to convert MS-P010 to P010
    std::vector<mfxU16> tmp;
    mfxU16* shortPtr;

    shortPtr = (mfxU16*)(plane + (pInfo->CropY * pData->Pitch / factor + pInfo->CropX) + i * pData->Pitch);
    if (pInfo->Shift)
    {
        // Convert MS-P010 to P010 and write
        tmp.resize(pData->Pitch);

        //Shifting right the data in a 16bits boundary
        //Because each 10bit pixel channel takes 2 bytes with the LSB on the right side of the 16bits
        //See this web page for the description of 10bit YUV format
        //https://msdn.microsoft.com/en-us/library/windows/desktop/bb970578(v=vs.85).aspx#overview
        for (int idx = 0; idx < pInfo->CropW; idx++)
        {
            tmp[idx] = shortPtr[idx] >> 6;
        }
        if (chunksize != fwrite(&tmp[0], 1, chunksize, fSink))
            return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    else {
        if (chunksize != fwrite(shortPtr, 1, chunksize, fSink))
            return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    return MFX_ERR_NONE;
}
//}}}
//{{{
mfxStatus WriteRaw10BitFrame (mfxFrameSurface1* surface, FILE* fSink) {

  mfxFrameInfo* pInfo = &surface->Info;
  mfxFrameData* pData = &surface->Data;
  mfxStatus status = MFX_ERR_NONE;


  for (mfxU16 i = 0; i < pInfo->CropH; i++) {
    status = WriteSection10Bit(pData->Y, 1, pInfo->CropW * 2, pInfo, pData, i, fSink);
    }

  for (mfxU16 i = 0; i < pInfo->CropH / 2; i++) {
    status = WriteSection10Bit(pData->UV, 2, pInfo->CropW * 2, pInfo, pData, i, fSink);
    }

  return status;
  }
//}}}

//{{{
int GetFreeTaskIndex (Task* pTaskPool, mfxU16 nPoolSize) {

  if (pTaskPool)
    for (int i = 0; i < nPoolSize; i++)
      if (!pTaskPool[i].syncp)
        return i;

  return MFX_ERR_NOT_FOUND;
  }
//}}}

//{{{
const mfxPluginUID& msdkGetPluginUID (mfxIMPL impl, msdkComponentType type, mfxU32 uCodecid) {

  if (impl == MFX_IMPL_SOFTWARE) {
    switch (type) {
      //{{{
      case MSDK_VDECODE:
        switch (uCodecid) {
          case MFX_CODEC_HEVC:
            return MFX_PLUGINID_HEVCD_SW;
          }
        break;
      //}}}
      //{{{
      case MSDK_VENCODE:
        switch (uCodecid) {
          case MFX_CODEC_HEVC:
            return MFX_PLUGINID_HEVCE_SW;
          }
        break;
      //}}}
      }
    }

  else if ((impl |= MFX_IMPL_HARDWARE)) {
    switch (type) {
      //{{{
      case MSDK_VDECODE:
        switch (uCodecid) {
          case MFX_CODEC_HEVC:
            return MFX_PLUGINID_HEVCD_HW; // MFX_PLUGINID_HEVCD_SW for now
          case MFX_CODEC_VP8:
             return MFX_PLUGINID_VP8D_HW;
          }
        break;
      //}}}
      //{{{
      case MSDK_VENCODE:
        switch (uCodecid) {
          case MFX_CODEC_HEVC:
            return MFX_PLUGINID_HEVCE_HW;
          }
        break;
      //}}}
      //{{{
      case MSDK_VENC:
        switch (uCodecid) {
          case MFX_CODEC_HEVC:
            return MFX_PLUGINID_HEVCE_FEI_HW;   // HEVC FEI uses ENC interface
          }
        break;
      //}}}
      }
    }

  return MSDK_PLUGINGUID_NULL;
  }
//}}}
//{{{
bool AreGuidsEqual (const mfxPluginUID& guid1, const mfxPluginUID& guid2) {

  for (size_t i = 0; i != sizeof(mfxPluginUID); i++) {
    if (guid1.Data[i] != guid2.Data[i])
      return false;
    }

  return true;
  }
//}}}
//{{{
char* ConvertGuidToString (const mfxPluginUID& guid) {

  static char szGuid[256] = { 0 };

  for (size_t i = 0; i != sizeof(mfxPluginUID); i++) {
    sprintf(&szGuid[2 * i], "%02x", guid.Data[i]);
    }

  return szGuid;
  }
//}}}

//{{{
mfxStatus ConvertFrameRate (mfxF64 dFrameRate, mfxU32* pnFrameRateExtN, mfxU32* pnFrameRateExtD) {

  MSDK_CHECK_POINTER(pnFrameRateExtN, MFX_ERR_NULL_PTR);
  MSDK_CHECK_POINTER(pnFrameRateExtD, MFX_ERR_NULL_PTR);

  mfxU32 fr;
  fr = (mfxU32)(dFrameRate + .5);

  if (fabs(fr - dFrameRate) < 0.0001) {
    *pnFrameRateExtN = fr;
    *pnFrameRateExtD = 1;
    return MFX_ERR_NONE;
    }

  fr = (mfxU32)(dFrameRate * 1.001 + .5);
  if (fabs(fr * 1000 - dFrameRate * 1001) < 10) {
    *pnFrameRateExtN = fr * 1000;
    *pnFrameRateExtD = 1001;
    return MFX_ERR_NONE;
    }

  *pnFrameRateExtN = (mfxU32)(dFrameRate * 10000 + .5);
  *pnFrameRateExtD = 10000;

  return MFX_ERR_NONE;
  }
//}}}
