#include "SimulatorApp.h"
#include <windows.h>
#include <windowsx.h>

namespace {
using App = voltra::SimulatorApp;
LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* app = reinterpret_cast<App*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        app = static_cast<App*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    if (!app) return DefWindowProc(hwnd, message, wParam, lParam);
    switch (message) {
    case WM_NCHITTEST: {
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ScreenToClient(hwnd, &point);
        return point.y < app->hardware.height()*3/100 ? HTCAPTION : HTCLIENT;
    }
    case WM_ERASEBKGND: return 1;
    case WM_LBUTTONDOWN:
        SetCapture(hwnd);
        app->hardware.pointer(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), true);
        app->ui.readTouch(); // Preserve clicks shorter than LVGL's polling interval.
        return 0;
    case WM_MOUSEMOVE: app->hardware.pointer(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (wParam & MK_LBUTTON) != 0); return 0;
    case WM_LBUTTONUP:
        app->hardware.pointer(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), false);
        app->ui.readTouch();
        ReleaseCapture();
        return 0;
    case WM_CAPTURECHANGED: case WM_KILLFOCUS: app->hardware.pointer(0, 0, false); return 0;
    case WM_MOUSEWHEEL: app->hardware.rotate(GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA); return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) { DestroyWindow(hwnd); return 0; }
        if (wParam == VK_LEFT) app->hardware.rotate(-1);
        if (wParam == VK_RIGHT) app->hardware.rotate(1);
        if ((wParam == VK_RETURN || wParam == VK_SPACE) && !(lParam & (1L<<30))) app->hardware.press();
        if (wParam >= VK_F1 && wParam <= VK_F11)
            app->functionKey(int(wParam - VK_F1 + 1), (lParam & (1L << 30)) != 0);
        return 0;
    case WM_TIMER: app->ui.tick(); InvalidateRect(hwnd, nullptr, FALSE); return 0;
    case WM_PAINT: {
        PAINTSTRUCT paint; HDC dc = BeginPaint(hwnd, &paint);
        BITMAPINFO bitmap{}; bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmap.bmiHeader.biWidth = app->hardware.width(); bitmap.bmiHeader.biHeight = -int(app->hardware.height());
        bitmap.bmiHeader.biPlanes = 1; bitmap.bmiHeader.biBitCount = 32; bitmap.bmiHeader.biCompression = BI_RGB;
        SetDIBitsToDevice(dc, 0, 0, app->hardware.width(), app->hardware.height(), 0, 0, 0,
            app->hardware.height(), app->hardware.pixels().data(), &bitmap, DIB_RGB_COLORS);
        EndPaint(hwnd, &paint); return 0;
    }
    case WM_DESTROY: PostQuitMessage(0); return 0;
    default: return DefWindowProc(hwnd, message, wParam, lParam);
    }
}
}
int main(int argc, char** argv) {
    try {
        const auto options = voltra::SimulatorOptions::parse(argc, argv);
        wchar_t executablePath[32768]{};
        if (!GetModuleFileNameW(nullptr, executablePath, 32768))
            throw std::runtime_error("Could not find executable path");
        App app(options, std::filesystem::path(executablePath).parent_path() / "recent-voltras.txt");
        if (app.render(options)) return 0;
        const int size = options.size;
        SetProcessDPIAware();
        WNDCLASSA windowClass{}; windowClass.lpfnWndProc = windowProc;
        windowClass.hInstance = GetModuleHandle(nullptr); windowClass.lpszClassName = "VoltraOfflineSimulator";
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassA(&windowClass);
        // Make the actual native window round, not just a circle in a square client area.
        constexpr DWORD style = WS_POPUP | WS_SYSMENU;
        HWND window = CreateWindowA(windowClass.lpszClassName, "VOLTRA simulator | F7 online, F8 rep, F9 hold, F10 move",
            style, (GetSystemMetrics(SM_CXSCREEN)-size)/2, (GetSystemMetrics(SM_CYSCREEN)-size)/2, size, size,
            nullptr, nullptr, windowClass.hInstance, &app);
        if (!window) throw std::runtime_error("Window creation failed");
        HRGN circle = CreateEllipticRgn(0, 0, size+1, size+1);
        if (!circle || !SetWindowRgn(window, circle, TRUE)) {
            if (circle) DeleteObject(circle);
            DestroyWindow(window);
            throw std::runtime_error("Could not apply round window mask");
        }
        SetTimer(window, 1, 16, nullptr);
        ShowWindow(window, SW_SHOW);
        MSG message;
        while (GetMessage(&message, nullptr, 0, 0) > 0) { TranslateMessage(&message); DispatchMessage(&message); }
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << "\n"; return 2; }
}
