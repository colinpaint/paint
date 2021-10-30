// cAudioDecoder.cpp
//{{{  includes
#include "cAudioDecoder.h"

#include <cstdint>
#include <string>
#include <vector>
#include <array>

#include "../imgui/imgui.h"
#include "../imgui/myImgui.h"

#include "../decoder/cAudioParser.h"
#include "../decoder/iAudioDecoder.h"
#include "../decoder/cFFmpegAudioDecoder.h"

#include "../utils/date.h"
#include "../utils/cLog.h"
#include "../utils/utils.h"

using namespace std;
//}}}
//{{{  defines
#define AVRB16(p) ((*(p) << 8) | *(p+1))

#define SCALEBITS 10
#define ONE_HALF  (1 << (SCALEBITS - 1))
#define FIX(x)    ((int) ((x) * (1<<SCALEBITS) + 0.5))
//}}}
namespace {
  }

// public:
//{{{
cAudioDecoder::cAudioDecoder (const std::string name) : cDecoder(name) {

  eAudioFrameType frameType = eAudioFrameType::eAacLatm;

  switch (frameType) {
    case eAudioFrameType::eMp3:
      cLog::log (LOGINFO, "createAudioDecoder ffmpeg mp3");
      mAudioDecoder = new cFFmpegAudioDecoder (frameType);
      break;

    case eAudioFrameType::eAacAdts:
      cLog::log (LOGINFO, "createAudioDecoder ffmpeg aacAdts");
      mAudioDecoder =  new cFFmpegAudioDecoder (frameType);
      break;

    case eAudioFrameType::eAacLatm:
      cLog::log (LOGINFO, "createAudioDecoder ffmpeg aacLatm");
      mAudioDecoder =  new cFFmpegAudioDecoder (frameType);
      break;

    default:
      cLog::log (LOGERROR, "createAudioDecoder frameType:%d", frameType);
      break;
    }
  }
//}}}
cAudioDecoder::~cAudioDecoder() {}

bool cAudioDecoder::decode (uint8_t* buf, int bufSize, int64_t pts) {

  log ("pes", fmt::format ("pts:{} size: {}", getFullPtsString (pts), bufSize));
  //logValue (pts, (float)bufSize);

  uint8_t* framePes = buf;
  uint8_t* framePesEnd = buf + bufSize;
  int64_t framePts = pts;

  int frameSize;
  while (cAudioParser::parseFrame (framePes, framePesEnd, frameSize)) {
    // decode a single frame from pes
    float* samples = mAudioDecoder->decodeFrame (framePes, frameSize, pts);
    if (samples) {
      //{{{  calc power,peak values for frame
      size_t numChannels = 2;
      size_t numSamples = 1024;

      array <float,6> powerValues = {0.f};
      array <float,6> peakValues = {0.f};
      for (size_t channel = 0; channel < numChannels; channel++) {
        powerValues[channel] = 0.f;
        peakValues[channel] = 0.f;
        }

      auto samplePtr = samples;
      for (size_t sample = 0; sample < numSamples; sample++) {
        for (size_t channel = 0; channel < numChannels; channel++) {
          float value = *samplePtr++;
          powerValues[channel] += value * value;
          peakValues[channel] = max (abs(peakValues[channel]), value);
          }
        }

      // max
      float maxPowerValue = 0.f;
      float maxPeakValue = 0.f;
      for (size_t channel = 0; channel < numChannels; channel++) {
        powerValues[channel] = sqrtf (powerValues[channel] / numSamples);
        maxPowerValue = max (maxPowerValue, powerValues[channel]);
        maxPeakValue = max (maxPeakValue, peakValues[channel]);
        }
      //}}}
      logValue (framePts, maxPowerValue);
      framePts += (mAudioDecoder->getNumSamplesPerFrame() * 90) / 48;

      // store samples in map , add player thread playing from first
      }

    // point to next frame in pes
    framePes += frameSize;
    }

  return false;
  }
