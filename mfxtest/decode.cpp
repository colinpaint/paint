// decode.cpp
//{{{  includes
#include <vector>
#include <string>
#include <thread>

#include "../libmfx/include/mfxvideo++.h"

#include "../utils/cLog.h"

using namespace std;
//}}}

int main(int argc, char** argv) {

  (void)argc;
  (void)argv;
  cLog::init (LOGINFO);
  cLog::log (LOGNOTICE, fmt::format ("mfxtest - decode"));

  mfxIMPL mfxImpl = MFX_IMPL_AUTO_ANY;
  mfxVersion mfxVersion = {{0,1}};
  MFXVideoSession mfxSession;
  mfxStatus mfxStatus = mfxSession.Init (mfxImpl, &mfxVersion);
  if (mfxStatus != MFX_ERR_NONE)
    cLog::log (LOGINFO, fmt::format ("session.Init failed {}", mfxStatus));

  //{{{  Query selected implementation and version
  mfxStatus = mfxSession.QueryIMPL (&mfxImpl);
  if (mfxStatus != MFX_ERR_NONE)
    cLog::log (LOGINFO, fmt::format ("QueryIMPL failed {}", mfxStatus));
  mfxStatus = mfxSession.QueryVersion (&mfxVersion);
  if (mfxStatus != MFX_ERR_NONE)
    cLog::log (LOGINFO, fmt::format ("QueryVersion failed {}", mfxStatus));

  cLog::log (LOGINFO, fmt::format ("mfxImpl:{} verMajor:{} verMinor:{}",
                                   (mfxImpl == MFX_IMPL_HARDWARE) ? "hw":"sw", mfxVersion.Major, mfxVersion.Minor));

  //}}}
  mfxSession.Close();

  this_thread::sleep_for (2000ms);
  return 0;
  }
