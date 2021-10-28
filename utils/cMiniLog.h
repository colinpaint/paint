// cMiniLog - mini log class, used by in class GUI loggers
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <vector>
#include <deque>
#include <chrono>

#include "../utils/formatCore.h"
#include "cLog.h"
//}}}

class cMiniLog {
public:
  cMiniLog (const std::string& name);
  //{{{
  class cTag {
  public:
    cTag (const std::string& name) : mName(name) {}

    std::string mName;
    bool mEnable = false;
    };
  //}}}
  //{{{
  class cLine {
  public:
    cLine (const std::string& text, uint8_t tagIndex, std::chrono::system_clock::duration timeStamp)
      : mText(text), mTagIndex(tagIndex), mTimeStamp(timeStamp) {}

    std::string mText;
    uint8_t mTagIndex;
    std::chrono::system_clock::duration mTimeStamp;
    };
  //}}}

  bool getEnable() const { return mEnable; }

  std::string getName() const { return mName; }
  std::string getHeader() const;
  std::string getFooter() const { return mFooter; }

  std::deque<cLine>& getLines() { return mLines; }
  std::vector<cTag>& getTags() { return mTags; }

  void setEnable (bool enable);
  void toggleEnable();

  void setHeader (const std::string& header) { mHeader = header; }
  void setFooter (const std::string& footer) { mFooter = footer; }

  void clear();

  void log (const std::string& tag, const std::string& text);

private:
  bool mEnable = false;

  const std::string mName;
  std::string mHeader;
  std::string mFooter;

  const std::chrono::system_clock::time_point mFirstTimePoint;

  std::deque <cLine> mLines;
  std::vector <cTag> mTags;
  };
