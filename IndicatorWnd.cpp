#include "IndicatorWnd.h"
#include "Logger.h"
#include <winrt/Windows.Foundation.h>

#define LOG_MODULE "IndicatorWnd"

class BrightnessOverlay : public CWnd
{
public:
    void Show(float brightness);

private:
    void Create();
    void DrawOverlay(float brightness);
    void PositionWindow();

    DECLARE_MESSAGE_MAP()
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
};

BEGIN_MESSAGE_MAP(BrightnessOverlay, CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_TIMER()
END_MESSAGE_MAP()

void BrightnessOverlay::Show(float brightness)
{
    if (!m_hWnd)
        Create();

    PositionWindow();
    DrawOverlay(brightness);

    SetTimer(1, 3000, nullptr);
}

void BrightnessOverlay::Create()
{
    CreateEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        AfxRegisterWndClass(0), _T("Brightness Overlay"), WS_POPUP,
        0, 0, 250, 70, // Width and height of the overlay
        nullptr, nullptr);
}

void BrightnessOverlay::PositionWindow()
{
    CRect screenRect;
    GetDesktopWindow()->GetWindowRect(&screenRect);
    
    CRect windowRect;
    GetWindowRect(&windowRect);
    
    int x = (screenRect.Width() - windowRect.Width()) / 2;
    int y = screenRect.bottom - windowRect.Height() - 20; // 20 pixels margin from bottom
    
    SetWindowPos(&wndTopMost, x, y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
}

void BrightnessOverlay::DrawOverlay(float brightness)
{
    CClientDC dc(this);
    CRect rect;
    GetClientRect(&rect);

    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap bitmap;
    bitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
    CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

    // Draw background
    memDC.FillSolidRect(rect, RGB(32, 32, 32)); // Dark gray background

    // Draw brightness bar
    CRect barRect = rect;
    barRect.DeflateRect(10, 30, 10, 30); // Adjust these values to position the bar
    memDC.FillSolidRect(barRect, RGB(64, 64, 64)); // Lighter gray for the empty part of the bar

    int barWidth = static_cast<int>(barRect.Width() * brightness);
    CRect filledBarRect = barRect;
    filledBarRect.right = filledBarRect.left + barWidth;
    memDC.FillSolidRect(filledBarRect, RGB(255, 255, 255)); // White for the filled part

    // Draw brightness percentage text
    CString text;
    text.Format(_T("%d%%"), static_cast<int>(brightness * 100));
    memDC.SetTextColor(RGB(255, 255, 255)); // White text
    memDC.SetBkMode(TRANSPARENT);
    CFont font;
    font.CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                    DEFAULT_PITCH | FF_DONTCARE, _T("Segoe UI"));
    CFont* pOldFont = memDC.SelectObject(&font);
    memDC.DrawText(text, rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    memDC.SelectObject(pOldFont);

    // Update the layered window
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    CPoint ptSrc(0, 0);
    CSize sizeWnd = rect.Size();
    UpdateLayeredWindow(&dc, NULL, &sizeWnd, &memDC, &ptSrc, 0, &blend, ULW_ALPHA);

    memDC.SelectObject(pOldBitmap);
}

void BrightnessOverlay::OnPaint()
{
    CPaintDC dc(this);
}

BOOL BrightnessOverlay::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}

void BrightnessOverlay::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1)
    {
        ShowWindow(SW_HIDE);
        KillTimer(1);
    }
    CWnd::OnTimer(nIDEvent);
}

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
    }
    else {
        LOGI_V_LN("changed brightness: ", Fac, "->", NewFac);
        Fac = NewFac;
        BrightnessCallback(int(round(Fac * 100.0f)));
        ShowWindowsBrightnessOverlay(Fac);
    }
}

void IndicatorWnd::ShowWindowsBrightnessOverlay(float brightness) {
    static BrightnessOverlay brightnessOverlay;
    brightnessOverlay.Show(brightness);
}

LRESULT IndicatorWnd::OnResetBrightness(WPARAM wParam, LPARAM lParam) {
    bool ShowIndicator = lParam;
    float NewFac = (int)wParam / 100.0f;
    if (ShowIndicator) {
        LOGI_V_LN("showing indicator");
        setBrightness(NewFac);
    }
    else {
        LOGI_V_LN("not showing indicator");
        resetBrightness(NewFac);
    }
    return 0;
}