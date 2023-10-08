// fileUtils.h - file utilities
#pragma once
#include <string>

#ifdef _WIN32
  //{{{  includes
  #include <shobjidl.h>
  #include <shlguid.h>
  #include <shellapi.h>
  #include <shlwapi.h>

  #pragma comment(lib,"shlwapi.lib")
  //}}}

  class cFileUtils {
  public:
    static std::string resolve (const std::string& shortcut) {
    // resolve windows shprtcut .lnk fileName

      // passthru empty string
      if (shortcut.empty())
        return shortcut;

      // get IShellLink interface
      IShellLinkA* iShellLink;
      if (CoCreateInstance (CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&iShellLink) == S_OK) {
        // get IPersistFile interface
        IPersistFile* iPersistFile;
        iShellLink->QueryInterface (IID_IPersistFile,(void**)&iPersistFile);

        // IPersistFile uses LPCOLESTR, ensure string is Unicode
        WCHAR wideShortcutFileName[MAX_PATH];
        MultiByteToWideChar (CP_ACP, 0, shortcut.c_str(), -1, wideShortcutFileName, MAX_PATH);

        // open shortcut file and init it from its contents
        if (iPersistFile->Load (wideShortcutFileName, STGM_READ) == S_OK) {
          // find target of shortcut, even if it has been moved or renamed
          if (iShellLink->Resolve (NULL, SLR_UPDATE) == S_OK) {
            // get the path to shortcut
            char szPath[MAX_PATH];
            WIN32_FIND_DATAA wfd;
            if (iShellLink->GetPath (szPath, MAX_PATH, &wfd, SLGP_RAWPATH) == S_OK) {
              // Get the description of the target
              char szDesc[MAX_PATH];
              if (iShellLink->GetDescription (szDesc, MAX_PATH) == S_OK)
                return std::string (szPath);
              }
            }
          }
        }
      return shortcut;
      }
    };

#endif

#ifdef __linux__
  class cFileUtils {
  public:
    static std::string resolve (const std::string& shortcut) { return shortcut; }
    };

#endif
