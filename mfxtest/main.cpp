// main.cpp - mfxtest
//{{{  includes
#include <cstdint>
#include <vector>
#include <string>
#include <thread>

#include "mfxvideo++.h"
#include "utils.h"

#include "../utils/cLog.h"

using namespace std;
//}}}

//{{{
void sessionTest() {

  cLog::log (LOGINFO, "--- sessionTest ----");

  //{{{  MFX_IMPL flags
  // MFX_IMPL_AUTO
  // MFX_IMPL_HARDWARE
  // MFX_IMPL_SOFTWARE
  // MFX_IMPL_AUTO_ANY
  // MFX_IMPL_VIA_D3D9
  // MFX_IMPL_VIA_D3D11
  //}}}
  mfxIMPL mfxImpl = MFX_IMPL_AUTO_ANY;
  mfxVersion mfxVersion = {{0,1}};

  MFXVideoSession mfxSession;

  //mfxStatus status = Initialize (mfxImpl, mfxVersion, &mfxSession, NULL);
  mfxStatus status = mfxSession.Init (mfxImpl, &mfxVersion);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "init failed - " + getMfxStatusString (status));

  //  query implementation
  status = mfxSession.QueryIMPL (&mfxImpl);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "init failed - " + getMfxStatusString (status));

  //  query version
  status = mfxSession.QueryVersion (&mfxVersion);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "QueryVersion failed " + getMfxStatusString (status));
  cLog::log (LOGINFO, getMfxInfoString (mfxImpl, mfxVersion));

  mfxSession.Close();
  }
