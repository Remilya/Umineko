#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <tlhelp32.h>
#include <shlobj.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "zip_inflate.hpp"

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

namespace umicore {

namespace fs = std::filesystem;
using umizip::Utf8ToWide;
using umizip::WideToUtf8;

static constexpr const char* APP_VERSION = "1.0.0";
static constexpr const wchar_t* GITHUB_REPO_URL_W = L"https://github.com/Remilya/Umineko";
static constexpr const wchar_t* AUTHOR_SITE_URL_W = L"https://remilya.dev";
static constexpr const wchar_t* MOD_07TH_URL_W    = L"https://07th-mod.com/home/";

struct InstallerCancelled : public std::runtime_error {
    InstallerCancelled() : std::runtime_error("Kurulum kullanıcı tarafından iptal edildi.") {}
};

using ProgressFn = std::function<void(const std::wstring& stage, double fraction, const std::wstring& detail)>;

enum class ArcStatus {
    Ready,
    Installed,
    Missing07thMod,
    WrongArc,
    NotFound
};

struct ArcInspection {
    std::string arc;
    fs::path path;
    ArcStatus status = ArcStatus::NotFound;
    std::string installed_version;
    bool has_backup = false;
    std::wstring message;
};

struct RemotePackageInfo {
    std::string arc;
    std::string version = APP_VERSION;
    std::wstring url;
    uint64_t size_bytes = 0;
    std::string sha256;
};

inline std::string ReadFileBytesStr(const fs::path& p, size_t max_bytes = 0) {
    HANDLE h = CreateFileW(p.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return "";
    LARGE_INTEGER sz{};
    if (!GetFileSizeEx(h, &sz) || sz.QuadPart <= 0) {
        CloseHandle(h);
        return "";
    }
    size_t to_read = (size_t)sz.QuadPart;
    if (max_bytes > 0 && to_read > max_bytes) to_read = max_bytes;
    std::string out(to_read, '\0');
    size_t done = 0;
    while (done < to_read) {
        DWORD step = (DWORD)std::min<size_t>(to_read - done, 8 * 1024 * 1024);
        DWORD got = 0;
        if (!ReadFile(h, out.data() + done, step, &got, nullptr) || got == 0) break;
        done += got;
    }
    out.resize(done);
    CloseHandle(h);
    return out;
}

inline bool WriteUtf8File(const fs::path& p, const std::string& content) {
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    HANDLE h = CreateFileW(p.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    BOOL ok = WriteFile(h, content.data(), (DWORD)content.size(), &written, nullptr);
    CloseHandle(h);
    return ok && written == content.size();
}

inline std::string JsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", (unsigned)c);
                    out += buf;
                } else {
                    out.push_back((char)c);
                }
        }
    }
    return out;
}

inline std::string JsonExtractString(const std::string& json, const std::string& key, size_t start_pos = 0) {
    std::string needle = "\"" + key + "\"";
    size_t kpos = json.find(needle, start_pos);
    if (kpos == std::string::npos) return "";
    size_t colon = json.find(':', kpos + needle.size());
    if (colon == std::string::npos) return "";
    size_t q1 = json.find('\"', colon + 1);
    if (q1 == std::string::npos) return "";
    std::string out;
    for (size_t i = q1 + 1; i < json.size(); ++i) {
        char c = json[i];
        if (c == '\\' && i + 1 < json.size()) {
            char nx = json[++i];
            if (nx == '\"' || nx == '\\' || nx == '/') out.push_back(nx);
            else if (nx == 'n') out.push_back('\n');
            else if (nx == 'r') out.push_back('\r');
            else if (nx == 't') out.push_back('\t');
            else out.push_back(nx);
        } else if (c == '\"') {
            return out;
        } else {
            out.push_back(c);
        }
    }
    return "";
}

inline uint64_t JsonExtractUint64(const std::string& json, const std::string& key, size_t start_pos = 0) {
    std::string needle = "\"" + key + "\"";
    size_t kpos = json.find(needle, start_pos);
    if (kpos == std::string::npos) return 0;
    size_t colon = json.find(':', kpos + needle.size());
    if (colon == std::string::npos) return 0;
    size_t p = colon + 1;
    while (p < json.size() && (json[p] == ' ' || json[p] == '\t' || json[p] == '\r' || json[p] == '\n')) ++p;
    uint64_t val = 0;
    while (p < json.size() && json[p] >= '0' && json[p] <= '9') {
        val = val * 10 + (uint64_t)(json[p] - '0');
        ++p;
    }
    return val;
}

inline bool JsonExtractBool(const std::string& json, const std::string& key, bool def_val = false) {
    std::string needle = "\"" + key + "\"";
    size_t kpos = json.find(needle);
    if (kpos == std::string::npos) return def_val;
    size_t colon = json.find(':', kpos + needle.size());
    if (colon == std::string::npos) return def_val;
    size_t p = colon + 1;
    while (p < json.size() && (json[p] == ' ' || json[p] == '\t' || json[p] == '\r' || json[p] == '\n')) ++p;
    if (json.compare(p, 4, "true") == 0) return true;
    if (json.compare(p, 5, "false") == 0) return false;
    return def_val;
}

inline std::vector<std::string> JsonExtractStringArray(const std::string& json, const std::string& key) {
    std::vector<std::string> res;
    std::string needle = "\"" + key + "\"";
    size_t kpos = json.find(needle);
    if (kpos == std::string::npos) return res;
    size_t b1 = json.find('[', kpos + needle.size());
    if (b1 == std::string::npos) return res;
    size_t b2 = json.find(']', b1 + 1);
    if (b2 == std::string::npos) return res;
    size_t p = b1 + 1;
    while (p < b2) {
        size_t q1 = json.find('\"', p);
        if (q1 == std::string::npos || q1 >= b2) break;
        std::string item;
        size_t i = q1 + 1;
        for (; i < b2; ++i) {
            if (json[i] == '\\' && i + 1 < b2) {
                char nx = json[++i];
                item.push_back(nx);
            } else if (json[i] == '\"') {
                break;
            } else {
                item.push_back(json[i]);
            }
        }
        res.push_back(std::move(item));
        p = i + 1;
    }
    return res;
}

