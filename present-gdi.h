// vim: set ts=2 sts=2 sw=2 tw=99 et:
#ifndef _xpresent_presenter_gdi_h_
#define _xpresent_presenter_gdi_h_

#include "presenter.h"

class GDIPresenter : public Presenter
{
 public:
  GDIPresenter(HWND hwnd);
  ~GDIPresenter() override;

 protected:
  bool BeginFrame() override;
  void ComposeBackground(COLORREF color) override;
  void ComposeText(const char* text) override;
  void EndFrame() override;
  const char* GetName() const override {
    return "GDI";
  }

 private:
  HDC dc_;
};

#endif // _xpresent_presenter_gdi_h_