//}}}
//{{{
void decodeSysMem (const string& filename) {

  cLog::log (LOGINFO, "--- decodeSysMem ---- " + filename);

  mfxIMPL mfxImpl = MFX_IMPL_AUTO_ANY;
  mfxVersion mfxVersion = {{0,1}};

  // init mfxSession
  MFXVideoSession mfxSession;
  mfxStatus status = mfxSession.Init (mfxImpl, &mfxVersion);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "init failed - " + getMfxStatusString (status));
  //{{{  query implementation
  status = mfxSession.QueryIMPL (&mfxImpl);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "init failed - " + getMfxStatusString (status));
  //}}}
  //{{{  query version
  status = mfxSession.QueryVersion (&mfxVersion);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "QueryVersion failed " + getMfxStatusString (status));
  //}}}
  cLog::log (LOGINFO, getMfxInfoString (mfxImpl, mfxVersion));

  // init hw
  Initialize ( &mfxSession, nullptr, false);

  //{{{  prepare mfx bitStream buffer, Arbitrary buffer size
  mfxBitstream bitStream = {0};

  bitStream.MaxLength = 1024 * 1024;
  std::vector<mfxU8> bstData (bitStream.MaxLength);
  bitStream.Data = bstData.data();

  FILE* srcFile = OpenFile (filename.c_str(), "rb");
  status = ReadBitStreamData (&bitStream, srcFile);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "ReadBitStreamData failed - " + getMfxStatusString (status));
  //}}}

  MFXVideoDECODE mfxDEC (mfxSession);
  //{{{  decodeHeader, set codec,memModel
  mfxVideoParam mfxVideoParams = {0};
  mfxVideoParams.mfx.CodecId = MFX_CODEC_AVC;
  mfxVideoParams.IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

  status = mfxDEC.DecodeHeader (&bitStream, &mfxVideoParams);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "DecodeHeader failed - " + getMfxStatusString (status));
  //}}}
  //{{{  validate video parameters, for encode
  status = mfxDEC.Query (&mfxVideoParams, &mfxVideoParams);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "Query failed - " + getMfxStatusString (status));
  //}}}
  //{{{  query decoder frame surface alloc info
  mfxFrameAllocRequest frameAllocRequest = {0};
  status = mfxDEC.QueryIOSurf (&mfxVideoParams, &frameAllocRequest);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "QueryIOSurf failed - " + getMfxStatusString (status));

  // extract width, height, numSurfaces
  mfxU16 width = (mfxU16)MSDK_ALIGN32 (frameAllocRequest.Info.Width);
  mfxU16 height = (mfxU16)MSDK_ALIGN32 (frameAllocRequest.Info.Height);
  mfxU16 numSurfaces = frameAllocRequest.NumFrameSuggested;
  //}}}

  //{{{  allocate sysMem surfaces, NV12 format 12bits per pixel
  mfxU8 bitsPerPixel = 12;
  mfxU32 surfaceSize = (width * height * bitsPerPixel) / 8;

  std::vector<mfxU8> surfaceBuffersData (surfaceSize * numSurfaces);
  mfxU8* surfaceBuffers = surfaceBuffersData.data();

  // allocate surface headers (mfxFrameSurface1) for decoder
  std::vector<mfxFrameSurface1> mfxSurfaces (numSurfaces);
  for (int i = 0; i < numSurfaces; i++) {
    mfxSurfaces[i] = {0};
    mfxSurfaces[i].Info = mfxVideoParams.mfx.FrameInfo;
    mfxSurfaces[i].Data.Y = &surfaceBuffers[surfaceSize * i];
    mfxSurfaces[i].Data.U = mfxSurfaces[i].Data.Y + width * height;
    mfxSurfaces[i].Data.V = mfxSurfaces[i].Data.U + 1;
    mfxSurfaces[i].Data.Pitch = width;
    }
  //}}}
  cLog::log (LOGINFO, fmt::format ("surfaces allocated ok {}x{} {}", width, height, numSurfaces));

  //  init mediaSDK decoder
  status = mfxDEC.Init (&mfxVideoParams);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "Init failed - " + getMfxStatusString (status));

  int surfaceIndex = 0;
  mfxU32 frameNumber = 0;

  mfxSyncPoint syncPoint;
  mfxFrameSurface1* mfxOutSurface = NULL;
  while ((status >= MFX_ERR_NONE) || (status == MFX_ERR_MORE_SURFACE) || (status == MFX_ERR_MORE_DATA)) {
    //{{{  decode loop
    if (status == MFX_WRN_DEVICE_BUSY)
      this_thread::sleep_for (1ms);

    if (status == MFX_ERR_MORE_DATA) {
      // Read more data into input bit stream
      status = ReadBitStreamData (&bitStream, srcFile);
      if (status != MFX_ERR_NONE) {
        cLog::log (LOGINFO, "ReadBitStreamData failed - " + getMfxStatusString (status));
        break;
        }
      }

    if ((status == MFX_ERR_MORE_SURFACE) || (status == MFX_ERR_NONE)) // Find free frame surface
      surfaceIndex = GetFreeSurfaceIndex (mfxSurfaces);

    // decode frame asychronously
    status = mfxDEC.DecodeFrameAsync (&bitStream, &mfxSurfaces[surfaceIndex], &mfxOutSurface, &syncPoint);
    if ((status >= MFX_ERR_NONE) && syncPoint)
      status = MFX_ERR_NONE;
    if (status == MFX_ERR_NONE)
      status = mfxSession.SyncOperation (syncPoint, 60000);

    if (status == MFX_ERR_NONE)
      if (!(++frameNumber % 1000))
        cLog::log (LOGINFO, fmt::format ("decode frame:{}", frameNumber));
    }
    //}}}
  while ((status >= MFX_ERR_NONE) || (status == MFX_ERR_MORE_SURFACE)) {
    //{{{  flush loop
    if (status == MFX_WRN_DEVICE_BUSY)
      this_thread::sleep_for (1ms);

    // decode frame asychronously
    surfaceIndex = GetFreeSurfaceIndex (mfxSurfaces);
    status = mfxDEC.DecodeFrameAsync (NULL, &mfxSurfaces[surfaceIndex], &mfxOutSurface, &syncPoint);
    if ((status >= MFX_ERR_NONE) && syncPoint)
      status = MFX_ERR_NONE;
    if (status == MFX_ERR_NONE)
      status = mfxSession.SyncOperation (syncPoint, 60000);

    if (status == MFX_ERR_NONE)
      if (!(++frameNumber % 1000))
        cLog::log (LOGINFO, fmt::format ("flush frame:{}", frameNumber));
    }
    //}}}

  CloseFile (srcFile);
  mfxDEC.Close();
  Release();
  }
