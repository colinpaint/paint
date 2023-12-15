// iAudio.h
#pragma once

class iAudio {
public:
  enum eMixDown { eBestMix, eFLFR, eBLBR, eCentre, eWoofer, eAllMix, eNoMix };

  virtual ~iAudio() = default;

  virtual int getDstNumChannels() = 0;
  virtual int getDstSampleRate() = 0;
  virtual int getDstChannelMask() = 0;

  virtual int getSrcNumChannels() = 0;

  virtual float getVolume() = 0;
  virtual float getDefaultVolume() = 0;
  virtual float getMaxVolume() = 0;
  virtual void setVolume (float volume) = 0;

  virtual void play (int srcChannels, void* srcSamples, int srcNumSamples, float pitch) = 0;
  };
