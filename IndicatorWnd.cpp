#include "IndicatorWnd.h"
#include "Logger.h"
#include <winrt/Windows.Graphics.Display.Core.h>
#include <winrt/Windows.Foundation.h>

#define LOG_MODULE "IndicatorWnd"

IndicatorWnd::IndicatorWnd() {
  winrt::init_apartment();
}

IndicatorWnd::~IndicatorWnd() {
  winrt::uninit_apartment();
}

void IndicatorWnd::setBrightnessCallback(std::function<void(int)> Callback) {
  BrightnessCallback = std::move(Callback);
}

void IndicatorWnd::resetBrightness(float NewFac) {
  LOGI_V_LN("reset brightness from ", Fac, " to ", NewFac);
  Fac = NewFac;
}

BEGIN_MESSAGE_MAP(IndicatorWnd, CWnd)
  ON_WM_CREATE()
  ON_WM_HOTKEY()
  ON_MESSAGE(WM_USER_RESET_BRIGHTNESS, OnResetBrightness)
END_MESSAGE_MAP()

INT IndicatorWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) {
  if (CWnd::OnCreate(lpCreateStruct) == -1)
    return -1;

  if (RegisterHotKey(m_hWnd, static_cast<int>(HotKeyType::BRIGHTNESS_DOWN),
                     MOD_CONTROL | MOD_WIN | MOD_SHIFT, VK_OEM_COMMA) == 0 ||
      RegisterHotKey(m_hWnd, static_cast<int>(HotKeyType::BRIGHTNESS_UP),
                     MOD_CONTROL | MOD_WIN | MOD_SHIFT, VK_OEM_PERIOD) == 0 ||
      RegisterHotKey(m_hWnd, static_cast<int>(HotKeyType::BRIGHTNESS_SYNC),
                     MOD_CONTROL | MOD_WIN | MOD_SHIFT, VK_OEM_2) == 0) {
    LOGE_V_LN("cannot register hotkeys: ", GetLastError());
  }

  return 0;
}

void IndicatorWnd::OnHotKey(UINT nHotKeyId, UINT nKey1, UINT nKey2) {
  LOGI_V_LN("detected a hotkey press");
  float NewFac = Fac;
  switch (static_cast<HotKeyType>(nHotKeyId)) {
  case HotKeyType::BRIGHTNESS_DOWN:
    NewFac = std::clamp(Fac - 0.1f, 0.f, 1.f);
    break;
  case HotKeyType::BRIGHTNESS_UP:
    NewFac = std::clamp(Fac + 0.1f, 0.f, 1.f);
    break;
  case HotKeyType::BRIGHTNESS_SYNC:
    NewFac = -1;
    break;
  default:
    return CWnd::OnHotKey(nHotKeyId, nKey1, nKey2);
  }

  setBrightness(NewFac);
  CWnd::OnHotKey(nHotKeyId, nKey1, nKey2);
}

void IndicatorWnd::setBrightness(float NewFac) {
  if (NewFac < 0) {
    LOGI_V_LN("invalidated brightness");
    BrightnessCallback(-1);
  } else {
    LOGI_V_LN("changed brightness: ", Fac, "->", NewFac);
    Fac = NewFac;
    BrightnessCallback(int(round(Fac * 100.0f)));
    ShowWindowsBrightnessOverlay(Fac);
  }
}

void IndicatorWnd::ShowWindowsBrightnessOverlay(float brightness) {
  try {
    winrt::Windows::Graphics::Display::Core::HdmiDisplayInformation::GetForCurrentView().ShowBrightnessSlider(brightness);
  }
  catch (winrt::hresult_error const& ex) {
    LOGE_V_LN("Failed to show brightness overlay: ", ex.message().c_str());
  }
}

LRESULT IndicatorWnd::OnResetBrightness(WPARAM wParam, LPARAM lParam) {
  bool ShowIndicator = lParam;
  float NewFac = (int)wParam / 100.0f;

  if (ShowIndicator) {
    LOGI_V_LN("showing indicator");
    setBrightness(NewFac);
  } else {
    LOGI_V_LN("not showing indicator");
    resetBrightness(NewFac);
  }
  return 0;
}
