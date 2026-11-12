#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <windowsx.h>
#include <algorithm>
using std::min;
using std::max;
#include <objidl.h>
#include <gdiplus.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <shellapi.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "resource.h"
#include "core_logic.hpp"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

namespace {

namespace fs = std::filesystem;
using namespace Gdiplus;

// Custom Window Messages
constexpr UINT WM_APP_LOG          = WM_APP + 10;
constexpr UINT WM_APP_PROGRESS     = WM_APP + 11;
constexpr UINT WM_APP_MANIFEST     = WM_APP + 12;
constexpr UINT WM_APP_INSTALL_DONE = WM_APP + 13;
constexpr UINT WM_APP_RESTORE_DONE = WM_APP + 14;
constexpr UINT WM_APP_STARTUP_POP  = WM_APP + 15;

constexpr UINT_PTR TIMER_SCREENSHOT = 901;

// Umineko Dark Mahogany / Crimson / Gold Palette
Color C_BG_MAIN(255, 19, 11, 15);           // #130b0f
Color C_BG_HEADER(255, 27, 15, 21);         // #1b0f15
Color C_BORDER_HDR(255, 46, 24, 33);        // #2e1821
Color C_BG_CARD(255, 30, 18, 24);           // #1e1218
Color C_BG_INPUT(255, 18, 10, 14);          // #120a0e
Color C_BORDER_SUBTLE(255, 59, 34, 45);     // #3b222d
Color C_BORDER_GOLD(255, 156, 124, 47);     // #9c7c2f
Color C_BORDER_LOCKED(255, 122, 36, 36);    // #7a2424
Color C_CRIMSON(255, 158, 27, 50);          // #9e1b32
Color C_CRIMSON_HOVER(255, 188, 34, 61);    // #bc223d
Color C_GOLD(255, 223, 184, 76);            // #dfb84c
Color C_TEXT_PRI(255, 245, 236, 238);       // #f5ecee
Color C_TEXT_MUTED(255, 181, 158, 165);     // #b59ea5
Color C_WARN_AMBER(255, 235, 168, 101);     // #eba865

struct BlockedItem {
    std::wstring game_title;
    std::wstring reason;
};

struct ProgressMsg {
    std::wstring stage;
    double fraction = 0.0;
    std::wstring detail;
};

struct InstallDoneMsg {
    bool ok = false;
    bool cancelled = false;
    std::vector<std::wstring> installed_names;
    std::wstring error_msg;
};

struct RestoreDoneMsg {
    bool ok = false;
    std::string arc;
    int restored_count = 0;
    std::wstring error_msg;
};

std::mutex g_ui_mtx;
HWND g_main_hwnd = nullptr;

template <typename T>
void PostUiMsg(UINT msg, T* payload) {
    std::lock_guard<std::mutex> lk(g_ui_mtx);
    if (g_main_hwnd && IsWindow(g_main_hwnd)) {
        if (!PostMessageW(g_main_hwnd, msg, 0, reinterpret_cast<LPARAM>(payload))) {
            delete payload;
        }
    } else {
        delete payload;
    }
}

void ApplyDarkTitleBar(HWND hwnd) {
    BOOL dark = TRUE;
    // DWMWA_USE_IMMERSIVE_DARK_MODE = 20 (Win11 / Win10 20H1+), fallback 19
    if (FAILED(DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark)))) {
        DwmSetWindowAttribute(hwnd, 19, &dark, sizeof(dark));
    }
}

UINT QueryWindowDpi(HWND hwnd) {
    using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
    static GetDpiForWindowFn fn = reinterpret_cast<GetDpiForWindowFn>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow")
    );
    if (fn && hwnd) {
        UINT d = fn(hwnd);
        if (d >= 96) return d;
    }
    HDC hdc = GetDC(hwnd);
    UINT d = hdc ? (UINT)GetDeviceCaps(hdc, LOGPIXELSX) : 96u;
    if (hdc) ReleaseDC(hwnd, hdc);
    return d >= 96 ? d : 96u;
}

void AddRoundedRect(GraphicsPath& path, const RectF& r, REAL radius) {
    if (radius <= 0.5f) {
        path.AddRectangle(r);
        return;
    }
    REAL d = std::min({radius * 2.0f, r.Width, r.Height});
    path.AddArc(r.X, r.Y, d, d, 180.0f, 90.0f);
    path.AddArc(r.X + r.Width - d, r.Y, d, d, 270.0f, 90.0f);
    path.AddArc(r.X + r.Width - d, r.Y + r.Height - d, d, d, 0.0f, 90.0f);
    path.AddArc(r.X, r.Y + r.Height - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
}

void FillRoundRect(Graphics& g, const RectF& r, REAL radius, const Color& fill) {
    GraphicsPath p;
    AddRoundedRect(p, r, radius);
    SolidBrush br(fill);
    g.FillPath(&br, &p);
}

void DrawRoundRect(Graphics& g, const RectF& r, REAL radius, const Color& stroke, REAL width = 1.0f) {
    GraphicsPath p;
    RectF inset(r.X + width * 0.5f, r.Y + width * 0.5f, r.Width - width, r.Height - width);
    AddRoundedRect(p, inset, std::max(0.0f, radius - width * 0.5f));
    Pen pen(stroke, width);
    g.DrawPath(&pen, &p);
}

void FillAndStrokeRoundRect(Graphics& g, const RectF& r, REAL radius, const Color& fill, const Color& stroke, REAL width = 1.0f) {
    FillRoundRect(g, r, radius, fill);
    DrawRoundRect(g, r, radius, stroke, width);
}

bool PtInRectF(const RectF& r, float x, float y) {
    return x >= r.X && x < (r.X + r.Width) && y >= r.Y && y < (r.Y + r.Height);
}

std::unique_ptr<Image> LoadPngResource(int res_id) {
    HMODULE hMod = GetModuleHandleW(nullptr);
    HRSRC hRes = FindResourceW(hMod, MAKEINTRESOURCEW(res_id), RT_BITMAP);
    if (!hRes) return nullptr;
    DWORD sz = SizeofResource(hMod, hRes);
    HGLOBAL hGlob = LoadResource(hMod, hRes);
    if (!hGlob || sz < sizeof(BITMAPINFOHEADER)) return nullptr;
    const auto* bih = static_cast<const BITMAPINFOHEADER*>(LockResource(hGlob));
    if (!bih || bih->biBitCount != 32 || bih->biWidth <= 0 || bih->biHeight == 0) return nullptr;

    int w = bih->biWidth;
    int h = std::abs(bih->biHeight);
    bool bottom_up = (bih->biHeight > 0);
    const BYTE* src_pixels = reinterpret_cast<const BYTE*>(bih) + bih->biSize;
    size_t needed = (size_t)w * (size_t)h * 4;
    if (bih->biSize + needed > sz) return nullptr;

    auto bmp = std::make_unique<Bitmap>(w, h, PixelFormat32bppARGB);
    BitmapData bd{};
    Rect full(0, 0, w, h);
    if (bmp->LockBits(&full, ImageLockModeWrite, PixelFormat32bppARGB, &bd) != Ok) return nullptr;
    for (int y = 0; y < h; ++y) {
        int src_y = bottom_up ? (h - 1 - y) : y;
        std::memcpy(
            static_cast<BYTE*>(bd.Scan0) + (size_t)y * (size_t)bd.Stride,
            src_pixels + (size_t)src_y * (size_t)w * 4,
            (size_t)w * 4
        );
    }
    bmp->UnlockBits(&bd);
    return bmp;
}

fs::path SettingsPath() {
    return umicore::GetAppDataDir() / L"settings.json";
}

bool IsStartupNoticeHidden() {
    fs::path p = SettingsPath();
    std::error_code ec;
    if (!fs::is_regular_file(p, ec)) return false;
    std::string j = umicore::ReadFileBytesStr(p, 16384);
    return umicore::JsonExtractBool(j, "hide_07th_startup_notice", false);
}

void SaveHideStartupNotice(bool hide) {
    std::string j = "{\n  \"hide_07th_startup_notice\": " + std::string(hide ? "true" : "false") + "\n}\n";
    umicore::WriteUtf8File(SettingsPath(), j);
}

std::wstring PickFolderDialog(HWND owner, const std::wstring& title, const fs::path& initial_dir) {
    std::wstring picked;
    IFileOpenDialog* pfd = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd))) || !pfd) {
        return L"";
    }
    DWORD opts = 0;
    pfd->GetOptions(&opts);
    pfd->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    pfd->SetTitle(title.c_str());

    std::error_code ec;
    if (!initial_dir.empty() && fs::is_directory(initial_dir, ec)) {
        IShellItem* psiFolder = nullptr;
        if (SUCCEEDED(SHCreateItemFromParsingName(initial_dir.c_str(), nullptr, IID_PPV_ARGS(&psiFolder))) && psiFolder) {
            pfd->SetFolder(psiFolder);
            psiFolder->Release();
        }
    }

    if (SUCCEEDED(pfd->Show(owner))) {
        IShellItem* psi = nullptr;
        if (SUCCEEDED(pfd->GetResult(&psi)) && psi) {
            PWSTR pszPath = nullptr;
            if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)) && pszPath) {
                picked = pszPath;
                CoTaskMemFree(pszPath);
            }
            psi->Release();
        }
    }
    pfd->Release();
    return picked;
}

std::wstring PickZipFileDialog(HWND owner) {
    std::wstring picked;
    IFileOpenDialog* pfd = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd))) || !pfd) {
        return L"";
    }
    COMDLG_FILTERSPEC spec[] = {
        {L"ZIP Arşivi (*.zip)", L"*.zip"}
    };
    pfd->SetFileTypes(1, spec);
    pfd->SetTitle(L"Türkçe yama ZIP paketini seçin");
    DWORD opts = 0;
    pfd->GetOptions(&opts);
    pfd->SetOptions(opts | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);

    if (SUCCEEDED(pfd->Show(owner))) {
        IShellItem* psi = nullptr;
        if (SUCCEEDED(pfd->GetResult(&psi)) && psi) {
            PWSTR pszPath = nullptr;
            if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)) && pszPath) {
                picked = pszPath;
                CoTaskMemFree(pszPath);
            }
            psi->Release();
        }
    }
    pfd->Release();
    return picked;
}

bool SaveWindowScreenshotPng(HWND hwnd, const fs::path& out_png) {
    RECT wr{};
    if (!GetWindowRect(hwnd, &wr)) return false;
    int w = wr.right - wr.left;
    int h = wr.bottom - wr.top;
    if (w <= 0 || h <= 0) return false;

    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hbm = CreateCompatibleBitmap(hdcScreen, w, h);
    HGDIOBJ old = SelectObject(hdcMem, hbm);

    // PW_RENDERFULLCONTENT = 0x00000002
    if (!PrintWindow(hwnd, hdcMem, 0x00000002)) {
        PrintWindow(hwnd, hdcMem, 0);
    }

    Bitmap bmp(hbm, nullptr);
    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size);
    bool saved = false;
    if (size > 0) {
        std::vector<BYTE> buf(size);
        auto* encoders = reinterpret_cast<ImageCodecInfo*>(buf.data());
        GetImageEncoders(num, size, encoders);
        for (UINT i = 0; i < num; ++i) {
            if (wcscmp(encoders[i].MimeType, L"image/png") == 0) {
                std::error_code ec;
                fs::create_directories(out_png.parent_path(), ec);
                saved = (bmp.Save(out_png.c_str(), &encoders[i].Clsid, nullptr) == Ok);
                break;
            }
        }
    }

    SelectObject(hdcMem, old);
    DeleteObject(hbm);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    return saved;
}

