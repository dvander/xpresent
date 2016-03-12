// vim: set ts=2 sts=2 sw=2 tw=99 et:
#include "console.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "pipe.h"
#include "winutils.h"
#include "child.h"
#include "presenter.h"
#include "present-gdi.h"
#include "present-d3d9.h"
#include "present-d3d11.h"

static const char* kClassname = "xParentIO";

static LRESULT CALLBACK MsgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
static HWND CreateMessageWindow(HINSTANCE hInstance);

static HWND sParentHwnd;
static ke::RefPtr<Presenter> sPresenter;

void ChildMain(HINSTANCE hInstance, DWORD key, int argc, char** argv)
{
  LinkConsole();

  ChildPipe pipe(key);

  char cmd;
  uintptr_t data;
  if (!pipe.Read(cmd, data) || cmd != kPipeHwndMsg)
    AbortWithGLE("pipe read");

  sParentHwnd = reinterpret_cast<HWND>(data);

  HWND msg_hwnd = CreateMessageWindow(hInstance);
  if (!msg_hwnd)
    AbortWithGLE("CreateMessageWindow");

  pipe.Send(kPipeHwndMsg, reinterpret_cast<uintptr_t>(msg_hwnd));

  sPresenter = new GDIPresenter(sParentHwnd);

  MSG msg;
  while (GetMessage(&msg, msg_hwnd, 0, 0)) {
    ::TranslateMessage(&msg);
    ::DispatchMessageA(&msg);
  }
}

static HWND
CreateMessageWindow(HINSTANCE hInstance)
{
  WNDCLASSEXA wc;
  if (!::GetClassInfoExA(hInstance, kClassname, &wc)) {
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MsgProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kClassname;
    if (!RegisterClassExA(&wc))
      AbortWithGLE("RegisterClassExA");
  }

  return CreateWindowA(kClassname, 0, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, hInstance, 0);
}

static LRESULT CALLBACK
MsgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg) {
  case kWM_PRESENT:
    if (sPresenter)
      sPresenter->Present((uint32_t)lparam, (COLORREF)wparam);
    return 0;

  case kWM_CHANGE_DEVICE:
  {
    sPresenter->Detach();
    switch (wparam) {
    case 0:
      sPresenter = new GDIPresenter(sParentHwnd);
      break;
    case 1:
      sPresenter = new D3D9Presenter(sParentHwnd, false);
      break;
    case 2:
      sPresenter = new D3D9Presenter(sParentHwnd, true);
      break;
    case 3:
      sPresenter = new D3D11Presenter(sParentHwnd);
      break;
    }
    break;
  }

  case WM_QUIT:
    ::PostQuitMessage(0);
    return 0;
  }
  return ::DefWindowProcA(hwnd, msg, wparam, lparam);
}
