// vim: set ts=2 sts=2 sw=2 tw=99 et:
#include <stdio.h>
#include <stdlib.h>
#include "present-d3d9.h"

D3D9Presenter::D3D9Presenter(HWND hwnd, bool useD3D9Ex)
  : Presenter(hwnd),
    use_d3d9ex_(useD3D9Ex)
{
  HMODULE d3d9 = LoadLibraryA("d3d9.dll");
  if (!d3d9) {
    SetBroken("d3d9.dll not found");
    return;
  }

  if (use_d3d9ex_) {
    auto d3d9CreateEx = reinterpret_cast<decltype(Direct3DCreate9Ex)*>(
      GetProcAddress(d3d9, "Direct3DCreate9Ex"));
    if (!d3d9CreateEx) {
      SetBroken("Direct3DCreate9Ex not found");
      return;
    }

    HRESULT hr = d3d9CreateEx(D3D_SDK_VERSION, api_ex_.byref());
    if (FAILED(hr) || !api_ex_) {
      SetBroken("Direct3DCreate9Ex failed: %x", hr);
      return;
    }

    api_ = api_ex_;
  } else {
    auto d3d9Create = reinterpret_cast<decltype(Direct3DCreate9)*>(
      GetProcAddress(d3d9, "Direct3DCreate9"));
    if (!d3d9Create) {
      SetBroken("Direct3DCreate9 not found");
      return;
    }

    api_ = d3d9Create(D3D_SDK_VERSION);
    if (!api_) {
      SetBroken("Direct3DCreate9 failed");
      return;
    }
  }

  D3DADAPTER_IDENTIFIER9 ident;
  HRESULT hr = api_->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &ident);
  if (FAILED(hr)) {
    SetBroken("GetAdapterIdentified failed: %x", hr);
    return;
  }

  D3DPRESENT_PARAMETERS pp;
  ZeroMemory(&pp, sizeof(pp));
  pp.BackBufferWidth = 1;
  pp.BackBufferHeight = 1;
  pp.BackBufferFormat = D3DFMT_A8R8G8B8;
  pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  pp.Windowed = TRUE;
  pp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
  pp.hDeviceWindow = hwnd_;

  if (use_d3d9ex_) {
    HRESULT hr = api_ex_->CreateDeviceEx(
      D3DADAPTER_DEFAULT,
      D3DDEVTYPE_HAL,
      hwnd_,
      D3DCREATE_FPU_PRESERVE |
      D3DCREATE_MULTITHREADED |
      D3DCREATE_MIXED_VERTEXPROCESSING,
      &pp,
      nullptr,
      device_ex_.byref());
    if (FAILED(hr) || !device_ex_) {
      SetBroken("CreateDeviceEx failed: %x", hr);
      return;
    }
    device_ = device_ex_;
  } else {
    HRESULT hr = api_->CreateDevice(
      D3DADAPTER_DEFAULT,
      D3DDEVTYPE_HAL,
      hwnd_,
      D3DCREATE_FPU_PRESERVE |
        D3DCREATE_MULTITHREADED |
        D3DCREATE_MIXED_VERTEXPROCESSING,
      &pp,
      device_.byref());
    if (FAILED(hr) || !device_) {
      SetBroken("CreateDevice failed: %x", hr);
      return;
    }
  }
}

D3D9Presenter::~D3D9Presenter()
{
}

bool
D3D9Presenter::BeginFrame()
{
  if (!EnsureSwapChain(bounds_))
    return false;

  HRESULT hr = device_->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 0, 0);
  if (FAILED(hr)) {
    SetBroken("Clear failed: %x", hr);
    return false;
  }

  hr = device_->BeginScene();
  if (FAILED(hr)) {
    SetBroken("BeginScene failed: %x", hr);
    return false;
  }

  return true;
}

void
D3D9Presenter::ComposeBackground(COLORREF color)
{
  D3DRECT rect;
  rect.x1 = bounds_.left;
  rect.y1 = bounds_.top;
  rect.x2 = bounds_.right;
  rect.y2 = bounds_.bottom;

  HRESULT hr = device_->Clear(
    1, &rect,
    D3DCLEAR_TARGET,
    D3DCOLOR_XRGB(GetRValue(color), GetGValue(color), GetBValue(color)),
    0, 0);
  if (FAILED(hr)) {
    SetBroken("Clear failed: %x", hr);
    return;
  }
}

