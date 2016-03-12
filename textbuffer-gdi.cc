// vim: set ts=2 sts=2 sw=2 tw=99 et:
#include "textbuffer-gdi.h"

GDITextBuffer::GDITextBuffer(int width, int height)
 : dc_(nullptr),
   width_(width),
   height_(height),
   bitmap_(nullptr),
   bits_(nullptr)
{
}

GDITextBuffer::~GDITextBuffer()
{
  if (bitmap_)
    ::DeleteObject(bitmap_);
  if (dc_)
    ::DeleteDC(dc_);
}

bool
GDITextBuffer::Init()
{
  dc_ = ::CreateCompatibleDC(nullptr);
  if (!dc_)
    return false;

  BITMAPINFO bmp;
  ZeroMemory(&bmp, sizeof(bmp));
  bmp.bmiHeader.biSize = sizeof(bmp.bmiHeader);
  bmp.bmiHeader.biWidth = width_;
  bmp.bmiHeader.biHeight = -height_;
  bmp.bmiHeader.biPlanes = 1;
  bmp.bmiHeader.biBitCount = 32;
  bmp.bmiHeader.biCompression = BI_RGB;

  bitmap_ = ::CreateDIBSection(dc_,
    &bmp,
    DIB_RGB_COLORS,
    (void **)&bits_,
    nullptr, 0);
  if (!bitmap_)
    return false;

  ::SelectObject(dc_, bitmap_);
  return true;
}

void
GDITextBuffer::Draw(const char* text)
{
  RECT r;
  r.left = 0;
  r.top = 0;
  r.right = width_;
  r.bottom = height_;

  HBRUSH brush = ::CreateSolidBrush(RGB(255, 255, 255));
  ::FillRect(dc_, &r, brush);
  ::DeleteObject(brush);
  ::DrawTextA(dc_, text, (int)strlen(text), &r, DT_LEFT | DT_TOP);
}