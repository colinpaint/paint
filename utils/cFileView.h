// cFileView.h - file view for analysers
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <chrono>
//}}}

class cFileView {
public:
  cFileView (const std::string& filename);
  virtual ~cFileView();

  // whole file
  uint8_t* getFileBuffer() { return mFileBuffer; }
  uint32_t getFileSize() { return mFileSize; }

  // file readBytes info
  uint8_t* getReadPtr() { return mReadPtr; }
  uint32_t getReadAddress() { return mReadAddress; }
  uint32_t getReadBytesLeft() { return mReadBytesLeft; }
  uint32_t getReadLineNumber() { return mReadLineNumber; }

  // file info
  std::string getFilename() { return mFilename; }
  std::string getCreationString() { return mCreationString; }
  std::string getAccessString() { return mAccessString; }
  std::string getWriteString() { return mWriteString; }

  std::chrono::time_point<std::chrono::system_clock> getCreationTimePoint() { return mCreationTimePoint; }
  std::chrono::time_point<std::chrono::system_clock> getAccessTimePoint() { return mAccessTimePoint; }
  std::chrono::time_point<std::chrono::system_clock> getWriteTimePoint() { return mWriteTimePoint; }

  //{{{
  void resetRead() {
    mReadPtr = mFileBuffer;
    mReadBytesLeft = mFileSize;
    mReadLineNumber = 0;
    mReadAddress = 0;
    }
  //}}}
  //{{{
  uint8_t* readBytes (uint32_t bytesToRead) {
  // return pointer to start of bytes in memoryMapped buffer if available, else return nullptr

    if (bytesToRead <= mReadBytesLeft) {
      uint8_t* ptr = mReadPtr;
      mReadPtr += bytesToRead;
      mReadBytesLeft -= bytesToRead;
      mReadAddress += bytesToRead;
      return ptr;
      }
    else
      return nullptr;
    }
  //}}}
  bool readLine (std::string& line, uint32_t& lineNumber, uint8_t*& ptr, uint32_t& address, uint32_t& numBytes);

private:
  // vars
  std::string mFilename;

  void* mFileHandle; // windows HANDLE
  void* mMapHandle;  // windows HANDLE

  uint8_t* mFileBuffer = nullptr;
  uint32_t mFileSize = 0;

  uint8_t* mReadPtr = nullptr;
  uint32_t mReadAddress = 0;
  uint32_t mReadBytesLeft = 0;
  uint32_t mReadLineNumber = 0;

  std::chrono::time_point<std::chrono::system_clock> mCreationTimePoint;
  std::chrono::time_point<std::chrono::system_clock> mAccessTimePoint;
  std::chrono::time_point<std::chrono::system_clock> mWriteTimePoint;

  std::string mCreationString;
  std::string mAccessString;
  std::string mWriteString;
  };
