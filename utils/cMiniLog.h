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
	cMiniLog(const std::string& name) : mName(name) {}
	~cMiniLog() = default;

	bool getEnable() const { return mEnable; }

	std::string getName() const { return mName; }
	std::string getHeader() const { return mHeader; }
	std::string getFooter() const { return mFooter; }

	std::deque<std::string>& getLog() { return mLog; }

	void setHeader (const std::string& header) { mHeader = header; }
	void setFooter (const std::string& footer) { mFooter = footer; }
	void setEnable (bool enable);
	void toggleEnable() { setEnable (!mEnable); }

	void log (const std::string& text);

private:
	std::string mName;

	std::string mHeader;
	std::string mFooter;

	bool mEnable = false;
	std::deque <std::string> mLog;
	};