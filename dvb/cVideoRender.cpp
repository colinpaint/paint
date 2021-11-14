// cVideoRender.cpp
//{{{  includes
#include "cVideoRender.h"

#ifdef _WIN32
  //{{{  windows directx11 headers
  #define NOMINMAX
  #include <intrin.h>

  // d3d11 headers
  #include <d3d11.h>
  #include <dxgi1_2.h>
  //}}}
#else
  //{{{  vaapi headers
  #include <stdio.h>
  #include <string.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <math.h>
  #include <stdlib.h>
  #include <assert.h>
  #include <sys/ioctl.h>
  #include <new>

  #include <drm/drm.h>
  #include <drm/drm_fourcc.h>

  #include "va/va.h"
  #include "va/va_drm.h"
  //}}}
#endif

#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include <array>
#include <algorithm>
#include <thread>
#include <functional>

#include "../imgui/imgui.h"
#include "../imgui/myImgui.h"

#include "../dvb/cDvbUtils.h"
#include "../graphics/cGraphics.h"

#include <mfxvideo++.h>

#include "../utils/date.h"
#include "../utils/cLog.h"
#include "../utils/utils.h"

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  }

#define INTEL_SSE2
#define INTEL_SSSE3 1
#include <emmintrin.h>
#include <tmmintrin.h>

using namespace std;
//}}}

