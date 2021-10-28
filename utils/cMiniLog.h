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
	class cTag {
	public:
		cTag (const std::string& name) : mName(name) {}
		std::string mName;
		bool mEnable = false;;
		};

	class cLine {
	public:
		cLine(const std::string& text) : mText(text) {}
		std::string mText;
		std::chrono::system_clock::time_point mTimePoint;
		uint8_t mTagIndex = 0;
		};

	cMiniLog (const std::string& name);

	bool getEnable() const { return mEnable; }

	std::string getName() const { return mName; }
	std::string getHeader() const;
	std::string getFooter() const { return mFooter; }

	std::deque<std::string>& getLog() { return mLog; }
	std::vector<std::string>& getTags() { return mTags; }
	std::vector<bool>& getFilters() { return mFilters; }
	uint8_t getTagPos() const { return 14; }

	void setEnable (bool enable);
	void setLevel (uint8_t level);
	void setHeader (const std::string& header) { mHeader = header; }
	void setFooter (const std::string& footer) { mFooter = footer; }

	void toggleEnable();

	void clear();

	void log (const std::string& text);

private:
	bool mEnable = false;

	const std::string mName;
	std::string mHeader;
	std::string mFooter;

	const std::chrono::system_clock::time_point mFirstTimePoint;

	std::deque <std::string> mLog;
	std::vector <std::string> mTags;
	std::vector <bool> mFilters;
	};