inline std::string IsoTimestampNow() {
    std::time_t t = std::time(nullptr);
    std::tm tmv{};
    localtime_s(&tmv, &t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tmv);
    return buf;
}

inline fs::path GetExeDir() {
    wchar_t buf[MAX_PATH * 4]{};
    DWORD n = GetModuleFileNameW(nullptr, buf, (DWORD)(sizeof(buf) / sizeof(wchar_t)));
    if (n == 0) return fs::current_path();
    return fs::path(buf).parent_path();
}

inline fs::path GetAppDataDir() {
    PWSTR p = nullptr;
    fs::path base;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &p)) && p) {
        base = fs::path(p) / L"UminekoTRInstaller";
        CoTaskMemFree(p);
    } else {
        base = GetExeDir() / L"_cache";
    }
    std::error_code ec;
    fs::create_directories(base / L"downloads", ec);
    return base;
}

inline fs::path GetCacheDir() {
    return GetAppDataDir() / L"downloads";
}

inline std::wstring IsGameRunning() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return L"";
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    std::wstring found;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"Umineko1to4.exe") == 0 ||
                _wcsicmp(pe.szExeFile, L"Umineko5to8.exe") == 0) {
                found = pe.szExeFile;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

inline std::string DetectFolderArcSignature(const fs::path& folder) {
    std::error_code ec;
    if (fs::exists(folder / L"Umineko1to4.exe", ec) || fs::exists(folder / L"Umineko1to4", ec)) return "question";
    if (fs::exists(folder / L"Umineko5to8.exe", ec) || fs::exists(folder / L"Umineko5to8", ec)) return "answer";
    if (fs::is_directory(folder / L"textbox_black", ec)) return "answer";
    if (fs::is_directory(folder / L"textbox" / L"black", ec)) return "question";

    fs::path u_path = folder / L"0.u";
    if (fs::is_regular_file(u_path, ec)) {
        std::string head = ReadFileBytesStr(u_path, 16384);
        if (head.find("Chiru") != std::string::npos || head.find("Answer") != std::string::npos) return "answer";
        if (head.find("Umineko") != std::string::npos) return "question";
    }
    return "";
}

inline std::pair<ArcStatus, std::wstring> Verify07thModFullEngine(const std::string& arc, const fs::path& p) {
    std::error_code ec;
    fs::path exe_win = p / (arc == "question" ? L"Umineko1to4.exe" : L"Umineko5to8.exe");
    fs::path exe_lin = p / (arc == "question" ? L"Umineko1to4" : L"Umineko5to8");
    fs::path active_exe;
    if (fs::is_regular_file(exe_win, ec)) active_exe = exe_win;
    else if (fs::is_regular_file(exe_lin, ec)) active_exe = exe_lin;

    bool has_any_trace = !active_exe.empty()
        || fs::is_regular_file(p / L"0.utf", ec)
        || fs::is_regular_file(p / L"arc.nsa", ec)
        || fs::is_regular_file(p / L"0.u", ec);
    if (!has_any_trace) {
        return {ArcStatus::NotFound, L"Bu klasörde geçerli bir Umineko kurulumu bulunamadı."};
    }

    if (active_exe.empty()) {
        return {ArcStatus::Missing07thMod, L"Oyun çalıştırma dosyası (exe) bulunamadı! Lütfen önce oyunu ve 07th-Mod'u kurun."};
    }

    bool has_leftover = fs::is_regular_file(p / L"0.u", ec)
        || fs::is_regular_file(p / L"installedVersionData.json", ec)
        || fs::is_regular_file(p / L"arc4.nsa", ec);

    std::string exe_bytes = ReadFileBytesStr(active_exe);
    if (exe_bytes.empty()) {
        return {ArcStatus::Missing07thMod, L"Oyun çalıştırma dosyası okunamadı."};
    }

    const std::string sig("\0000.u\000", 5);
    if (exe_bytes.find(sig) == std::string::npos) {
        if (has_leftover) {
            return {
                ArcStatus::Missing07thMod,
                L"Steam oyun motorunu düz sürüme sıfırlamış! Klasörde eski mod kalıntıları olsa da 07th-Mod aktif değil. Önce 07th-Mod'u yeniden kurun (veya Quick Repair yapın)."
            };
        }
        return {
            ArcStatus::Missing07thMod,
            L"Düz (modsuz) oyun algılandı! Türkçe yama kurmadan önce 07th-Mod (Full / Ses + Grafik) kurulmalıdır."
        };
    }

    fs::path script_0u = p / L"0.u";
    if (!fs::is_regular_file(script_0u, ec) || fs::file_size(script_0u, ec) < 1000000ULL) {
        return {
            ArcStatus::Missing07thMod,
            L"07th-Mod kurulumu eksik veya yarım kalmış ('0.u' senaryo dosyası bulunamadı). Lütfen 07th-Mod kurulumunu tamamlayın."
        };
    }

    if (arc == "question") {
        bool has_full = fs::is_regular_file(p / L"arc4.nsa", ec)
            || fs::is_directory(p / L"en" / L"bmp", ec)
            || fs::is_directory(p / L"textbox" / L"black", ec);
        if (!has_full) {
            return {
                ArcStatus::Missing07thMod,
                L"07th-Mod grafik/arayüz dosyaları eksik (yalnızca ses yaması kurulmuş olabilir). Lütfen 07th-Mod 'Full Patch' kurun."
            };
        }
    } else {
        bool has_full = fs::is_directory(p / L"textbox_black", ec)
            || fs::is_directory(p / L"en" / L"bmp", ec)
            || fs::is_directory(p / L"bmp" / L"title", ec);
        if (!has_full) {
            return {
                ArcStatus::Missing07thMod,
                L"07th-Mod Cevap Arkları (Chiru) grafik/arayüz dosyaları eksik. Lütfen önce 07th-Mod kurun."
            };
        }
    }

    return {ArcStatus::Ready, L"OK"};
}

inline ArcInspection InspectGameDir(const std::string& arc, const fs::path& folder) {
    ArcInspection res;
    res.arc = arc;
    res.path = folder;

    if (folder.empty()) {
        res.status = ArcStatus::NotFound;
        res.message = L"Oyun klasörü otomatik bulunamadı. Lütfen 'Klasör Seç' ile dizini gösterin.";
        return res;
    }

    std::error_code ec;
    if (!fs::exists(folder, ec) || !fs::is_directory(folder, ec)) {
        res.status = ArcStatus::NotFound;
        res.message = L"Seçilen klasör mevcut değil.";
        return res;
    }

    std::string sig = DetectFolderArcSignature(folder);
    if (!sig.empty() && sig != arc) {
        res.status = ArcStatus::WrongArc;
        res.message = (sig == "answer")
            ? L"Yanlış klasör! Bu dizin Cevap Arkları (Chiru / Bölüm 5-8) kurulumuna ait."
            : L"Yanlış klasör! Bu dizin Soru Arkları (Bölüm 1-4) kurulumuna ait.";
        return res;
    }

    auto [eng_status, eng_reason] = Verify07thModFullEngine(arc, folder);
    if (eng_status != ArcStatus::Ready) {
        res.status = eng_status;
        res.message = eng_reason;
        return res;
    }

    res.has_backup = fs::is_regular_file(folder / L"_tr_backup" / L"backup_manifest.json", ec)
        || fs::is_regular_file(folder / L"en" / L"bmp" / L"title" / L"logo.png.en_orig", ec)
        || fs::is_regular_file(folder / L"bmp" / L"title" / L"logo.png.en_orig", ec);

    fs::path manifest_file = folder / L"tr_patch_manifest.json";
    if (fs::is_regular_file(manifest_file, ec)) {
        std::string mjson = ReadFileBytesStr(manifest_file, 65536);
        std::string ver = JsonExtractString(mjson, "version");
        if (ver.empty()) ver = APP_VERSION;
        res.status = ArcStatus::Installed;
        res.installed_version = ver;
        res.message = L"Türkçe yama kurulu (v" + Utf8ToWide(ver) + L"). İsterseniz güncelleyebilir veya yedeği geri yükleyebilirsiniz.";
        return res;
    }

    std::string head_0u = ReadFileBytesStr(folder / L"0.u", 24576);
    if (head_0u.find("Martılar Ağladığında") != std::string::npos) {
        res.status = ArcStatus::Installed;
        res.installed_version = APP_VERSION;
        res.message = L"Türkçe yama kurulu. İsterseniz yeniden kurabilir veya İngilizce yedeğe dönebilirsiniz.";
        return res;
    }

    res.status = ArcStatus::Ready;
    res.message = L"07th-Mod algılandı. Türkçe yama kurulumuna hazır.";
    return res;
}

inline std::wstring ReadRegString(HKEY root, const wchar_t* subkey, const wchar_t* valname) {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(root, subkey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) return L"";
    wchar_t buf[1024]{};
    DWORD cb = sizeof(buf);
    DWORD type = 0;
    std::wstring out;
    if (RegQueryValueExW(hKey, valname, nullptr, &type, (LPBYTE)buf, &cb) == ERROR_SUCCESS &&
        (type == REG_SZ || type == REG_EXPAND_SZ)) {
        out = buf;
    }
    RegCloseKey(hKey);
    return out;
}

inline std::vector<fs::path> GetSteamCommonDirs() {
    std::vector<fs::path> roots;
    std::wstring s1 = ReadRegString(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath");
    if (!s1.empty()) roots.emplace_back(s1);
    std::wstring s2 = ReadRegString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Valve\\Steam", L"InstallPath");
    if (!s2.empty()) roots.emplace_back(s2);
    std::wstring s3 = ReadRegString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Valve\\Steam", L"InstallPath");
    if (!s3.empty()) roots.emplace_back(s3);

    for (wchar_t drv = L'C'; drv <= L'Z'; ++drv) {
        wchar_t p1[64], p2[64], p3[64];
        swprintf_s(p1, L"%c:\\Program Files (x86)\\Steam", drv);
        swprintf_s(p2, L"%c:\\SteamLibrary", drv);
        swprintf_s(p3, L"%c:\\Steam", drv);
        roots.emplace_back(p1);
        roots.emplace_back(p2);
        roots.emplace_back(p3);
    }

    std::vector<fs::path> commons;
    std::set<std::wstring> seen;
    auto add_common = [&](const fs::path& lib) {
        std::error_code ec;
        fs::path c = lib / L"steamapps" / L"common";
        if (fs::is_directory(c, ec)) {
            fs::path canon = fs::canonical(c, ec);
            if (ec || canon.empty()) canon = c.lexically_normal().make_preferred();
            std::wstring k = canon.wstring();
            std::transform(k.begin(), k.end(), k.begin(), ::towlower);
            if (seen.insert(k).second) commons.push_back(canon);
        }
    };

    for (const auto& r : roots) {
        add_common(r);
        std::error_code ec;
        fs::path vdf = r / L"steamapps" / L"libraryfolders.vdf";
        if (fs::is_regular_file(vdf, ec)) {
            std::string txt = ReadFileBytesStr(vdf, 262144);
            size_t pos = 0;
            while ((pos = txt.find("\"path\"", pos)) != std::string::npos) {
                size_t q1 = txt.find('\"', pos + 6);
                if (q1 == std::string::npos) break;
                size_t q2 = txt.find('\"', q1 + 1);
                if (q2 == std::string::npos) break;
                std::string raw = txt.substr(q1 + 1, q2 - q1 - 1);
                std::string unesc;
                for (size_t i = 0; i < raw.size(); ++i) {
                    if (raw[i] == '\\' && i + 1 < raw.size() && raw[i + 1] == '\\') {
                        unesc.push_back('\\');
                        ++i;
                    } else {
                        unesc.push_back(raw[i]);
                    }
                }
                add_common(fs::path(Utf8ToWide(unesc)));
                pos = q2 + 1;
            }
        }
    }
    return commons;
}

inline void DetectGamePaths(fs::path& out_question, fs::path& out_answer) {
    out_question.clear();
    out_answer.clear();

    fs::path exe_dir = GetExeDir();
    std::error_code ec;
    for (const auto& check : {exe_dir, exe_dir.parent_path()}) {
        if (check.empty()) continue;
        if (out_question.empty() && Verify07thModFullEngine("question", check).first == ArcStatus::Ready) {
            out_question = check;
        }
        if (out_answer.empty() && Verify07thModFullEngine("answer", check).first == ArcStatus::Ready) {
            out_answer = check;
        }
    }

    auto commons = GetSteamCommonDirs();
    const std::vector<std::wstring> q_names = {L"Umineko", L"Umineko When They Cry - Question Arcs"};
    const std::vector<std::wstring> a_names = {L"Umineko Chiru", L"Umineko When They Cry - Answer Arcs"};

    if (out_question.empty()) {
        for (int pass = 0; pass < 3 && out_question.empty(); ++pass) {
            for (const auto& c : commons) {
                for (const auto& nm : q_names) {
                    fs::path cand = c / nm;
                    if (!fs::is_directory(cand, ec)) continue;
                    if (pass == 0 && Verify07thModFullEngine("question", cand).first == ArcStatus::Ready) { out_question = cand; break; }
                    if (pass == 1 && (fs::exists(cand / L"Umineko1to4.exe", ec) || fs::exists(cand / L"0.utf", ec))) { out_question = cand; break; }
                    if (pass == 2 && fs::exists(cand / L"0.u", ec)) { out_question = cand; break; }
                }
                if (!out_question.empty()) break;
            }
        }
    }

    if (out_answer.empty()) {
        for (int pass = 0; pass < 3 && out_answer.empty(); ++pass) {
            for (const auto& c : commons) {
                for (const auto& nm : a_names) {
                    fs::path cand = c / nm;
                    if (!fs::is_directory(cand, ec)) continue;
                    if (pass == 0 && Verify07thModFullEngine("answer", cand).first == ArcStatus::Ready) { out_answer = cand; break; }
                    if (pass == 1 && (fs::exists(cand / L"Umineko5to8.exe", ec) || fs::exists(cand / L"0.utf", ec))) { out_answer = cand; break; }
                    if (pass == 2 && fs::exists(cand / L"0.u", ec)) { out_answer = cand; break; }
                }
                if (!out_answer.empty()) break;
            }
        }
    }
}

inline void ValidatePatchZip(const fs::path& zip_path, const std::string& expected_arc) {
    std::error_code ec;
    if (!fs::is_regular_file(zip_path, ec)) {
        throw std::runtime_error("Paket dosyası bulunamadı: " + WideToUtf8(zip_path.wstring()));
    }
    umizip::ZipReader zr(zip_path);
    const auto* e0 = zr.FindEntry("0.u");
    if (!e0) {
        throw std::runtime_error("Geçersiz yama arşivi ('0.u' bulunamadı): " + WideToUtf8(zip_path.filename().wstring()));
    }
    std::vector<uint8_t> head_vec = zr.ExtractToMemory(*e0, 24576);
    std::string head((const char*)head_vec.data(), head_vec.size());
    bool is_chiru = head.find("Chiru") != std::string::npos;
    if (expected_arc == "question" && is_chiru) {
        throw std::runtime_error("Seçilen ZIP dosyası Cevap Arkları (Chiru) için, Soru Arkları için değil!");
    }
    if (expected_arc == "answer" && !is_chiru) {
        throw std::runtime_error("Seçilen ZIP dosyası Soru Arkları için, Cevap Arkları (Chiru) için değil!");
    }
}

inline fs::path FindLocalPackageZip(const std::string& arc) {
    std::wstring fname = (arc == "question") ? L"Umineko_Question_TR.zip" : L"Umineko_Answer_TR.zip";
    fs::path exe_dir = GetExeDir();
    std::vector<fs::path> candidates = {
        exe_dir / fname,
        exe_dir / L"releases" / fname,
        exe_dir / L"dist" / L"releases" / fname,
        fs::current_path() / fname,
        fs::current_path() / L"dist" / L"releases" / fname,
    };
    std::error_code ec;
    for (const auto& c : candidates) {
        if (fs::is_regular_file(c, ec) && fs::file_size(c, ec) > 1024 * 1024) {
            return c;
        }
    }
    return {};
}

inline std::string Sha256File(const fs::path& path, const std::atomic<bool>* cancel_flag = nullptr) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) {
        throw std::runtime_error("SHA-256 saglayicisi baslatilamadi.");
    }
    DWORD objLen = 0, cbData = 0;
    BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&objLen, sizeof(objLen), &cbData, 0);
    std::vector<uint8_t> objBuf(objLen);
    if (BCryptCreateHash(hAlg, &hHash, objBuf.data(), objLen, nullptr, 0, 0) != 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        throw std::runtime_error("SHA-256 nesnesi olusturulamadi.");
    }

    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        throw std::runtime_error("SHA-256 icin dosya acilamadi.");
    }

    std::vector<uint8_t> buf(524288);
    DWORD got = 0;
    while (ReadFile(hFile, buf.data(), (DWORD)buf.size(), &got, nullptr) && got > 0) {
        if (cancel_flag && cancel_flag->load()) {
            CloseHandle(hFile);
            BCryptDestroyHash(hHash);
            BCryptCloseAlgorithmProvider(hAlg, 0);
            throw InstallerCancelled();
        }
        BCryptHashData(hHash, buf.data(), got, 0);
    }
    CloseHandle(hFile);

    uint8_t digest[32]{};
    BCryptFinishHash(hHash, digest, 32, 0);
    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    static const char hex[] = "0123456789abcdef";
    std::string out;
    out.reserve(64);
    for (int i = 0; i < 32; ++i) {
        out.push_back(hex[digest[i] >> 4]);
        out.push_back(hex[digest[i] & 0x0F]);
    }
    return out;
}

