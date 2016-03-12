// vim: set ts=2 sts=2 sw=2 tw=99 et:
#ifndef _xpresent_presenter_d3d9_h_
#define _xpresent_presenter_d3d9_h_

#include "presenter.h"
#include "textbuffer-gdi.h"
#include <d3d9.h>

class D3D9Presenter : public Presenter
{
 public:
  D3D9Presenter(HWND hwnd, bool useD3D9Ex);
  ~D3D9Presenter() override;

 protected:
  bool BeginFrame() override;
  void ComposeBackground(COLORREF color) override;
  void ComposeText(const char* text) override;
  void EndFrame() override;
  const char* GetName() const override {
    return use_d3d9ex_ ? "D3D9Ex" : "D3D9";
  }

 private:
  bool EnsureSwapChain(const RECT& r);

 private:
  bool use_d3d9ex_;
  ke::RefPtr<IDirect3D9> api_;
  ke::RefPtr<IDirect3D9Ex> api_ex_;
  ke::RefPtr<IDirect3DDevice9> device_;
  ke::RefPtr<IDirect3DDevice9Ex> device_ex_;
  ke::RefPtr<IDirect3DSwapChain9> swap_chain_;
  ke::RefPtr<IDirect3DSurface9> backbuffer_;
  ke::RefPtr<IDirect3DSurface9> text_surf_;
  ke::RefPtr<IDirect3DSurface9> text_data_;
  ke::AutoPtr<GDITextBuffer> text_buffer_;
};

#endif // _xpresent_presenter_d3d9_h_