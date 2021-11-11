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
    default: statusString = "Error code";
    }

  return fmt::format ("status {} {}", status, statusString);
  }
//}}}
//{{{
string getMfxInfoString (mfxIMPL mfxImpl, mfxVersion mfxVersion) {

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

  //{{{  MFX_IMPL flags
  // MFX_IMPL_AUTO
  // MFX_IMPL_HARDWARE
  // MFX_IMPL_SOFTWARE
  // MFX_IMPL_AUTO_ANY
  // MFX_IMPL_VIA_D3D9
  // MFX_IMPL_VIA_D3D11
  //}}}
  mfxIMPL mfxImpl = MFX_IMPL_AUTO_ANY | MFX_IMPL_VIA_D3D11;
  mfxVersion mfxVersion = {{0,1}};

  MFXVideoSession mfxSession;
  mfxStatus status = mfxSession.Init (mfxImpl, &mfxVersion);
  //mfxStatus status = Initialize (mfxImpl, mfxVersion, &mfxSession, NULL);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "init failed - " + getMfxStatusString (status));

  // query implementation
  status = mfxSession.QueryIMPL (&mfxImpl);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "init failed - " + getMfxStatusString (status));

  // query version
  status = mfxSession.QueryVersion (&mfxVersion);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "QueryVersion failed " + getMfxStatusString (status));

  cLog::log (LOGINFO, getMfxInfoString (mfxImpl, mfxVersion));

  FILE* srcFile = OpenFile (filename.c_str(), "rb");

  // Create Media SDK decoder
  MFXVideoDECODE mfxDEC (mfxSession);

  // Set required video parameters for decode
  mfxVideoParam mfxVideoParams;
  memset (&mfxVideoParams, 0, sizeof(mfxVideoParams));
  mfxVideoParams.mfx.CodecId = MFX_CODEC_AVC;
  mfxVideoParams.IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

  //  Prepare Media SDK bit stream buffer, Arbitrary buffer size
  mfxBitstream bitStream;
  memset (&bitStream, 0, sizeof(bitStream));
  bitStream.MaxLength = 1024 * 1024;
  std::vector<mfxU8> bstData (bitStream.MaxLength);
  bitStream.Data = bstData.data();

  // read a chunk of data from stream file into bit stream buffer
  // - Parse bit stream, searching for header and fill video parameters structure
  // - Abort if bit stream header is not found in the first bit stream buffer chunk
  status = ReadBitStreamData (&bitStream, srcFile);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "ReadBitStreamData failed - " + getMfxStatusString (status));

  status = mfxDEC.DecodeHeader (&bitStream, &mfxVideoParams);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "DecodeHeader failed - " + getMfxStatusString (status));

  // validate video decode parameters (optional)
  status = mfxDEC.Query (&mfxVideoParams, &mfxVideoParams);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "Query failed - " + getMfxStatusString (status));

  // query decoder surface info
  mfxFrameAllocRequest frameAllocRequest;
  memset (&frameAllocRequest, 0, sizeof(frameAllocRequest));
  status = mfxDEC.QueryIOSurf (&mfxVideoParams, &frameAllocRequest);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "QueryIOSurf failed - " + getMfxStatusString (status));

  // extract width, height, numSurfaces
  mfxU16 width = (mfxU16)MSDK_ALIGN32 (frameAllocRequest.Info.Width);
  mfxU16 height = (mfxU16)MSDK_ALIGN32 (frameAllocRequest.Info.Height);
  mfxU16 numSurfaces = frameAllocRequest.NumFrameSuggested;

  // allocate decoder surfacesr, NV12 format 12bits per pixel
  mfxU8 bitsPerPixel = 12;
  mfxU32 surfaceSize = (width * height * bitsPerPixel) / 8;
  std::vector<mfxU8> surfaceBuffersData (surfaceSize * numSurfaces);
  mfxU8* surfaceBuffers = surfaceBuffersData.data();

  // allocate surface headers (mfxFrameSurface1) for decoder
  std::vector<mfxFrameSurface1> mfxSurfaces (numSurfaces);
  for (int i = 0; i < numSurfaces; i++) {
    memset(&mfxSurfaces[i], 0, sizeof(mfxFrameSurface1));
    mfxSurfaces[i].Info = mfxVideoParams.mfx.FrameInfo;
    mfxSurfaces[i].Data.Y = &surfaceBuffers[surfaceSize * i];
    mfxSurfaces[i].Data.U = mfxSurfaces[i].Data.Y + width * height;
    mfxSurfaces[i].Data.V = mfxSurfaces[i].Data.U + 1;
    mfxSurfaces[i].Data.Pitch = width;
    }
  cLog::log (LOGINFO, fmt::format ("surfcaes allocated ok {}x{} {}", width, height, numSurfaces));

  // init Media SDK decoder
  status = mfxDEC.Init (&mfxVideoParams);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, "Init failed - " + getMfxStatusString (status));

  // Start decoding the frames
  int surfaceIndex = 0;
  mfxU32 frameNumber = 0;

  mfxSyncPoint syncPoint;
  mfxFrameSurface1* mfxOutSurface = NULL;
  while ((status >= MFX_ERR_NONE) || (status == MFX_ERR_MORE_DATA) || (status == MFX_ERR_MORE_SURFACE)) {
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
        cLog::log (LOGINFO, fmt::format ("output1 frame:{}", frameNumber));
    }

  // flush buffer
  while ((status >= MFX_ERR_NONE) || (status == MFX_ERR_MORE_SURFACE)) {
    if (status == MFX_WRN_DEVICE_BUSY)
      this_thread::sleep_for (1ms);
    surfaceIndex = GetFreeSurfaceIndex (mfxSurfaces);

    // decode fram asychronously
    status = mfxDEC.DecodeFrameAsync (NULL, &mfxSurfaces[surfaceIndex], &mfxOutSurface, &syncPoint);
    if ((status >= MFX_ERR_NONE) && syncPoint)
      status = MFX_ERR_NONE;
    if (status == MFX_ERR_NONE)
      status = mfxSession.SyncOperation (syncPoint, 60000);

    if (status == MFX_ERR_NONE) {
      if (!(++frameNumber % 1000))
        cLog::log (LOGINFO, fmt::format ("output2 frame:{}", frameNumber));
      }
    }

  CloseFile (srcFile);
  mfxDEC.Close();
  Release();
  mfxSession.Close();

  this_thread::sleep_for (5000ms);
  return 0;
  }