inline std::string HttpGetUtf8(const std::wstring& url, DWORD timeout_ms = 12000) {
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256]{};
    wchar_t urlpath[2048]{};
    uc.lpszHostName = host;
    uc.dwHostNameLength = 255;
    uc.lpszUrlPath = urlpath;
    uc.dwUrlPathLength = 2047;
    if (!WinHttpCrackUrl(url.c_str(), (DWORD)url.size(), 0, &uc)) return "";

    HINTERNET hSession = WinHttpOpen(L"UminekoTRInstaller/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return "";
    WinHttpSetTimeouts(hSession, timeout_ms, timeout_ms, timeout_ms, timeout_ms);

    HINTERNET hConnect = WinHttpConnect(hSession, host, uc.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return ""; }

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hReq = WinHttpOpenRequest(hConnect, L"GET", urlpath, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hReq) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return ""; }

    std::string body;
    if (WinHttpSendRequest(hReq, L"Accept: application/vnd.github+json\r\n", (DWORD)-1L, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hReq, nullptr)) {
        DWORD status = 0, sz = sizeof(status);
        WinHttpQueryHeaders(hReq, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &sz, WINHTTP_NO_HEADER_INDEX);
        if (status >= 200 && status < 300) {
            DWORD avail = 0;
            while (WinHttpQueryDataAvailable(hReq, &avail) && avail > 0) {
                size_t old = body.size();
                body.resize(old + avail);
                DWORD got = 0;
                WinHttpReadData(hReq, body.data() + old, avail, &got);
                body.resize(old + got);
            }
        }
    }

    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return body;
}