namespace {
  //{{{
  string getMfxStatusString (mfxStatus status) {

    string statusString;
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
  string getMfxInfoString (mfxIMPL mfxImpl, mfxVersion mfxVersion) {

    return fmt::format ("mfxImpl:{:x}{}{}{}{}{}{}{} verMajor:{} verMinor:{}",
                        mfxImpl,
                        ((mfxImpl & 0x0007) == MFX_IMPL_HARDWARE)  ? " hw":"",
                        ((mfxImpl & 0x0007) == MFX_IMPL_SOFTWARE)  ? " sw":"",
                        ((mfxImpl & 0x0007) == MFX_IMPL_AUTO_ANY)  ? " autoAny":"",
                        ((mfxImpl & 0x0700) == MFX_IMPL_VIA_ANY)   ? " any":"",
                        ((mfxImpl & 0x0700) == MFX_IMPL_VIA_D3D9)  ? " d3d9":"",
                        ((mfxImpl & 0x0700) == MFX_IMPL_VIA_D3D11) ? " d3d11":"",
                        ((mfxImpl & 0x0700) == MFX_IMPL_VIA_VAAPI) ? " vaapi":"",
                        mfxVersion.Major, mfxVersion.Minor);
    }
  //}}}

  #ifdef _WIN32
    //{{{  windows directx11
    #define D3DFMT_NV12 (D3DFORMAT)MAKEFOURCC('N','V','1','2')
    #define D3DFMT_YV12 (D3DFORMAT)MAKEFOURCC('Y','V','1','2')
    #define WILL_READ 0x1000
    #define WILL_WRITE 0x2000

    //{{{  const struct implTypes
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
    //{{{  CustomMemId
    typedef struct {
      mfxMemId  mMemId;
      mfxMemId  mMemIdStage;
      mfxU16    mRw;
      } CustomMemId;
    //}}}

    // vars
    ID3D11Device* D3D11Device;
    ID3D11DeviceContext* D3D11Ctx;

    map <mfxMemId*, mfxHDL> allocResponses;
    map <mfxHDL, mfxFrameAllocResponse> allocDecodeResponses;
    map <mfxHDL, int> allocDecodeRefCount;

    // allocate
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
      CustomMemId** memIds = (CustomMemId**)calloc (request->NumFrameSuggested, sizeof(CustomMemId*));
      if (!memIds)
        return MFX_ERR_MEMORY_ALLOC;

      for (int i = 0; i < request->NumFrameSuggested; i++) {
        memIds[i] = (CustomMemId*)calloc (1, sizeof(CustomMemId));
        if (!memIds[i])
          return MFX_ERR_MEMORY_ALLOC;
        memIds[i]->mRw = request->Type & 0xF000; // Set intended read/write operation
        }
      request->Type = request->Type & 0x0FFF;

      // because P8 data (bitstream) for h264 encoder should be allocated by CreateBuffer()
      // - but P8 data (MBData) for MPEG2 encoder should be allocated by CreateTexture2D()
      if (request->Info.FourCC == MFX_FOURCC_P8) {
        D3D11_BUFFER_DESC desc = {0};
        if (!request->NumFrameSuggested)
          return MFX_ERR_MEMORY_ALLOC;
        desc.ByteWidth = request->Info.Width * request->Info.Height;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        ID3D11Buffer* buffer = nullptr;
        if (FAILED (D3D11Device->CreateBuffer(&desc, 0, &buffer)))
          return MFX_ERR_MEMORY_ALLOC;
        memIds[0]->mMemId = reinterpret_cast<ID3D11Texture2D*>(buffer);
        }
      else {
        D3D11_TEXTURE2D_DESC desc = {0};
        desc.Width = request->Info.Width;
        desc.Height = request->Info.Height;
        desc.MipLevels = 1;
        desc.ArraySize = 1; // number of subresources is 1 in this case
        desc.Format = format;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_DECODER;
        desc.MiscFlags = 0;
        //desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

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

        // create surface textures
        ID3D11Texture2D* texture2D;
        for (size_t i = 0; i < request->NumFrameSuggested / desc.ArraySize; i++) {
          if (FAILED (D3D11Device->CreateTexture2D (&desc, NULL, &texture2D)))
            return MFX_ERR_MEMORY_ALLOC;
          memIds[i]->mMemId = texture2D;
          }

        desc.ArraySize = 1;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ; // | D3D11_CPU_ACCESS_WRITE;
        desc.BindFlags = 0;
        desc.MiscFlags = 0;
        //desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

        // create surface staging textures
        for (size_t i = 0; i < request->NumFrameSuggested; i++) {
          if (FAILED (D3D11Device->CreateTexture2D (&desc, NULL, &texture2D)))
            return MFX_ERR_MEMORY_ALLOC;
          memIds[i]->mMemIdStage = texture2D;
          }
        }

      response->mids = (mfxMemId*)memIds;
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

            ID3D11Texture2D* surface = (ID3D11Texture2D*)mid->mMemId;
            if (surface)
              surface->Release();

            ID3D11Texture2D* stage = (ID3D11Texture2D*)mid->mMemIdStage;
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
    mfxStatus simpleAlloc (mfxHDL pthis, mfxFrameAllocRequest* request, mfxFrameAllocResponse* response) {

      mfxStatus status = MFX_ERR_NONE;

      if (request->Type & MFX_MEMTYPE_SYSTEM_MEMORY)
        return MFX_ERR_UNSUPPORTED;

      if (allocDecodeResponses.find (pthis) != allocDecodeResponses.end() &&
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
    mfxStatus simpleLock (mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr) {

      pthis;

      HRESULT hRes = S_OK;

      D3D11_TEXTURE2D_DESC desc = {0};
      D3D11_MAPPED_SUBRESOURCE lockedRect = {0};
      D3D11_MAP mapType  = D3D11_MAP_READ;
      UINT mapFlags = D3D11_MAP_FLAG_DO_NOT_WAIT;
      CustomMemId* memId = (CustomMemId*)mid;
      ID3D11Texture2D* surface = (ID3D11Texture2D*)memId->mMemId;
      ID3D11Texture2D* stage = (ID3D11Texture2D*)memId->mMemIdStage;
      if (!stage) {
        hRes = D3D11Ctx->Map (surface, 0, mapType, mapFlags, &lockedRect);
        desc.Format = DXGI_FORMAT_P8;
        }
      else {
        surface->GetDesc (&desc);

       // copy data only in case of user wants to read from stored surface
        if (memId->mRw & WILL_READ)
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

      pthis;

      CustomMemId* memId = (CustomMemId*)mid;
      ID3D11Texture2D* surface = (ID3D11Texture2D*)memId->mMemId;
      ID3D11Texture2D* stage = (ID3D11Texture2D*)memId->mMemIdStage;
      if (!stage)
        D3D11Ctx->Unmap (surface, 0);
      else {
        D3D11Ctx->Unmap (stage, 0);
        // copy data only in case of user wants to write to stored surface
        if (memId->mRw & WILL_WRITE)
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

      pthis;

      if (!handle)
        return MFX_ERR_INVALID_HANDLE;

      CustomMemId* memId = (CustomMemId*)mid;
      mfxHDLPair* pPair = (mfxHDLPair*)handle;
      pPair->first  = memId->mMemId; // surface texture
      pPair->second = 0;

      return MFX_ERR_NONE;
      }
    //}}}

    // create
    //{{{
    mfxStatus createHWDevice (mfxSession session, mfxHDL* deviceHandle) {

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

    // interface
    //{{{
    mfxStatus mfxInitialize (mfxIMPL mfxImpl, mfxVersion mfxVersion, MFXVideoSession& session, mfxFrameAllocator* mfxAllocator) {

      mfxStatus status = session.Init (mfxImpl | MFX_IMPL_VIA_D3D11, &mfxVersion);
      if (status != MFX_ERR_NONE)
        return status;

      // if mfxFrameAllocator, setup DirectX device and memory allocator
      if (mfxAllocator) {
        // create DirectX11 device,context
        mfxHDL deviceHandle;
        status = createHWDevice (session, &deviceHandle);
        if (status != MFX_ERR_NONE)
          return status;

        // set MediaSDK deviceManager
        status = session.SetHandle (MFX_HANDLE_D3D11_DEVICE, deviceHandle);
        if (status != MFX_ERR_NONE)
          return status;

        // use MediaSDK session ID as the allocation identifier
        mfxAllocator->pthis = session;
        mfxAllocator->Alloc = simpleAlloc;
        mfxAllocator->Free = simpleFree;
        mfxAllocator->Lock = simpleLock;
        mfxAllocator->Unlock = simpleUnlock;
        mfxAllocator->GetHDL = simpleGethdl;

        // Since we are using video memory we must provide Media SDK with an external allocator
        status = session.SetFrameAllocator (mfxAllocator);
        }

      return status;
      }
    //}}}
    //{{{
    void mfxRelease() {
      D3D11Device->Release();
      D3D11Ctx->Release();
      }
    //}}}
    //}}}
  #else
    //{{{  vaapi
    //{{{
    // VAAPI Allocator internal Mem ID
    struct vaapiMemId {
      VASurfaceID* mSurface;
      VAImage mImage;

      // variables for VAAPI Allocator inernal color conversion
      unsigned int mFourcc;
      mfxU8* mSysBuffer;
      mfxU8* mVaBuffer;
      };
    //}}}
    //{{{
    struct sharedResponse {
      mfxFrameAllocResponse mMfxResponse;
      int mRefCount;
      };
    //}}}

    // vars
    int mFd = -1;
    VADisplay mVaDisplayHandle = NULL;

    map <mfxMemId*, mfxHDL> allocResponses;
    map<mfxHDL, sharedResponse> allocDecodeResponses;

    // utils
    //{{{
    mfxStatus vaToMfxStatus (VAStatus vaStatus) {

      mfxStatus mfxRes = MFX_ERR_NONE;

      switch (vaStatus) {
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
    unsigned int convertMfxFourccToVAFormat (mfxU32 fourcc) {

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

    // allocate
    //{{{
    mfxStatus _simpleAlloc (mfxFrameAllocRequest* request, mfxFrameAllocResponse* response) {

      mfxStatus status = MFX_ERR_NONE;
      bool createSrfSucceeded = false;

      memset (response, 0, sizeof(mfxFrameAllocResponse));

      mfxU16 numSurfaces = request->NumFrameSuggested;
      if (!numSurfaces)
        return MFX_ERR_MEMORY_ALLOC;

      mfxU32 fourcc = request->Info.FourCC;
      unsigned int vaFourcc = convertMfxFourccToVAFormat (fourcc);
      if (!vaFourcc || ((VA_FOURCC_NV12 != vaFourcc) &&
                        (VA_FOURCC_YV12 != vaFourcc) && (VA_FOURCC_YUY2 != vaFourcc) &&
                        (VA_FOURCC_ARGB != vaFourcc) && (VA_FOURCC_P208 != vaFourcc)))
        return MFX_ERR_MEMORY_ALLOC;

      VASurfaceID* surfaces = NULL;
      vaapiMemId* vaapiMids = NULL;
      mfxMemId* mids = NULL;
      if (MFX_ERR_NONE == status) {
        //{{{  allocate surfaces, vaapiMids, mids
        surfaces = (VASurfaceID*)calloc (numSurfaces, sizeof(VASurfaceID));
        vaapiMids = (vaapiMemId*)calloc (numSurfaces, sizeof(vaapiMemId));
        mids = (mfxMemId*)calloc (numSurfaces, sizeof(mfxMemId));
        if ((!surfaces) || (!vaapiMids) || (!mids))
          status = MFX_ERR_MEMORY_ALLOC;
        }
        //}}}

      mfxU16 numAllocated = 0;
      if (MFX_ERR_NONE == status) {
        if (VA_FOURCC_P208 != vaFourcc) {
          //{{{  create surfaces
          VASurfaceAttrib attrib;
          attrib.type = VASurfaceAttribPixelFormat;
          attrib.value.type = VAGenericValueTypeInteger;
          attrib.value.value.i = vaFourcc;
          attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
          status = vaToMfxStatus (
            vaCreateSurfaces (mVaDisplayHandle, VA_RT_FORMAT_YUV420,
                              request->Info.Width, request->Info.Height, surfaces, numSurfaces, &attrib, 1));
          createSrfSucceeded = (MFX_ERR_NONE == status);
          }
          //}}}
        else {
          //{{{  create buffer
          // from libva spec
          VAContextID context_id = request->reserved[0];
          int codedbuf_size = (request->Info.Width * request->Info.Height) * 400 / (16 * 16);
          for (numAllocated = 0; numAllocated < numSurfaces; numAllocated++) {
            VABufferID coded_buf;
            status = vaToMfxStatus (
              vaCreateBuffer (mVaDisplayHandle, context_id, VAEncCodedBufferType, codedbuf_size, 1, NULL, &coded_buf));
            if (MFX_ERR_NONE != status)
              break;
            surfaces[numAllocated] = coded_buf;
            }
          }
          //}}}
        }

      if (MFX_ERR_NONE == status) {
        //{{{  set mids
        for (mfxU16 i = 0; i < numSurfaces; ++i) {
          vaapiMemId* vaapiMid = &(vaapiMids[i]);
          vaapiMid->mFourcc = fourcc;
          vaapiMid->mSurface = &(surfaces[i]);
          mids[i] = vaapiMid;
          }
        }
        //}}}

      if (MFX_ERR_NONE == status) {
        //{{{  successful, set response
        response->mids = mids;
        response->NumFrameActual = numSurfaces;
        }
        //}}}
      else {
        //{{{  unsuccessful, set response, cleanup
        // i.e. MFX_ERR_NONE != status
        response->mids = NULL;
        response->NumFrameActual = 0;

        if (VA_FOURCC_P208 != vaFourcc) {
          if (createSrfSucceeded)
            vaDestroySurfaces (mVaDisplayHandle, surfaces, numSurfaces);
          }
        else {
          for (mfxU16 i = 0; i < numAllocated; i++)
            vaDestroyBuffer (mVaDisplayHandle, surfaces[i]);
          }

        if (mids) {
          free (mids);
          mids = NULL;
          }

        if (vaapiMids) {
          free (vaapiMids);
          vaapiMids = NULL;
          }

        if (surfaces) {
          free(surfaces);
          surfaces = NULL;
          }
        }
        //}}}

      return status;
      }
    //}}}
    //{{{
    mfxStatus _simpleFree (mfxHDL pthis, mfxFrameAllocResponse* response) {

      bool actualFreeMemory = false;
      if (0 == memcmp (response, &(allocDecodeResponses[pthis].mMfxResponse), sizeof(*response))) {
        // Decode free response handling
        allocDecodeResponses[pthis].mRefCount--;
        if (0 == allocDecodeResponses[pthis].mRefCount)
          actualFreeMemory = true;
        }
      else
        // Encode and VPP free response handling
        actualFreeMemory = true;

      if (actualFreeMemory) {
        if (response->mids) {
          vaapiMemId* vaapiMids = (vaapiMemId*)(response->mids[0]);
          bool isBitstreamMemory = (MFX_FOURCC_P8 == vaapiMids->mFourcc) ? true : false;

          VASurfaceID* surfaces = vaapiMids->mSurface;
          for (mfxU32 i = 0; i < response->NumFrameActual; ++i) {
            if (MFX_FOURCC_P8 == vaapiMids[i].mFourcc)
              vaDestroyBuffer (mVaDisplayHandle, surfaces[i]);
            else if (vaapiMids[i].mSysBuffer)
              free (vaapiMids[i].mSysBuffer);
            }

          free (vaapiMids);
          free (response->mids);
          response->mids = NULL;
          if (!isBitstreamMemory)
            vaDestroySurfaces (mVaDisplayHandle, surfaces, response->NumFrameActual);
          free (surfaces);
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

      if (request->NumFrameSuggested <= allocDecodeResponses[pthis].mMfxResponse.NumFrameActual
          && MFX_MEMTYPE_EXTERNAL_FRAME & request->Type
          && MFX_MEMTYPE_FROM_DECODE & request->Type
          &&allocDecodeResponses[pthis].mMfxResponse.NumFrameActual != 0) {
        // Memory for this request was already allocated during manual allocation stage. Return saved response
        //   When decode acceleration device (VAAPI) is created it requires a list of VAAPI surfaces to be passed.
        //   Therefore Media SDK will ask for the surface info/mids again at Init() stage, thus requiring us to return the saved response
        //   (No such restriction applies to Encode or VPP)
        *response = allocDecodeResponses[pthis].mMfxResponse;
        allocDecodeResponses[pthis].mRefCount++;
        }
      else {
        status = _simpleAlloc (request, response);
        if (MFX_ERR_NONE == status) {
          if ((MFX_MEMTYPE_EXTERNAL_FRAME & request->Type) && (MFX_MEMTYPE_FROM_DECODE & request->Type)) {
            // Decode alloc response handling
            allocDecodeResponses[pthis].mMfxResponse = *response;
            allocDecodeResponses[pthis].mRefCount++;
            //allocDecodeRefCount[pthis]++;
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

      if (allocResponses.find (response->mids) == allocResponses.end()) {
        // Decode free response handling
        if (--allocDecodeResponses[pthis].mRefCount == 0) {
          _simpleFree (pthis, response);
          allocDecodeResponses.erase (pthis);
          }
        }
      else {
        // Encode and VPP free response handling
        allocResponses.erase (response->mids);
        _simpleFree (pthis,response);
        }

      return MFX_ERR_NONE;
      }
    //}}}
    //{{{
    mfxStatus simpleLock (mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr) {

      (void) pthis;
      mfxStatus mfx_res = MFX_ERR_NONE;
      VAStatus va_res = VA_STATUS_SUCCESS;
      vaapiMemId* vaapi_mid = (vaapiMemId*)mid;
      mfxU8* pBuffer = 0;

      if (!vaapi_mid || !(vaapi_mid->mSurface))
        return MFX_ERR_INVALID_HANDLE;

      if (MFX_FOURCC_P8 == vaapi_mid->mFourcc) {
        // bitstream processing
        VACodedBufferSegment* coded_buffer_segment;
        va_res = vaMapBuffer (mVaDisplayHandle, *(vaapi_mid->mSurface), (void**)(&coded_buffer_segment));
        mfx_res = vaToMfxStatus (va_res);
        ptr->Y = (mfxU8*)coded_buffer_segment->buf;
        }

      else {
        // Image processing
        va_res = vaSyncSurface (mVaDisplayHandle, *(vaapi_mid->mSurface));
        mfx_res = vaToMfxStatus (va_res);

        if (MFX_ERR_NONE == mfx_res) {
          va_res = vaDeriveImage (mVaDisplayHandle, *(vaapi_mid->mSurface), &(vaapi_mid->mImage));
          mfx_res = vaToMfxStatus (va_res);
          }

        if (MFX_ERR_NONE == mfx_res) {
          va_res = vaMapBuffer (mVaDisplayHandle, vaapi_mid->mImage.buf, (void**)&pBuffer);
          mfx_res = vaToMfxStatus (va_res);
          }

        if (MFX_ERR_NONE == mfx_res) {
          switch (vaapi_mid->mImage.format.fourcc) {
            //{{{
            case VA_FOURCC_NV12:
              if (vaapi_mid->mFourcc == MFX_FOURCC_NV12) {
                ptr->Pitch = (mfxU16) vaapi_mid->mImage.pitches[0];
                ptr->Y = pBuffer + vaapi_mid->mImage.offsets[0];
                ptr->U = pBuffer + vaapi_mid->mImage.offsets[1];
                ptr->V = ptr->U + 1;
                }
              else
                mfx_res = MFX_ERR_LOCK_MEMORY;
              break;
            //}}}
            //{{{
            case VA_FOURCC_YV12:
              if (vaapi_mid->mFourcc == MFX_FOURCC_YV12) {
                ptr->Pitch = (mfxU16) vaapi_mid->mImage.pitches[0];
                ptr->Y = pBuffer + vaapi_mid->mImage.offsets[0];
                ptr->V = pBuffer + vaapi_mid->mImage.offsets[1];
                ptr->U = pBuffer + vaapi_mid->mImage.offsets[2];
                }
              else
                 mfx_res = MFX_ERR_LOCK_MEMORY;
              break;
            //}}}
            //{{{
            case VA_FOURCC_YUY2:
              if (vaapi_mid->mFourcc == MFX_FOURCC_YUY2) {
                ptr->Pitch = (mfxU16) vaapi_mid->mImage.pitches[0];
                ptr->Y = pBuffer + vaapi_mid->mImage.offsets[0];
                ptr->U = ptr->Y + 1;
                ptr->V = ptr->Y + 3;
                }
              else
                 mfx_res = MFX_ERR_LOCK_MEMORY;
              break;
            //}}}
            //{{{
            case VA_FOURCC_ARGB:
              if (vaapi_mid->mFourcc == MFX_FOURCC_RGB4) {
                ptr->Pitch = (mfxU16) vaapi_mid->mImage.pitches[0];
                ptr->B = pBuffer + vaapi_mid->mImage.offsets[0];
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

      (void)pthis;
      vaapiMemId* vaapi_mid = (vaapiMemId*)mid;

      if (!vaapi_mid || !(vaapi_mid->mSurface))
        return MFX_ERR_INVALID_HANDLE;

      if (MFX_FOURCC_P8 == vaapi_mid->mFourcc)
        // bitstream processing
        vaUnmapBuffer (mVaDisplayHandle, *(vaapi_mid->mSurface));
      else {
        // Image processing
        vaUnmapBuffer (mVaDisplayHandle, vaapi_mid->mImage.buf);
        vaDestroyImage (mVaDisplayHandle, vaapi_mid->mImage.image_id);

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
      vaapiMemId* vaapi_mid = (vaapiMemId*)mid;

      if (!handle || !vaapi_mid || !(vaapi_mid->mSurface))
        return MFX_ERR_INVALID_HANDLE;

      *handle = vaapi_mid->mSurface; //VASurfaceID* <-> mfxHDL
      return MFX_ERR_NONE;
      }
    //}}}

    // create
    //{{{
    int getDrmDriverName (int fd, char* name, int name_size) {

      drm_version_t version = {};
      version.name_len = name_size;
      version.name = name;
      return ioctl (fd, DRM_IOWR (0, drm_version), &version);
      }
    //}}}
    //{{{
    int openIntelAdapter() {

      constexpr uint32_t DRI_MAX_NODES_NUM = 16;
      constexpr uint32_t DRI_RENDER_START_INDEX = 128;
      constexpr  uint32_t DRM_DRIVER_NAME_LEN = 4;

      const char* DRI_PATH = "/dev/dri/";
      const char* DRI_NODE_RENDER = "renderD";
      const char* DRM_INTEL_DRIVER_NAME = "i915";

      string adapterPath = DRI_PATH;
      adapterPath += DRI_NODE_RENDER;

      char driverName[DRM_DRIVER_NAME_LEN + 1] = {};
      mfxU32 nodeIndex = DRI_RENDER_START_INDEX;

      for (mfxU32 i = 0; i < DRI_MAX_NODES_NUM; ++i) {
        string curAdapterPath = adapterPath + to_string(nodeIndex + i);

        int fd = open (curAdapterPath.c_str(), O_RDWR);
        if (fd < 0)
          continue;
        if (!getDrmDriverName (fd, driverName, DRM_DRIVER_NAME_LEN) &&
            !strcmp (driverName, DRM_INTEL_DRIVER_NAME)) {
          return fd;
          }
        close(fd);
        }

      return -1;
      }
    //}}}
    //{{{
    mfxStatus createVAEnvDRM (mfxHDL* displayHandle) {

      mfxStatus status = MFX_ERR_NONE;

      mFd = openIntelAdapter();
      if (mFd < 0)
        status = MFX_ERR_NOT_INITIALIZED;

      if (MFX_ERR_NONE == status) {
        mVaDisplayHandle = vaGetDisplayDRM (mFd);
        *displayHandle = mVaDisplayHandle;
        if (!mVaDisplayHandle) {
          close (mFd);
          status = MFX_ERR_NULL_PTR;
          }
        }

      if (MFX_ERR_NONE == status) {
        int major_version = 0;
        int minor_version = 0;
        status = vaToMfxStatus (vaInitialize (mVaDisplayHandle, &major_version, &minor_version));
        if (MFX_ERR_NONE != status) {
          close (mFd);
          mFd = -1;
          }
        }

      if (MFX_ERR_NONE != status)
        throw bad_alloc();

      return MFX_ERR_NONE;
      }
    //}}}

    // interface
    //{{{
    mfxStatus mfxInitialize (mfxIMPL mfxImpl, mfxVersion mfxVersion, MFXVideoSession& session, mfxFrameAllocator* mfxAllocator) {

      mfxStatus status = session.Init (mfxImpl, &mfxVersion);
      if (status != MFX_ERR_NONE)
        return status;

      // Create VA display
      mfxHDL displayHandle = {0};
      status = createVAEnvDRM (&displayHandle);
      if (status != MFX_ERR_NONE)
        cLog::log (LOGERROR, "CreateHWDevice failed " + getMfxStatusString (status));

      // provide VA display handle to MediaSDK
      status = session.SetHandle (static_cast <mfxHandleType>(MFX_HANDLE_VA_DISPLAY), displayHandle);
      if (status != MFX_ERR_NONE)
        cLog::log (LOGERROR, "CreateHWDevice failed " + getMfxStatusString (status));

      // If mfxFrameAllocator is provided it means we need to setup  memory allocator
      if (mfxAllocator) {
        // use MediaSDK session ID as the allocation identifier
        mfxAllocator->pthis = session;
        mfxAllocator->Alloc = simpleAlloc;
        mfxAllocator->Free = simpleFree;
        mfxAllocator->Lock = simpleLock;
        mfxAllocator->Unlock = simpleUnlock;
        mfxAllocator->GetHDL = simpleGethdl;

        // using vidMem, set MediaSDK external allocator
        status = session.SetFrameAllocator (mfxAllocator);
        if (status != MFX_ERR_NONE)
          cLog::log (LOGERROR, "CreateHWDevice failed " + getMfxStatusString (status));
        }

      return status;
      }
    //}}}
    //{{{
    void mfxRelease() {

      if (mVaDisplayHandle)
        vaTerminate (mVaDisplayHandle);

      if (mFd >= 0)
        close (mFd);
      }
    //}}}
    //}}}
  #endif
  }

constexpr uint32_t kVideoPoolSize = 50;
//{{{
class cVideoFrame : public cFrame {
public:
  // factory create
  static cVideoFrame* createVideoFrame (int64_t pts, int64_t ptsDuration);

  cVideoFrame (int64_t pts, int64_t ptsDuration) : cFrame (pts, ptsDuration) {}
  //{{{
  virtual ~cVideoFrame() {

    #ifdef _WIN32
      _aligned_free (mPixels);
    #else
      free (mPixels);
    #endif
    }
  //}}}

  virtual void init (uint16_t width, uint16_t height, uint16_t stride, uint32_t pesSize, int64_t decodeTime) = 0;

  // gets
  uint16_t getWidth() const {return mWidth; }
  uint16_t getHeight() const {return mHeight; }
  uint8_t* getPixels() const { return (uint8_t*)mPixels; }

  uint32_t getPesSize() const { return mPesSize; }
  int64_t getDecodeTime() const { return mDecodeTime; }
  int64_t getYuvRgbTime() const { return mYuvRgbTime; }

  // sets
  virtual void setNv12 (uint8_t* nv12) = 0;
  virtual void setY (void* context, uint8_t** data, int* linesize)  = 0;
  virtual void setYuv420 (void* context, uint8_t** data, int* linesize)  = 0;
  void setYuvRgbTime (int64_t time) { mYuvRgbTime = time; }

protected:
  uint16_t mWidth = 0;
  uint16_t mHeight = 0;
  uint16_t mStride = 0;
  uint32_t* mPixels = nullptr;
  uint32_t mPesSize = 0;
  int64_t mDecodeTime = 0;
  int64_t mYuvRgbTime = 0;
  };
//}}}

//{{{
class cMfxVideoDecoder : public cDecoder {
public:
  //{{{
  cMfxVideoDecoder (uint8_t streamType, uint16_t decoderMask)
     : cDecoder(), mDecoderMask(decoderMask) {

    cLog::log (LOGINFO, fmt::format ("cMfxVideoDecoder stream:{}", streamType));

    mfxStatus status = mfxInitialize (MFX_IMPL_HARDWARE, {{0,1}}, mMfxSession, &mMfxAllocator);
    if (status != MFX_ERR_NONE)
      cLog::log (LOGERROR, fmt::format ("cMfxVideoDecoder Initialize failed {} {}",
                                        streamType, getMfxStatusString (status)));

    // query selected implementation and version
    mfxIMPL mfxImpl;
    status = mMfxSession.QueryIMPL (&mfxImpl);
    if (status != MFX_ERR_NONE)
      cLog::log (LOGERROR, "QueryIMPL failed " + getMfxStatusString (status));

    mfxVersion mfxVersion;
    status = mMfxSession.QueryVersion (&mfxVersion);
    if (status != MFX_ERR_NONE)
      cLog::log (LOGERROR, "QueryVersion failed " + getMfxStatusString (status));
    cLog::log (LOGINFO, getMfxInfoString (mfxImpl, mfxVersion));

    mH264 = (streamType == 27);
    mMfxVideoParams.mfx.CodecId = mH264 ? MFX_CODEC_AVC : MFX_CODEC_MPEG2;

    if (getVidMem())
      mMfxVideoParams.IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    else
      mMfxVideoParams.IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    }
  //}}}
  //{{{
  virtual ~cMfxVideoDecoder() {

    MFXVideoDECODE_Close (mMfxSession);
    mMfxSession.Close();

    for (auto& surface : mMfxSurfaces)
      delete surface.Data.Y;

    mMfxSurfaces.clear();
    mfxRelease();
    }
  //}}}

  //{{{
  virtual int64_t decode (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts,
                          function<void (cFrame* frame)> addFrameCallback) final {

    (void)dts;

    if (!mGotIframe) {
      //{{{  skip until first Iframe
      char frameType = cDvbUtils::getFrameType (pes, pesSize, mH264);
      if (frameType == 'I')
        mGotIframe = true;
      else {
        cLog::log (LOGINFO, fmt::format ("skip:{} {} {} size:{}",
                                         frameType, getPtsString (pts), getPtsString (dts), pesSize));
        return pts;
        }
      }
      //}}}

    // init bitstream
    mfxBitstream bitstream = {0};
    bitstream = {0};
    bitstream.Data = pes;
    bitstream.DataLength = pesSize;
    bitstream.MaxLength = pesSize;
    bitstream.TimeStamp = pts;

    if (!mNumSurfaces) {
      //{{{  decodeHeader, query surfaces, extract width,height,numSurfaces, allocate surfaces, initDecoder
      mfxStatus status = MFXVideoDECODE_DecodeHeader (mMfxSession, &bitstream, &mMfxVideoParams);
      if (status != MFX_ERR_NONE) {
        //{{{  return on error
        cLog::log (LOGERROR, "MFXVideoDECODE_DecodeHeader failed " + getMfxStatusString (status));
        return pts;
        }
        //}}}

      mfxFrameAllocRequest frameAllocRequest = {0};
      status =  MFXVideoDECODE_QueryIOSurf (mMfxSession, &mMfxVideoParams, &frameAllocRequest);
      if ((status < MFX_ERR_NONE) || !frameAllocRequest.NumFrameSuggested) {
        //{{{  return on error
        cLog::log (LOGERROR, "MFXVideoDECODE_QueryIOSurf failed " + getMfxStatusString (status));
        return pts;
        }
        //}}}

      // align to 32 pixel boundaries
      mWidth = (frameAllocRequest.Info.Width+31) & (~(mfxU16)31);
      mHeight = (frameAllocRequest.Info.Height+31) & (~(mfxU16)31);
      mNumSurfaces = frameAllocRequest.NumFrameSuggested;

      mfxFrameAllocResponse frameAllocResponse = {0};
      if (getVidMem()) {
        // call allocater to allocate vidMem surfaces, all surfaces in one call
        frameAllocRequest.Type |= 0x1000; // WILL_READ windows d3d11 only
        status = mMfxAllocator.Alloc (mMfxAllocator.pthis, &frameAllocRequest, &frameAllocResponse);
        if (status != MFX_ERR_NONE) {
          //{{{  return on error
          cLog::log (LOGERROR, "Alloc failed - " + getMfxStatusString (status));
          return pts;
          }
          //}}}
        }

      for (size_t i = 0; i < mNumSurfaces; i++) {
        mMfxSurfaces.push_back (mfxFrameSurface1());
        memset (&mMfxSurfaces[i], 0, sizeof(mfxFrameSurface1));
        mMfxSurfaces[i].Info = mMfxVideoParams.mfx.FrameInfo;

        if (getVidMem())
          mMfxSurfaces[i].Data.MemId = frameAllocResponse.mids[i]; // use mId
        else {
          size_t nv12SizeLuma = mWidth * mHeight;
          size_t nv12SizeAll = (nv12SizeLuma * 12) / 8;

          // alloc sysMem surfaces, NV12, planar y followed by planar u, planar v
          mMfxSurfaces[i].Data.Y = new mfxU8[nv12SizeAll];
          mMfxSurfaces[i].Data.U = mMfxSurfaces[i].Data.Y + nv12SizeLuma;
          mMfxSurfaces[i].Data.V = mMfxSurfaces[i].Data.U + 1;
          mMfxSurfaces[i].Data.Pitch = mWidth;
          }
        }

      cLog::log (LOGINFO, fmt::format ("sysMem surfaces allocated {}x{} {}", mWidth, mHeight, mNumSurfaces));

      // init decoder
      status = MFXVideoDECODE_Init (mMfxSession, &mMfxVideoParams);
      if (status != MFX_ERR_NONE) {
        //{{{  return on error
        cLog::log (LOGERROR, "MFXVideoDECODE_Init failed " + getMfxStatusString (status));
        return pts;
        }
        //}}}
      }
      //}}}

    int surfaceIndex = 0;
    mfxStatus status = MFX_ERR_NONE;
    while ((status >= MFX_ERR_NONE) || (status == MFX_ERR_MORE_SURFACE)) {
      if (status == MFX_WRN_DEVICE_BUSY) {
        cLog::log (LOGINFO, "MFX decode - MFX_WRN_DEVICE_BUSY ");
        this_thread::sleep_for (1ms);
        }
      if ((status == MFX_ERR_MORE_SURFACE) || (status == MFX_ERR_NONE)) // Find free frame surface
        surfaceIndex = getFreeSurfaceIndex();

      auto timePoint = chrono::system_clock::now();

      mfxFrameSurface1* surface = nullptr;
      mfxSyncPoint decodeSyncPoint = nullptr;
      status = MFXVideoDECODE_DecodeFrameAsync (
        mMfxSession, &bitstream, &mMfxSurfaces[surfaceIndex], &surface, &decodeSyncPoint);
      if (status == MFX_ERR_NONE) {
        status = mMfxSession.SyncOperation (decodeSyncPoint, 60000);
        int64_t decodeTime = chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - timePoint).count();

        cVideoFrame* videoFrame = cVideoFrame::createVideoFrame (surface->Data.TimeStamp, 90000/25);

        // copy from output surface to videoFrame rgba pixels
        timePoint = chrono::system_clock::now();
        if (getVidMem()) {
          //{{{  lock surface
          status = mMfxAllocator.Lock (mMfxAllocator.pthis, surface->Data.MemId, &(surface->Data));
          if (status != MFX_ERR_NONE)
            cLog::log (LOGERROR, "Unlock failed - " + getMfxStatusString (status));
          }
          //}}}
        videoFrame->init (mWidth, mHeight, surface->Data.Pitch, pesSize, decodeTime);
        videoFrame->setNv12 (surface->Data.Y);
        if (getVidMem()) {
          //{{{  unlock surface
          status = mMfxAllocator.Unlock (mMfxAllocator.pthis, surface->Data.MemId, &(surface->Data));
          if (status != MFX_ERR_NONE)
            cLog::log (LOGERROR, "Unlock failed - " + getMfxStatusString (status));
          }
          //}}}
        videoFrame->setYuvRgbTime (
          chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - timePoint).count());

        addFrameCallback (videoFrame);

        pts += videoFrame->getPtsDuration();
        }
      }

    return pts;
    }
  //}}}

private:
  bool getVidMem() const { return mDecoderMask & cRender::kVidMem; }
  //{{{
  int getFreeSurfaceIndex() {

    for (mfxU16 surfaceIndex = 0; surfaceIndex < mNumSurfaces; surfaceIndex++)
      if (!mMfxSurfaces[surfaceIndex].Data.Locked)
        return surfaceIndex;

    return MFX_ERR_NOT_FOUND;
    }
  //}}}

  const uint16_t mDecoderMask;

  MFXVideoSession mMfxSession;
  mfxVideoParam mMfxVideoParams = {0};
  mfxFrameAllocator mMfxAllocator = {0};

  mfxU32 mCodecId = MFX_CODEC_AVC;
  mfxU16 mWidth = 0;
  mfxU16 mHeight = 0;

  mfxU16 mNumSurfaces = 0;
  vector <mfxFrameSurface1> mMfxSurfaces;

  bool mH264 = false;
  bool mGotIframe = false;
  };
//}}}
//{{{
class cFFmpegVideoDecoder : public cDecoder {
public:
  //{{{
  cFFmpegVideoDecoder (uint8_t streamType, uint16_t decoderMask)
     : cDecoder(), mDecoderMask(decoderMask),
       mAvCodec (avcodec_find_decoder ((streamType == 27) ? AV_CODEC_ID_H264 : AV_CODEC_ID_MPEG2VIDEO)) {

    cLog::log (LOGINFO, fmt::format ("cFFmpegVideoDecoder stream:{}", streamType));

    mAvParser = av_parser_init ((streamType == 27) ? AV_CODEC_ID_H264 : AV_CODEC_ID_MPEG2VIDEO);
    mAvContext = avcodec_alloc_context3 (mAvCodec);
    avcodec_open2 (mAvContext, mAvCodec, NULL);

    mH264 = (streamType == 27) ;
    }
  //}}}
  //{{{
  virtual ~cFFmpegVideoDecoder() {

    if (mAvContext)
      avcodec_close (mAvContext);
    if (mAvParser)
      av_parser_close (mAvParser);
    if (mSwsContext)
      sws_freeContext (mSwsContext);
    }
  //}}}

  //{{{
  virtual int64_t decode (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts,
                  function<void (cFrame* frame)> addFrameCallback) final {

    char frameType = cDvbUtils::getFrameType (pes, pesSize, mH264);
    if (frameType == 'I') {
      //{{{  pts seems wrong, frames decode in presentation order, only correct on Iframe
      if ((mInterpolatedPts != -1) && (mInterpolatedPts != dts))
        cLog::log (LOGERROR, fmt::format ("lost:{} pts:{} dts:{} size:{}",
                                          frameType, getPtsString (pts), getPtsString (dts), pesSize));
      mInterpolatedPts = dts;
      mGotIframe = true;
      }
      //}}}
    if (!mGotIframe) {
      //{{{  skip until first Iframe
      cLog::log (LOGINFO, fmt::format ("skip:{} pts:{} dts:{} size:{}",
                                       frameType, getPtsString (pts), getPtsString (dts), pesSize));
      return pts;
      }
      //}}}

    AVPacket* avPacket = av_packet_alloc();
    AVFrame* avFrame = av_frame_alloc();
    uint8_t* frame = pes;
    uint32_t frameSize = pesSize;
    while (frameSize) {
      auto timePoint = chrono::system_clock::now();
      int bytesUsed = av_parser_parse2 (mAvParser, mAvContext, &avPacket->data, &avPacket->size,
                                        frame, (int)frameSize, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
      if (avPacket->size) {
        int ret = avcodec_send_packet (mAvContext, avPacket);
        while (ret >= 0) {
          ret = avcodec_receive_frame (mAvContext, avFrame);
          if ((ret == AVERROR(EAGAIN)) || (ret == AVERROR_EOF) || (ret < 0))
            break;
          int64_t decodeTime =
            chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - timePoint).count();

          // create new videoFrame
          cVideoFrame* videoFrame = cVideoFrame::createVideoFrame (mInterpolatedPts,
            (kPtsPerSecond * mAvContext->framerate.den) / mAvContext->framerate.num);
          videoFrame->init (static_cast<uint16_t>(avFrame->width),
                            static_cast<uint16_t>(avFrame->height),
                            static_cast<uint16_t>(avFrame->width),
                            frameSize, decodeTime);
          mInterpolatedPts += videoFrame->getPtsDuration();

          // yuv420 -> rgba
          timePoint = chrono::system_clock::now();
          if (!mSwsContext)
            //{{{  init swsContext with known width,height
            mSwsContext = sws_getContext (videoFrame->getWidth(), videoFrame->getHeight(), AV_PIX_FMT_YUV420P,
                                          videoFrame->getWidth(), videoFrame->getHeight(), AV_PIX_FMT_RGBA,
                                          SWS_BILINEAR, NULL, NULL, NULL);
            //}}}
          if (getShaderYuv())
            videoFrame->setY (mSwsContext, avFrame->data, avFrame->linesize);
          else
            videoFrame->setYuv420 (mSwsContext, avFrame->data, avFrame->linesize);
          videoFrame->setYuvRgbTime (
            chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - timePoint).count());
          av_frame_unref (avFrame);  // ??? nnn ??? 

          // add videoFrame
          addFrameCallback (videoFrame);
          }
        }
      frame += bytesUsed;
      frameSize -= bytesUsed;
      }

    av_frame_free (&avFrame);
    av_packet_free (&avPacket);
    return mInterpolatedPts;
    }
  //}}}

private:
  bool getShaderYuv() const { return mDecoderMask & cRender::kShaderYuv; }

  const uint16_t mDecoderMask;
  const AVCodec* mAvCodec = nullptr;

  AVCodecParserContext* mAvParser = nullptr;
  AVCodecContext* mAvContext = nullptr;
  SwsContext* mSwsContext = nullptr;

  bool mH264 = false;
  bool mGotIframe = false;
  int64_t mInterpolatedPts = -1;
  };
//}}}

// cVideoRender
//{{{
cVideoRender::cVideoRender (const string name, uint8_t streamType, uint16_t decoderMask)
    : cRender(name, streamType, decoderMask), mMaxPoolSize(kVideoPoolSize) {

  if (getMfx())
    mDecoder = new cMfxVideoDecoder (streamType, decoderMask);
  else if (getFFmpeg())
    mDecoder = new cFFmpegVideoDecoder (streamType, decoderMask);
  else
    cLog::log (LOGERROR, fmt::format ("cVideoRender - no decoder {:x}", decoderMask));
  }
//}}}
//{{{
cVideoRender::~cVideoRender() {

  for (auto frame : mFrames)
    delete frame.second;
  mFrames.clear();
  }
//}}}

//{{{
string cVideoRender::getInfoString() const {
  return fmt::format ("{} {}x{} {:5d}:{}", mFrames.size(), mWidth, mHeight, mDecodeTime, mYuvRgbTime);
  }
//}}}
//{{{
cTexture* cVideoRender::getTexture (int64_t playPts, cGraphics& graphics) {

  // locked
  shared_lock<shared_mutex> lock (mSharedMutex);

  if (playPts != mTexturePts) {
    // new pts to display
    uint8_t* pixels = nullptr;

    if (mPtsDuration > 0) {
      auto it = mFrames.find (playPts / mPtsDuration);
      if (it != mFrames.end()) // match found
        pixels =  (*it).second->getPixels();
      }

    if (!pixels) {
     // match notFound, try first
     auto it = mFrames.begin();
      if (it != mFrames.end())
        pixels = (*it).second->getPixels();
      }

    if (pixels) {
      if (mTexture == nullptr) // create
        mTexture = graphics.createTexture (getShaderYuv() ? 1 : 0, {getWidth(), getHeight()}, pixels);
      else
        mTexture->setPixels (pixels);
      mTexturePts = playPts;
      }
    }

  return mTexture;
  }
//}}}

//{{{
void cVideoRender::addFrame (cVideoFrame* frame) {

  // locked
  unique_lock<shared_mutex> lock (mSharedMutex);

  mFrames.emplace(frame->getPts() / frame->getPtsDuration(), frame);

  if (mFrames.size() >= mMaxPoolSize) {
    // delete youngest frame
    auto it = mFrames.begin();
    auto frameToDelete = (*it).second;
    it = mFrames.erase (it);
    delete frameToDelete;
    }
  }
//}}}
//{{{
void cVideoRender::processPes (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts,  bool skip) {

  (void)pts;
  (void)skip;
  //log ("pes", fmt::format ("pts:{} size:{}", getFullPtsString (pts), pesSize));
  //logValue (pts, (float)pesSize);

  mDecoder->decode (pes, pesSize, pts, dts, [&](cFrame* frame) noexcept {
    // addFrame lambda
    cVideoFrame* videoFrame = dynamic_cast<cVideoFrame*>(frame);
    mWidth = videoFrame->getWidth();
    mHeight = videoFrame->getHeight();
    mLastPts = videoFrame->getPts();
    mPtsDuration = videoFrame->getPtsDuration();
    mDecodeTime = videoFrame->getDecodeTime();
    mYuvRgbTime = videoFrame->getYuvRgbTime();

    addFrame (videoFrame);
    logValue (videoFrame->getPts(), (float)mDecodeTime);
    });
  }
//}}}

// cVideoFrame factory classes
//{{{
class cVideoFramePlanarRgba : public cVideoFrame {
public:
  cVideoFramePlanarRgba (int64_t pts, int64_t ptsDuration) : cVideoFrame(pts, ptsDuration) {}
  //{{{
  virtual ~cVideoFramePlanarRgba() {
    #ifdef _WIN32
      _aligned_free (mYbuf);
      _aligned_free (mUbuf);
      _aligned_free (mVbuf);
    #else
      free (mYbuf);
      free (mUbuf);
      free (mVbuf);
    #endif
    }
  //}}}

  //{{{
  virtual void init (uint16_t width, uint16_t height, uint16_t stride, uint32_t pesSize, int64_t decodeTime) final {

    mWidth = width;
    mHeight = height;
    mStride = stride;

    mPesSize = pesSize;
    mDecodeTime = decodeTime;

    mYStride = mStride;
    mUVStride = mStride/2;

    mArgbStride = mWidth;

    #ifdef _WIN32
      if (!mPixels)
        mPixels = (uint32_t*)_aligned_malloc (mWidth * mHeight * 4, 128);
      if (!mYbuf)
        mYbuf = (uint8_t*)_aligned_malloc (mHeight * mYStride * 3 / 2, 128);
      if (!mUbuf)
        mUbuf = (uint8_t*)_aligned_malloc ((mHeight/2) * mUVStride, 128);
      if (!mVbuf)
        mVbuf = (uint8_t*)_aligned_malloc ((mHeight/2) * mUVStride, 128);
    #else
      if (!mPixels)
        mPixels = (uint32_t*)aligned_alloc (128, mWidth * mHeight * 4);
      if (!mYbuf)
        mYbuf = (uint8_t*)aligned_alloc (128, mHeight * mYStride * 3 / 2);
      if (!mUbuf)
        mUbuf = (uint8_t*)aligned_alloc (128, (mHeight/2) * mUVStride);
      if (!mVbuf)
        mVbuf = (uint8_t*)aligned_alloc (128, (mHeight/2) * mUVStride);
    #endif
    }
  //}}}

  //{{{
  virtual void setNv12 (uint8_t* nv12) final {

    // copy y of nv12 to y
    memcpy (mYbuf, nv12, mHeight * mYStride * 3 / 2);

    // copy u and v of nv12 to planar uv
    uint8_t* uv = mYbuf + (mHeight * mYStride);
    uint8_t* u = mUbuf;
    uint8_t* v = mVbuf;
    for (int i = 0; i < (mHeight/2) * mUVStride; i++) {
      *u++ = *uv++;
      *v++ = *uv++;
      }

    // constants
    __m128i ysub  = _mm_set1_epi32 (0x00100010);
    __m128i uvsub = _mm_set1_epi32 (0x00800080);
    __m128i facy  = _mm_set1_epi32 (0x004a004a);
    __m128i facrv = _mm_set1_epi32 (0x00660066);
    __m128i facgu = _mm_set1_epi32 (0x00190019);
    __m128i facgv = _mm_set1_epi32 (0x00340034);
    __m128i facbu = _mm_set1_epi32 (0x00810081);
    __m128i zero  = _mm_set1_epi32 (0x00000000);
    __m128i opaque = _mm_set1_epi32 (0xFFFFFFFF);

    for (uint16_t y = 0; y < mHeight; y += 2) {
      __m128i* srcy128r0 = (__m128i *)(mYbuf + (mYStride * y));
      __m128i* srcy128r1 = (__m128i *)(mYbuf + (mYStride * y) + mYStride);
      __m64* srcu64 = (__m64 *)(mUbuf + mUVStride * (y/2));
      __m64* srcv64 = (__m64 *)(mVbuf + mUVStride * (y/2));

      __m128i* dstrgb128r0 = (__m128i *)(mPixels + (mArgbStride * y));
      __m128i* dstrgb128r1 = (__m128i *)(mPixels + (mArgbStride * y) + mArgbStride);

      for (uint16_t x = 0; x < mWidth; x += 16) {
        __m128i u0 = _mm_loadl_epi64 ((__m128i*)srcu64); srcu64++;
        __m128i v0 = _mm_loadl_epi64 ((__m128i*)srcv64); srcv64++;

        __m128i y0r0 = _mm_load_si128( srcy128r0++ );
        __m128i y0r1 = _mm_load_si128( srcy128r1++ );

        // constant y factors
        __m128i y00r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (y0r0, zero), ysub), facy);
        __m128i y01r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (y0r0, zero), ysub), facy);
        __m128i y00r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (y0r1, zero), ysub), facy);
        __m128i y01r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (y0r1, zero), ysub), facy);

        // expand u and v so they're aligned with y values
        u0 = _mm_unpacklo_epi8 (u0, zero);
        __m128i u00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (u0, u0), uvsub);
        __m128i u01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (u0, u0), uvsub);

        v0 = _mm_unpacklo_epi8 (v0,  zero);
        __m128i v00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (v0, v0), uvsub);
        __m128i rv00 = _mm_mullo_epi16 (facrv, v00);
        __m128i gv00 = _mm_mullo_epi16 (facgv, v00);
        __m128i gu00 = _mm_mullo_epi16 (facgu, u00);
        __m128i bu00 = _mm_mullo_epi16 (facbu, u00);

        __m128i v01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (v0, v0), uvsub);
        __m128i rv01 = _mm_mullo_epi16 (facrv, v01);
        __m128i gv01 = _mm_mullo_epi16 (facgv, v01);
        __m128i gu01 = _mm_mullo_epi16 (facgu, u01);
        __m128i bu01 = _mm_mullo_epi16 (facbu, u01);

        // row 0
        __m128i r00 = _mm_srai_epi16 (_mm_add_epi16 (y00r0, rv00), 6);
        __m128i r01 = _mm_srai_epi16 (_mm_add_epi16 (y01r0, rv01), 6);
        r00 = _mm_packus_epi16 (r00, r01);  // rrrr.. saturated

        __m128i g00 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y00r0, gu00), gv00), 6);
        __m128i g01 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y01r0, gu01), gv01), 6);
        g00 = _mm_packus_epi16 (g00, g01);  // gggg.. saturated

        __m128i b00 = _mm_srai_epi16 (_mm_add_epi16 (y00r0, bu00), 6);
        __m128i b01 = _mm_srai_epi16 (_mm_add_epi16 (y01r0, bu01), 6);
        b00 = _mm_packus_epi16 (b00, b01);  // bbbb.. saturated

        __m128i gbgb = _mm_unpacklo_epi8 (r00, g00);       // gbgb..
        r01 = _mm_unpacklo_epi8 (b00, opaque);             // arar..
        __m128i rgb0123 = _mm_unpacklo_epi16 (gbgb, r01);  // argbargb..
        __m128i rgb4567 = _mm_unpackhi_epi16 (gbgb, r01);  // argbargb..

        gbgb = _mm_unpackhi_epi8 (r00, g00 );
        r01  = _mm_unpackhi_epi8 (b00, opaque);
        __m128i rgb89ab = _mm_unpacklo_epi16 (gbgb, r01);
        __m128i rgbcdef = _mm_unpackhi_epi16 (gbgb, r01);

        _mm_stream_si128 (dstrgb128r0++, rgb0123);
        _mm_stream_si128 (dstrgb128r0++, rgb4567);
        _mm_stream_si128 (dstrgb128r0++, rgb89ab);
        _mm_stream_si128 (dstrgb128r0++, rgbcdef);

        // row 1
        r00 = _mm_srai_epi16 (_mm_add_epi16 (y00r1, rv00), 6);
        r01 = _mm_srai_epi16 (_mm_add_epi16 (y01r1, rv01), 6);
        g00 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y00r1, gu00), gv00), 6);
        g01 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y01r1, gu01), gv01), 6);
        b00 = _mm_srai_epi16 (_mm_add_epi16 (y00r1, bu00), 6);
        b01 = _mm_srai_epi16 (_mm_add_epi16 (y01r1, bu01), 6);

        r00 = _mm_packus_epi16 (r00, r01);  // rrrr.. saturated
        g00 = _mm_packus_epi16 (g00, g01);  // gggg.. saturated
        b00 = _mm_packus_epi16 (b00, b01);  // bbbb.. saturated

        gbgb = _mm_unpacklo_epi8 (r00,  g00);     // gbgb..
        r01  = _mm_unpacklo_epi8 (b00,  opaque);  // arar..
        rgb0123 = _mm_unpacklo_epi16 (gbgb, r01); // argbargb..
        rgb4567 = _mm_unpackhi_epi16 (gbgb, r01); // argbargb..

        gbgb = _mm_unpackhi_epi8 (r00, g00);
        r01  = _mm_unpackhi_epi8 (b00, opaque);
        rgb89ab = _mm_unpacklo_epi16 (gbgb, r01);
        rgbcdef = _mm_unpackhi_epi16 (gbgb, r01);

        _mm_stream_si128 (dstrgb128r1++, rgb0123);
        _mm_stream_si128 (dstrgb128r1++, rgb4567);
        _mm_stream_si128 (dstrgb128r1++, rgb89ab);
        _mm_stream_si128 (dstrgb128r1++, rgbcdef);
        }
      }
    }
  //}}}

  virtual void setY (void* context, uint8_t** data, int* linesize) final {

    (void)context;

    // copy y of data to pixels
    uint8_t* yBuffer = data[0];
    int yStride = linesize[0];
    memcpy (mPixels, yBuffer, mHeight * yStride);
    }

  #if defined(INTEL_SSE2)
    //{{{
    virtual void setYuv420 (void* context, uint8_t** data, int* linesize) final {
    // intel intrinsics sse2 convert, fast, but sws is same sort of thing may be a bit faster in the loops
      (void)context;

      uint8_t* yBuffer = data[0];
      uint8_t* uBuffer = data[1];
      uint8_t* vBuffer = data[2];

      int yStride = linesize[0];
      int uvStride = linesize[1];

      __m128i ysub  = _mm_set1_epi32 (0x00100010);
      __m128i uvsub = _mm_set1_epi32 (0x00800080);
      __m128i facy  = _mm_set1_epi32 (0x004a004a);
      __m128i facrv = _mm_set1_epi32 (0x00660066);
      __m128i facgu = _mm_set1_epi32 (0x00190019);
      __m128i facgv = _mm_set1_epi32 (0x00340034);
      __m128i facbu = _mm_set1_epi32 (0x00810081);
      __m128i zero  = _mm_set1_epi32 (0x00000000);
      __m128i alpha = _mm_set1_epi32 (0xFFFFFFFF);

      // dst row pointers
      __m128i* dstrgb128r0 = (__m128i*)mPixels;
      __m128i* dstrgb128r1 = (__m128i*)(mPixels + mWidth);

      for (int y = 0; y < mHeight; y += 2) {
        // calc src row pointers
        __m128i* srcY128r0 = (__m128i*)(yBuffer + yStride*y);
        __m128i* srcY128r1 = (__m128i*)(yBuffer + yStride*y + yStride);
        __m64* srcU64 = (__m64*)(uBuffer + uvStride * (y/2));
        __m64* srcV64 = (__m64*)(vBuffer + uvStride * (y/2));

        for (int x = 0; x < mWidth; x += 16) {
          //{{{  process row
          // row01 u = 0.u0 0.u1 0.u2 0.u3 0.u4 0.u5 0.u6 0.u7
          __m128i temp = _mm_unpacklo_epi8 (_mm_loadl_epi64 ((__m128i*)srcU64++), zero);
          // row01 u00 = 0.u0 0.u0 0.u1 0.u1 0.u2 0.u2 0.u3 0.u3
          __m128i u00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (temp, temp), uvsub);
          // row01 u01 = 0.u4 0.u4 0.u5 0.u5 0.u6 0.u6 0.u7 0.u7
          __m128i u01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (temp, temp), uvsub);

          // row01 v
          temp = _mm_unpacklo_epi8 (_mm_loadl_epi64 ((__m128i*)srcV64++), zero); // 0.v0 0.v1 0.v2 0.v3 0.v4 0.v5 0.v6 0.v7
          __m128i v00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (temp, temp), uvsub);
          __m128i v01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (temp, temp), uvsub);

          // row0
          temp = _mm_load_si128 (srcY128r0++);
          __m128i y00r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (temp, zero), ysub), facy);
          __m128i y01r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (temp, zero), ysub), facy);

          __m128i rv00 = _mm_mullo_epi16 (facrv, v00);
          __m128i rv01 = _mm_mullo_epi16 (facrv, v01);
          __m128i r00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r0, rv00), 6),
                                          _mm_srai_epi16 (_mm_add_epi16 (y01r0, rv01), 6)); // rrrr.. saturated

          __m128i gu00 = _mm_mullo_epi16 (facgu, u00);
          __m128i gu01 = _mm_mullo_epi16 (facgu, u01);
          __m128i gv00 = _mm_mullo_epi16 (facgv, v00);
          __m128i gv01 = _mm_mullo_epi16 (facgv, v01);
          __m128i g00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y00r0, gu00), gv00), 6),
                                          _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y01r0, gu01), gv01), 6)); // gggg.. saturated

          __m128i bu00 = _mm_mullo_epi16 (facbu, u00);
          __m128i bu01 = _mm_mullo_epi16 (facbu, u01);
          __m128i b00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r0, bu00), 6),
                                          _mm_srai_epi16 (_mm_add_epi16 (y01r0, bu01), 6)); // bbbb.. saturated

          __m128i abab = _mm_unpacklo_epi8 (b00, alpha);
          __m128i grgr = _mm_unpacklo_epi8 (r00, g00);
          _mm_stream_si128 (dstrgb128r0++, _mm_unpacklo_epi16 (grgr, abab));
          _mm_stream_si128 (dstrgb128r0++, _mm_unpackhi_epi16 (grgr, abab));

          abab = _mm_unpackhi_epi8 (b00, alpha);
          grgr = _mm_unpackhi_epi8 (r00, g00);
          _mm_stream_si128 (dstrgb128r0++, _mm_unpacklo_epi16 (grgr, abab));
          _mm_stream_si128 (dstrgb128r0++, _mm_unpackhi_epi16 (grgr, abab));

          // row1
          temp = _mm_load_si128 (srcY128r1++);
          __m128i y00r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (temp, zero), ysub), facy);
          __m128i y01r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (temp, zero), ysub), facy);

          r00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r1, rv00), 6),
                                  _mm_srai_epi16 (_mm_add_epi16 (y01r1, rv01), 6)); // rrrr.. saturated
          g00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y00r1, gu00), gv00), 6),
                                  _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y01r1, gu01), gv01), 6)); // gggg.. saturated
          b00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r1, bu00), 6),
                                  _mm_srai_epi16 (_mm_add_epi16 (y01r1, bu01), 6)); // bbbb.. saturated

          abab = _mm_unpacklo_epi8 (b00, alpha);
          grgr = _mm_unpacklo_epi8 (r00, g00);
          _mm_stream_si128 (dstrgb128r1++, _mm_unpacklo_epi16 (grgr, abab));
          _mm_stream_si128 (dstrgb128r1++, _mm_unpackhi_epi16 (grgr, abab));

          abab = _mm_unpackhi_epi8 (b00, alpha);
          grgr = _mm_unpackhi_epi8 (r00, g00);
          _mm_stream_si128 (dstrgb128r1++, _mm_unpacklo_epi16 (grgr, abab));
          _mm_stream_si128 (dstrgb128r1++, _mm_unpackhi_epi16 (grgr, abab));
          }
          //}}}

        // skip a dst row
        dstrgb128r0 += mWidth / 4;
        dstrgb128r1 += mWidth / 4;
        }
      }
    //}}}
  #else // ARM NEON
    //{{{
    virtual void setYuv420 (void* context, uint8_t** data, int* linesize) final {
    // ARM NEON covert, maybe slower than sws table lookup

      // constants
      int const heightDiv2 = mHeight / 2;
      int const widthDiv8 = mWidth / 8;

      uint8x8_t const yOffset = vdup_n_u8 (16);
      int16x8_t const uvOffset = vdupq_n_u16 (128);
      int32x4_t const rounding = vdupq_n_s32 (128);

      uint8_t* y0 = (uint8_t*)data[0];
      uint8_t* y1 = (uint8_t*)data[0] + linesize[0];
      uint8_t* u = (uint8_t*)data[1];
      uint8_t* v = (uint8_t*)data[2];
      uint8_t* dst0 = (uint8_t*)mBuffer8888;
      uint8_t* dst1 = (uint8_t*)mBuffer8888 + (mWidth * 4);

      uint8x8x4_t block;
      block.val[3] = vdup_n_u8 (0xFF);

      for (int j = 0; j < heightDiv2; ++j) {
        for (int i = 0; i < widthDiv8; ++i) {
          //{{{  2 rows of 8 pix
          // U load 8 u8 - u01234567 = u0 u1 u2 u3 u4 u5 u6 u7
          uint8x8_t const u01234567 = vld1_u8 (u);
          u += 4;
          // - uu01234567 =  0.u0 0.u1 0.u2 0.u3 0.u4 0.u5 0.u6 0.u7
          int16x8_t const uu01234567 = (int16x8_t)vmovl_u8 (u01234567);
          // - u01234567o = (0.u0 0.u1 0.u2 0.u3 0.u4 0.u5 0.u6 0.u7) - half
          int16x8_t const uu01234567o = vsubq_s16 (uu01234567, uvOffset);
          // - u0123h = 0.u0 0.u1 0.u2 0.u3 - u4567 unused, unwind loop?
          int16x4_t const uu0123o = vget_low_s16 (uu01234567o);

          // V load 8 u8 - v01234567 = v0 v1 v2 v3 v4 v5 v6 v7
          uint8x8_t const v01234567 = vld1_u8 (v);
          v += 4;
          // - vv01234567 =  0.v0 0.v1 0.v2 0.v3 0.v4 0.v5 0.v6 0.v7
          int16x8_t const vv01234567 = (int16x8_t)vmovl_u8 (v01234567);
          // - vv01234567o = (0.v0 0.v1 0.v2 0.v3 0.v4 0.v5 0.v6 0.v7) - half
          int16x8_t const vv01234567o = vsubq_s16 (vv01234567, uvOffset);
          // - vv0123 = 0.v0 0.v1 0.v2 0.v3 - v4567 unused, unwind loop?
          int16x4_t const vv0123o = vget_low_s16 (vv01234567o);

          // r = 128 + 409v
          int32x4_t const r0123os = vmlal_n_s16 (rounding, uu0123o, 409);
          int32x4x2_t const r00112233os = vzipq_s32 (r0123os, r0123os);
          // g = 128 - 100u - 208v
          int32x4_t const g0123os = vmlal_n_s16 (vmlal_n_s16 (rounding, uu0123o, -208), vv0123o, -100);
          int32x4x2_t const g00112233os = vzipq_s32 (g0123os, g0123os);
          // b = 128 + 516u
          int32x4_t const b0123os = vmlal_n_s16 (rounding, vv0123o, 516);
          int32x4x2_t const b00112233os = vzipq_s32 (b0123os, b0123os);

          // row0
          uint8x8_t const row0y01234567 = vld1_u8 (y0);
          y0 += 8;
          uint8x8_t const row0y01234567o = vqsub_u8 (row0y01234567, yOffset);
          uint16x8_t const row0yy01234567o = vmovl_u8 (row0y01234567o);

          int32x4_t const row0y0123os = vmulq_n_u32 (vmovl_u16 (vget_low_u16 (row0yy01234567o)), 298);
          int32x4_t const row0y4567os = vmulq_n_u32 (vmovl_u16 (vget_high_u16 (row0yy01234567o)), 298);

          block.val[0] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (b00112233os.val[0], row0y0123os)),
                                                    vqmovun_s32 (vaddq_s32 (b00112233os.val[1], row0y4567os))), 8);
          block.val[1] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (g00112233os.val[0], row0y0123os)),
                                                    vqmovun_s32 (vaddq_s32 (g00112233os.val[1], row0y4567os))), 8),
          block.val[2] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (r00112233os.val[0], row0y0123os)),
                                                    vqmovun_s32 (vaddq_s32 (r00112233os.val[1], row0y4567os))), 8),
          vst4_u8 (dst0, block);
          dst0 += 8 * 4;

          // row1
          uint8x8_t const row1y01234567 = vld1_u8 (y1);
          y1 += 8;
          uint8x8_t const row1y01234567o = vqsub_u8 (row1y01234567, yOffset);
          uint16x8_t const row1yy01234567o = vmovl_u8 (row1y01234567o);

          int32x4_t const row1y0123os = vmulq_n_u32 (vmovl_u16 (vget_low_u16 (row1yy01234567o)), 298);
          int32x4_t const row1y4567os = vmulq_n_u32 (vmovl_u16 (vget_high_u16 (row1yy01234567o)), 298);

          block.val[0] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (b00112233os.val[0], row1y0123os)),
                                                    vqmovun_s32 (vaddq_s32 (b00112233os.val[1], row1y4567os))), 8);
          block.val[1] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (g00112233os.val[0], row1y0123os)),
                                                    vqmovun_s32 (vaddq_s32 (g00112233os.val[1], row1y4567os))), 8),
          block.val[2] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (r00112233os.val[0], row1y0123os)),
                                                    vqmovun_s32 (vaddq_s32 (r00112233os.val[1], row1y4567os))), 8),
          vst4_u8 (dst1, block);

          dst1 += 8 * 4;
          }
          //}}}
        y0 += linesize[0];
        y1 += linesize[0];
        }
      }
    //}}}
  #endif