//}}}
//{{{
void decodeVidMem (const string& filename) {

  cLog::log (LOGINFO, "--- decodeVidMem ----" + filename);

  mfxIMPL mfxImpl = MFX_IMPL_AUTO_ANY;
  mfxVersion mfxVersion = {{0,1}};

  // init mfxSession
  MFXVideoSession mfxSession;
  mfxStatus status = mfxSession.Init (mfxImpl, &mfxVersion);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "init failed - " + getMfxStatusString (status));
  //{{{  query implementation
  status = mfxSession.QueryIMPL (&mfxImpl);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "init failed - " + getMfxStatusString (status));
  //}}}
  //{{{  query version
  status = mfxSession.QueryVersion (&mfxVersion);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "QueryVersion failed " + getMfxStatusString (status));
  //}}}
  cLog::log (LOGINFO, getMfxInfoString (mfxImpl, mfxVersion));

  // init hw
  mfxFrameAllocator mfxAllocator = {0};
  Initialize (&mfxSession, &mfxAllocator, false);

  //{{{  prepare mfx bitStream buffer, Arbitrary buffer size
  mfxBitstream bitStream = {0};

  bitStream.MaxLength = 1024 * 1024;
  std::vector<mfxU8> bstData (bitStream.MaxLength);
  bitStream.Data = bstData.data();

  FILE* srcFile = OpenFile (filename.c_str(), "rb");
  status = ReadBitStreamData (&bitStream, srcFile);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "ReadBitStreamData failed - " + getMfxStatusString (status));
  //}}}

  MFXVideoDECODE mfxDEC (mfxSession);
  //{{{  decodeHeader, set codec memModel
  mfxVideoParam mfxVideoParams = {0};
  mfxVideoParams.mfx.CodecId = MFX_CODEC_AVC;
  mfxVideoParams.IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;

  status = mfxDEC.DecodeHeader (&bitStream, &mfxVideoParams);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "DecodeHeader failed - " + getMfxStatusString (status));
  //}}}
  //{{{  validate video parameters, for encode
  status = mfxDEC.Query (&mfxVideoParams, &mfxVideoParams);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "Query failed - " + getMfxStatusString (status));
  //}}}
  //{{{  query decoder frame surface alloc info
  mfxFrameAllocRequest frameAllocRequest = {0};
  status = mfxDEC.QueryIOSurf (&mfxVideoParams, &frameAllocRequest);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "QueryIOSurf failed - " + getMfxStatusString (status));

  // extract width, height, numSurfaces
  mfxU16 width = (mfxU16)MSDK_ALIGN32 (frameAllocRequest.Info.Width);
  mfxU16 height = (mfxU16)MSDK_ALIGN32 (frameAllocRequest.Info.Height);
  mfxU16 numSurfaces = frameAllocRequest.NumFrameSuggested;
  //}}}
  //{{{  allocate vidMem surfaces
  frameAllocRequest.Type |= WILL_READ; // windows d3d11 only
  mfxFrameAllocResponse frameAllocResponse;
  status = mfxAllocator.Alloc (mfxAllocator.pthis, &frameAllocRequest, &frameAllocResponse);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "Alloc failed - " + getMfxStatusString (status));

  std::vector <mfxFrameSurface1> mfxSurfaces(numSurfaces);
  for (int i = 0; i < numSurfaces; i++) {
    memset (&mfxSurfaces[i], 0, sizeof(mfxFrameSurface1));
    mfxSurfaces[i].Info = mfxVideoParams.mfx.FrameInfo;
    mfxSurfaces[i].Data.MemId = frameAllocResponse.mids[i]; // MID (memory id) represents one video NV12 surface
    }
  cLog::log (LOGINFO, fmt::format ("surfaces allocated ok {}x{} {}", width, height, numSurfaces));
  //}}}

  // init mediaSDK decoder
  status = mfxDEC.Init (&mfxVideoParams);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "Init failed - " + getMfxStatusString (status));

  int surfaceIndex = 0;
  mfxU32 frameNumber = 0;

  mfxSyncPoint syncPoint;
  mfxFrameSurface1* mfxOutSurface = nullptr;
  while ((status >= MFX_ERR_NONE) || (status == MFX_ERR_MORE_SURFACE) || (status == MFX_ERR_MORE_DATA)) {
    //{{{  decode loop
    if (status == MFX_WRN_DEVICE_BUSY)
      this_thread::sleep_for (1ms);

    if (status == MFX_ERR_MORE_DATA) {
      // Read more data into input bit stream
      status = ReadBitStreamData (&bitStream, srcFile);
      if (status != MFX_ERR_NONE) {
        cLog::log (LOGINFO, "ReadBitStreamData failed - " + getMfxStatusString (status));
        break;
        }
      }

    if ((status == MFX_ERR_MORE_SURFACE) || (status == MFX_ERR_NONE)) // Find free frame surface
      surfaceIndex = GetFreeSurfaceIndex (mfxSurfaces);

    // decode frame asychronously
    status = mfxDEC.DecodeFrameAsync (&bitStream, &mfxSurfaces[surfaceIndex], &mfxOutSurface, &syncPoint);
    if ((status >= MFX_ERR_NONE) && syncPoint)
      status = MFX_ERR_NONE;
    if (status == MFX_ERR_NONE)
      status = mfxSession.SyncOperation (syncPoint, 60000);

    if (status == MFX_ERR_NONE) {
      if (!(++frameNumber % 1000))
        cLog::log (LOGINFO, fmt::format ("decode frame:{}", frameNumber));

      status = mfxAllocator.Lock (mfxAllocator.pthis, mfxOutSurface->Data.MemId, &(mfxOutSurface->Data));
      if (status != MFX_ERR_NONE) {
        //{{{  break on error
        cLog::log (LOGINFO, "Lock failed - " + getMfxStatusString (status));
        break;
        }
        //}}}
      //status = WriteRawFrame (mfxOutSurface, fSink.get());
      status = mfxAllocator.Unlock (mfxAllocator.pthis, mfxOutSurface->Data.MemId, &(mfxOutSurface->Data));
      if (status != MFX_ERR_NONE) {
        //{{{  break on error
        cLog::log (LOGINFO, "Unlock failed - " + getMfxStatusString (status));
        break;
        }
        //}}}
      }
    }
    //}}}
  while ((status >= MFX_ERR_NONE) || (status == MFX_ERR_MORE_SURFACE)) {
    //{{{  flush loop
    if (status == MFX_WRN_DEVICE_BUSY)
      this_thread::sleep_for (1ms);

    // decode frame asychronously
    surfaceIndex = GetFreeSurfaceIndex (mfxSurfaces);
    status = mfxDEC.DecodeFrameAsync (NULL, &mfxSurfaces[surfaceIndex], &mfxOutSurface, &syncPoint);
    if ((status >= MFX_ERR_NONE) && syncPoint)
      status = MFX_ERR_NONE;
    if (status == MFX_ERR_NONE)
      status = mfxSession.SyncOperation (syncPoint, 60000);

    if (status == MFX_ERR_NONE) {
      if (!(++frameNumber % 1000))
        cLog::log (LOGINFO, fmt::format ("flush frame:{}", frameNumber));

      status = mfxAllocator.Lock (mfxAllocator.pthis, mfxOutSurface->Data.MemId, &(mfxOutSurface->Data));
      if (status != MFX_ERR_NONE) {
        //{{{  break on error
        cLog::log (LOGINFO, "Lock failed - " + getMfxStatusString (status));
        break;
        }
        //}}}
      //status = WriteRawFrame (mfxOutSurface, fSink.get());
      status = mfxAllocator.Unlock(mfxAllocator.pthis, mfxOutSurface->Data.MemId, &(mfxOutSurface->Data));
      if (status != MFX_ERR_NONE) {
        //{{{  break on error
        cLog::log (LOGINFO, "Unlock failed - " + getMfxStatusString (status));
        break;
        }
        //}}}
      }
    }
    //}}}

  CloseFile (srcFile);
  mfxDEC.Close();
  Release();
  }
//}}}

int main(int numArgs, char** args) {

  eLogLevel logLevel = LOGINFO;
  string filename = "E:/h264/nnn.h264";
  //{{{  parse command line args to params
  // parse params
  for (int i = 1; i < numArgs; i++) {
    string param = args[i];

    if (param == "log1") { logLevel = LOGINFO1; }
    else if (param == "log2") { logLevel = LOGINFO2; }
    else if (param == "log3") { logLevel = LOGINFO3; }
    else
      filename = param;
    }
  //}}}
  cLog::init (logLevel);
  cLog::log (LOGNOTICE, "mfxtest");

  sessionTest();
  decodeVidMem (filename);
  decodeSysMem (filename);

  this_thread::sleep_for (5000ms);
  return 0;
  }