// ============================================================================
// Modal 07th-Mod Prerequisite / Blocked Popup Dialog
// ============================================================================
class Mod07thDialog {
public:
    static void Show(HWND parent, const std::vector<BlockedItem>& blocked, bool allow_dismiss_forever) {
        Mod07thDialog dlg(parent, blocked, allow_dismiss_forever);
        dlg.RunModal();
    }

private:
    HWND parent_ = nullptr;
    HWND hwnd_ = nullptr;
    std::vector<BlockedItem> blocked_;
    bool is_blocked_ = false;
    bool is_steam_reset_ = false;
    bool allow_dismiss_ = false;
    bool dont_show_checked_ = false;
    bool closed_ = false;
    int hover_id_ = 0; // 1 = open 07th, 2 = close, 3 = checkbox

    RectF rc_checkbox_{};
    RectF rc_btn_open_{};
    RectF rc_btn_close_{};

    Mod07thDialog(HWND parent, const std::vector<BlockedItem>& blocked, bool allow_dismiss)
        : parent_(parent), blocked_(blocked) {
        is_blocked_ = !blocked_.empty();
        allow_dismiss_ = allow_dismiss && !is_blocked_;
        for (const auto& b : blocked_) {
            if (b.reason.find(L"Steam") != std::wstring::npos || b.reason.find(L"sıfırlamış") != std::wstring::npos) {
                is_steam_reset_ = true;
            }
        }
    }

    float S(float v) const {
        return v * (float)QueryWindowDpi(hwnd_ ? hwnd_ : parent_) / 96.0f;
    }

    void RunModal() {
        static bool registered = false;
        HINSTANCE hInst = GetModuleHandleW(nullptr);
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = &Mod07thDialog::StaticWndProc;
            wc.hInstance = hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_APP_ICON));
            wc.hIconSm = wc.hIcon;
            wc.lpszClassName = L"UminekoTR_Mod07thDlg";
            RegisterClassExW(&wc);
            registered = true;
        }

        std::wstring wtitle;
        if (is_blocked_) {
            wtitle = is_steam_reset_
                ? L"DİKKAT: 07th-Mod Aktif Değil · Kurulum Kilitlendi!"
                : L"DİKKAT: 07th-Mod Gerekli · Kurulum Kilitlendi!";
        } else {
            wtitle = L"Önemli Uyarı · 07th-Mod Ön Koşulu";
        }

        UINT dpi = QueryWindowDpi(parent_);
        int client_w = (int)std::lround(585.0 * dpi / 96.0);
        int client_h = (int)std::lround((is_blocked_ ? 440.0 : (allow_dismiss_ ? 400.0 : 370.0)) * dpi / 96.0);

        DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
        RECT wr{0, 0, client_w, client_h};
        AdjustWindowRectEx(&wr, style, FALSE, 0);
        int win_w = wr.right - wr.left;
        int win_h = wr.bottom - wr.top;

        RECT pr{};
        GetWindowRect(parent_, &pr);
        int x = pr.left + std::max<int>(0, (int)((pr.right - pr.left) - win_w) / 2);
        int y = pr.top + std::max<int>(0, (int)((pr.bottom - pr.top) - win_h) / 2);

        if (parent_) EnableWindow(parent_, FALSE);

        hwnd_ = CreateWindowExW(
            WS_EX_DLGMODALFRAME,
            L"UminekoTR_Mod07thDlg",
            wtitle.c_str(),
            style,
            x, y, win_w, win_h,
            parent_, nullptr, hInst, this
        );
        ApplyDarkTitleBar(hwnd_);
        ShowWindow(hwnd_, SW_SHOW);
        UpdateWindow(hwnd_);
        MessageBeep(is_blocked_ ? MB_ICONHAND : MB_ICONEXCLAMATION);

        MSG msg{};
        while (!closed_ && GetMessageW(&msg, nullptr, 0, 0) > 0) {
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
                CloseDialog();
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if (parent_ && IsWindow(parent_)) {
            EnableWindow(parent_, TRUE);
            SetForegroundWindow(parent_);
        }
        if (hwnd_ && IsWindow(hwnd_)) {
            DestroyWindow(hwnd_);
            hwnd_ = nullptr;
        }
    }

    void CloseDialog() {
        if (allow_dismiss_ && dont_show_checked_) {
            SaveHideStartupNotice(true);
        }
        closed_ = true;
    }

    void Paint(HDC hdc) {
        RECT cr{};
        GetClientRect(hwnd_, &cr);
        int cw = cr.right - cr.left;
        int ch = cr.bottom - cr.top;
        if (cw <= 0 || ch <= 0) return;

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, cw, ch);
        HGDIOBJ oldBmp = SelectObject(memDC, memBmp);

        Graphics g(memDC);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

        SolidBrush bgBrush(Color(255, 23, 12, 17));
        g.FillRectangle(&bgBrush, 0, 0, cw, ch);

        FontFamily ff(L"Segoe UI");
        Font fBanner(&ff, S(14.0f), FontStyleBold, UnitPixel);
        Font fBold(&ff, S(12.0f), FontStyleBold, UnitPixel);
        Font fReg(&ff, S(12.0f), FontStyleRegular, UnitPixel);
        Font fSmall(&ff, S(11.5f), FontStyleRegular, UnitPixel);
        Font fBtn(&ff, S(13.0f), FontStyleBold, UnitPixel);

        // Top Banner
        REAL ban_h = S(46.0f);
        Color banBg = is_blocked_ ? Color(255, 92, 20, 32) : Color(255, 74, 39, 18);
        Color banFg = is_blocked_ ? Color(255, 255, 214, 220) : Color(255, 255, 226, 189);
        SolidBrush banBrush(banBg);
        g.FillRectangle(&banBrush, 0.0f, 0.0f, (REAL)cw, ban_h);

        std::wstring banTxt;
        if (is_blocked_) {
            banTxt = is_steam_reset_
                ? L"⚠  07th-Mod AKTİF DEĞİL (STEAM SIFIRLAMIŞ) · KURULUM KİLİTLENDİ!"
                : L"⚠  07th-Mod EKSİK / UYUMSUZ · KURULUM KİLİTLENDİ!";
        } else {
            banTxt = L"⚠  KURULUMDAN ÖNCE ZORUNLU 07th-Mod UYARISI";
        }

        StringFormat sfLeft;
        sfLeft.SetLineAlignment(StringAlignmentCenter);
        SolidBrush banTxtBr(banFg);
        RectF rBan(S(18.0f), 0.0f, (REAL)cw - S(36.0f), ban_h);
        g.DrawString(banTxt.c_str(), -1, &fBanner, rBan, &sfLeft, &banTxtBr);

        REAL pad = S(20.0f);
        REAL y = ban_h + S(14.0f);
        REAL content_w = (REAL)cw - pad * 2.0f;

        StringFormat sfWrap;
        sfWrap.SetAlignment(StringAlignmentNear);
        sfWrap.SetLineAlignment(StringAlignmentNear);

        if (is_blocked_) {
            std::wstring alert_txt;
            bool same_reason = blocked_.size() == 1 ||
                (blocked_.size() == 2 && blocked_[0].reason == blocked_[1].reason);
            if (same_reason) {
                std::wstring gnames = blocked_[0].game_title;
                if (blocked_.size() > 1) gnames += L", " + blocked_[1].game_title;
                alert_txt = L"Kilitlenen kurulum: " + gnames + L"\n\nTespit: " + blocked_[0].reason +
                            L"\nOyununuzun bozulmaması için bu klasörlere Türkçe yama kurulumu engellendi.";
            } else {
                for (const auto& b : blocked_) {
                    alert_txt += L"• " + b.game_title + L": " + b.reason + L"\n";
                }
                alert_txt += L"\nOyununuzun bozulmaması için bu klasörlere Türkçe yama kurulumu engellendi.";
            }

            RectF measureOut;
            g.MeasureString(alert_txt.c_str(), -1, &fBold, RectF(0, 0, content_w - S(24.0f), S(300.0f)), &sfWrap, &measureOut);
            REAL box_h = measureOut.Height + S(20.0f);
            RectF rAlert(pad, y, content_w, box_h);
            FillAndStrokeRoundRect(g, rAlert, S(8.0f), Color(255, 43, 17, 23), Color(255, 128, 35, 51), S(1.0f));

            SolidBrush alertFg(Color(255, 255, 154, 168));
            RectF rAlertTxt(pad + S(12.0f), y + S(10.0f), content_w - S(24.0f), box_h - S(16.0f));
            g.DrawString(alert_txt.c_str(), -1, &fBold, rAlertTxt, &sfWrap, &alertFg);
            y += box_h + S(12.0f);
        } else {
            std::wstring intro =
                L"Bu Türkçe yama düz (modsuz) Steam veya GOG oyununa doğrudan kurulamaz!\n"
                L"Düz oyunun üzerine kurulmaya çalışılırsa oyun açılmaz; bu yüzden araç modsuz klasörleri otomatik kilitler.";
            RectF mIntro;
            g.MeasureString(intro.c_str(), -1, &fBold, RectF(0, 0, content_w, S(200.0f)), &sfWrap, &mIntro);
            SolidBrush introBr(Color(255, 245, 203, 167));
            g.DrawString(intro.c_str(), -1, &fBold, RectF(pad, y, content_w, mIntro.Height + S(4.0f)), &sfWrap, &introBr);
            y += mIntro.Height + S(14.0f);
        }

        // Steps Box
        std::wstring steps =
            L"07th-Mod kurmak veya onarmak için sırasıyla şu 3 adımı uygulayın:\n\n"
            L"  1. Aşağıdaki turuncu düğmeden resmi 07th-Mod Kurulum Aracını indirin / açın.\n"
            L"  2. Oyunların üzerine 07th-Mod kurulumunu tamamlayın\n"
            L"      (Soru Arkları için 'full', Cevap Arkları / Chiru için 'adv-mode' seçin;\n"
            L"       eski mod dosyaları duruyorsa hızlıca onaracaktır).\n"
            L"  3. İşlem bitince bu pencereye dönüp 'Yeniden Tara' düğmesine basın.";
        RectF mSteps;
        g.MeasureString(steps.c_str(), -1, &fReg, RectF(0, 0, content_w - S(28.0f), S(300.0f)), &sfWrap, &mSteps);
        REAL steps_h = mSteps.Height + S(24.0f);
        RectF rSteps(pad, y, content_w, steps_h);
        FillAndStrokeRoundRect(g, rSteps, S(8.0f), Color(255, 33, 19, 25), Color(255, 61, 36, 48), S(1.0f));
        SolidBrush priBr(C_TEXT_PRI);
        g.DrawString(steps.c_str(), -1, &fReg, RectF(pad + S(14.0f), y + S(12.0f), content_w - S(28.0f), steps_h - S(16.0f)), &sfWrap, &priBr);
        y += steps_h + S(12.0f);

        // Optional checkbox
        if (allow_dismiss_) {
            REAL csz = S(18.0f);
            rc_checkbox_ = RectF(pad, y, content_w, S(24.0f));
            RectF rBox(pad, y + S(3.0f), csz, csz);
            if (dont_show_checked_) {
                FillRoundRect(g, rBox, S(4.0f), hover_id_ == 3 ? C_CRIMSON_HOVER : C_CRIMSON);
                Pen ckPen(Color(255, 255, 255, 255), S(2.0f));
                ckPen.SetStartCap(LineCapRound);
                ckPen.SetEndCap(LineCapRound);
                g.DrawLine(&ckPen, rBox.X + csz * 0.22f, rBox.Y + csz * 0.52f, rBox.X + csz * 0.44f, rBox.Y + csz * 0.74f);
                g.DrawLine(&ckPen, rBox.X + csz * 0.44f, rBox.Y + csz * 0.74f, rBox.X + csz * 0.78f, rBox.Y + csz * 0.26f);
            } else {
                FillAndStrokeRoundRect(g, rBox, S(4.0f), C_BG_INPUT, Color(255, 110, 72, 86), S(1.2f));
            }
            SolidBrush mutBr(C_TEXT_MUTED);
            g.DrawString(
                L"07th-Mod zaten kurulu, bu hatırlatmayı açılışta bir daha gösterme",
                -1, &fSmall,
                RectF(pad + csz + S(8.0f), y, content_w - csz - S(8.0f), S(24.0f)),
                &sfLeft, &mutBr
            );
        } else {
            rc_checkbox_ = RectF();
        }

        // Bottom Buttons
        REAL btn_h = S(38.0f);
        REAL btn_y = (REAL)ch - pad - btn_h + S(4.0f);
        REAL close_w = S(122.0f);
        rc_btn_open_ = RectF(pad, btn_y, content_w - close_w - S(10.0f), btn_h);
        rc_btn_close_ = RectF(pad + content_w - close_w, btn_y, close_w, btn_h);

        Color openBg = (hover_id_ == 1) ? Color(255, 212, 94, 25) : Color(255, 184, 80, 20);
        FillRoundRect(g, rc_btn_open_, S(7.0f), openBg);

        Color closeBg = (hover_id_ == 2) ? Color(255, 69, 41, 54) : Color(255, 47, 28, 37);
        FillAndStrokeRoundRect(g, rc_btn_close_, S(7.0f), closeBg, Color(255, 82, 49, 64), S(1.0f));

        StringFormat sfCenter;
        sfCenter.SetAlignment(StringAlignmentCenter);
        sfCenter.SetLineAlignment(StringAlignmentCenter);
        SolidBrush whiteBr(Color(255, 255, 255, 255));
        g.DrawString(L"07th-Mod İndirme ve Rehber Sayfasını Aç", -1, &fBtn, rc_btn_open_, &sfCenter, &whiteBr);
        g.DrawString(is_blocked_ ? L"Anladım" : L"Devam Et", -1, &fBold, rc_btn_close_, &sfCenter, &priBr);

        BitBlt(hdc, 0, 0, cw, ch, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);
    }

    int HitTest(float x, float y) const {
        if (PtInRectF(rc_btn_open_, x, y)) return 1;
        if (PtInRectF(rc_btn_close_, x, y)) return 2;
        if (allow_dismiss_ && PtInRectF(rc_checkbox_, x, y)) return 3;
        return 0;
    }

    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        Mod07thDialog* self = nullptr;
        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
            self = static_cast<Mod07thDialog*>(cs->lpCreateParams);
            self->hwnd_ = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        } else {
            self = reinterpret_cast<Mod07thDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }
        return self ? self->WndProc(msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
    }

    LRESULT WndProc(UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
            case WM_ERASEBKGND:
                return 1;
            case WM_PAINT: {
                PAINTSTRUCT ps{};
                HDC hdc = BeginPaint(hwnd_, &ps);
                Paint(hdc);
                EndPaint(hwnd_, &ps);
                return 0;
            }
            case WM_MOUSEMOVE: {
                float x = (float)GET_X_LPARAM(lp);
                float y = (float)GET_Y_LPARAM(lp);
                int h = HitTest(x, y);
                if (h != hover_id_) {
                    hover_id_ = h;
                    InvalidateRect(hwnd_, nullptr, FALSE);
                }
                SetCursor(LoadCursorW(nullptr, h > 0 ? IDC_HAND : IDC_ARROW));
                return 0;
            }
            case WM_SETCURSOR:
                if (LOWORD(lp) == HTCLIENT && hover_id_ > 0) {
                    SetCursor(LoadCursorW(nullptr, IDC_HAND));
                    return TRUE;
                }
                break;
            case WM_LBUTTONUP: {
                float x = (float)GET_X_LPARAM(lp);
                float y = (float)GET_Y_LPARAM(lp);
                int h = HitTest(x, y);
                if (h == 1) {
                    ShellExecuteW(hwnd_, L"open", umicore::MOD_07TH_URL_W, nullptr, nullptr, SW_SHOWNORMAL);
                } else if (h == 2) {
                    CloseDialog();
                } else if (h == 3) {
                    dont_show_checked_ = !dont_show_checked_;
                    InvalidateRect(hwnd_, nullptr, FALSE);
                }
                return 0;
            }
            case WM_CLOSE:
                CloseDialog();
                return 0;
        }
        return DefWindowProcW(hwnd_, msg, wp, lp);
    }
};

