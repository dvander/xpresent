// vim: set ts=2 sts=2 sw=2 tw=99 et:
#include "console.h"
#include <string>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <amtl/am-utility.h>
#include <amtl/am-vector.h>
#include "pipe.h"
#include "winutils.h"

static const char* kClassname = "xPresent";

static LRESULT CALLBACK OnMessage(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam);
static HWND CreateParentWindow(HINSTANCE hInstance);
static void LaunchChild(DWORD key, int argc, char** argv);

static WNDPROC sOldWndProc;
static UINT_PTR sTimer = 1;
static HWND sChildHwnd;
static COLORREF sLastColor = RGB(255, 0, 0);
static uint32_t sFrameNo = 0;

void
ParentMain(HINSTANCE hInstance, int nCmdShow, int argc, char** argv)
{
  int first_arg = 1;

  ShowConsole();

  ParentPipe pipe;

  LaunchChild(pipe.Key(), argc - first_arg, &argv[first_arg]);

  if (!pipe.Wait())
    AbortWithGLE("WaitNamedPipe");

  HWND hwnd = CreateParentWindow(hInstance);

  pipe.Send('w', reinterpret_cast<uintptr_t>(hwnd));

  char cmd;
  uintptr_t data;
  if (!pipe.Read(cmd, data) || cmd != kPipeHwndMsg)
    AbortWithGLE("parent pipe read");

  sChildHwnd = reinterpret_cast<HWND>(data);

  ::ShowWindow(hwnd, nCmdShow);

  sTimer = ::SetTimer(hwnd, sTimer, 15, nullptr);
  if (!sTimer)
    AbortWithGLE("SetTimer");

  fprintf(stdout, "MENU:  \n");
  fprintf(stdout, "  1 - Use GDI\n");
  fprintf(stdout, "  2 - Use D3D9\n");
  fprintf(stdout, "  3 - Use D3D9Ex\n");
  fprintf(stdout, "  4 - Use D3D11\n");
  fprintf(stdout, "  Click - Change color\n");

  {
    MSG msg;
    while (::GetMessageA(&msg, nullptr, 0, 0)) {
      ::TranslateMessage(&msg);
      ::DispatchMessageA(&msg);
    }
  }

  ::PostMessageA(sChildHwnd, WM_QUIT, 0, 0);
}

static bool
IsInheritable(HANDLE handle)
{
  if (!handle)
    return false;
  if (handle == INVALID_HANDLE_VALUE)
    return false;
  DWORD handle_type = GetFileType(handle);
  return (handle_type == FILE_TYPE_DISK || handle_type == FILE_TYPE_PIPE);
}

void
LaunchChild(DWORD key, int argc, char** argv)
{
  char exe[MAX_PATH];
  ::GetModuleFileNameA(nullptr, exe, sizeof(exe));

  char keystring[12];
  _snprintf(keystring, sizeof(keystring), "%d", key);

  std::string cmdline;
  cmdline += std::string("\"") + std::string(keystring) + std::string("\" ");
  cmdline += std::string("child ") + std::string(keystring);
  for (int i = 0; i < argc; i++) {
    cmdline += " ";
    cmdline += argv[i];
  }

  char cmdline_buf[512] = {0};
  _snprintf(cmdline_buf, sizeof(cmdline_buf) - 1, "%s", cmdline.c_str());

  STARTUPINFOEXA startup;
  ZeroMemory(&startup, sizeof(startup));

  startup.StartupInfo.cb = sizeof(startup.StartupInfo);
  startup.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
  startup.StartupInfo.wShowWindow = SW_HIDE;

  DWORD startupFlags = 0;
  HANDLE inherited[3] = {
    ::GetStdHandle(STD_INPUT_HANDLE),
    ::GetStdHandle(STD_OUTPUT_HANDLE),
    ::GetStdHandle(STD_ERROR_HANDLE),
  };

  ke::AutoPtr<char> buffer;
  LPPROC_THREAD_ATTRIBUTE_LIST list = nullptr;
  if (IsVistaOrLater()) {
    ke::Vector<HANDLE> handles;
    for (size_t i = 0; i < ARRAYSIZE(inherited); i++) {
      if (IsInheritable(inherited[i]))
        handles.append(inherited[i]);
    }

    if (handles.length()) {
      SIZE_T threadAttrSize;
      if (!InitializeProcThreadAttributeListFn(nullptr, 1, 0, &threadAttrSize) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER)
      {
        AbortWithGLE("InitializeProcThreadAttributeList");
      }

      buffer = new char[threadAttrSize];
      list = reinterpret_cast<decltype(list)>(buffer.get());

      if (!InitializeProcThreadAttributeListFn(list, 1, 0, &threadAttrSize))
        AbortWithGLE("InitializeProcThreadAttributeList");

      if (!UpdateProcThreadAttributeFn(list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                       handles.buffer(), sizeof(HANDLE) * handles.length(),
                                       nullptr, nullptr))
      {
        AbortWithGLE("UpdateProcThreadAttributeFn");
      }

      startup.StartupInfo.cb = sizeof(startup);
      startup.lpAttributeList = list;
      startupFlags |= EXTENDED_STARTUPINFO_PRESENT;
    }
  }

  startup.StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
  startup.StartupInfo.hStdInput = inherited[0];
  startup.StartupInfo.hStdOutput = inherited[1];
  startup.StartupInfo.hStdError = inherited[2];

  PROCESS_INFORMATION info;
  BOOL rv = ::CreateProcessA(
    exe,
    cmdline_buf,
    nullptr, nullptr,
    TRUE,
    startupFlags,
    nullptr,
    nullptr,
    &startup.StartupInfo,
    &info);

  if (list)
    DeleteProcThreadAttributeListFn(list);

  if (!rv)
    AbortWithGLE("CreateProcess");

  ::CloseHandle(info.hThread);
  ::CloseHandle(info.hProcess);
}