inline std::map<std::string, RemotePackageInfo> FetchRemoteManifest() {
    std::map<std::string, RemotePackageInfo> pkgs;
    pkgs["question"] = {"question", APP_VERSION, L"https://github.com/Remilya/Umineko/releases/latest/download/Umineko_Question_TR.zip", 0, ""};
    pkgs["answer"]   = {"answer",   APP_VERSION, L"https://github.com/Remilya/Umineko/releases/latest/download/Umineko_Answer_TR.zip", 0, ""};

    std::string mjson = HttpGetUtf8(L"https://github.com/Remilya/Umineko/releases/latest/download/release_manifest.json");
    if (!mjson.empty()) {
        std::string ver = JsonExtractString(mjson, "version");
        if (ver.empty()) ver = APP_VERSION;
        for (const std::string& arc : {"question", "answer"}) {
            size_t apos = mjson.find("\"" + arc + "\"");
            if (apos != std::string::npos) {
                pkgs[arc].version = ver;
                std::string u = JsonExtractString(mjson, "url", apos);
                if (!u.empty()) pkgs[arc].url = Utf8ToWide(u);
                pkgs[arc].size_bytes = JsonExtractUint64(mjson, "size_bytes", apos);
                pkgs[arc].sha256 = JsonExtractString(mjson, "sha256", apos);
            }
        }
    }
    return pkgs;
}