void
D3D9Presenter::ComposeText(const char* text)
{
  static const int kWidth = 200;
  static const int kHeight = 20;

  if (!text_surf_) {
    HRESULT hr = device_->CreateOffscreenPlainSurface(
      kWidth, kHeight,
      D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM,
      text_data_.byref(), nullptr);
    if (FAILED(hr) || !text_data_) {
      SetBroken("CreateOffscreenPlainSurface failed: %x", hr);
      return;
    }

    hr = device_->CreateOffscreenPlainSurface(
      kWidth, kHeight,
      D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
      text_surf_.byref(), nullptr);
    if (FAILED(hr) || !text_surf_) {
      SetBroken("CreateOffscreenPlainSurface failed: %x", hr);
      return;
    }

    text_buffer_ = new GDITextBuffer(kWidth, kHeight);
    if (!text_buffer_->Init()) {
      SetBroken("GDITextBuffer init failed");
      return;
    }
  }

  text_buffer_->Draw(text);
  {
    D3DLOCKED_RECT lock;
    HRESULT hr = text_data_->LockRect(&lock, nullptr, 0);
    if (FAILED(hr)) {
      SetBroken("LockRect failed: %x", hr);
      return;
    }
    if (lock.Pitch == text_buffer_->pitch())
      memcpy(lock.pBits, text_buffer_->bits(), lock.Pitch * kHeight);
    else
      SetBroken("pitch mismatch");
    text_data_->UnlockRect();
  }

  RECT destRect = {
    bounds_.left,
    bounds_.top,
    bounds_.left + kWidth,
    bounds_.top + kHeight
  };

  HRESULT hr = device_->UpdateSurface(text_data_, nullptr, text_surf_, nullptr);
  if (FAILED(hr)) {
    SetBroken("UpdateSurface failed: %x", hr);
    return;
  }
  hr = device_->StretchRect(
    text_surf_,
    nullptr,
    backbuffer_,
    &destRect,
    D3DTEXF_NONE);
  if (FAILED(hr)) {
    SetBroken("StretchRect failed: %x", hr);
    return;
  }
}

void
D3D9Presenter::EndFrame()
{
  HRESULT hr = device_->EndScene();
  if (FAILED(hr)) {
    SetBroken("EndScene failed: %x", hr);
    return;
  }
  hr = swap_chain_->Present(nullptr, nullptr, 0, 0, 0);
  if (FAILED(hr)) {
    SetBroken("Present failed: %x", hr);
    return;
  }
}

bool
D3D9Presenter::EnsureSwapChain(const RECT& r)
{
  if (swap_chain_) {
    HRESULT hr = swap_chain_->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, backbuffer_.byref());
    if (FAILED(hr)) {
      SetBroken("GetBackBuffer failed: %x", hr);
      return false;
    }

    D3DSURFACE_DESC desc;
    hr = backbuffer_->GetDesc(&desc);
    if (FAILED(hr)) {
      SetBroken("GetDesc failed: %x", hr);
      return false;
    }

    if (desc.Width == r.right - r.left &&
        desc.Height == r.bottom - r.top)
    {
      HRESULT hr = device_->SetRenderTarget(0, backbuffer_);
      if (FAILED(hr)) {
        SetBroken("SetRenderTarget failed: %x", hr);
        return false;
      }
      return true;
    }

    swap_chain_ = nullptr;
  }

  D3DPRESENT_PARAMETERS pp;
  ZeroMemory(&pp, sizeof(pp));
  pp.BackBufferFormat = D3DFMT_A8R8G8B8;
  pp.SwapEffect = D3DSWAPEFFECT_COPY;
  pp.Windowed = TRUE;
  pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
  pp.hDeviceWindow = hwnd_;
  if (r.left == r.right || r.top == r.bottom) {
    pp.BackBufferHeight = 1;
    pp.BackBufferWidth = 1;
  }

  HRESULT hr = device_->CreateAdditionalSwapChain(&pp, swap_chain_.byref());
  if (FAILED(hr)) {
    SetBroken("CreateAdditionalSwapChain failed: %x", hr);
    return false;
  }

  hr = swap_chain_->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, backbuffer_.byref());
  if (FAILED(hr)) {
    SetBroken("GetBackBuffer failed: %x", hr);
    return false;
  }

  hr = device_->SetRenderTarget(0, backbuffer_);
  if (FAILED(hr)) {
    SetBroken("SetRenderTarget failed: %x", hr);
    return false;
  }

  return true;
}