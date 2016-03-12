// vim: set ts=2 sts=2 sw=2 tw=99 et:
#include <stdio.h>
#include <stdlib.h>
#include "console.h"
#include "present-d3d11.h"
#include <dwrite.h>
#include <d2d1_1.h>

D3D11Presenter::D3D11Presenter(HWND hwnd)
  : Presenter(hwnd),
    use_dwrite_(false)
{
  if (IsWin8OrLater())
    feature_levels_.append(D3D_FEATURE_LEVEL_11_1);
  feature_levels_.append(D3D_FEATURE_LEVEL_11_0);
  feature_levels_.append(D3D_FEATURE_LEVEL_10_1);
  feature_levels_.append(D3D_FEATURE_LEVEL_10_0);
  feature_levels_.append(D3D_FEATURE_LEVEL_9_3);

  HMODULE dxgi = LoadLibraryA("dxgi.dll");
  if (!dxgi) {
    SetBroken("dxgi.dll not found");
    return;
  }

  auto createDXGIFactory1 = reinterpret_cast<decltype(CreateDXGIFactory1)*>(
    GetProcAddress(dxgi, "CreateDXGIFactory1"));
  if (!createDXGIFactory1) {
    SetBroken("CreateDXGIFactory1 not found");
    return;
  }

  HRESULT hr = createDXGIFactory1(__uuidof(IDXGIFactory1), dxgi_.address());
  if (FAILED(hr) || !dxgi_) {
    SetBroken("CreateDXGIFactory1 failed: %x", hr);
    return;
  }

  hr = dxgi_->EnumAdapters1(0, adapter_.byref());
  if (FAILED(hr) || !adapter_) {
    SetBroken("EnumAdapters1 failed: %x", hr);
    return;
  }

  HMODULE d3d11 = LoadLibraryA("d3d11.dll");
  if (!d3d11) {
    SetBroken("d3d11.dll not found");
    return;
  }

  auto createDevice = reinterpret_cast<decltype(D3D11CreateDevice)*>(
    GetProcAddress(d3d11, "D3D11CreateDevice"));
  if (!createDevice) {
    SetBroken("D3D11CreateDevice not found");
    return;
  }

  hr = createDevice(
    adapter_,
    D3D_DRIVER_TYPE_UNKNOWN,
    nullptr,
    D3D11_CREATE_DEVICE_BGRA_SUPPORT |
      D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS,
    feature_levels_.buffer(), (UINT)feature_levels_.length(),
    D3D11_SDK_VERSION,
    device_.byref(),
    nullptr, nullptr);
  if (FAILED(hr) || !device_) {
    SetBroken("D3D11CreateDevice failed: %x", hr);
    return;
  }

  DXGI_SWAP_CHAIN_DESC cd;
  ZeroMemory(&cd, sizeof(cd));
  cd.BufferDesc.Width = 0;
  cd.BufferDesc.Height = 0;
  cd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  cd.BufferDesc.RefreshRate.Numerator = 60;
  cd.BufferDesc.RefreshRate.Denominator = 1;
  cd.SampleDesc.Count = 1;
  cd.SampleDesc.Quality = 0;
  cd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  cd.BufferCount = 1;
  cd.OutputWindow = hwnd_;
  cd.Windowed = TRUE;
  cd.Flags = 0;
  cd.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;

  hr = dxgi_->CreateSwapChain(device_, &cd, swap_chain_.byref());
  if (FAILED(hr) || !swap_chain_) {
    SetBroken("CreateSwapChain: %x", hr);
    return;
  }

  device_->GetImmediateContext(context_.byref());
  if (!context_) {
    SetBroken("GetImmediateContext failed");
    return;
  }

  // D3D11 setup done. Set up D2D/DWrite now.

  HMODULE d2d1 = ::LoadLibraryA("d2d1.dll");
  HMODULE dwrite = ::LoadLibraryA("dwrite.dll");
  if (!d2d1 || !dwrite)
    return;

  HRESULT(WINAPI *d2d1CreateFactoryFn)(
    D2D1_FACTORY_TYPE factoryType,
    REFIID iid,
    const D2D1_FACTORY_OPTIONS*pFactoryOptions,
    void** address);
  d2d1CreateFactoryFn = reinterpret_cast<decltype(d2d1CreateFactoryFn)>(
      ::GetProcAddress(d2d1, "D2D1CreateFactory"));
  if (!d2d1CreateFactoryFn) {
    SetBroken("D2D1CreateFactory not found");
    return;
  }

  D2D1_FACTORY_OPTIONS options;
  options.debugLevel = D2D1_DEBUG_LEVEL_NONE;

  ke::RefPtr<ID2D1Factory> factory;
  hr = d2d1CreateFactoryFn(
    D2D1_FACTORY_TYPE_MULTI_THREADED,
    __uuidof(ID2D1Factory),
    &options,
    factory.address());
  if (FAILED(hr) || !factory) {
    SetBroken("Failed to create a D2D1 factory");
    return;
  }
  hr = factory->QueryInterface(d2d1_.byref());
  if (FAILED(hr) || !d2d1_)
    return;

  auto dwriteCreateFactory = reinterpret_cast<decltype(DWriteCreateFactory)*>(
    ::GetProcAddress(dwrite, "DWriteCreateFactory"));
  if (!dwriteCreateFactory) {
    SetBroken("DWriteCreateFactory not found");
    d2d1_ = nullptr;
    return;
  }

  hr = dwriteCreateFactory(
    DWRITE_FACTORY_TYPE_SHARED,
    __uuidof(IDWriteFactory),
    reinterpret_cast<IUnknown**>(dwrite_.address()));
  if (FAILED(hr) || !dwrite_) {
    SetBroken("DWriteCreateFactory failed: %x", hr);
    return;
  }

  use_dwrite_ = true;
}