inline fs::path DownloadPackageZip(
    const RemotePackageInfo& pkg,
    const ProgressFn& progress_cb,
    const std::atomic<bool>* cancel_flag
) {
    fs::path cache_dir = GetCacheDir();
    std::wstring stem = (pkg.arc == "question") ? L"Umineko_Question_TR" : L"Umineko_Answer_TR";
    fs::path final_zip = cache_dir / (stem + L"_v" + Utf8ToWide(pkg.version) + L".zip");

    std::error_code ec;
    if (fs::is_regular_file(final_zip, ec)) {
        try {
            if (pkg.sha256.empty() || _stricmp(Sha256File(final_zip, cancel_flag).c_str(), pkg.sha256.c_str()) == 0) {
                ValidatePatchZip(final_zip, pkg.arc);
                return final_zip;
            }
        } catch (const InstallerCancelled&) {
            throw;
        } catch (...) {}
    }

    fs::path part_file = final_zip;
    part_file += L".part";

    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256]{};
    wchar_t urlpath[2048]{};
    uc.lpszHostName = host;
    uc.dwHostNameLength = 255;
    uc.lpszUrlPath = urlpath;
    uc.dwUrlPathLength = 2047;
    if (!WinHttpCrackUrl(pkg.url.c_str(), (DWORD)pkg.url.size(), 0, &uc)) {
        throw std::runtime_error("Gecersiz indirme baglantisi.");
    }

    HINTERNET hSession = WinHttpOpen(L"UminekoTRInstaller/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) throw std::runtime_error("Ag oturumu baslatilamadi.");
    WinHttpSetTimeouts(hSession, 20000, 20000, 30000, 30000);

    HINTERNET hConnect = WinHttpConnect(hSession, host, uc.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); throw std::runtime_error("Sunucuya baglanilamadi."); }

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hReq = WinHttpOpenRequest(hConnect, L"GET", urlpath, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hReq) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); throw std::runtime_error("HTTP istegi olusturulamadi."); }

    std::wstring arc_short = (pkg.arc == "question") ? L"Soru Arkları" : L"Cevap Arkları";
    if (!WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(hReq, nullptr)) {
        WinHttpCloseHandle(hReq); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        throw std::runtime_error("Yama paketi indirilirken ag baglantisi kesildi.");
    }

    DWORD status = 0, ssz = sizeof(status);
    WinHttpQueryHeaders(hReq, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &ssz, WINHTTP_NO_HEADER_INDEX);
    if (status == 404) {
        WinHttpCloseHandle(hReq); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        throw std::runtime_error("GitHub Releases uzerinde yama paketi henuz yayinlanmamis (HTTP 404). 'Yerel ZIP Sec' kullanabilirsiniz.");
    }
    if (status < 200 || status >= 300) {
        WinHttpCloseHandle(hReq); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        throw std::runtime_error("Sunucu hatasi: HTTP " + std::to_string(status));
    }

    wchar_t clen_buf[64]{};
    DWORD clen_sz = sizeof(clen_buf);
    uint64_t total = pkg.size_bytes;
    if (WinHttpQueryHeaders(hReq, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX, clen_buf, &clen_sz, WINHTTP_NO_HEADER_INDEX)) {
        uint64_t parsed = _wcstoui64(clen_buf, nullptr, 10);
        if (parsed > 0) total = parsed;
    }

    HANDLE hOut = CreateFileW(part_file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hOut == INVALID_HANDLE_VALUE) {
        WinHttpCloseHandle(hReq); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        throw std::runtime_error("Indirme onbellek dosyasi olusturulamadi.");
    }

    std::vector<uint8_t> buf(262144);
    uint64_t downloaded = 0;
    auto t0 = std::chrono::steady_clock::now();
    auto last_ui = t0;

    try {
        for (;;) {
            if (cancel_flag && cancel_flag->load()) throw InstallerCancelled();
            DWORD avail = 0;
            if (!WinHttpQueryDataAvailable(hReq, &avail)) throw std::runtime_error("Ag veri okuma hatasi.");
            if (avail == 0) break;
            DWORD to_get = std::min<DWORD>(avail, (DWORD)buf.size());
            DWORD got = 0;
            if (!WinHttpReadData(hReq, buf.data(), to_get, &got) || got == 0) break;
            DWORD written = 0;
            if (!WriteFile(hOut, buf.data(), got, &written, nullptr) || written != got) {
                throw std::runtime_error("Disk yazma hatasi.");
            }
            downloaded += got;

            auto now = std::chrono::steady_clock::now();
            double since_ui = std::chrono::duration<double>(now - last_ui).count();
            if (progress_cb && (since_ui >= 0.08 || (total > 0 && downloaded >= total))) {
                last_ui = now;
                double elapsed = std::max(0.001, std::chrono::duration<double>(now - t0).count());
                double speed_mb = (downloaded / (1024.0 * 1024.0)) / elapsed;
                double dl_mb = downloaded / (1024.0 * 1024.0);
                wchar_t detail[128];
                double frac = 0.0;
                if (total > 0) {
                    frac = std::min(1.0, (double)downloaded / (double)total);
                    double tot_mb = total / (1024.0 * 1024.0);
                    swprintf_s(detail, L"%.1f / %.1f MB (%.1f MB/sn)", dl_mb, tot_mb, speed_mb);
                } else {
                    swprintf_s(detail, L"%.1f MB indirildi (%.1f MB/sn)", dl_mb, speed_mb);
                }
                progress_cb(arc_short + L" paketi indiriliyor...", frac, detail);
            }
        }
        CloseHandle(hOut);
        WinHttpCloseHandle(hReq);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
    } catch (...) {
        CloseHandle(hOut);
        WinHttpCloseHandle(hReq);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        DeleteFileW(part_file.c_str());
        throw;
    }

    if (!pkg.sha256.empty()) {
        if (progress_cb) progress_cb(arc_short + L" paketi doğrulanıyor (SHA-256)...", 1.0, L"Bütünlük kontrolü");
        std::string actual = Sha256File(part_file, cancel_flag);
        if (_stricmp(actual.c_str(), pkg.sha256.c_str()) != 0) {
            DeleteFileW(part_file.c_str());
            throw std::runtime_error("Indirilen dosyanin SHA-256 ozeti uyusmuyor (bozuk indirme). Lutfen tekrar deneyin.");
        }
    }

    ValidatePatchZip(part_file, pkg.arc);
    MoveFileExW(part_file.c_str(), final_zip.c_str(), MOVEFILE_REPLACE_EXISTING);
    return final_zip;
}