private:
  uint16_t mYStride = 0;
  uint16_t mUVStride = 0;
  uint16_t mArgbStride = 0;

  uint8_t* mYbuf = nullptr;
  uint8_t* mUbuf = nullptr;
  uint8_t* mVbuf = nullptr;
  };
//}}}
//{{{
class cVideoFramePlanarRgbaSws : public cVideoFrame {
public:
  cVideoFramePlanarRgbaSws (int64_t pts, int64_t ptsDuration) : cVideoFrame(pts, ptsDuration) {}
  virtual ~cVideoFramePlanarRgbaSws() = default;

  //{{{
  virtual void init (uint16_t width, uint16_t height, uint16_t stride,
                     uint32_t pesSize, int64_t decodeTime) final {

    mWidth = width;
    mHeight = height;
    mStride = stride;

    mPesSize = pesSize;
    mDecodeTime = decodeTime;

    #ifdef _WIN32
      if (!mPixels)
        // allocate aligned buffer
        mPixels = (uint32_t*)_aligned_malloc (width * height * 4, 128);
    #else
      if (!mPixels)
        // allocate aligned buffer
        mPixels = (uint32_t*)aligned_alloc (128, width * height * 4);
    #endif
    }
  //}}}

  virtual void setY (void* context, uint8_t** data, int* linesize) final {
    (void)context;
    (void)data;
    (void)linesize;
    }

  virtual void setNv12 (uint8_t* nv12) final {
    (void)nv12;
    cLog::log (LOGERROR, "cVideoFramePlanarRgbaSws setNv12 unimplemented");
    }

  virtual void setYuv420 (void* context, uint8_t** data, int* linesize) final {
    // ffmpeg libswscale convert data to mPixels using swsContext
    uint8_t* dstData[1] = { (uint8_t*)mPixels };
    int dstStride[1] = { mWidth * 4 };
    sws_scale ((SwsContext*)context, data, linesize, 0, mHeight, dstData, dstStride);
    }
  };
//}}}

//{{{
cVideoFrame* cVideoFrame::createVideoFrame (int64_t pts, int64_t ptsDuration) {

  return new cVideoFramePlanarRgba (pts, ptsDuration);
  //return new cVideoFramePlanarRgbaSws(pts, ptsDuration);
  }
//}}}
