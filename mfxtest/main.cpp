// main.cpp - mfxtest
//{{{  includes
#include <vector>
#include <string>
#include <thread>

#include "mfxvideo++.h"
//#include "../libmfx/include/mfxvideo++.h"
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

  mfxSession.Close();

  this_thread::sleep_for (5000ms);
  return 0;
  }