inline int SyncExtraFontFolders(
    const fs::path& game_root,
    std::set<std::string>& backed_set,
    const fs::path& backup_root,
    bool had_tr_before
) {
    std::error_code ec;
    fs::path src_fonts = game_root / L"fonts";
    if (!fs::is_directory(src_fonts, ec)) return 0;

    std::vector<fs::path> extra_dirs = {
        game_root / L"fonts" / L"adv",
        game_root / L"fonts" / L"default",
        game_root / L"fonts_adv",
        game_root / L"fonts_default",
    };
    std::vector<std::wstring> font_names = {
        L"face0.otf", L"face1.otf", L"face2.otf", L"face3.otf",
        L"face4.ttf", L"face4.otf", L"fixedface0.ttf"
    };

    int mirrored = 0;
    for (const auto& ed : extra_dirs) {
        if (!fs::is_directory(ed, ec)) continue;
        for (const auto& fn : font_names) {
            fs::path target = ed / fn;
            fs::path src = src_fonts / (fn == L"face4.otf" ? L"face4.ttf" : fn);
            if (!fs::is_regular_file(target, ec) || !fs::is_regular_file(src, ec)) continue;

            std::string rel_posix = WideToUtf8(fs::relative(target, game_root, ec).generic_wstring());
            fs::path bak_dest = backup_root / Utf8ToWide(rel_posix);
            if (!fs::exists(bak_dest, ec)) {
                fs::path jp_orig = target;
                jp_orig += L".jp_orig";
                if (fs::is_regular_file(jp_orig, ec)) {
                    fs::create_directories(bak_dest.parent_path(), ec);
                    fs::copy_file(jp_orig, bak_dest, fs::copy_options::overwrite_existing, ec);
                    backed_set.insert(rel_posix);
                } else if (!had_tr_before) {
                    fs::create_directories(bak_dest.parent_path(), ec);
                    fs::copy_file(target, bak_dest, fs::copy_options::overwrite_existing, ec);
                    backed_set.insert(rel_posix);
                }
            }
            fs::copy_file(src, target, fs::copy_options::overwrite_existing, ec);
            ++mirrored;
        }
    }
    return mirrored;
}

