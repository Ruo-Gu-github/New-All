#pragma once
#include <string>

// 定义 NOMINMAX 避免 Windows.h 的 min/max 宏冲突
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

// ==================== 字符串编码转换 ====================

// UTF-8 转 UTF-16 (wstring) - 用于 Windows 文件系统 API
inline std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();
    
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
    if (wlen == 0) {
        return std::wstring();
    }
    
    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], wlen);
    
    // 移除末尾的 null 字符
    if (!wstr.empty() && wstr.back() == L'\0') {
        wstr.pop_back();
    }
    
    return wstr;
}

// UTF-8 转 ANSI (GBK) - inline 避免链接错误
// 注意：强制使用 GBK (CP936) 编码，而不是系统当前代码页
inline std::string Utf8ToAnsi(const std::string& utf8) {
    if (utf8.empty()) return std::string();
    
    // UTF-8 -> UTF-16
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
    if (wlen == 0) {
        throw std::runtime_error("Failed to convert UTF-8 to UTF-16: Invalid UTF-8 string");
    }
    
    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], wlen);
    
    // UTF-16 -> GBK (CP936) - 强制使用 GBK 而不是 CP_ACP
    // CP936 = GBK，中文简体编码
    const UINT TARGET_CP = 936;  // GBK 编码
    int alen = WideCharToMultiByte(TARGET_CP, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (alen == 0) {
        DWORD error = GetLastError();
        throw std::runtime_error("Failed to get GBK buffer size, error code: " + std::to_string(error));
    }
    
    std::string ansi(alen, 0);
    int result = WideCharToMultiByte(TARGET_CP, 0, wstr.c_str(), -1, &ansi[0], alen, NULL, NULL);
    
    if (result == 0) {
        DWORD error = GetLastError();
        throw std::runtime_error("Failed to convert UTF-16 to GBK, error code: " + std::to_string(error));
    }
    
    // 移除末尾的 null 字符
    if (!ansi.empty() && ansi.back() == '\0') {
        ansi.pop_back();
    }
    
    return ansi;
}

// ANSI (GBK) 转 UTF-8 - inline 避免链接错误
// 注意：强制使用 GBK (CP936) 编码
inline std::string AnsiToUtf8(const std::string& ansi) {
    if (ansi.empty()) return std::string();
    
    // GBK (CP936) -> UTF-16
    const UINT SOURCE_CP = 936;  // GBK 编码
    int wlen = MultiByteToWideChar(SOURCE_CP, 0, ansi.c_str(), -1, NULL, 0);
    if (wlen == 0) return std::string();
    
    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(SOURCE_CP, 0, ansi.c_str(), -1, &wstr[0], wlen);
    
    // UTF-16 -> UTF-8
    int utf8len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (utf8len == 0) return std::string();
    
    std::string utf8(utf8len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8[0], utf8len, NULL, NULL);
    
    // 移除末尾的 null 字符
    if (!utf8.empty() && utf8.back() == '\0') {
        utf8.pop_back();
    }
    
    return utf8;
}
