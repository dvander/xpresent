// vim: set ts=2 sts=2 sw=2 tw=99 et:
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <io.h>

void
AbortWithGLE(const char *msg)
{
  DWORD err = GetLastError();

  char buffer[1024];
  DWORD rval = FormatMessage(
    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    nullptr,
    err,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPSTR)buffer,
    sizeof(buffer),
    nullptr);
  if (!rval)
    _snprintf(buffer, sizeof(buffer), "error code %08x formatting error %08x", GetLastError(), err);

  fprintf(stderr, "%s: %s\n", msg, buffer);
#if !defined(NDEBUG) && defined(_M_X86)
  __asm {
    int 3;
  };
#endif
  getchar();
  exit(1);
}

void
ShowConsole()
{
  if (!AllocConsole())
    AbortWithGLE("AllocConsole");

  auto reroute = [](DWORD id, const char* con, const char* mode, FILE* out) -> void {
    HANDLE h = GetStdHandle(id);
    if (!h)
      AbortWithGLE("GetStdHandle");
    int crt = _open_osfhandle((intptr_t)h, _O_TEXT);
    if (crt == -1)
      AbortWithGLE("_open_osfhandle");
    FILE* fp = _fdopen(crt, mode);
    if (!fp)
      AbortWithGLE("_fdopen");
    *out = *fp;
    if (!freopen(con, mode, out))
      AbortWithGLE("freopen");
  };
  reroute(STD_INPUT_HANDLE, "CONIN$", "r", stdin);
  reroute(STD_OUTPUT_HANDLE, "CONOUT$", "w", stdout);
  reroute(STD_ERROR_HANDLE, "CONOUT$", "w", stderr);
}

void
LinkConsole()
{
  if (!AttachConsole(ATTACH_PARENT_PROCESS))
    return;
  if (_fileno(stdout) == -2 || _get_osfhandle(fileno(stdout)) == -2)
    freopen("CONOUT$", "w", stdout);
  if (_fileno(stderr) == -2 || _get_osfhandle(fileno(stderr)) == -2)
    freopen("CONOUT$", "w", stderr);
  if (_fileno(stdin) == -2 || _get_osfhandle(fileno(stdin)) == -2)
    freopen("CONIN$", "r", stdin);
}

inline bool
IsWindowsVersionOrLater(uint32_t aVersion)
{
  static uint32_t minVersion = 0;
  static uint32_t maxVersion = UINT32_MAX;

  if (minVersion >= aVersion) {
    return true;
  }

  if (aVersion >= maxVersion) {
    return false;
  }

  OSVERSIONINFOEX info;
  ZeroMemory(&info, sizeof(OSVERSIONINFOEX));
  info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
  info.dwMajorVersion = aVersion >> 24;
  info.dwMinorVersion = (aVersion >> 16) & 0xFF;
  info.wServicePackMajor = (aVersion >> 8) & 0xFF;
  info.wServicePackMinor = aVersion & 0xFF;

  DWORDLONG conditionMask = 0;
  VER_SET_CONDITION(conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
  VER_SET_CONDITION(conditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
  VER_SET_CONDITION(conditionMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
  VER_SET_CONDITION(conditionMask, VER_SERVICEPACKMINOR, VER_GREATER_EQUAL);

  if (VerifyVersionInfo(&info,
    VER_MAJORVERSION | VER_MINORVERSION |
    VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
    conditionMask)) {
    minVersion = aVersion;
    return true;
  }

  maxVersion = aVersion;
  return false;
}

bool
IsVistaOrLater()
{
  return IsWindowsVersionOrLater(0x06000000ul);
}

bool
IsWin8OrLater()
{
  return IsWindowsVersionOrLater(0x06020000ul);
}
