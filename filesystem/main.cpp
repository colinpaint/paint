// main.cpp - std::filesystem testbed, learn it, implement code snippets
//{{{  includes
#include <vector>
#include <map>
#include <string>

#include <iostream>
#include <sstream>
#include <fstream>

#include <iomanip>
#include <numeric>
#include <regex>
#include <filesystem>

#include "../utils/cLog.h"

using namespace std;
//}}}

namespace {
  //{{{
  string getSizeString (size_t size) {

    if (size >= 1000000000)
      return fmt::format ("{:4d}g", size / 1000000000);
    else if (size >= 1000000)
      return fmt::format ("{:4d}m", size / 1000000);
    else if (size >= 1000)
      return fmt::format ("{:4d}k", size / 1000);
    else
      return fmt::format ("{:4d}b", size);
    }
  //}}}
  //{{{
  string getFileTypeString (filesystem::file_status fileStatus) {

    if (filesystem::is_directory (fileStatus))
      return "dir ";
    else if (filesystem::is_symlink (fileStatus))
      return "sym ";
    else if (filesystem::is_character_file (fileStatus))
      return "chr ";
    else if (filesystem::is_block_file (fileStatus))
      return "bin ";
    else if (filesystem::is_fifo (fileStatus))
      return "fifo";
    else if (filesystem::is_socket (fileStatus))
      return "sock";
    else if (filesystem::is_other (fileStatus))
      return "oth ";
    else if (filesystem::is_regular_file (fileStatus))
      return "file";

    return "??? ";
    }
  //}}}
  //{{{
  string getFilePermissionsString (filesystem::perms filePerms) {

    auto check ([filePerms](filesystem::perms bit, char ch) {
        return (filePerms & bit) == filesystem::perms::none ? '-' : ch;
        }
      );

    return { check (filesystem::perms::owner_read,  'r'),
             check (filesystem::perms::owner_write, 'w'),
             check (filesystem::perms::owner_exec,  'x'),
             ' ',
             check (filesystem::perms::group_read,  'r'),
             check (filesystem::perms::group_write, 'w'),
             check (filesystem::perms::group_exec,  'x'),
             ' ',
             check (filesystem::perms::others_read,  'r'),
             check (filesystem::perms::others_write, 'w'),
             check (filesystem::perms::others_exec,  'x')
           };
    }
  //}}}

  //{{{
  size_t getSize (const filesystem::directory_entry& entry) {

    if (!entry.is_directory())
      return entry.file_size();

    return accumulate (filesystem::directory_iterator {entry}, {}, 0u,
                       [](unsigned int accumulatedSize, const filesystem::directory_entry& directoryEntry) {
                           return static_cast<unsigned int>(accumulatedSize + getSize (directoryEntry));
                           }
                         );
    }
  //}}}
  //{{{
  tuple <filesystem::path, filesystem::file_status, size_t> getFileInfo (const filesystem::directory_entry& entry) {

    return { entry.path(), entry.status(), filesystem::is_regular_file (entry.status()) ? entry.file_size() : 0u };
    }
  //}}}

  //{{{
  map <string, pair <size_t, size_t>> extensionTotalSizeMap (const filesystem::path directoryPath) {

    map <string, pair<size_t, size_t>> sizeMap;

    for (const filesystem::directory_entry& entry : filesystem::recursive_directory_iterator {directoryPath}) {
      if (entry.is_directory())
        cLog::log (LOGINFO, fmt::format ("skipping directory {}", entry.path().string()));

      const filesystem::file_status status { entry.status() };
      const size_t size { entry.file_size() };

      const string extensionString = entry.path().extension().string();
      if (extensionString.length() == 0)
        continue;

      auto& [accumulatedSize, count] = sizeMap[extensionString];
      accumulatedSize += size;
      count += 1;
      }

    return sizeMap;
    }
  //}}}

  //{{{
  vector <pair<size_t, string>> match (const filesystem::path& filePath, const regex& pattern) {

    // result
    vector <pair<size_t, string>> matches;

    // input file stream
    ifstream inputFileStream { filePath.string() };

    smatch matchSpec;
    string lineString;
    for (size_t line = 1; getline (inputFileStream, lineString); ++line)
      if (regex_search (lineString, matchSpec, pattern)) {
        cLog::log (LOGINFO,
          fmt::format ("{:16s} line:{:4d} {}:{:2d}:{:2d} {}",
                       filePath.filename().string(),
                       line,
                       matchSpec.size(), matchSpec.prefix().length(), matchSpec.suffix().length(),
                       lineString));
        matches.emplace_back (line, move (lineString));
        }

    return matches;
    }
  //}}}
  //{{{
  template <typename T> string replace (string srcString, const T& replacements) {

    for (const auto &[pattern, replacement] : replacements)
      srcString = regex_replace (srcString, pattern, replacement);

    return srcString;
    }
  //}}}
  }

