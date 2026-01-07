#include "pch.h"
#include "EncodingUtils.h"
#include <Windows.h>
#include <vector>

namespace EncodingUtils {

std::string GbkToUtf8(const char* gbkStr) {
    if (!gbkStr || gbkStr[0] == '\0') {
        return std::string();
    }
    
    // GBK -> UTF-16
    int wlen = MultiByteToWideChar(936, 0, gbkStr, -1, NULL, 0);
    if (wlen <= 0) {
        return std::string();
    }
    
    std::wstring wideStr(wlen, 0);
    MultiByteToWideChar(936, 0, gbkStr, -1, &wideStr[0], wlen);
    
    // 移除末尾的空字符
    if (!wideStr.empty() && wideStr.back() == L'\0') {
        wideStr.pop_back();
    }
    
    // UTF-16 -> UTF-8
    return WideToUtf8(wideStr);
}

std::wstring GbkToWide(const char* gbkStr) {
    if (!gbkStr || gbkStr[0] == '\0') {
        return std::wstring();
    }
    
    int wlen = MultiByteToWideChar(936, 0, gbkStr, -1, NULL, 0);
    if (wlen <= 0) {
        return std::wstring();
    }
    
    std::wstring wideStr(wlen, 0);
    MultiByteToWideChar(936, 0, gbkStr, -1, &wideStr[0], wlen);
    
    // 移除末尾的空字符
    if (!wideStr.empty() && wideStr.back() == L'\0') {
        wideStr.pop_back();
    }
    
    return wideStr;
}

std::string Utf8ToGbk(const char* utf8Str) {
    if (!utf8Str || utf8Str[0] == '\0') {
        return std::string();
    }
    
    // UTF-8 -> UTF-16
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0);
    if (wlen <= 0) {
        return std::string();
    }
    
    std::wstring wideStr(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, &wideStr[0], wlen);
    
    if (!wideStr.empty() && wideStr.back() == L'\0') {
        wideStr.pop_back();
    }
    
    // UTF-16 -> GBK
    int gbkLen = WideCharToMultiByte(936, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
    if (gbkLen <= 0) {
        return std::string();
    }
    
    std::string gbkStr(gbkLen, 0);
    WideCharToMultiByte(936, 0, wideStr.c_str(), -1, &gbkStr[0], gbkLen, NULL, NULL);
    
    if (!gbkStr.empty() && gbkStr.back() == '\0') {
        gbkStr.pop_back();
    }
    
    return gbkStr;
}

std::string WideToUtf8(const std::wstring& wideStr) {
    if (wideStr.empty()) {
        return std::string();
    }
    
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
    if (utf8Len <= 0) {
        return std::string();
    }
    
    std::string utf8Str(utf8Len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], utf8Len, NULL, NULL);
    
    if (!utf8Str.empty() && utf8Str.back() == '\0') {
        utf8Str.pop_back();
    }
    
    return utf8Str;
}

std::wstring Utf8ToWide(const char* utf8Str) {
    if (!utf8Str || utf8Str[0] == '\0') {
        return std::wstring();
    }
    
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0);
    if (wlen <= 0) {
        return std::wstring();
    }
    
    std::wstring wideStr(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, &wideStr[0], wlen);
    
    if (!wideStr.empty() && wideStr.back() == L'\0') {
        wideStr.pop_back();
    }
    
    return wideStr;
}

std::string GetUtf8Path(const char* gbkPath) {
    if (!gbkPath || gbkPath[0] == '\0') {
        return std::string();
    }
    
    // GBK -> Wide -> filesystem::path -> u8string
    std::wstring widePath = GbkToWide(gbkPath);
    std::filesystem::path fsPath(widePath);
    return fsPath.u8string();
}

FILE* OpenFile(const char* path, const char* mode) {
    if (!path || !mode) {
        return nullptr;
    }
    
    // 转换为宽字符以支持中文路径
    std::wstring widePath = GbkToWide(path);
    std::wstring wideMode = GbkToWide(mode);
    
    FILE* file = nullptr;
    _wfopen_s(&file, widePath.c_str(), wideMode.c_str());
    return file;
}

} // namespace EncodingUtils