inline int InstallArcPackage(
    const std::string& arc,
    const fs::path& game_root,
    const fs::path& zip_path,
    const std::string& version,
    const ProgressFn& progress_cb,
    const std::atomic<bool>* cancel_flag
) {
    std::wstring running = IsGameRunning();
    if (!running.empty()) {
        throw std::runtime_error("Oyun şu anda açık (" + WideToUtf8(running) + "). Lütfen oyunu kapatıp kurulumu tekrar başlatın.");
    }

    ArcInspection insp = InspectGameDir(arc, game_root);
    if (insp.status == ArcStatus::NotFound || insp.status == ArcStatus::WrongArc || insp.status == ArcStatus::Missing07thMod) {
        throw std::runtime_error(WideToUtf8(insp.message));
    }

    ValidatePatchZip(zip_path, arc);

    std::wstring arc_short = (arc == "question") ? L"Soru Arkları" : L"Cevap Arkları";
    bool had_tr_before = (insp.status == ArcStatus::Installed);
    fs::path backup_root = game_root / L"_tr_backup";
    fs::path backup_manifest = backup_root / L"backup_manifest.json";

    std::set<std::string> backed_set;
    std::set<std::string> created_set;
    std::error_code ec;
    if (fs::is_regular_file(backup_manifest, ec)) {
        std::string bjson = ReadFileBytesStr(backup_manifest, 524288);
        for (const auto& f : JsonExtractStringArray(bjson, "files")) backed_set.insert(f);
        for (const auto& f : JsonExtractStringArray(bjson, "created_files")) created_set.insert(f);
    }

    fs::path root_canon = fs::weakly_canonical(game_root, ec);
    std::wstring root_prefix = root_canon.wstring();
    std::transform(root_prefix.begin(), root_prefix.end(), root_prefix.begin(), ::towlower);

    umizip::ZipReader zr(zip_path);
    std::vector<const umizip::ZipEntryInfo*> files;
    for (const auto& e : zr.Entries()) {
        if (!e.is_dir && !e.name_utf8.empty()) files.push_back(&e);
    }
    size_t total = std::max<size_t>(1, files.size());
    int installed_count = 0;

    for (size_t idx = 0; idx < files.size(); ++idx) {
        if (cancel_flag && cancel_flag->load()) throw InstallerCancelled();

        const auto& info = *files[idx];
        std::string rel = info.name_utf8;
        while (!rel.empty() && rel.front() == '/') rel.erase(rel.begin());
        if (rel.empty() || rel.find("..") != std::string::npos) continue;
        if (rel.size() > 8 && (rel.substr(rel.size() - 8) == ".en_orig" || rel.substr(rel.size() - 8) == ".jp_orig")) continue;

        fs::path dest_path = game_root / Utf8ToWide(rel);
        fs::path dest_canon = fs::weakly_canonical(dest_path, ec);
        std::wstring dp_low = dest_canon.wstring();
        std::transform(dp_low.begin(), dp_low.end(), dp_low.begin(), ::towlower);
        if (dp_low.rfind(root_prefix, 0) != 0) {
            throw std::runtime_error("Guvenlik hatasi: Arsiv icinde gecersiz dosya yolu (" + rel + ").");
        }

        fs::path bak_path = backup_root / Utf8ToWide(rel);
        if (fs::exists(dest_path, ec) && !fs::exists(bak_path, ec)) {
            fs::path en_orig = dest_path; en_orig += L".en_orig";
            fs::path jp_orig = dest_path; jp_orig += L".jp_orig";
            fs::path src_for_bak;
            if (fs::is_regular_file(en_orig, ec)) src_for_bak = en_orig;
            else if (fs::is_regular_file(jp_orig, ec)) src_for_bak = jp_orig;
            else if (!had_tr_before) src_for_bak = dest_path;

            if (!src_for_bak.empty()) {
                fs::create_directories(bak_path.parent_path(), ec);
                fs::copy_file(src_for_bak, bak_path, fs::copy_options::overwrite_existing, ec);
                backed_set.insert(rel);
            }
        } else if (!fs::exists(dest_path, ec) && !fs::exists(bak_path, ec)) {
            created_set.insert(rel);
        }

        if (rel == "0.u") {
            fs::path tmp_0u = dest_path.parent_path() / L"0.u.tr_install_tmp";
            zr.ExtractToFile(info, tmp_0u);
            MoveFileExW(tmp_0u.c_str(), dest_path.c_str(), MOVEFILE_REPLACE_EXISTING);
        } else {
            zr.ExtractToFile(info, dest_path);
        }
        ++installed_count;

        if (progress_cb && ((idx + 1) % 12 == 0 || idx + 1 == total)) {
            wchar_t det[256];
            swprintf_s(det, L"%zu / %zu dosya (%s)", idx + 1, total, Utf8ToWide(rel).c_str());
            progress_cb(arc_short + L" dosyaları kuruluyor...", (double)(idx + 1) / (double)total, det);
        }
    }

    int mirrored_fonts = SyncExtraFontFolders(game_root, backed_set, backup_root, had_tr_before);

    if (!backed_set.empty() || !created_set.empty()) {
        std::ostringstream oss;
        oss << "{\n"
            << "  \"arc\": \"" << JsonEscape(arc) << "\",\n"
            << "  \"created_at\": \"" << IsoTimestampNow() << "\",\n"
            << "  \"files\": [\n";
        size_t i = 0;
        for (const auto& f : backed_set) {
            oss << "    \"" << JsonEscape(f) << "\"" << (++i < backed_set.size() ? "," : "") << "\n";
        }
        oss << "  ],\n"
            << "  \"created_files\": [\n";
        i = 0;
        for (const auto& f : created_set) {
            oss << "    \"" << JsonEscape(f) << "\"" << (++i < created_set.size() ? "," : "") << "\n";
        }
        oss << "  ]\n}\n";
        WriteUtf8File(backup_manifest, oss.str());
    }

    {
        std::ostringstream pm;
        pm << "{\n"
           << "  \"arc\": \"" << JsonEscape(arc) << "\",\n"
           << "  \"version\": \"" << JsonEscape(version) << "\",\n"
           << "  \"installed_at\": \"" << IsoTimestampNow() << "\",\n"
           << "  \"source_zip\": \"" << JsonEscape(WideToUtf8(zip_path.filename().wstring())) << "\",\n"
           << "  \"files_installed_count\": " << installed_count << ",\n"
           << "  \"extra_fonts_synced\": " << mirrored_fonts << "\n"
           << "}\n";
        WriteUtf8File(game_root / L"tr_patch_manifest.json", pm.str());
    }

    return installed_count;
}