int main (int argc, char* argv[]) {

  cLog::init (LOGINFO);
  cLog::log (LOGNOTICE, fmt::format ("filesystem"));

  filesystem::path srcPath = (argc > 1) ? argv[1] : ".";
  if (!filesystem::exists (srcPath)) {
    //{{{  error, return
    cLog::log (LOGERROR, fmt::format ("path {} does not exist", srcPath.string()));
    return 1;
    }
    //}}}

  if (!filesystem::is_directory (srcPath)) {
    //{{{  file info
    cLog::log (LOGINFO, "---- File info");
    cLog::log (LOGINFO, fmt::format ("file rootPath      {}", srcPath.root_path().string()));
    cLog::log (LOGINFO, fmt::format ("file rootDirectory {}", srcPath.root_directory().string()));
    cLog::log (LOGINFO, fmt::format ("file rootName      {}", srcPath.root_name().string()));
    cLog::log (LOGINFO, fmt::format ("file relativePath  {}", srcPath.relative_path().string()));
    cLog::log (LOGINFO, fmt::format ("file parentPath    {}", srcPath.parent_path().string()));
    cLog::log (LOGINFO, fmt::format ("file stem          {}", srcPath.stem().string()));
    cLog::log (LOGINFO, fmt::format ("file extensionh    {}", srcPath.extension().string()));

    int i = 0;
    for (const auto& part : srcPath)
      cLog::log (LOGINFO, fmt::format ("part {}  - {}", i++, part.string()));

    srcPath = srcPath.parent_path();
    }
    //}}}

  //{{{  directory or parent directory info
  cLog::log (LOGINFO, "---- Directory info");

  cLog::log (LOGINFO, fmt::format ("dir path      {}", srcPath.string()));
  cLog::log (LOGINFO, fmt::format ("dir canonical {}", canonical (srcPath).string()));
  cLog::log (LOGINFO, fmt::format ("dir absolute  {}", filesystem::absolute (srcPath).string()));
  cLog::log (LOGINFO, fmt::format ("dir current_path {}", filesystem::current_path().string()));
  //}}}
  //{{{  simple list
  cLog::log (LOGINFO, "---- SimpleList");

  for (const filesystem::directory_entry& entry : filesystem::directory_iterator (srcPath))
    cLog::log (LOGINFO, fmt::format ("{:5s} {} {}",
               getSizeString (getSize (entry)),
               getFileTypeString (entry.status()),
               entry.path().filename().string()));
  //}}}
  //{{{  full list
  cLog::log (LOGINFO, "---- FullList");

  vector <tuple<filesystem::path, filesystem::file_status, size_t>> fileInfoVector;
  transform (filesystem::recursive_directory_iterator (srcPath), {}, back_inserter (fileInfoVector), getFileInfo);

  for (const auto& [path, status, size] : fileInfoVector) {
    cLog::log (LOGINFO, fmt::format ("{} {} {} {:10s}{}",
      getFileTypeString (status),
      getFilePermissionsString (status.permissions()),
      getSizeString (size),
      path.filename().stem().string(),
      path.filename().extension().string()
      ));
    }
  //}}}
  //{{{  report extension sizes
  cLog::log (LOGINFO, "---- ExtensionSizes");

  for (const auto& [extension, stats] : extensionTotalSizeMap (srcPath)) {
    const auto& [accumulatedSize, extensionFileCount] = stats;
    cLog::log (LOGINFO, fmt::format ("{:6s} num:{:3d} total:{}",
               extension, extensionFileCount, getSizeString (accumulatedSize)));
    }
  //}}}

  // search
  cLog::log (LOGINFO, "---- Search for drawContents");
  regex pattern {"drawContents"};
  for (const filesystem::directory_entry& entry : filesystem::recursive_directory_iterator (srcPath))
    vector <pair<size_t, string>> matches = match (entry.path(), pattern);

  while (true) {}

  //{{{  rename
  vector <pair <regex, string>> patterns;
  patterns.emplace_back ("cpp", "cPlusPlus");

  for (const filesystem::directory_entry& entry : filesystem::recursive_directory_iterator { srcPath }) {
    filesystem::path originalPath {entry.path()};
    string newName { replace (originalPath.filename().string(), patterns) };

    filesystem::path renamePath {originalPath};
    renamePath.replace_filename (newName);

    cLog::log (LOGINFO, fmt::format ("iterate {} {}", originalPath.string(), renamePath.filename().string()));


    if (originalPath != renamePath) {
      cLog::log (LOGINFO, fmt::format ("{} renaming to {}", originalPath.string(), renamePath.filename().string()));
      if (exists (renamePath))
        cLog::log (LOGINFO, fmt::format ("- cannot rename"));
      else
        filesystem::rename (originalPath, renamePath);
      }
    }
  //}}}
  while (true) {}
  }
