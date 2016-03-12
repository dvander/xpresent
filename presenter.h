// vim: set ts=2 sts=2 sw=2 tw=99 et:
#ifndef _xpresent_presenter_h_
#define _xpresent_presenter_h_

#include <Windows.h>
#include <string>
#include <amtl/am-refcounting.h>

class Presenter : public ke::Refcounted<Presenter>
{
 public:
  Presenter(HWND hwnd);
  virtual ~Presenter();

  virtual const char* GetName() const = 0;

  void Present(uint32_t frame, COLORREF color);
  void Detach();

 protected:
  virtual bool BeginFrame() = 0;
  virtual void ComposeBackground(COLORREF ref) = 0;
  virtual void ComposeText(const char* text) = 0;
  virtual void EndFrame() = 0;

  void SetBroken(const char* message, ...);
  
 private:
  void PaintFrame(uint32_t frame, COLORREF color);
  bool IsBroken() const;

 private:
  static const size_t kFrameHistory = 16;

 protected:
  HWND hwnd_;
  RECT bounds_;
  std::string message_;

 private:
  LARGE_INTEGER qpc_freq_;
  double frame_times_[kFrameHistory];
  size_t frame_count_;
  size_t frame_index_;
};

#endif // _xpresent_presenter_h_