D3D11Presenter::~D3D11Presenter()
{
}

bool D3D11Presenter::BeginFrame()
{
  if (!EnsureSwapChain(bounds_))
    return false;
  if (!default_view_ && !UpdateBackbuffer())
    return false;

  FLOAT color[4] = { 1.0, 1.0, 1.0, 1.0 };
  context_->ClearRenderTargetView(default_view_, color);

  return true;
}

void
D3D11Presenter::ComposeBackground(COLORREF color)
{
  FLOAT rgba[4] = {
    GetRValue(color) / 255.0f,
    GetGValue(color) / 255.0f,
    GetBValue(color) / 255.0f,
    1.0f
  };
  context_->ClearRenderTargetView(default_view_, rgba);
}

void
D3D11Presenter::ComposeText(const char* text)
{
  if (use_dwrite_ && !ComposeTextDWrite(text)) {
    // Don't try to use DWrite again.
    use_dwrite_ = false;

    ComposeTextGDI(text);
  }
}

bool
D3D11Presenter::ComposeTextDWrite(const char* text)
{
  static const int kWidth = 200;
  static const int kHeight = 20;

  if (!text_format_) {
    HRESULT hr = dwrite_->CreateTextFormat(
      L"Consolas",
      nullptr,
      DWRITE_FONT_WEIGHT_REGULAR,
      DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL,
      12.0f,
      L"en-us",
      text_format_.byref());
    if (FAILED(hr) || !text_format_)
      return false;

    hr = text_format_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    if (FAILED(hr))
      return false;
    hr = text_format_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    if (FAILED(hr))
      return false;
  }

  if (!text_surface_) {
    CD3D11_TEXTURE2D_DESC desc(DXGI_FORMAT_B8G8R8A8_UNORM, kWidth, kHeight, 1, 1);
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

    HRESULT hr = device_->CreateTexture2D(&desc, nullptr, text_surface_.byref());
    if (FAILED(hr) || !text_surface_)
      return false;
  }

  if (!text_rt_) {
    ke::RefPtr<IDXGISurface> surface;
    HRESULT hr = text_surface_->QueryInterface(__uuidof(IDXGISurface), surface.address());
    if (FAILED(hr) || !surface)
      return false;

    D2D1_RENDER_TARGET_PROPERTIES props =
      D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));

    hr = d2d1_->CreateDxgiSurfaceRenderTarget(surface, props, text_rt_.byref());
    if (FAILED(hr) || !text_rt_)
      return false;
  }

  if (!text_brush_) {
    HRESULT hr = text_rt_->CreateSolidColorBrush(
      D2D1::ColorF(D2D1::ColorF::Black, 1.0f),
      text_brush_.byref());
    if (FAILED(hr) || !text_brush_)
      return false;
  }

  WCHAR buffer[256];
  int rv = MultiByteToWideChar(
    CP_ACP, 0,
    text, -1,
    buffer,
    sizeof(buffer) / sizeof(WCHAR));
  if (rv == 0)
    return false;

  text_rt_->BeginDraw();
  text_rt_->Clear(D2D1::ColorF(D2D1::ColorF::White, 1.0f));
  text_rt_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_ALIASED);
  text_rt_->DrawTextA(
    buffer, rv,
    text_format_,
    D2D1::RectF(0, 0, float(kWidth), float(kHeight)),
    text_brush_);
  HRESULT hr = text_rt_->EndDraw();
  if (FAILED(hr))
    return false;

  context_->CopySubresourceRegion(
    backbuffer_, 0,
    0, 0, 0,
    text_surface_, 0,
    nullptr);

  return true;
}

