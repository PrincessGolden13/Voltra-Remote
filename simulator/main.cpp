#include "DesktopHardwarePlatform.h"
#include "MockTransport.h"
#include "ui/UiController.h"
#include "DesktopDeviceMemory.h"
#include <windows.h>
#include <windowsx.h>
#include <fstream>
#include <iostream>
#include <string>

namespace {
struct App {
    voltra::DesktopHardwarePlatform hardware;
    voltra::MockTransport transport;
    voltra::VoltraController controller;
    voltra::DesktopDeviceMemory memory;
    voltra::TrainingApplication application;
    voltra::UiController ui;
    static std::filesystem::path memoryPath() {
        wchar_t path[32768]{};
        GetModuleFileNameW(nullptr, path, 32768);
        return std::filesystem::path(path).parent_path() / "recent-voltras.txt";
    }
    explicit App(uint16_t size, bool persist) : hardware(size), controller(transport, {}, &transport),
        memory(memoryPath()), application(transport, controller, transport, persist ? &memory : nullptr),
        ui(hardware, application) {}
};
void saveBitmap(const voltra::DesktopHardwarePlatform& display, const std::string& path) {
    BITMAPFILEHEADER file{};
    BITMAPINFOHEADER info{};
    info.biSize = sizeof(info); info.biWidth = display.width(); info.biHeight = -int(display.height());
    info.biPlanes = 1; info.biBitCount = 32; info.biCompression = BI_RGB;
    info.biSizeImage = DWORD(display.pixels().size()*sizeof(uint32_t));
    file.bfType = 0x4d42; file.bfOffBits = sizeof(file)+sizeof(info);
    file.bfSize = file.bfOffBits + info.biSizeImage;
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(&file), sizeof(file));
    out.write(reinterpret_cast<const char*>(&info), sizeof(info));
    out.write(reinterpret_cast<const char*>(display.pixels().data()), info.biSizeImage);
    if (!out) throw std::runtime_error("Could not save render");
}
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
        return point.y < app->hardware.height()*10/100 ? HTCAPTION : HTCLIENT;
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
        if (wParam >= VK_F1 && wParam <= VK_F4) {
            app->application.leave();
            app->application.scan();
            app->application.select(voltra::MockTransport::deviceId);
            app->transport.setProfile(static_cast<voltra::SimulatorProfile>(wParam-VK_F1));
            app->ui.navigate(voltra::Screen::Main);
        }
        if (wParam == VK_F5) { app->application.leave(); app->ui.navigate(voltra::Screen::Devices); }
        if (wParam == VK_F6) app->application.scan();
        if (wParam == VK_F7) app->transport.toggleAvailability();
        if (wParam == VK_F8 && !(lParam & (1L<<30))) app->transport.completeRep();
        if (wParam == VK_F9) app->transport.setHeld(true);
        if (wParam == VK_F10) app->transport.setHeld(false);
        if (wParam == VK_F11) app->transport.ignoreNextRequest();
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
        int size = 480, profile = 0, screen = 0, device = 0, dropMode = 0;
        std::string renderPath;
        for (int i = 1; i < argc; ++i) {
            const std::string option = argv[i];
            if (i+1 >= argc) throw std::runtime_error("Missing argument value");
            if (option == "--size") size = std::stoi(argv[++i]);
            else if (option == "--render") renderPath = argv[++i];
            else if (option == "--profile") profile = std::stoi(argv[++i]);
            else if (option == "--screen") screen = std::stoi(argv[++i]);
            else if (option == "--device") device = std::stoi(argv[++i]);
            else if (option == "--drop-mode") dropMode = std::stoi(argv[++i]);
            else throw std::runtime_error("Unknown argument");
        }
        if ((size != 360 && size != 466 && size != 480) || profile < 0 || profile > 3)
            throw std::runtime_error("Supported sizes: 360, 466, 480. Profiles: 0 unloaded, 1 loaded, 2 stale, 3 coworker unknown.");
        if (screen < 0 || screen > 4 || device < 0 || device > 4 || dropMode < 0 || dropMode > 1)
            throw std::runtime_error("Screens 0-4; devices 0-4; drops 0 stairs, 1 rest-pause.");
        App app{uint16_t(size), renderPath.empty()};
        app.ui.tick();
        if (screen != 0) {
            app.application.select(device == 0 ? voltra::MockTransport::deviceId :
                std::string(voltra::MockTransport::deviceId)+'-'+std::to_string(device));
            app.transport.setProfile(static_cast<voltra::SimulatorProfile>(profile));
            app.application.tick(app.hardware.nowMs());
            if (dropMode == 1) {
                auto d = app.application.drops(); d.mode = static_cast<voltra::DropMode>(dropMode);
                app.application.applyDrops(d);
            }
            app.ui.navigate(static_cast<voltra::Screen>(screen));
            app.ui.tick();
        }
        if (!renderPath.empty()) {
            // Native LVGL off-screen output; no window, radio, browser or Internet.
            lv_refr_now(nullptr);
            saveBitmap(app.hardware, renderPath);
            std::cout << "Rendered offline LVGL simulator to " << renderPath << "\n";
            return 0;
        }
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
