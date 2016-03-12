// vim: set ts=2 sts=2 sw=2 tw=99 et:
#ifndef _xpresent_presenter_d3d11_h_
#define _xpresent_presenter_d3d11_h_

#include "presenter.h"
#include "textbuffer-gdi.h"
#include <amtl/am-vector.h>
#include <d3d11.h>
#include <d2d1_1.h>

class D3D11Presenter : public Presenter
{
 public:
  D3D11Presenter(HWND hwnd);
  ~D3D11Presenter() override;

 protected:
  bool BeginFrame() override;
  void ComposeBackground(COLORREF color) override;
  void ComposeText(const char* text) override;
  void EndFrame() override;
  const char* GetName() const override {
    return "D3D11";
  }

 private:
  bool EnsureSwapChain(const RECT& r);
  bool UpdateBackbuffer();
  void ComposeTextGDI(const char* text);
  bool ComposeTextDWrite(const char* text);

 private:
  ke::Vector<D3D_FEATURE_LEVEL> feature_levels_;
  ke::RefPtr<IDXGIFactory1> dxgi_;
  ke::RefPtr<IDXGIAdapter1> adapter_;
  ke::RefPtr<ID3D11Device> device_;
  ke::RefPtr<IDXGISwapChain> swap_chain_;
  ke::RefPtr<ID3D11Texture2D> backbuffer_;
  ke::RefPtr<ID3D11DeviceContext> context_;
  ke::RefPtr<ID3D11RenderTargetView> default_view_;

  // DWrite text rendering.
  ke::RefPtr<ID2D1Factory1> d2d1_;
  ke::RefPtr<IDWriteFactory> dwrite_;
  ke::RefPtr<IDWriteTextFormat> text_format_;
  ke::RefPtr<ID3D11Texture2D> text_surface_;
  ke::RefPtr<ID2D1RenderTarget> text_rt_;
  ke::RefPtr<ID2D1SolidColorBrush> text_brush_;
  bool use_dwrite_;

  // GDI text rendering.
  ke::RefPtr<ID3D11Texture2D> text_data_;
  ke::AutoPtr<GDITextBuffer> text_buffer_;
};

#endif // _xpresent_presenter_d3d11_h_