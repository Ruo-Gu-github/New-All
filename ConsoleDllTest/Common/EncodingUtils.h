#pragma once

#include <string>
#include <filesystem>

namespace EncodingUtils {

/// 将GBK编码的字符串转换为UTF-8
/// @param gbkStr GBK编码的输入字符串
/// @return UTF-8编码的字符串
std::string GbkToUtf8(const char* gbkStr);

/// 将GBK编码的字符串转换为宽字符串(UTF-16)
/// @param gbkStr GBK编码的输入字符串
/// @return UTF-16宽字符串
std::wstring GbkToWide(const char* gbkStr);

/// 将UTF-8编码的字符串转换为GBK
/// @param utf8Str UTF-8编码的输入字符串
/// @return GBK编码的字符串
std::string Utf8ToGbk(const char* utf8Str);

/// 将宽字符串(UTF-16)转换为UTF-8
/// @param wideStr UTF-16宽字符串
/// @return UTF-8编码的字符串
std::string WideToUtf8(const std::wstring& wideStr);

/// 将UTF-8编码的字符串转换为宽字符串(UTF-16)
/// @param utf8Str UTF-8编码的输入字符串
/// @return UTF-16宽字符串
std::wstring Utf8ToWide(const char* utf8Str);

/// 获取UTF-8路径字符串（适用于GDCM等库）
/// @param gbkPath GBK编码的路径
/// @return UTF-8编码的路径字符串
std::string GetUtf8Path(const char* gbkPath);

/// 打开文件（支持中文路径）
/// @param path 文件路径（GBK编码）
/// @param mode 打开模式（如"rb", "wb"等）
/// @return 文件指针，失败返回nullptr
FILE* OpenFile(const char* path, const char* mode);

} // namespace EncodingUtils