// ============================================================================
// Main Umineko TR Installer Window
// ============================================================================
enum HitAreaId {
    HIT_NONE = 0,
    HIT_HDR_GUIDE,
    HIT_Q_CHECK,
    HIT_Q_BADGE,
    HIT_Q_BROWSE,
    HIT_Q_MOD07,
    HIT_Q_ZIP,
    HIT_Q_RESTORE,
    HIT_A_CHECK,
    HIT_A_BADGE,
    HIT_A_BROWSE,
    HIT_A_MOD07,
    HIT_A_ZIP,
    HIT_A_RESTORE,
    HIT_LINK_GITHUB,
    HIT_LINK_SITE,
    HIT_LINK_MOD07,
    HIT_BTN_RESCAN,
    HIT_BTN_INSTALL
};

struct CardState {
    std::string arc;
    std::wstring title;
    std::wstring subtitle;
    fs::path current_path;
    fs::path custom_zip;
    fs::path detected_local_zip;
    bool has_remote_info = false;
    umicore::RemotePackageInfo remote_info;
    umicore::ArcInspection inspection;
    bool selected = false;

    // Layout hit rects
    RectF rc_card{};
    RectF rc_check{};
    RectF rc_badge{};
    RectF rc_browse{};
    RectF rc_mod07{};
    RectF rc_zip{};
    RectF rc_restore{};

    bool CanSelect() const {
        return inspection.status == umicore::ArcStatus::Ready ||
               inspection.status == umicore::ArcStatus::Installed;
    }

    std::wstring SourceLabel(Color& out_color) const {
        std::error_code ec;
        if (!custom_zip.empty() && fs::is_regular_file(custom_zip, ec)) {
            double mb = (double)fs::file_size(custom_zip, ec) / (1024.0 * 1024.0);
            wchar_t buf[256];
            swprintf_s(buf, L"Paket kaynağı: Seçilen ZIP (%s · %.0f MB)", custom_zip.filename().c_str(), mb);
            out_color = Color(255, 126, 226, 168);
            return buf;
        }
        if (!detected_local_zip.empty() && fs::is_regular_file(detected_local_zip, ec)) {
            double mb = (double)fs::file_size(detected_local_zip, ec) / (1024.0 * 1024.0);
            wchar_t buf[256];
            swprintf_s(buf, L"Paket kaynağı: Yerel ZIP hazır (%s · %.0f MB)", detected_local_zip.filename().c_str(), mb);
            out_color = Color(255, 126, 226, 168);
            return buf;
        }
        if (has_remote_info && remote_info.size_bytes > 0) {
            double mb = (double)remote_info.size_bytes / (1024.0 * 1024.0);
            wchar_t buf[256];
            swprintf_s(buf, L"Paket kaynağı: GitHub Releases üzerinden indirilecek (v%s · %.0f MB)",
                       umicore::Utf8ToWide(remote_info.version).c_str(), mb);
            out_color = Color(255, 163, 184, 204);
            return buf;
        }
        out_color = Color(255, 148, 163, 184);
        return L"Paket kaynağı: GitHub Releases üzerinden otomatik indirilecek";
    }
};

class MainInstallerWindow {
public:
    explicit MainInstallerWindow(const fs::path& screenshot_out = {})
        : screenshot_out_(screenshot_out) {
        q_card_.arc = "question";
        q_card_.title = L"Soru Arkları (Question Arcs)";
        q_card_.subtitle = L"1-4. Bölümler";
        q_card_.inspection = umicore::InspectGameDir("question", {});

        a_card_.arc = "answer";
        a_card_.title = L"Cevap Arkları (Answer Arcs / Chiru)";
        a_card_.subtitle = L"5-8. Bölümler";
        a_card_.inspection = umicore::InspectGameDir("answer", {});
    }

    int Run(int nCmdShow) {
        HINSTANCE hInst = GetModuleHandleW(nullptr);
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = &MainInstallerWindow::StaticWndProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_APP_ICON));
        wc.hIconSm = wc.hIcon;
        wc.lpszClassName = L"UminekoTR_MainWnd";
        RegisterClassExW(&wc);

        logo_img_ = LoadPngResource(IDR_HEADER_LOGO);
        log_bg_brush_ = CreateSolidBrush(RGB(18, 10, 14));

        UINT dpi = QueryWindowDpi(nullptr);
        int cw = (int)std::lround(810.0 * dpi / 96.0);
        int ch = (int)std::lround(685.0 * dpi / 96.0);
        DWORD style = WS_OVERLAPPEDWINDOW;
        RECT wr{0, 0, cw, ch};
        AdjustWindowRectEx(&wr, style, FALSE, 0);
        int win_w = wr.right - wr.left;
        int win_h = wr.bottom - wr.top;

        int sx = (GetSystemMetrics(SM_CXSCREEN) - win_w) / 2;
        int sy = (GetSystemMetrics(SM_CYSCREEN) - win_h) / 2;

        std::wstring title = L"Umineko When They Cry · Türkçe Yama Kurulum Aracı v" + umicore::Utf8ToWide(umicore::APP_VERSION);
        hwnd_ = CreateWindowExW(
            0,
            L"UminekoTR_MainWnd",
            title.c_str(),
            style,
            std::max(0, sx), std::max(0, sy), win_w, win_h,
            nullptr, nullptr, hInst, this
        );
        if (!hwnd_) return 1;

        {
            std::lock_guard<std::mutex> lk(g_ui_mtx);
            g_main_hwnd = hwnd_;
        }

        ApplyDarkTitleBar(hwnd_);
        ShowWindow(hwnd_, nCmdShow);
        UpdateWindow(hwnd_);

        RescanInstallations(true);

        // Fetch GitHub release manifest asynchronously
        std::thread([]() {
            auto pkgs = umicore::FetchRemoteManifest();
            auto* heap_map = new std::map<std::string, umicore::RemotePackageInfo>(std::move(pkgs));
            PostUiMsg(WM_APP_MANIFEST, heap_map);
        }).detach();

        if (!screenshot_out_.empty()) {
            SetTimer(hwnd_, TIMER_SCREENSHOT, 650, nullptr);
        }

        MSG msg{};
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        {
            std::lock_guard<std::mutex> lk(g_ui_mtx);
            g_main_hwnd = nullptr;
        }
        if (log_font_) DeleteObject(log_font_);
        if (log_bg_brush_) DeleteObject(log_bg_brush_);
        return (int)msg.wParam;
    }

