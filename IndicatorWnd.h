#pragma once
#include <afxwin.h>
#include <functional>
#include "resource.h"

#define WM_USER_RESET_BRIGHTNESS (WM_USER + 2)

class IndicatorWnd : public CWnd {
public:
  IndicatorWnd();
  ~IndicatorWnd();
  void setBrightnessCallback(std::function<void(int)> Callback);
  void setBrightness(float NewFac);
  void resetBrightness(float NewFac);
protected:
  DECLARE_MESSAGE_MAP()
  afx_msg INT OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnHotKey(UINT nHotKeyId, UINT nKey1, UINT nKey2);
  afx_msg LRESULT OnResetBrightness(WPARAM wParam, LPARAM lParam);
private:
  float Fac = 1;
  std::function<void(int)> BrightnessCallback;
  enum class HotKeyType { BRIGHTNESS_DOWN, BRIGHTNESS_UP, BRIGHTNESS_SYNC };
  void ShowWindowsBrightnessOverlay(float brightness);
};
