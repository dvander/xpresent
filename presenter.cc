// vim: set ts=2 sts=2 sw=2 tw=99 et:
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#define NOMINMAX
#include "present-gdi.h"

Presenter::Presenter(HWND hwnd)
 : hwnd_(hwnd),
   frame_count_(0),
   frame_index_(0)
{
  ::QueryPerformanceFrequency(&qpc_freq_);
}

Presenter::~Presenter()
{
}

void
Presenter::SetBroken(const char* message, ...)
{
  char buffer[256];
  va_list ap;
  va_start(ap, message);
  _vsnprintf(buffer, sizeof(buffer), message, ap);
  va_end(ap);
  message_ = buffer;

  fprintf(stdout, "[RENDERER] %s\n", message_.c_str());
}

bool
Presenter::IsBroken() const
{
  return message_.length() > 0;
}

void
Presenter::Present(uint32_t frame, COLORREF color)
{
  if (!::GetClientRect(hwnd_, &bounds_)) {
    SetBroken("GetClientRect failed: %d", GetLastError());
    return;
  }

  LARGE_INTEGER before, after;
  ::QueryPerformanceCounter(&before);
  {
    PaintFrame(frame, color);
  }
  ::QueryPerformanceCounter(&after);

  // Record the frame time.
  frame_count_ = std::min(frame_count_ + 1, kFrameHistory);
  frame_times_[frame_index_] =
    (double(after.QuadPart - before.QuadPart) / double(qpc_freq_.QuadPart)) * 1000;
  frame_index_ = frame_index_ == kFrameHistory - 1
                 ? 0
                 : frame_index_ + 1;

  // Check IsBroken() again in case the frame failed.
  if (IsBroken()) {
    char buffer[256];
    _snprintf(buffer, sizeof(buffer), "%s BROKEN: %s", GetName(), message_.c_str());

    HDC dc = ::GetDC(hwnd_);
    ::DrawTextA(dc, buffer, (int)strlen(buffer), &bounds_, DT_LEFT | DT_TOP);
    ::ReleaseDC(hwnd_, dc);
  }
}

void
Presenter::PaintFrame(uint32_t frame, COLORREF color)
{
  if (IsBroken())
    return;

  if (!BeginFrame())
    return;

  double total = 0.0;
  for (size_t i = 0; i < frame_count_; i++)
    total += frame_times_[i];
  double average = total / frame_count_;

  if (isnan(average))
    average = 0;

  char buffer[256];
  _snprintf(buffer, sizeof(buffer), "%s %.2fms frame %d", GetName(), average, frame);

  ComposeBackground(color);
  ComposeText(buffer);
  EndFrame();
}

void
Presenter::Detach()
{
  RECT r;
  ::GetClientRect(hwnd_, &r);

  HDC dc = ::GetDC(hwnd_);

  HBRUSH brush = ::CreateSolidBrush(RGB(255, 255, 255));
  ::FillRect(dc, &r, brush);
  ::DeleteObject(brush);
  ::ReleaseDC(hwnd_, dc);
}