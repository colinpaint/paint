// decode.cpp
//{{{  includes
#include <vector>
#include <string>
#include <thread>

#include "../libmfx/include/mfxvideo++.h"

#include "../utils/cLog.h"

using namespace std;
//}}}

//{{{
string getMfxStatus (mfxStatus status) {

  switch (status) {
    case   0: return "No error";
    case  -1: return "Unknown error";
    case  -2: return "Null pointer";
    case  -3: return "Unsupported feature/library load error";
    case  -4: return "Could not allocate memory" ;
    case  -5: return "Insufficient IO buffers" ;
    case  -6: return "Invalid handle";
    case  -7: return "Memory lock failure" ;
    case  -8: return "Function called before initialization" ;
    case  -9: return "Specified object not found" ;
    case -10: return "More input data expected";
    case -11: return "More output surfaces expected";
    case -12: return "Operation aborted" ;
    case -13: return "HW device lost";
    case -14: return "Incompatible video parameters" ;
    case -15: return "Invalid video parameters";
    case -16: return "Undefined behavior";
    case -17: return "Device operation failure";
    case -18: return "More bitstream data expected";
    case -19: return "Incompatible audio parameters";
    case -20: return "Invalid audio parameters" ;
    default: return"Error code";
    }
  }
//}}}
//{{{
string getMfxInfo (mfxIMPL mfxImpl, mfxVersion mfxVersion) {

  return fmt::format ("mfxImpl:{:x}{}{}{}{} verMajor:{} verMinor:{}",
                      mfxImpl,
                      (mfxImpl & MFX_IMPL_HARDWARE) ? " hw":"",
                      (mfxImpl & MFX_IMPL_SOFTWARE) ? " sw":"",
                      (mfxImpl & MFX_IMPL_VIA_D3D9) ? " d3d9":"",
                      (mfxImpl & MFX_IMPL_VIA_D3D11) ? " d3d11":"",
                      mfxVersion.Major, mfxVersion.Minor);
  }
//}}}

int main(int numArgs, char** args) {

  eLogLevel logLevel = LOGINFO;
  //{{{  parse command line args to params
  // parse params
  for (int i = 1; i < numArgs; i++) {
    string param = args[i];

    if (param == "log1") { logLevel = LOGINFO1; }
    else if (param == "log2") { logLevel = LOGINFO2; }
    else if (param == "log3") { logLevel = LOGINFO3; }
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
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, fmt::format ("session.Init failed - status:{}:{}", status, getMfxStatus (status)));

  // query implementation
  status = mfxSession.QueryIMPL (&mfxImpl);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, fmt::format ("QueryIMPL failed - status:{}:{}", status, getMfxStatus (status)));

  // query version
  status = mfxSession.QueryVersion (&mfxVersion);
  if (status != MFX_ERR_NONE)
    cLog::log (LOGINFO, fmt::format ("QueryVersion failed - status:{}:{}}", status, getMfxStatus (status)));

  cLog::log (LOGINFO, getMfxInfo (mfxImpl, mfxVersion));

  mfxSession.Close();

  this_thread::sleep_for (5000ms);
  return 0;
  }
