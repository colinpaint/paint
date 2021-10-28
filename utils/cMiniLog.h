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
	cMiniLog(const std::string& name);
	~cMiniLog() = default;

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
	const std::string mName;
	const std::chrono::system_clock::time_point mFirstTimePoint;

	std::string mHeader;
	std::string mFooter;

	bool mEnable = false;

	std::deque <std::string> mLog;
	std::vector <std::string> mTags;
	std::vector <bool> mFilters;
	};