private:
    HWND hwnd_ = nullptr;
    HWND log_edit_ = nullptr;
    HFONT log_font_ = nullptr;
    HBRUSH log_bg_brush_ = nullptr;
    UINT dpi_ = 96;
    fs::path screenshot_out_;

    std::unique_ptr<Image> logo_img_;
    CardState q_card_;
    CardState a_card_;

    bool is_busy_ = false;
    std::atomic<bool> cancel_flag_{false};
    std::wstring stage_text_ = L"Kuruluma hazır.";
    std::wstring detail_text_;
    double progress_frac_ = 0.0;
    Color progress_color_ = C_CRIMSON;
    std::map<std::string, umicore::RemotePackageInfo> remote_manifest_;

    int hover_id_ = HIT_NONE;
    RectF rc_hdr_guide_{};
    RectF rc_log_frame_{};
    RectF rc_link_github_{};
    RectF rc_link_site_{};
    RectF rc_link_mod07_{};
    RectF rc_btn_rescan_{};
    RectF rc_btn_install_{};

    float S(float v) const {
        return v * (float)dpi_ / 96.0f;
    }

    void RebuildLogFont() {
        HFONT nf = CreateFontW(
            -(int)std::lround(S(12.0f)), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            FIXED_PITCH | FF_MODERN, L"Consolas"
        );
        if (log_edit_ && nf) {
            SendMessageW(log_edit_, WM_SETFONT, reinterpret_cast<WPARAM>(nf), TRUE);
        }
        if (log_font_) DeleteObject(log_font_);
        log_font_ = nf;
    }

    void AppendLog(const std::wstring& msg) {
        if (!log_edit_) return;
        std::time_t t = std::time(nullptr);
        std::tm tmv{};
        localtime_s(&tmv, &t);
        wchar_t ts[16]{};
        wcsftime(ts, 16, L"%H:%M:%S", &tmv);

        std::wstring line = L"[" + std::wstring(ts) + L"] " + msg + L"\r\n";
        int len = GetWindowTextLengthW(log_edit_);
        SendMessageW(log_edit_, EM_SETSEL, len, len);
        SendMessageW(log_edit_, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(line.c_str()));
        SendMessageW(log_edit_, EM_SCROLLCARET, 0, 0);
    }

    void SetCardFolder(CardState& c, const fs::path& folder, bool auto_select) {
        std::error_code ec;
        fs::path norm = folder;
        if (!folder.empty() && fs::exists(folder, ec)) {
            fs::path cn = fs::canonical(folder, ec);
            if (!ec && !cn.empty()) norm = cn;
            else norm = folder.lexically_normal().make_preferred();
        }
        c.current_path = norm;
        c.inspection = umicore::InspectGameDir(c.arc, norm);
        c.detected_local_zip = c.custom_zip.empty() ? umicore::FindLocalPackageZip(c.arc) : c.custom_zip;
        if (c.CanSelect()) {
            if (auto_select) {
                c.selected = (c.inspection.status == umicore::ArcStatus::Ready);
            }
        } else {
            c.selected = false;
        }
        if (hwnd_) UpdateLayout();
    }

    void RescanInstallations(bool initial) {
        if (is_busy_) return;
        fs::path dq, da;
        umicore::DetectGamePaths(dq, da);
        SetCardFolder(q_card_, dq, true);
        SetCardFolder(a_card_, da, true);

        if (!q_card_.selected && !a_card_.selected) {
            if (q_card_.inspection.status == umicore::ArcStatus::Installed) q_card_.selected = true;
            if (a_card_.inspection.status == umicore::ArcStatus::Installed) a_card_.selected = true;
        }

        std::vector<BlockedItem> blocked;
        auto log_card = [&](const CardState& c, const wchar_t* short_lbl) {
            if (!c.current_path.empty()) {
                const wchar_t* st_s = L"NOT_FOUND";
                switch (c.inspection.status) {
                    case umicore::ArcStatus::Ready:          st_s = L"READY"; break;
                    case umicore::ArcStatus::Installed:      st_s = L"INSTALLED"; break;
                    case umicore::ArcStatus::Missing07thMod: st_s = L"MISSING_07TH_MOD"; break;
                    case umicore::ArcStatus::WrongArc:       st_s = L"WRONG_ARC"; break;
                    default: break;
                }
                AppendLog(std::wstring(short_lbl) + L": " + c.current_path.wstring() + L" -> " + st_s);
                if (c.inspection.status == umicore::ArcStatus::Missing07thMod) {
                    blocked.push_back({short_lbl, c.inspection.message});
                }
            } else if (initial) {
                AppendLog(std::wstring(short_lbl) + L": Otomatik kurulum dizini bulunamadı.");
            }
        };
        log_card(q_card_, L"Soru Arkları (1-4)");
        log_card(a_card_, L"Cevap Arkları / Chiru (5-8)");

        InvalidateRect(hwnd_, nullptr, FALSE);

        // Do not block automated screenshot capture with modal popup
        if (!screenshot_out_.empty()) return;

        if (!blocked.empty()) {
            auto* payload = new std::pair<std::vector<BlockedItem>, bool>(std::move(blocked), false);
            PostMessageW(hwnd_, WM_APP_STARTUP_POP, 0, reinterpret_cast<LPARAM>(payload));
        } else if (initial && !IsStartupNoticeHidden()) {
            auto* payload = new std::pair<std::vector<BlockedItem>, bool>({}, true);
            PostMessageW(hwnd_, WM_APP_STARTUP_POP, 0, reinterpret_cast<LPARAM>(payload));
        }
    }

    std::vector<CardState*> SelectedCards() {
        std::vector<CardState*> out;
        if (q_card_.selected && !q_card_.current_path.empty()) out.push_back(&q_card_);
        if (a_card_.selected && !a_card_.current_path.empty()) out.push_back(&a_card_);
        return out;
    }

    std::wstring PrimaryButtonLabel(bool& out_enabled, Color& out_bg) {
        if (is_busy_) {
            out_enabled = true;
            out_bg = (hover_id_ == HIT_BTN_INSTALL) ? Color(255, 115, 45, 62) : Color(255, 89, 37, 49);
            return L"İptal Et";
        }
        auto sel = SelectedCards();
        if (sel.empty()) {
            out_enabled = false;
            out_bg = Color(255, 56, 35, 44);
            bool any_missing = (q_card_.inspection.status == umicore::ArcStatus::Missing07thMod) ||
                               (a_card_.inspection.status == umicore::ArcStatus::Missing07thMod);
            return any_missing ? L"Önce 07th-Mod Kurun" : L"Kurulacak Oyunu Seçin";
        }
        out_enabled = true;
        out_bg = (hover_id_ == HIT_BTN_INSTALL) ? C_CRIMSON_HOVER : C_CRIMSON;
        bool all_inst = true;
        for (auto* c : sel) {
            if (c->inspection.status != umicore::ArcStatus::Installed) all_inst = false;
        }
        wchar_t buf[96];
        swprintf_s(buf, all_inst ? L"Seçili Yamayı Güncelle (%zu)" : L"Seçili Yamaları Kur (%zu)", sel.size());
        return buf;
    }

    void UpdateLayout() {
        RECT cr{};
        GetClientRect(hwnd_, &cr);
        float cw = (float)(cr.right - cr.left);
        float ch = (float)(cr.bottom - cr.top);
        if (cw <= 10.0f || ch <= 10.0f) return;

        float pad = S(18.0f);
        float hdr_h = S(104.0f);

        rc_hdr_guide_ = RectF(cw - pad - S(138.0f), S(14.0f), S(138.0f), S(26.0f));

        float card_y0 = hdr_h + S(14.0f);
        float card_h = S(124.0f);
        q_card_.rc_card = RectF(pad, card_y0, cw - pad * 2.0f, card_h);
        a_card_.rc_card = RectF(pad, card_y0 + card_h + S(10.0f), cw - pad * 2.0f, card_h);

        auto layout_card_children = [&](CardState& c) {
            const RectF& r = c.rc_card;
            c.rc_check = RectF(r.X + S(14.0f), r.Y + S(12.0f), S(22.0f), S(22.0f));
            c.rc_browse = RectF(r.X + r.Width - S(14.0f) - S(98.0f), r.Y + S(42.0f), S(98.0f), S(30.0f));

            float bx = r.X + r.Width - S(14.0f);
            float by = r.Y + r.Height - S(12.0f) - S(26.0f);
            if (c.inspection.status == umicore::ArcStatus::Installed && c.inspection.has_backup) {
                bx -= S(90.0f);
                c.rc_restore = RectF(bx, by, S(90.0f), S(26.0f));
                bx -= S(6.0f);
            } else {
                c.rc_restore = RectF();
            }

            bx -= S(98.0f);
            c.rc_zip = RectF(bx, by, S(98.0f), S(26.0f));

            if (c.inspection.status == umicore::ArcStatus::Missing07thMod) {
                bx -= (S(6.0f) + S(110.0f));
                c.rc_mod07 = RectF(bx, by, S(110.0f), S(26.0f));
            } else {
                c.rc_mod07 = RectF();
            }
        };
        layout_card_children(q_card_);
        layout_card_children(a_card_);

        float prog_top = a_card_.rc_card.Y + a_card_.rc_card.Height + S(12.0f);
        float log_top = prog_top + S(38.0f);
        float ftr_h = S(52.0f);
        float log_h = std::max(S(74.0f), ch - log_top - ftr_h - S(6.0f));
        rc_log_frame_ = RectF(pad, log_top, cw - pad * 2.0f, log_h);

        if (log_edit_) {
            int in_x = (int)std::lround(rc_log_frame_.X + S(8.0f));
            int in_y = (int)std::lround(rc_log_frame_.Y + S(6.0f));
            int in_w = std::max(20, (int)std::lround(rc_log_frame_.Width - S(12.0f)));
            int in_h = std::max(20, (int)std::lround(rc_log_frame_.Height - S(12.0f)));
            MoveWindow(log_edit_, in_x, in_y, in_w, in_h, TRUE);
        }

        float ftr_y = ch - S(14.0f) - S(36.0f);
        rc_btn_install_ = RectF(cw - pad - S(204.0f), ftr_y, S(204.0f), S(36.0f));
        rc_btn_rescan_  = RectF(rc_btn_install_.X - S(10.0f) - S(112.0f), ftr_y, S(112.0f), S(36.0f));

        float ly = ftr_y + S(4.0f);
        rc_link_github_ = RectF(pad, ly, S(120.0f), S(28.0f));
        rc_link_site_   = RectF(pad + S(124.0f), ly, S(86.0f), S(28.0f));
        rc_link_mod07_  = RectF(pad + S(214.0f), ly, S(104.0f), S(28.0f));
    }

    void DrawCard(Graphics& g, CardState& c, int check_hit, int badge_hit, int browse_hit, int mod07_hit, int zip_hit, int restore_hit) {
        const RectF& r = c.rc_card;
        Color border = C_BORDER_SUBTLE;
        if (c.inspection.status == umicore::ArcStatus::Missing07thMod) border = C_BORDER_LOCKED;
        else if (c.selected) border = C_BORDER_GOLD;

        FillAndStrokeRoundRect(g, r, S(10.0f), C_BG_CARD, border, c.selected ? S(1.4f) : S(1.0f));

        FontFamily ff(L"Segoe UI");
        FontFamily ffMono(L"Consolas");
        Font fTitle(&ff, S(14.0f), FontStyleBold, UnitPixel);
        Font fSub(&ff, S(12.0f), FontStyleRegular, UnitPixel);
        Font fBadge(&ff, S(11.0f), FontStyleBold, UnitPixel);
        Font fPath(&ffMono, S(11.5f), FontStyleRegular, UnitPixel);
        Font fBtn(&ff, S(12.0f), FontStyleRegular, UnitPixel);
        Font fSmallBtn(&ff, S(11.0f), FontStyleRegular, UnitPixel);
        Font fSmallBold(&ff, S(11.0f), FontStyleBold, UnitPixel);
        Font fMsg(&ff, S(11.0f), FontStyleRegular, UnitPixel);

        // Checkbox
        bool can_sel = c.CanSelect() && !is_busy_;
        RectF rChk(c.rc_check.X, c.rc_check.Y + S(1.0f), S(20.0f), S(20.0f));
        if (c.selected) {
            Color cbg = !can_sel ? Color(255, 105, 42, 54) : (hover_id_ == check_hit ? C_CRIMSON_HOVER : C_CRIMSON);
            FillRoundRect(g, rChk, S(5.0f), cbg);
            Pen ckPen(Color(255, 255, 255, 255), S(2.1f));
            ckPen.SetStartCap(LineCapRound);
            ckPen.SetEndCap(LineCapRound);
            g.DrawLine(&ckPen, rChk.X + rChk.Width * 0.22f, rChk.Y + rChk.Height * 0.52f, rChk.X + rChk.Width * 0.44f, rChk.Y + rChk.Height * 0.74f);
            g.DrawLine(&ckPen, rChk.X + rChk.Width * 0.44f, rChk.Y + rChk.Height * 0.74f, rChk.X + rChk.Width * 0.78f, rChk.Y + rChk.Height * 0.26f);
        } else {
            Color strk = can_sel ? Color(255, 110, 72, 86) : Color(255, 65, 42, 50);
            FillAndStrokeRoundRect(g, rChk, S(5.0f), C_BG_INPUT, strk, S(1.2f));
        }

        // Title + Subtitle
        StringFormat sfLeft;
        sfLeft.SetLineAlignment(StringAlignmentCenter);
        sfLeft.SetFormatFlags(StringFormatFlagsNoWrap);
        SolidBrush priBr(C_TEXT_PRI);
        SolidBrush mutBr(C_TEXT_MUTED);

        float tx = rChk.X + rChk.Width + S(9.0f);
        RectF mTitle;
        g.MeasureString(c.title.c_str(), -1, &fTitle, PointF(tx, rChk.Y), &mTitle);
        g.DrawString(c.title.c_str(), -1, &fTitle, RectF(tx, rChk.Y - S(1.0f), mTitle.Width + S(6.0f), S(22.0f)), &sfLeft, &priBr);

        std::wstring sub = L" · " + c.subtitle;
        g.DrawString(sub.c_str(), -1, &fSub, RectF(tx + mTitle.Width, rChk.Y, S(160.0f), S(22.0f)), &sfLeft, &mutBr);

        // Status Badge
        std::wstring badge_txt = L"Bulunamadı";
        Color badge_bg(255, 41, 28, 34);
        Color badge_fg(255, 156, 133, 140);
        switch (c.inspection.status) {
            case umicore::ArcStatus::Ready:
                badge_txt = L"Hazır · 07th-Mod";
                badge_bg = Color(255, 23, 59, 41);
                badge_fg = Color(255, 92, 224, 155);
                break;
            case umicore::ArcStatus::Installed:
                badge_txt = c.inspection.installed_version.empty()
                    ? L"Türkçe Yama Kurulu"
                    : (L"Türkçe Kurulu (v" + umicore::Utf8ToWide(c.inspection.installed_version) + L")");
                badge_bg = Color(255, 56, 44, 18);
                badge_fg = Color(255, 242, 203, 97);
                break;
            case umicore::ArcStatus::Missing07thMod:
                badge_txt = L"07th-Mod Eksik (Kilitli)";
                badge_bg = (hover_id_ == badge_hit) ? Color(255, 96, 30, 30) : Color(255, 77, 25, 25);
                badge_fg = Color(255, 255, 138, 117);
                break;
            case umicore::ArcStatus::WrongArc:
                badge_txt = L"Yanlış Klasör";
                badge_bg = Color(255, 71, 21, 30);
                badge_fg = Color(255, 255, 107, 129);
                break;
            default:
                break;
        }

        RectF mBadge;
        g.MeasureString(badge_txt.c_str(), -1, &fBadge, PointF(0, 0), &mBadge);
        float bw = mBadge.Width + S(18.0f);
        c.rc_badge = RectF(r.X + r.Width - S(14.0f) - bw, r.Y + S(11.0f), bw, S(23.0f));
        FillRoundRect(g, c.rc_badge, S(6.0f), badge_bg);
        StringFormat sfCenter;
        sfCenter.SetAlignment(StringAlignmentCenter);
        sfCenter.SetLineAlignment(StringAlignmentCenter);
        SolidBrush bfgBr(badge_fg);
        g.DrawString(badge_txt.c_str(), -1, &fBadge, c.rc_badge, &sfCenter, &bfgBr);

        // Path box + Browse button
        RectF rPath(r.X + S(14.0f), c.rc_browse.Y, c.rc_browse.X - (r.X + S(14.0f)) - S(8.0f), c.rc_browse.Height);
        FillAndStrokeRoundRect(g, rPath, S(6.0f), C_BG_INPUT, Color(255, 51, 29, 38), S(1.0f));

        std::wstring pstr = c.current_path.empty() ? L"Klasör seçilmedi" : c.current_path.wstring();
        StringFormat sfPath;
        sfPath.SetLineAlignment(StringAlignmentCenter);
        sfPath.SetTrimming(StringTrimmingEllipsisPath);
        sfPath.SetFormatFlags(StringFormatFlagsNoWrap);
        SolidBrush pathBr(c.current_path.empty() ? C_TEXT_MUTED : Color(255, 223, 210, 214));
        g.DrawString(pstr.c_str(), -1, &fPath, RectF(rPath.X + S(9.0f), rPath.Y, rPath.Width - S(18.0f), rPath.Height), &sfPath, &pathBr);

        Color brwBg = is_busy_ ? Color(255, 36, 22, 29) : (hover_id_ == browse_hit ? Color(255, 66, 40, 52) : Color(255, 47, 28, 37));
        FillAndStrokeRoundRect(g, c.rc_browse, S(6.0f), brwBg, Color(255, 77, 47, 61), S(1.0f));
        SolidBrush brwFg(is_busy_ ? C_TEXT_MUTED : C_TEXT_PRI);
        g.DrawString(L"Klasör Seç", -1, &fBtn, c.rc_browse, &sfCenter, &brwFg);

        // Bottom info column + action buttons
        float right_btns_left = c.rc_zip.X;
        if (c.rc_mod07.Width > 0.0f) right_btns_left = c.rc_mod07.X;
        float info_w = std::max(S(140.0f), right_btns_left - (r.X + S(14.0f)) - S(10.0f));

        bool is_err_st = (c.inspection.status == umicore::ArcStatus::Missing07thMod ||
                          c.inspection.status == umicore::ArcStatus::WrongArc);
        SolidBrush msgBr(is_err_st ? Color(255, 255, 138, 117) : C_TEXT_MUTED);
        StringFormat sfMsg;
        sfMsg.SetTrimming(StringTrimmingEllipsisCharacter);
        g.DrawString(c.inspection.message.c_str(), -1, &fMsg, RectF(r.X + S(14.0f), rPath.Y + rPath.Height + S(5.0f), info_w, S(30.0f)), &sfMsg, &msgBr);

        Color srcCol;
        std::wstring srcTxt = c.SourceLabel(srcCol);
        SolidBrush srcBr(srcCol);
        StringFormat sfSingle;
        sfSingle.SetFormatFlags(StringFormatFlagsNoWrap);
        sfSingle.SetTrimming(StringTrimmingEllipsisCharacter);
        g.DrawString(srcTxt.c_str(), -1, &fMsg, RectF(r.X + S(14.0f), r.Y + r.Height - S(22.0f), info_w, S(18.0f)), &sfSingle, &srcBr);

        if (c.rc_mod07.Width > 0.0f) {
            Color mbg = (hover_id_ == mod07_hit) ? Color(255, 173, 71, 23) : Color(255, 138, 56, 18);
            FillRoundRect(g, c.rc_mod07, S(6.0f), mbg);
            SolidBrush mfg(Color(255, 255, 240, 229));
            g.DrawString(L"07th-Mod İndir", -1, &fSmallBold, c.rc_mod07, &sfCenter, &mfg);
        }

        Color zbg = is_busy_ ? Color(255, 31, 18, 24) : (hover_id_ == zip_hit ? Color(255, 56, 34, 44) : Color(255, 37, 22, 29));
        FillAndStrokeRoundRect(g, c.rc_zip, S(6.0f), zbg, Color(255, 64, 39, 51), S(1.0f));
        g.DrawString(L"Yerel ZIP Seç", -1, &fSmallBtn, c.rc_zip, &sfCenter, &mutBr);

        if (c.rc_restore.Width > 0.0f) {
            Color rbg = is_busy_ ? Color(255, 34, 20, 26) : (hover_id_ == restore_hit ? Color(255, 89, 29, 43) : Color(255, 43, 23, 30));
            FillAndStrokeRoundRect(g, c.rc_restore, S(6.0f), rbg, Color(255, 84, 40, 54), S(1.0f));
            SolidBrush rfg(Color(255, 232, 181, 192));
            g.DrawString(L"Yedeğe Dön", -1, &fSmallBtn, c.rc_restore, &sfCenter, &rfg);
        }
    }

    void Paint(HDC hdc) {
        RECT cr{};
        GetClientRect(hwnd_, &cr);
        int cw = cr.right - cr.left;
        int ch = cr.bottom - cr.top;
        if (cw <= 0 || ch <= 0) return;

        // Clip out child HWND (log_edit_) so BitBlt never flickers over the native scrollbar/edit
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, cw, ch);
        HGDIOBJ oldBmp = SelectObject(memDC, memBmp);

        Graphics g(memDC);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

        SolidBrush mainBg(C_BG_MAIN);
        g.FillRectangle(&mainBg, 0, 0, cw, ch);

        FontFamily ff(L"Segoe UI");
        FontFamily ffMono(L"Consolas");

        // 1. Header
        float pad = S(18.0f);
        float hdr_h = S(104.0f);
        SolidBrush hdrBg(C_BG_HEADER);
        g.FillRectangle(&hdrBg, 0.0f, 0.0f, (REAL)cw, hdr_h);
        Pen hdrLine(C_BORDER_HDR, S(1.0f));
        g.DrawLine(&hdrLine, 0.0f, hdr_h, (REAL)cw, hdr_h);

        float txt_x = pad;
        if (logo_img_) {
            float lw = S(236.0f);
            float lh = S(83.0f);
            g.DrawImage(logo_img_.get(), RectF(pad, (hdr_h - lh) * 0.5f, lw, lh));
            txt_x = pad + lw + S(14.0f);
        }

        Font fHdrTitle(&ff, S(19.0f), FontStyleBold, UnitPixel);
        Font fVer(&ff, S(11.0f), FontStyleBold, UnitPixel);
        Font fHdrDesc(&ff, S(12.0f), FontStyleRegular, UnitPixel);
        Font fHdrWarn(&ff, S(11.0f), FontStyleBold, UnitPixel);

        SolidBrush priBr(C_TEXT_PRI);
        SolidBrush mutBr(C_TEXT_MUTED);
        SolidBrush goldBr(C_GOLD);
        SolidBrush warnBr(C_WARN_AMBER);

        StringFormat sfLeft;
        sfLeft.SetLineAlignment(StringAlignmentCenter);
        sfLeft.SetFormatFlags(StringFormatFlagsNoWrap);

        std::wstring hTitle = L"Türkçe Yama Kurulum Aracı";
        RectF mHt;
        g.MeasureString(hTitle.c_str(), -1, &fHdrTitle, PointF(txt_x, S(14.0f)), &mHt);
        g.DrawString(hTitle.c_str(), -1, &fHdrTitle, RectF(txt_x, S(13.0f), mHt.Width + S(8.0f), S(28.0f)), &sfLeft, &priBr);

        std::wstring verStr = L"v" + umicore::Utf8ToWide(umicore::APP_VERSION);
        RectF rVer(txt_x + mHt.Width + S(8.0f), S(17.0f), S(52.0f), S(21.0f));
        FillRoundRect(g, rVer, S(5.0f), Color(255, 51, 29, 38));
        StringFormat sfCenter;
        sfCenter.SetAlignment(StringAlignmentCenter);
        sfCenter.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(verStr.c_str(), -1, &fVer, rVer, &sfCenter, &goldBr);

        // 07th-Mod Guide Header Button
        Color gBg = (hover_id_ == HIT_HDR_GUIDE) ? Color(255, 105, 51, 25) : Color(255, 74, 36, 18);
        FillRoundRect(g, rc_hdr_guide_, S(6.0f), gBg);
        SolidBrush gFg(Color(255, 255, 217, 179));
        g.DrawString(L"⚠ 07th-Mod Rehberi", -1, &fVer, rc_hdr_guide_, &sfCenter, &gFg);

        float desc_w = (float)cw - txt_x - pad;
        g.DrawString(
            L"Bölüm 1-8 tam metin, renkli gerçekler, arayüz, bulmacalar ve hibrit Kanji font paketi.",
            -1, &fHdrDesc, RectF(txt_x, S(46.0f), desc_w, S(22.0f)), &sfLeft, &mutBr
        );
        g.DrawString(
            L"Zorunlu ön koşul: Yamayı kurmadan önce oyunların üzerine 07th-Mod kurulmuş olmalıdır.",
            -1, &fHdrWarn, RectF(txt_x, S(70.0f), desc_w, S(20.0f)), &sfLeft, &warnBr
        );

        // 2. Cards
        DrawCard(g, q_card_, HIT_Q_CHECK, HIT_Q_BADGE, HIT_Q_BROWSE, HIT_Q_MOD07, HIT_Q_ZIP, HIT_Q_RESTORE);
        DrawCard(g, a_card_, HIT_A_CHECK, HIT_A_BADGE, HIT_A_BROWSE, HIT_A_MOD07, HIT_A_ZIP, HIT_A_RESTORE);

        // 3. Progress Section
        float prog_top = a_card_.rc_card.Y + a_card_.rc_card.Height + S(10.0f);
        Font fStage(&ff, S(12.0f), FontStyleBold, UnitPixel);
        Font fDetail(&ffMono, S(11.0f), FontStyleRegular, UnitPixel);

        g.DrawString(stage_text_.c_str(), -1, &fStage, RectF(pad, prog_top, (float)cw * 0.58f, S(20.0f)), &sfLeft, &priBr);
        StringFormat sfRight;
        sfRight.SetAlignment(StringAlignmentFar);
        sfRight.SetLineAlignment(StringAlignmentCenter);
        sfRight.SetFormatFlags(StringFormatFlagsNoWrap);
        g.DrawString(detail_text_.c_str(), -1, &fDetail, RectF((float)cw * 0.45f, prog_top, (float)cw * 0.55f - pad, S(20.0f)), &sfRight, &goldBr);

        RectF rTrack(pad, prog_top + S(22.0f), (float)cw - pad * 2.0f, S(10.0f));
        FillRoundRect(g, rTrack, S(5.0f), Color(255, 38, 22, 29));
        double clamped = std::max(0.0, std::min(1.0, progress_frac_));
        if (clamped > 0.002) {
            float fw = std::max(S(10.0f), (float)(rTrack.Width * clamped));
            FillRoundRect(g, RectF(rTrack.X, rTrack.Y, fw, rTrack.Height), S(5.0f), progress_color_);
        }

        // 4. Log Console Frame
        FillAndStrokeRoundRect(g, rc_log_frame_, S(8.0f), C_BG_INPUT, Color(255, 45, 25, 34), S(1.0f));

        // 5. Footer Links & Action Buttons
        Font fLink(&ff, S(11.5f), FontStyleRegular, UnitPixel);
        auto draw_link = [&](const RectF& rl, const wchar_t* lbl, int hid, bool is_gold) {
            if (hover_id_ == hid) {
                FillRoundRect(g, rl, S(5.0f), Color(255, 38, 22, 29));
            }
            g.DrawString(lbl, -1, &fLink, rl, &sfCenter, is_gold ? &goldBr : &mutBr);
        };
        draw_link(rc_link_github_, L"@Remilya · GitHub", HIT_LINK_GITHUB, true);
        draw_link(rc_link_site_,   L"remilya.dev",       HIT_LINK_SITE,   true);
        draw_link(rc_link_mod07_,  L"07th-Mod İndir",    HIT_LINK_MOD07,  false);

        Font fRescan(&ff, S(12.0f), FontStyleRegular, UnitPixel);
        Color rscBg = is_busy_ ? Color(255, 32, 19, 25) : (hover_id_ == HIT_BTN_RESCAN ? Color(255, 59, 35, 47) : Color(255, 41, 24, 32));
        FillAndStrokeRoundRect(g, rc_btn_rescan_, S(7.0f), rscBg, Color(255, 71, 42, 56), S(1.0f));
        g.DrawString(L"Yeniden Tara", -1, &fRescan, rc_btn_rescan_, &sfCenter, is_busy_ ? &mutBr : &priBr);

        bool pri_en = false;
        Color pri_bg;
        std::wstring pri_lbl = PrimaryButtonLabel(pri_en, pri_bg);
        Font fPriBtn(&ff, S(13.0f), FontStyleBold, UnitPixel);
        FillRoundRect(g, rc_btn_install_, S(7.0f), pri_bg);
        SolidBrush whiteBr(pri_en ? Color(255, 255, 255, 255) : C_TEXT_MUTED);
        g.DrawString(pri_lbl.c_str(), -1, &fPriBtn, rc_btn_install_, &sfCenter, &whiteBr);

        if (log_edit_ && IsWindowVisible(log_edit_)) {
            RECT er{};
            GetWindowRect(log_edit_, &er);
            MapWindowPoints(HWND_DESKTOP, hwnd_, reinterpret_cast<LPPOINT>(&er), 2);
            ExcludeClipRect(hdc, er.left, er.top, er.right, er.bottom);
        }
        BitBlt(hdc, 0, 0, cw, ch, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);
    }

    int HitTest(float x, float y) const {
        if (PtInRectF(rc_hdr_guide_, x, y)) return HIT_HDR_GUIDE;

        if (!is_busy_) {
            if (q_card_.CanSelect() && PtInRectF(q_card_.rc_check, x, y)) return HIT_Q_CHECK;
            if (PtInRectF(q_card_.rc_browse, x, y)) return HIT_Q_BROWSE;
            if (PtInRectF(q_card_.rc_zip, x, y)) return HIT_Q_ZIP;
            if (q_card_.rc_restore.Width > 0 && PtInRectF(q_card_.rc_restore, x, y)) return HIT_Q_RESTORE;

            if (a_card_.CanSelect() && PtInRectF(a_card_.rc_check, x, y)) return HIT_A_CHECK;
            if (PtInRectF(a_card_.rc_browse, x, y)) return HIT_A_BROWSE;
            if (PtInRectF(a_card_.rc_zip, x, y)) return HIT_A_ZIP;
            if (a_card_.rc_restore.Width > 0 && PtInRectF(a_card_.rc_restore, x, y)) return HIT_A_RESTORE;
            if (PtInRectF(rc_btn_rescan_, x, y)) return HIT_BTN_RESCAN;
        }

        if (q_card_.inspection.status == umicore::ArcStatus::Missing07thMod && PtInRectF(q_card_.rc_badge, x, y)) return HIT_Q_BADGE;
        if (q_card_.rc_mod07.Width > 0 && PtInRectF(q_card_.rc_mod07, x, y)) return HIT_Q_MOD07;

        if (a_card_.inspection.status == umicore::ArcStatus::Missing07thMod && PtInRectF(a_card_.rc_badge, x, y)) return HIT_A_BADGE;
        if (a_card_.rc_mod07.Width > 0 && PtInRectF(a_card_.rc_mod07, x, y)) return HIT_A_MOD07;

        if (PtInRectF(rc_link_github_, x, y)) return HIT_LINK_GITHUB;
        if (PtInRectF(rc_link_site_, x, y))   return HIT_LINK_SITE;
        if (PtInRectF(rc_link_mod07_, x, y))  return HIT_LINK_MOD07;

        if (PtInRectF(rc_btn_install_, x, y)) {
            if (is_busy_) return HIT_BTN_INSTALL;
            if ((q_card_.selected && !q_card_.current_path.empty()) ||
                (a_card_.selected && !a_card_.current_path.empty())) {
                return HIT_BTN_INSTALL;
            }
        }
        return HIT_NONE;
    }

    void OnBrowseCard(CardState& c) {
        fs::path init = (!c.current_path.empty()) ? c.current_path : fs::path(L"C:\\");
        std::wstring picked = PickFolderDialog(hwnd_, c.title + L" klasörünü seçin", init);
        if (picked.empty()) return;

        fs::path p(picked);
        SetCardFolder(c, p, false);
        InvalidateRect(hwnd_, nullptr, FALSE);

        AppendLog(L"[" + c.title + L"] Klasör seçildi: " + p.wstring());

        if (c.inspection.status == umicore::ArcStatus::Missing07thMod) {
            Mod07thDialog::Show(hwnd_, {{c.title, c.inspection.message}}, false);
        } else if (c.inspection.status == umicore::ArcStatus::WrongArc) {
            std::wstring msg = c.inspection.message + L"\n\nOyunun bozulmaması için bu klasöre kurulum kilitlendi.";
            MessageBoxW(hwnd_, msg.c_str(), L"Yanlış Oyun Klasörü Seçildi", MB_OK | MB_ICONWARNING);
        } else if (c.inspection.status == umicore::ArcStatus::NotFound) {
            MessageBoxW(
                hwnd_,
                L"Seçtiğiniz dizinde geçerli bir Umineko kurulumu bulunamadı.\nLütfen Steam veya GOG içindeki ana oyun klasörünü seçin.",
                L"Oyun Klasörü Bulunamadı",
                MB_OK | MB_ICONWARNING
            );
        } else {
            c.selected = true;
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
    }

    void OnChooseZip(CardState& c) {
        std::wstring picked = PickZipFileDialog(hwnd_);
        if (picked.empty()) return;
        fs::path zp(picked);
        try {
            umicore::ValidatePatchZip(zp, c.arc);
            c.custom_zip = zp;
            c.detected_local_zip = zp;
            AppendLog(L"Yerel paket seçildi: " + zp.filename().wstring());
            InvalidateRect(hwnd_, nullptr, FALSE);
        } catch (const std::exception& ex) {
            MessageBoxW(hwnd_, umicore::Utf8ToWide(ex.what()).c_str(), L"Geçersiz Yama Paketi", MB_OK | MB_ICONERROR);
        }
    }

    void OnStartRestore(CardState& c) {
        if (is_busy_ || c.current_path.empty()) return;
        std::wstring short_lbl = (c.arc == "question") ? L"Soru Arkları (1-4)" : L"Cevap Arkları / Chiru (5-8)";
        std::wstring ask = short_lbl + L" için Türkçe yamayı kaldırıp orijinal 07th-Mod (İngilizce) dosyalarını geri yüklemek istiyor musunuz?";
        if (MessageBoxW(hwnd_, ask.c_str(), L"Yedeği Geri Yükle", MB_YESNO | MB_ICONQUESTION) != IDYES) {
            return;
        }

        is_busy_ = true;
        progress_color_ = C_GOLD;
        progress_frac_ = 0.0;
        stage_text_ = L"Yedek geri yükleniyor...";
        detail_text_.clear();
        InvalidateRect(hwnd_, nullptr, FALSE);

        std::string arc = c.arc;
        fs::path path = c.current_path;
        std::thread([arc, path]() {
            auto* done = new RestoreDoneMsg();
            done->arc = arc;
            try {
                done->restored_count = umicore::UninstallArcPatch(
                    arc, path,
                    [](const std::wstring& st, double fr, const std::wstring& det) {
                        PostUiMsg(WM_APP_PROGRESS, new ProgressMsg{st, fr, det});
                    }
                );
                done->ok = true;
            } catch (const std::exception& ex) {
                done->ok = false;
                done->error_msg = umicore::Utf8ToWide(ex.what());
            }
            PostUiMsg(WM_APP_RESTORE_DONE, done);
        }).detach();
    }

    void OnPrimaryInstallOrCancel() {
        if (is_busy_) {
            cancel_flag_.store(true);
            AppendLog(L"İptal isteği gönderildi, lütfen bekleyin...");
            return;
        }

        auto sel = SelectedCards();
        if (sel.empty()) return;

        // Re-verify 07th-Mod right before installation
        for (auto* c : sel) {
            if (umicore::InspectGameDir(c->arc, c->current_path).status == umicore::ArcStatus::Missing07thMod) {
                RescanInstallations(false);
                return;
            }
        }

        struct Job {
            std::string arc;
            std::wstring short_name;
            fs::path game_root;
            fs::path custom_zip;
            bool has_remote;
            umicore::RemotePackageInfo remote;
        };
        std::vector<Job> jobs;
        for (auto* c : sel) {
            std::wstring sn = (c->arc == "question") ? L"Soru Arkları (1-4)" : L"Cevap Arkları / Chiru (5-8)";
            jobs.push_back({c->arc, sn, c->current_path, c->custom_zip, c->has_remote_info, c->remote_info});
        }

        cancel_flag_.store(false);
        is_busy_ = true;
        progress_color_ = C_CRIMSON;
        progress_frac_ = 0.0;
        stage_text_ = L"Kurulum başlatılıyor...";
        detail_text_.clear();
        InvalidateRect(hwnd_, nullptr, FALSE);

        std::atomic<bool>* cflag = &cancel_flag_;
        std::thread([jobs, cflag]() {
            auto* done = new InstallDoneMsg();
            try {
                for (size_t i = 0; i < jobs.size(); ++i) {
                    const auto& jb = jobs[i];
                    wchar_t hdr[160];
                    swprintf_s(hdr, L"[%zu/%zu] %s kurulumu başlatılıyor...", i + 1, jobs.size(), jb.short_name.c_str());
                    PostUiMsg(WM_APP_LOG, new std::wstring(hdr));

                    fs::path zp = jb.custom_zip.empty() ? umicore::FindLocalPackageZip(jb.arc) : jb.custom_zip;
                    std::string ver = umicore::APP_VERSION;
                    auto prog_fn = [](const std::wstring& st, double fr, const std::wstring& det) {
                        PostUiMsg(WM_APP_PROGRESS, new ProgressMsg{st, fr, det});
                    };

                    if (!zp.empty()) {
                        PostUiMsg(WM_APP_LOG, new std::wstring(L"Yerel paket bulundu, indirme atlanıyor: " + zp.filename().wstring()));
                    } else {
                        umicore::RemotePackageInfo pkg = jb.remote;
                        if (!jb.has_remote || pkg.url.empty()) {
                            auto mp = umicore::FetchRemoteManifest();
                            pkg = mp[jb.arc];
                        }
                        ver = pkg.version;
                        PostUiMsg(WM_APP_LOG, new std::wstring(L"Paket indiriliyor: " + pkg.url));
                        zp = umicore::DownloadPackageZip(pkg, prog_fn, cflag);
                    }

                    int cnt = umicore::InstallArcPackage(jb.arc, jb.game_root, zp, ver, prog_fn, cflag);
                    wchar_t okm[192];
                    swprintf_s(okm, L"%s başarıyla kuruldu (%d dosya güncellendi, yedek alındı).", jb.short_name.c_str(), cnt);
                    PostUiMsg(WM_APP_LOG, new std::wstring(okm));
                    done->installed_names.push_back(jb.short_name);
                }
                done->ok = true;
            } catch (const umicore::InstallerCancelled& ce) {
                done->cancelled = true;
                done->error_msg = umicore::Utf8ToWide(ce.what());
            } catch (const std::exception& ex) {
                done->ok = false;
                done->error_msg = umicore::Utf8ToWide(ex.what());
            }
            PostUiMsg(WM_APP_INSTALL_DONE, done);
        }).detach();
    }

    void HandleClick(int hit) {
        switch (hit) {
            case HIT_HDR_GUIDE:
                Mod07thDialog::Show(hwnd_, {}, false);
                break;
            case HIT_Q_CHECK:
                if (q_card_.CanSelect()) { q_card_.selected = !q_card_.selected; InvalidateRect(hwnd_, nullptr, FALSE); }
                break;
            case HIT_A_CHECK:
                if (a_card_.CanSelect()) { a_card_.selected = !a_card_.selected; InvalidateRect(hwnd_, nullptr, FALSE); }
                break;
            case HIT_Q_BADGE:
            case HIT_Q_MOD07:
                Mod07thDialog::Show(hwnd_, {{q_card_.title, q_card_.inspection.message}}, false);
                break;
            case HIT_A_BADGE:
            case HIT_A_MOD07:
                Mod07thDialog::Show(hwnd_, {{a_card_.title, a_card_.inspection.message}}, false);
                break;
            case HIT_Q_BROWSE:
                OnBrowseCard(q_card_);
                break;
            case HIT_A_BROWSE:
                OnBrowseCard(a_card_);
                break;
            case HIT_Q_ZIP:
                OnChooseZip(q_card_);
                break;
            case HIT_A_ZIP:
                OnChooseZip(a_card_);
                break;
            case HIT_Q_RESTORE:
                OnStartRestore(q_card_);
                break;
            case HIT_A_RESTORE:
                OnStartRestore(a_card_);
                break;
            case HIT_LINK_GITHUB:
                ShellExecuteW(hwnd_, L"open", umicore::GITHUB_REPO_URL_W, nullptr, nullptr, SW_SHOWNORMAL);
                break;
            case HIT_LINK_SITE:
                ShellExecuteW(hwnd_, L"open", umicore::AUTHOR_SITE_URL_W, nullptr, nullptr, SW_SHOWNORMAL);
                break;
            case HIT_LINK_MOD07:
                ShellExecuteW(hwnd_, L"open", umicore::MOD_07TH_URL_W, nullptr, nullptr, SW_SHOWNORMAL);
                break;
            case HIT_BTN_RESCAN:
                RescanInstallations(false);
                break;
            case HIT_BTN_INSTALL:
                OnPrimaryInstallOrCancel();
                break;
            default:
                break;
        }
    }

    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        MainInstallerWindow* self = nullptr;
        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
            self = static_cast<MainInstallerWindow*>(cs->lpCreateParams);
            self->hwnd_ = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        } else {
            self = reinterpret_cast<MainInstallerWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }
        return self ? self->WndProc(msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
    }

    LRESULT WndProc(UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
            case WM_CREATE: {
                dpi_ = QueryWindowDpi(hwnd_);
                log_edit_ = CreateWindowExW(
                    0, L"EDIT", L"",
                    WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                    0, 0, 100, 80,
                    hwnd_, reinterpret_cast<HMENU>(2001), GetModuleHandleW(nullptr), nullptr
                );
                SetWindowTheme(log_edit_, L"DarkMode_Explorer", nullptr);
                RebuildLogFont();
                UpdateLayout();
                return 0;
            }
            case WM_SIZE:
                UpdateLayout();
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;

            case WM_GETMINMAXINFO: {
                auto* mmi = reinterpret_cast<MINMAXINFO*>(lp);
                DWORD style = (DWORD)GetWindowLongW(hwnd_, GWL_STYLE);
                RECT mr{0, 0, (int)std::lround(S(750.0f)), (int)std::lround(S(640.0f))};
                AdjustWindowRectEx(&mr, style, FALSE, 0);
                mmi->ptMinTrackSize.x = mr.right - mr.left;
                mmi->ptMinTrackSize.y = mr.bottom - mr.top;
                return 0;
            }

            case WM_DPICHANGED: {
                dpi_ = HIWORD(wp);
                RebuildLogFont();
                auto* nr = reinterpret_cast<const RECT*>(lp);
                SetWindowPos(hwnd_, nullptr, nr->left, nr->top, nr->right - nr->left, nr->bottom - nr->top, SWP_NOZORDER | SWP_NOACTIVATE);
                UpdateLayout();
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }

            case WM_CTLCOLORSTATIC:
            case WM_CTLCOLOREDIT: {
                HDC hdcEdit = reinterpret_cast<HDC>(wp);
                HWND hCtrl = reinterpret_cast<HWND>(lp);
                if (hCtrl == log_edit_) {
                    SetTextColor(hdcEdit, RGB(203, 186, 192));
                    SetBkColor(hdcEdit, RGB(18, 10, 14));
                    return reinterpret_cast<LRESULT>(log_bg_brush_);
                }
                break;
            }

            case WM_ERASEBKGND:
                return 1;

            case WM_PAINT: {
                PAINTSTRUCT ps{};
                HDC hdc = BeginPaint(hwnd_, &ps);
                Paint(hdc);
                EndPaint(hwnd_, &ps);
                return 0;
            }

            case WM_MOUSEMOVE: {
                float x = (float)GET_X_LPARAM(lp);
                float y = (float)GET_Y_LPARAM(lp);
                int h = HitTest(x, y);
                if (h != hover_id_) {
                    hover_id_ = h;
                    InvalidateRect(hwnd_, nullptr, FALSE);
                }
                SetCursor(LoadCursorW(nullptr, h != HIT_NONE ? IDC_HAND : IDC_ARROW));
                return 0;
            }

            case WM_SETCURSOR:
                if (LOWORD(lp) == HTCLIENT && hover_id_ != HIT_NONE) {
                    SetCursor(LoadCursorW(nullptr, IDC_HAND));
                    return TRUE;
                }
                break;

            case WM_LBUTTONUP: {
                float x = (float)GET_X_LPARAM(lp);
                float y = (float)GET_Y_LPARAM(lp);
                HandleClick(HitTest(x, y));
                return 0;
            }

            case WM_TIMER:
                if (wp == TIMER_SCREENSHOT) {
                    KillTimer(hwnd_, TIMER_SCREENSHOT);
                    SaveWindowScreenshotPng(hwnd_, screenshot_out_);
                    DestroyWindow(hwnd_);
                    return 0;
                }
                break;

            case WM_APP_LOG: {
                std::unique_ptr<std::wstring> m(reinterpret_cast<std::wstring*>(lp));
                if (m) AppendLog(*m);
                return 0;
            }

            case WM_APP_PROGRESS: {
                std::unique_ptr<ProgressMsg> p(reinterpret_cast<ProgressMsg*>(lp));
                if (p) {
                    stage_text_ = p->stage;
                    progress_frac_ = p->fraction;
                    detail_text_ = p->detail;
                    InvalidateRect(hwnd_, nullptr, FALSE);
                }
                return 0;
            }

            case WM_APP_MANIFEST: {
                std::unique_ptr<std::map<std::string, umicore::RemotePackageInfo>> mp(
                    reinterpret_cast<std::map<std::string, umicore::RemotePackageInfo>*>(lp)
                );
                if (mp) {
                    remote_manifest_ = std::move(*mp);
                    auto iq = remote_manifest_.find("question");
                    if (iq != remote_manifest_.end()) { q_card_.remote_info = iq->second; q_card_.has_remote_info = true; }
                    auto ia = remote_manifest_.find("answer");
                    if (ia != remote_manifest_.end()) { a_card_.remote_info = ia->second; a_card_.has_remote_info = true; }
                    InvalidateRect(hwnd_, nullptr, FALSE);
                }
                return 0;
            }

            case WM_APP_STARTUP_POP: {
                std::unique_ptr<std::pair<std::vector<BlockedItem>, bool>> p(
                    reinterpret_cast<std::pair<std::vector<BlockedItem>, bool>*>(lp)
                );
                if (p) {
                    Mod07thDialog::Show(hwnd_, p->first, p->second);
                }
                return 0;
            }

            case WM_APP_INSTALL_DONE: {
                std::unique_ptr<InstallDoneMsg> d(reinterpret_cast<InstallDoneMsg*>(lp));
                is_busy_ = false;
                if (d) {
                    if (d->ok) {
                        progress_color_ = Color(255, 46, 160, 99);
                        progress_frac_ = 1.0;
                        stage_text_ = L"Kurulum tamamlandı! İyi okumalar.";
                        detail_text_ = L"%100";
                        if (!q_card_.current_path.empty()) SetCardFolder(q_card_, q_card_.current_path, false);
                        if (!a_card_.current_path.empty()) SetCardFolder(a_card_, a_card_.current_path, false);
                        UpdateLayout();
                        InvalidateRect(hwnd_, nullptr, FALSE);

                        std::wstring info = L"Türkçe yama başarıyla kuruldu:\n\n";
                        for (const auto& nm : d->installed_names) info += L"• " + nm + L"\n";
                        info += L"\nOyunu Steam üzerinden veya doğrudan başlatabilirsiniz.";
                        MessageBoxW(hwnd_, info.c_str(), L"Kurulum Tamamlandı", MB_OK | MB_ICONINFORMATION);
                    } else if (d->cancelled) {
                        progress_frac_ = 0.0;
                        stage_text_ = L"İşlem iptal edildi.";
                        detail_text_.clear();
                        AppendLog(d->error_msg);
                        InvalidateRect(hwnd_, nullptr, FALSE);
                    } else {
                        stage_text_ = L"Hata oluştu.";
                        detail_text_.clear();
                        AppendLog(L"HATA: " + d->error_msg);
                        InvalidateRect(hwnd_, nullptr, FALSE);
                        MessageBoxW(hwnd_, d->error_msg.c_str(), L"Kurulum Hatası", MB_OK | MB_ICONERROR);
                    }
                }
                return 0;
            }

            case WM_APP_RESTORE_DONE: {
                std::unique_ptr<RestoreDoneMsg> d(reinterpret_cast<RestoreDoneMsg*>(lp));
                is_busy_ = false;
                if (d) {
                    if (d->ok) {
                        progress_frac_ = 1.0;
                        stage_text_ = L"Yedek geri yüklendi.";
                        wchar_t det[64];
                        swprintf_s(det, L"%d dosya", d->restored_count);
                        detail_text_ = det;
                        wchar_t lg[128];
                        swprintf_s(lg, L"Orijinal yedek geri yüklendi (%d dosya).", d->restored_count);
                        AppendLog(lg);

                        CardState& c = (d->arc == "question") ? q_card_ : a_card_;
                        SetCardFolder(c, c.current_path, true);
                        UpdateLayout();
                        InvalidateRect(hwnd_, nullptr, FALSE);

                        wchar_t mb[128];
                        swprintf_s(mb, L"%d orijinal dosya başarıyla geri yüklendi.", d->restored_count);
                        MessageBoxW(hwnd_, mb, L"Geri Yükleme Tamamlandı", MB_OK | MB_ICONINFORMATION);
                    } else {
                        stage_text_ = L"Hata oluştu.";
                        detail_text_.clear();
                        AppendLog(L"HATA: " + d->error_msg);
                        InvalidateRect(hwnd_, nullptr, FALSE);
                        MessageBoxW(hwnd_, d->error_msg.c_str(), L"Kurulum Hatası", MB_OK | MB_ICONERROR);
                    }
                }
                return 0;
            }

            case WM_CLOSE:
                if (is_busy_) {
                    if (MessageBoxW(
                            hwnd_,
                            L"Kurulum devam ediyor. İptal edip çıkmak istiyor musunuz?",
                            L"Kurulum Devam Ediyor",
                            MB_YESNO | MB_ICONWARNING) != IDYES) {
                        return 0;
                    }
                    cancel_flag_.store(true);
                }
                DestroyWindow(hwnd_);
                return 0;

            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
        }
        return DefWindowProcW(hwnd_, msg, wp, lp);
    }
};

