// vim: set ts=2 sts=2 sw=2 tw=99 et:
#include <stdio.h>
#include <stdlib.h>
#include "present-gdi.h"

GDIPresenter::GDIPresenter(HWND hwnd)
  : Presenter(hwnd)
{
}

GDIPresenter::~GDIPresenter()
{
  if (dc_)
    ::ReleaseDC(hwnd_, dc_);
}

bool
GDIPresenter::BeginFrame()
{
  dc_ = ::GetDC(hwnd_);
  if (!dc_) {
    SetBroken("GetDC failed: %d", GetLastError());
    return false;
  }

  return true;
}

void
GDIPresenter::ComposeBackground(COLORREF color)
{
  HBRUSH brush = ::CreateSolidBrush(color);
  if (!brush) {
    SetBroken("CreateSolidBrush failed: %d", GetLastError());
    return;
  }
  if (!::FillRect(dc_, &bounds_, brush)) {
    SetBroken("FillRect failed: %d", GetLastError());
    return;
  }
  ::DeleteObject(brush);
  return;
}

void
GDIPresenter::ComposeText(const char* text)
{
  ::DrawTextA(dc_, text, (int)strlen(text), &bounds_, DT_LEFT | DT_TOP);
}

void
GDIPresenter::EndFrame()
{
  ::ReleaseDC(hwnd_, dc_);
  dc_ = nullptr;
}
