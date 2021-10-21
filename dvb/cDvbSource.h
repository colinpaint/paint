// cDvbSource.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>

#ifdef __linux__
  #include <poll.h>
  #include <linux/dvb/frontend.h>
#endif

class cTsBlock;
class cTsBlockPool;
//}}}

class cDvbSource {
public:
  cDvbSource (int frequency, int adapter);
  virtual ~cDvbSource();

  bool ok() const;

  std::string getTuneString() const { return mTuneString; }
  std::string getStatusString() const;

  int getBlock (uint8_t* block, int blockSize);

  int setFilter (uint16_t pid);
  void unsetFilter (int fd, uint16_t pid);

  void reset();
  void tune (int frequency);

  #ifdef _WIN32
    uint8_t* getBlockBDA (int& len);
    void releaseBlock (int len);
    void run();
  #endif

  #ifdef __linux__
    cTsBlock* getBlocks (cTsBlockPool* blockPool);
  #endif

private:
  int mFrequency = 0;
  int mAdapter = 0;

  std::string mTuneString = "untuned";

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

    int mDvr = 0;
  #endif
  //}}}
  };
