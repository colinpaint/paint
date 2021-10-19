// cDvbSource.h
//{{{  includes
#pragma once

#ifdef _WIN32
  #include <wrl.h>
  #include <initguid.h>
  #include <control.h>
  #include <bdatif.h>
#endif

#ifdef __linux__
  #include <poll.h>
  #include <linux/dvb/frontend.h>

  class cTsBlock;
  class cTsBlockPool;
#endif
//}}}

class cDvbSource {
public:
  cDvbSource (int frequency, int adapter);
  virtual ~cDvbSource();

  std::string getStatusString() const;
  int getBlock (uint8_t*& block, int& blockSize);

  int setFilter (uint16_t pid);
  void unsetFilter (int fd, uint16_t pid);

  void reset();
  void tune (int frequency);

  std::string mErrorStr = "waiting";
  std::string mTuneStr = "untuned";
  std::string mSignalStr = "no signal";

  //{{{
  #ifdef _WIN32
    uint8_t* getBlockBDA (int& len);
    void releaseBlock (int len);

    inline static Microsoft::WRL::ComPtr<IMediaControl> mMediaControl;
    inline static Microsoft::WRL::ComPtr<IScanningTuner> mScanningTuner;
  #endif
  //}}}
  //{{{
  #ifdef __linux__
    int mDvr = 0;

    cTsBlock* getBlocks (cTsBlockPool* blockPool);
  #endif
  //}}}

private:
  int mFrequency;
  int mAdapter;

  //{{{
  #ifdef __linux__
    fe_hierarchy_t getHierarchy();
    fe_guard_interval_t getGuard();
    fe_transmit_mode_t getTransmission();
    fe_spectral_inversion_t getInversion();
    fe_code_rate_t getFEC (fe_caps_t fe_caps, int fecValue);

    void frontendInfo (struct dvb_frontend_info& info, uint32_t version,
                       fe_delivery_system_t* systems, int numSystems);
    void frontendSetup();

    int mFrontEnd = 0;
    int mDemux = 0;
    struct pollfd fds[1];

    int mBandwidth = 8;
    int mFeNum = 0;
    int mInversion = -1;
    int mFec = 999;
    int mFecLp = 999;
    int mGuard = -1;
    int mTransmission = -1;
    int mHierarchy = -1;
  #endif
  //}}}
  };