static HWND
CreateParentWindow(HINSTANCE hInstance)
{
  WNDCLASSEXA wc;
  if (!::GetClassInfoExA(hInstance, kClassname, &wc)) {
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = ::DefWindowProcA;
    wc.hInstance = hInstance;
    wc.hIcon = ::LoadIconA(nullptr, IDI_APPLICATION);
    wc.hIconSm = ::LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = ::LoadCursorA(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = kClassname;
    if (!RegisterClassExA(&wc))
      AbortWithGLE("RegisterClassExA");
  }

  DWORD style = WS_OVERLAPPEDWINDOW;
  HWND hwnd = ::CreateWindowA(
    kClassname, "xPresent", style,
    CW_USEDEFAULT, CW_USEDEFAULT,
    800, 800,
    nullptr, nullptr, hInstance, nullptr);
  if (!hwnd)
    AbortWithGLE("CreateWindowA");

  LONG_PTR exstyle = ::GetWindowLongPtrA(hwnd, GWL_EXSTYLE);
  exstyle |= WS_EX_WINDOWEDGE;

  ::SetWindowLongPtrA(hwnd, GWL_EXSTYLE, exstyle);

  sOldWndProc = reinterpret_cast<decltype(sOldWndProc)>(::GetWindowLongPtrA(hwnd, GWLP_WNDPROC));
  ::SetWindowLongPtrA(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(OnMessage));

  return hwnd;
}

static LRESULT CALLBACK
OnMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg) {
  case WM_TIMER:
    assert(wparam == sTimer);
    ::PostMessage(sChildHwnd, kWM_PRESENT, sLastColor, sFrameNo);
    sFrameNo++;
    return 0;

  case WM_KEYUP:
  {
    if (wparam >= 0x31 && wparam <= 0x34) {
      ::PostMessage(sChildHwnd, kWM_CHANGE_DEVICE, wparam - 0x31, sFrameNo);
      ::PostMessage(sChildHwnd, kWM_PRESENT, sLastColor, sFrameNo);
      sFrameNo++;
      return 0;
    }
    break;
  }

  case WM_LBUTTONUP:
  {
    sLastColor = RGB(rand() % 256, rand() % 256, rand() % 256);
    break;
  }

  case WM_PAINT:
  {
    PAINTSTRUCT p;
    ::BeginPaint(hwnd, &p);
    ::EndPaint(hwnd, &p);
    ::PostMessage(sChildHwnd, kWM_PRESENT, sLastColor, sFrameNo);
    sFrameNo++;
    return 1;
  }

  case WM_DESTROY:
    if (sTimer) {
      ::KillTimer(hwnd, sTimer);
      sTimer = 0;
    }
    ::PostQuitMessage(0);
    break;
  }
  return sOldWndProc(hwnd, msg, wparam, lparam);
}