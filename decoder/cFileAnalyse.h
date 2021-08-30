// cFileAnalyse.h - file analyser
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <chrono>
//}}}

class cFileAnalyse {
public:
  cFileAnalyse (const std::string& filename);
  virtual ~cFileAnalyse();

  // whole file
  uint8_t* getFileBuffer() { return mFileBuffer; }
  uint32_t getFileSize() { return mFileSize; }

  // file readBytes info
  uint8_t* getReadPtr() { return mReadPtr; }
  uint32_t getReadBytesLeft() { return mReadBytesLeft; }

  // file info
  std::string getFilename() { return mFilename; }
  std::string getCreationString() { return mCreationString; }
  std::string getAccessString() { return mAccessString; }
  std::string getWriteString() { return mWriteString; }

  std::chrono::time_point<std::chrono::system_clock> getCreationTimePoint() { return mCreationTimePoint; }
  std::chrono::time_point<std::chrono::system_clock> getAccessTimePoint() { return mAccessTimePoint; }
  std::chrono::time_point<std::chrono::system_clock> getWriteTimePoint() { return mWriteTimePoint; }

  //{{{
  void resetReadBytes() {
    mReadPtr = mFileBuffer;
    mReadBytesLeft = mFileSize;
    }
  //}}}
  //{{{
  uint8_t* readBytes (uint32_t bytesToRead) {
  // return pointer to start of bytes in memoryMapped buffer if available, else return nullptr

    if (bytesToRead <= mReadBytesLeft) {
      uint8_t* ptr = mReadPtr;
      mReadPtr += bytesToRead;
      mReadBytesLeft -= bytesToRead;
      return ptr;
      }
    else
      return nullptr;
    }
  //}}}
  bool readLine (uint8_t*& beginPtr, uint8_t*& endPtr);

private:
  // vars
  std::string mFilename;

  void* mFileHandle; // windows HANDLE
  void* mMapHandle;  // windows HANDLE

  uint8_t* mFileBuffer = nullptr;
  uint32_t mFileSize = 0;

  uint8_t* mReadPtr = nullptr;
  uint32_t mReadBytesLeft = 0;

  std::chrono::time_point<std::chrono::system_clock> mCreationTimePoint;
  std::chrono::time_point<std::chrono::system_clock> mAccessTimePoint;
  std::chrono::time_point<std::chrono::system_clock> mWriteTimePoint;

  std::string mCreationString;
  std::string mAccessString;
  std::string mWriteString;
  };
