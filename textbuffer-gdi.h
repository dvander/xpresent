// vim: set ts=2 sts=2 sw=2 tw=99 et:
#ifndef _xpresent_textbuffer_gdi_h_
#define _xpresent_textbuffer_gdi_h_

#include <stdint.h>
#include <Windows.h>

class GDITextBuffer
{
 public:
  GDITextBuffer(int width, int height);
  ~GDITextBuffer();
  bool Init();

  void Draw(const char* text);

  int width() const {
    return width_;
  }
  int height() const {
    return height_;
  }
  uint8_t* bits() const {
    return bits_;
  }
  unsigned pitch() const {
    return width_ * 4;
  }

 private:
  HDC dc_;
  int width_;
  int height_;

  HBITMAP bitmap_;
  uint8_t* bits_;
};

#endif // _xpresent_textbuffer_gdi_h_