inline int UninstallArcPatch(
    const std::string& arc,
    const fs::path& game_root,
    const ProgressFn& progress_cb
) {
    std::wstring running = IsGameRunning();
    if (!running.empty()) {
        throw std::runtime_error("Oyun şu anda açık (" + WideToUtf8(running) + "). Yedeği geri yüklemeden önce oyunu kapatın.");
    }

    std::wstring arc_short = (arc == "question") ? L"Soru Arkları" : L"Cevap Arkları";
    fs::path backup_root = game_root / L"_tr_backup";
    fs::path backup_manifest = backup_root / L"backup_manifest.json";
    int restored = 0;
    std::error_code ec;

    if (fs::is_regular_file(backup_manifest, ec)) {
        std::string bjson = ReadFileBytesStr(backup_manifest, 524288);
        auto files = JsonExtractStringArray(bjson, "files");
        auto created = JsonExtractStringArray(bjson, "created_files");
        size_t total = std::max<size_t>(1, files.size());

        for (size_t i = 0; i < files.size(); ++i) {
            fs::path src = backup_root / Utf8ToWide(files[i]);
            fs::path dst = game_root / Utf8ToWide(files[i]);
            if (fs::is_regular_file(src, ec)) {
                fs::create_directories(dst.parent_path(), ec);
                if (files[i] == "0.u") {
                    fs::path tmp = dst.parent_path() / L"0.u.restore_tmp";
                    fs::copy_file(src, tmp, fs::copy_options::overwrite_existing, ec);
                    MoveFileExW(tmp.c_str(), dst.c_str(), MOVEFILE_REPLACE_EXISTING);
                } else {
                    fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
                }
                ++restored;
            }
            if (progress_cb && ((i + 1) % 20 == 0 || i + 1 == total)) {
                wchar_t det[64];
                swprintf_s(det, L"%zu / %zu dosya", i + 1, total);
                progress_cb(arc_short + L" orijinal dosyaları geri yükleniyor...", (double)(i + 1) / (double)total, det);
            }
        }
        for (const auto& cf : created) {
            if (cf.find("..") == std::string::npos) {
                fs::remove(game_root / Utf8ToWide(cf), ec);
            }
        }
        fs::remove_all(backup_root, ec);
    } else {
        std::vector<fs::path> origs;
        for (auto it = fs::recursive_directory_iterator(game_root, ec); it != fs::recursive_directory_iterator(); it.increment(ec)) {
            if (!it->is_regular_file(ec)) continue;
            std::wstring nm = it->path().filename().wstring();
            if (nm.size() > 8 && (nm.substr(nm.size() - 8) == L".en_orig" || nm.substr(nm.size() - 8) == L".jp_orig")) {
                origs.push_back(it->path());
            }
        }
        if (origs.empty()) {
            throw std::runtime_error("Bu klasorde geri yuklenecek yedek (_tr_backup) bulunamadi.");
        }
        for (size_t i = 0; i < origs.size(); ++i) {
            std::wstring full = origs[i].wstring();
            fs::path dst(full.substr(0, full.size() - 8));
            fs::copy_file(origs[i], dst, fs::copy_options::overwrite_existing, ec);
            ++restored;
        }
    }

    fs::remove(game_root / L"tr_patch_manifest.json", ec);
    return restored;
}

} // namespace umicore
