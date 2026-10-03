#pragma once
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

static inline std::string wide_to_utf8(const std::wstring& w) {
    if (w.empty()) return {};
#ifdef _WIN32
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(len > 0 ? len - 1 : 0, '\0');
    if (len > 0)
        WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, out.data(), len, nullptr, nullptr);
    return out;
#else
    return std::string(w.begin(), w.end());
#endif
}

static inline std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return {};
#ifdef _WIN32
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out(len > 0 ? len - 1 : 0, L'\0');
    if (len > 0)
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), len);
    return out;
#else
    return std::wstring(s.begin(), s.end());
#endif
}