// ============================================================================
// Headless Self-Test Runner (--self-test <q_zip> <a_zip> <out_json>)
// ============================================================================
int RunHeadlessSelfTest(const fs::path& q_zip, const fs::path& a_zip, const fs::path& out_json) {
    try {
        std::string q_sha = umicore::Sha256File(q_zip);
        std::string a_sha = umicore::Sha256File(a_zip);

        umicore::ValidatePatchZip(q_zip, "question");
        umicore::ValidatePatchZip(a_zip, "answer");

        bool cross_rejected = false;
        try {
            umicore::ValidatePatchZip(q_zip, "answer");
        } catch (...) {
            cross_rejected = true;
        }
        if (!cross_rejected) throw std::runtime_error("Cross-arc ZIP check failed!");

        // Create temporary mock directory under %TEMP%
        wchar_t tmp_buf[MAX_PATH]{};
        GetTempPathW(MAX_PATH, tmp_buf);
        fs::path sandbox = fs::path(tmp_buf) / (L"umi_native_selftest_" + std::to_wstring(GetCurrentProcessId()));
        std::error_code ec;
        fs::remove_all(sandbox, ec);

        // 1. Test vanilla detection -> Missing07thMod
        fs::path van_q = sandbox / L"vanilla_q";
        umicore::WriteUtf8File(van_q / L"Umineko1to4.exe", "vanilla_ponscripter_only_0.utf");
        umicore::WriteUtf8File(van_q / L"0.utf", "vanilla");
        if (umicore::InspectGameDir("question", van_q).status != umicore::ArcStatus::Missing07thMod) {
            throw std::runtime_error("Vanilla Question folder was not blocked!");
        }

        // 2. Test full Question install + restore
        fs::path mod_q = sandbox / L"mod_q";
        std::string mod_exe_bin("PONSC", 5);
        mod_exe_bin.append("\0000.u\000", 5);
        mod_exe_bin.append("TAIL");
        umicore::WriteUtf8File(mod_q / L"Umineko1to4.exe", mod_exe_bin);
        umicore::WriteUtf8File(mod_q / L"0.u", std::string(1200000, 'Q'));
        umicore::WriteUtf8File(mod_q / L"arc4.nsa", "NSA");
        umicore::WriteUtf8File(mod_q / L"fonts" / L"default" / L"face0.otf", "ORIG_FONT");

        if (umicore::InspectGameDir("question", mod_q).status != umicore::ArcStatus::Ready) {
            throw std::runtime_error("Mock 07th-Mod Question folder was not Ready!");
        }
        if (umicore::InspectGameDir("answer", mod_q).status != umicore::ArcStatus::WrongArc) {
            throw std::runtime_error("WrongArc check failed!");
        }

        int q_installed = umicore::InstallArcPackage("question", mod_q, q_zip, "1.0.0", nullptr, nullptr);
        if (umicore::InspectGameDir("question", mod_q).status != umicore::ArcStatus::Installed) {
            throw std::runtime_error("Question status did not become Installed!");
        }
        if (!fs::is_regular_file(mod_q / L"bmp" / L"title" / L"tr_credit.png", ec)) {
            throw std::runtime_error("tr_credit.png missing after Question install!");
        }
        int q_restored = umicore::UninstallArcPatch("question", mod_q, nullptr);
        if (fs::exists(mod_q / L"bmp" / L"title" / L"tr_credit.png", ec)) {
            throw std::runtime_error("tr_credit.png was not cleaned up on restore!");
        }
        if (umicore::InspectGameDir("question", mod_q).status != umicore::ArcStatus::Ready) {
            throw std::runtime_error("Question status did not return to Ready after restore!");
        }

        // 3. Test full Answer install + restore
        fs::path mod_a = sandbox / L"mod_a";
        umicore::WriteUtf8File(mod_a / L"Umineko5to8.exe", mod_exe_bin);
        umicore::WriteUtf8File(mod_a / L"0.u", "Chiru header " + std::string(1200000, 'A'));
        fs::create_directories(mod_a / L"textbox_black", ec);
        if (umicore::InspectGameDir("answer", mod_a).status != umicore::ArcStatus::Ready) {
            throw std::runtime_error("Mock 07th-Mod Answer folder was not Ready!");
        }
        int a_installed = umicore::InstallArcPackage("answer", mod_a, a_zip, "1.0.0", nullptr, nullptr);
        int a_restored = umicore::UninstallArcPatch("answer", mod_a, nullptr);

        fs::remove_all(sandbox, ec);

        std::ostringstream js;
        js << "{\n"
           << "  \"status\": \"PASS\",\n"
           << "  \"question_sha256\": \"" << q_sha << "\",\n"
           << "  \"answer_sha256\": \"" << a_sha << "\",\n"
           << "  \"question_files_installed\": " << q_installed << ",\n"
           << "  \"question_files_restored\": " << q_restored << ",\n"
           << "  \"answer_files_installed\": " << a_installed << ",\n"
           << "  \"answer_files_restored\": " << a_restored << "\n"
           << "}\n";
        umicore::WriteUtf8File(out_json, js.str());
        return 0;
    } catch (const std::exception& ex) {
        std::string err = "{\n  \"status\": \"FAIL\",\n  \"error\": \"" + umicore::JsonEscape(ex.what()) + "\"\n}\n";
        umicore::WriteUtf8File(out_json, err);
        return 1;
    }
}

} // namespace

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int nCmdShow) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    fs::path screenshot_out;
    if (argv && argc >= 5 && wcscmp(argv[1], L"--self-test") == 0) {
        fs::path qz(argv[2]), az(argv[3]), oj(argv[4]);
        LocalFree(argv);
        return RunHeadlessSelfTest(qz, az, oj);
    }
    if (argv && argc >= 3 && wcscmp(argv[1], L"--screenshot-test") == 0) {
        screenshot_out = argv[2];
    }
    if (argv) LocalFree(argv);

    HRESULT hrCo = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    GdiplusStartupInput gdiInput;
    ULONG_PTR gdiToken = 0;
    GdiplusStartup(&gdiToken, &gdiInput, nullptr);

    int rc = 0;
    {
        MainInstallerWindow app(screenshot_out);
        rc = app.Run(nCmdShow);
    }

    GdiplusShutdown(gdiToken);
    if (SUCCEEDED(hrCo)) CoUninitialize();
    return rc;
}
