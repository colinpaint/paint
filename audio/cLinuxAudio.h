// cLinuxAudio.h
#pragma once
//{{{  includes
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#ifdef ALSA
  #include <alsa/asoundlib.h>
#else
  #include <pulse/simple.h>
  #include <pulse/error.h>
#endif

#include "../common/cLog.h"

#include "iAudio.h"
//}}}

#ifdef ALSA
  //{{{  alsa linux
  class cAudio : public iAudio {
  public:
    //{{{
    cAudio (int srcNumChannels, int srcSampleRate, int latency, bool bit16) {
      (void)srcNumChannels;
      (void)srcSampleRate;

      int err = snd_pcm_open (&mHandle, "default", SND_PCM_STREAM_PLAYBACK, 0);
      if (err < 0)
        printf ("audOpen - snd_pcm_open error: %s\n", snd_strerror (err));

      //SND_PCM_FORMAT_FLOAT
      err = snd_pcm_set_params (mHandle,
                                bit16 ? SND_PCM_FORMAT_S16 : SND_PCM_FORMAT_FLOAT,
                                SND_PCM_ACCESS_RW_INTERLEAVED,
                                2,      // channels
                                48000,  // sampleRate
                                1,      // softResample
                                latency);
      if (err < 0)
        printf ("audOpen - snd_pcm_set_params  error: %s\n", snd_strerror(err));
      }
    //}}}
    virtual ~cAudio() { snd_pcm_close (mHandle); }

    int getDstNumChannels() { return 2; }
    int getDstSampleRate() { return 480000; }
    int getDstChannelMask() { return 0xFF; }
    int getSrcNumChannels() { return 2; }

    //{{{
    float getVolume() {
      return mVolume;
      }
    //}}}
    //{{{
    void setVolume (float volume) {

      if (volume < 0)
        mVolume = 0;
      else if (volume > 100.0f)
        mVolume = 100.0f;
      else
        mVolume = volume;
      }
    //}}}
    float getDefaultVolume() { return 0.7f; };
    float getMaxVolume() { return 1.0f; }

    bool getMixedFL() { return true; }
    bool getMixedFR() { return true; }
    bool getMixedC()  { return true; }
    bool getMixedW()  { return true; }
    bool getMixedBL() { return true; }
    bool getMixedBR() { return true; }
    eMixDown getMixDown() { return mMixDown; }
    void setMixDown (eMixDown mixDown) { mMixDown = mixDown; }

    //{{{
    void play (int srcNumChannels, void* srcSamples, int srcNumSamples, float pitch) {
      (void)srcNumChannels;
      (void)pitch;

      snd_pcm_sframes_t frames = snd_pcm_writei (mHandle, srcSamples, srcNumSamples);
      if (frames < 0)
        frames = snd_pcm_recover (mHandle, frames, 0);

      if (frames < 0)
        printf ("audPlay - snd_pcm_writei failed: %s\n", snd_strerror(frames));
      }
    //}}}

    float mVolume = 0.8f;

  private:
    snd_pcm_t* mHandle;
    eMixDown mMixDown = eBestMix;
    };
  //}}}
#else
  // pulseAudio linux
  class cAudio : public iAudio {
  public:
    //{{{
    cAudio (int srcNumChannels, int srcSampleRate, int latency, bool bit16)
        : mSrcNumChannels(srcNumChannels), mLatency(latency), mBit16(bit16) {

      cLog::log (LOGINFO, fmt::format ("cAudio pulseAudio create - numChannels:{} sampleRate:{} {}",
                                       srcNumChannels, srcSampleRate, bit16 ? "16bit":"32bit"));

      const pa_sample_spec kSampleSpec = { bit16 ? PA_SAMPLE_S16LE : PA_SAMPLE_FLOAT32,
                                           (unsigned int)srcSampleRate,
                                           (unsigned char)srcNumChannels };

      mSimple = pa_simple_new (NULL, "default", PA_STREAM_PLAYBACK, NULL, "playback",
                               &kSampleSpec, NULL, NULL, NULL);
      if (!mSimple) {
        cLog::log (LOGINFO, fmt::format ("pa_simple_new failed"));
        return;
        }
      }
    //}}}
    //{{{
    virtual ~cAudio() { 
      pa_simple_free (mSimple); 
      }
    //}}}

    int getDstNumChannels() { return 2; }
    int getDstSampleRate() { return 480000; }
    int getDstChannelMask() { return 0xFF; }
    int getSrcNumChannels() { return 2; }

    //{{{
    float getVolume() {
      return mVolume;
      }
    //}}}
    //{{{
    void setVolume (float volume) {

      if (volume < 0)
        mVolume = 0;
      else if (volume > 100.0f)
        mVolume = 100.0f;
      else
        mVolume = volume;
      }
    //}}}
    float getDefaultVolume() { return 0.7f; };
    float getMaxVolume() { return 1.0f; }

    bool getMixedFL() { return true; }
    bool getMixedFR() { return true; }
    bool getMixedC()  { return true; }
    bool getMixedW()  { return true; }
    bool getMixedBL() { return true; }
    bool getMixedBR() { return true; }
    eMixDown getMixDown() { return mMixDown; }
    void setMixDown (eMixDown mixDown) { mMixDown = mixDown; }

    //{{{
    void play (int srcNumChannels, void* srcSamples, int srcNumSamples, float pitch) {
      (void)pitch;

      if (mSimple) {
        pa_simple_write (mSimple, srcSamples, (size_t)(srcNumSamples * srcNumChannels * (mBit16 ? 2 : 4)), NULL);
        mLatencyUsec = pa_simple_get_latency (mSimple, NULL);
        //cLog::log (LOGINFO, fmt::format ("srcNumSamples:{} latency:{}", srcNumSamples, latencyUsec));
        }
      }
    //}}}

    float mVolume = 0.8f;

  private:
    const int mSrcNumChannels;
    const int mLatency;
    const bool mBit16 = false;

    pa_simple* mSimple = nullptr;
    pa_usec_t mLatencyUsec = 0;

    eMixDown mMixDown = eBestMix;
    };

#endif