void
D3D11Presenter::ComposeTextGDI(const char* text)
{
  static const int kWidth = 200;
  static const int kHeight = 20;

  if (!text_data_) {
    CD3D11_TEXTURE2D_DESC desc(DXGI_FORMAT_B8G8R8A8_UNORM, kWidth, kHeight);
    desc.MipLevels = 1;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;

    HRESULT hr = device_->CreateTexture2D(&desc, nullptr, text_data_.byref());
    if (FAILED(hr) || !text_data_) {
      SetBroken("CreateTexture2D failed: %x", hr);
      return;
    }

    text_buffer_ = new GDITextBuffer(kWidth, kHeight);
    if (!text_buffer_->Init()) {
      SetBroken("GDITextBuffer init failed");
      return;
    }
  }

  D3D11_MAPPED_SUBRESOURCE map;
  HRESULT hr = context_->Map(text_data_, 0, D3D11_MAP_WRITE, 0, &map);
  if (FAILED(hr)) {
    SetBroken("Map failed: %x", hr);
    return;
  }

  text_buffer_->Draw(text);

  if (map.RowPitch == text_buffer_->pitch()) {
    memcpy(map.pData, text_buffer_->bits(), map.RowPitch * kHeight);
  } else {
    uint8_t* dest = (uint8_t*)map.pData;
    uint8_t* src = text_buffer_->bits();
    for (int i = 0; i < kHeight; i++) {
      memcpy(dest, src, kWidth * 4);
      dest += map.RowPitch;
      src += text_buffer_->pitch();
    }
  }

  context_->Unmap(text_data_, 0);
  context_->CopySubresourceRegion(
    backbuffer_, 0,
    0, 0, 0,
    text_data_, 0,
    nullptr);
}

void D3D11Presenter::EndFrame()
{
  HRESULT hr = swap_chain_->Present(0, 0);
  if (FAILED(hr)) {
    SetBroken("EndFrame failed: %x", hr);
    return;
  }
}

bool
D3D11Presenter::EnsureSwapChain(const RECT& r)
{
  DXGI_SWAP_CHAIN_DESC cd;
  HRESULT hr = swap_chain_->GetDesc(&cd);
  if (FAILED(hr)) {
    SetBroken("SwapChain GetDesc failed: %x", hr);
    return false;
  }

  if (cd.BufferDesc.Width == r.right - r.left &&
      cd.BufferDesc.Height == r.bottom - r.top)
  {
    return true;
  }

  backbuffer_ = nullptr;
  default_view_ = nullptr;

  hr = swap_chain_->ResizeBuffers(
    1,
    r.right - r.left,
    r.bottom - r.top,
    DXGI_FORMAT_B8G8R8A8_UNORM,
    0);
  if (FAILED(hr)) {
    SetBroken("ResizeBuffers failed: %x", hr);
    return false;
  }

  return true;
}

bool
D3D11Presenter::UpdateBackbuffer()
{
  HRESULT hr = swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), backbuffer_.address());
  if (FAILED(hr) || !backbuffer_) {
    SetBroken("GetBuffer failed: %x", hr);
    return false;
  }

  hr = device_->CreateRenderTargetView(backbuffer_, nullptr, default_view_.byref());
  if (FAILED(hr) || !default_view_) {
    SetBroken("CreateRenderTargetView failed: %x", hr);
    return false;
  }

  return true;
}