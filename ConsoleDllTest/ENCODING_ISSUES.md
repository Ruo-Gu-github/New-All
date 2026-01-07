# Windows 中文路径编码问题记录

## 问题描述

在 Windows 平台上使用 GDCM 库加载 DICOM 文件时，遇到中文路径无法正确识别的问题。

### 错误示例
```
路径：D:\Scripts\Example\股骨\0.372
错误：directory_iterator::directory_iterator: The system cannot find the path specified
```

## 根本原因

### 编码体系冲突

1. **Node.js / Electron**：使用 UTF-8 编码
2. **Windows API (ANSI)**：使用系统代码页（中文系统为 GBK / CP936）
3. **C++ std::filesystem**：在 Windows 上对 UTF-8 支持不完善
4. **GDCM 库**：需要 UTF-8 编码的路径

### 数据流编码转换链

```
JavaScript (UTF-8) 
  ↓
Node.js Native Addon (接收 UTF-8)
  ↓
转换为 GBK (使用 MultiByteToWideChar + WideCharToMultiByte)
  ↓
DLL API (接收 GBK)
  ↓
转换为 wstring (使用 fs::path::wstring())
  ↓
转换为 UTF-8 (使用 fs::path::u8string())
  ↓
GDCM 库 (使用 UTF-8)
```

## 尝试的方案

### 方案 1：全部使用 UTF-8 编译选项 ❌

**做法**：在所有 .vcxproj 文件中添加 `/utf-8` 编译选项

```xml
<AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>
```

**问题**：
- 第三方依赖库（GDCM、ITK 等）都是用 GBK 编译的
- 链接时会出现符号不匹配问题
- 无法强制所有依赖都重新编译为 UTF-8

**结论**：不可行，已回滚

### 方案 2：使用 std::filesystem::u8path ❌

**做法**：使用 C++17 的 `fs::u8path()` 从 UTF-8 字符串创建路径对象

```cpp
// 尝试的代码
auto dirPath = fs::u8path(directory);
fs::directory_iterator iter(dirPath);

auto firstPath = fs::u8path(dicomFiles[0]);
firstReader.SetFileName(firstPath.u8string().c_str());
```

**问题**：
- 在 MSVC 编译器中，`fs::u8path` 实现有问题
- `fs::directory_iterator` 无法正确处理 u8path 创建的路径
- `fs::exists` 也无法正确识别
- 错误信息："The system cannot find the path specified"

**结论**：理论上正确，但在 Windows/MSVC 实现中有 bug，不可行

### 方案 3：UTF-8 → GBK 转换（当前方案）✅

**做法**：在 Native Addon 层手动转换编码

```cpp
// 辅助函数
std::wstring Utf8ToWide(const std::string& utf8) {
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
    std::wstring wide(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], len);
    return wide;
}

std::string WideToAnsi(const std::wstring& wide) {
    int len = WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, NULL, 0, NULL, NULL);
    std::string ansi(len, 0);
    WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, &ansi[0], len, NULL, NULL);
    return ansi;
}

std::string Utf8ToAnsi(const std::string& utf8) {
    return WideToAnsi(Utf8ToWide(utf8));
}

// 在 LoadFromFolder 中使用
std::string folderPathGbk = Utf8ToAnsi(folderPathUtf8);
Dicom_Volume_LoadFromDicomSeries(volumeHandle_, folderPathGbk.c_str());
```

**在 DLL 内部**（DicomApi.cpp）：

```cpp
// 从 GBK 路径转为 UTF-8 传给 GDCM
std::wstring wpath = fs::path(filepath).wstring();  // GBK → UTF-16
std::string utf8path = fs::path(wpath).u8string();  // UTF-16 → UTF-8
ctx->reader.SetFileName(utf8path.c_str());          // GDCM 使用 UTF-8
```

**优点**：
- 实际可用，能够正确处理中文路径
- 不依赖编译器的 UTF-8 支持
- 与现有 GBK 编译的依赖库兼容

**缺点**：
- 需要手动管理编码转换
- 代码比较复杂
- 性能有轻微损失（多次转换）

**结论**：目前最可靠的方案，已恢复使用

## C++ std::filesystem 在 Windows 上的问题

### UTF-8 支持缺陷

1. **fs::u8path() 的问题**
   - C++17 引入，C++20 中已被弃用（deprecated）
   - MSVC 实现不完善，无法与 directory_iterator、exists 等函数配合使用
   - 创建的路径对象在某些操作中会失败

2. **推荐的 C++20 方案**（需要编译器支持）
   ```cpp
   std::u8string utf8_path = u8"D:\\Scripts\\Example\\股骨\\0.372";
   fs::path p(utf8_path);  // C++20 中直接支持
   ```
   但需要 MSVC 2019 16.7+ 且启用 `/std:c++20`

3. **Windows 原生 API 方案**
   - 使用宽字符 API（`_wfopen`、`CreateFileW` 等）
   - 所有路径使用 `wchar_t*` 类型
   - 需要大量重构现有代码

## 后续可能的改进方向

### 短期方案
- ✅ 继续使用 UTF-8 → GBK → UTF-8 转换方案
- 在关键位置添加编码转换函数
- 确保所有路径传递都经过正确的转换

### 中期方案
- 升级到 C++20
- 使用 `std::u8string` 和新的 `fs::path` 构造函数
- 需要验证 MSVC 编译器版本和依赖库兼容性

### 长期方案
- 考虑使用 Windows 宽字符 API
- 将所有路径处理改为 UTF-16（`std::wstring`）
- 可能需要重构大量代码

## 相关文件

### Native Addon
- `hiscan-analyzer/native/console-dll/src/dicom_wrapper.cpp`
  - `Utf8ToWide()` - UTF-8 转 UTF-16
  - `WideToAnsi()` - UTF-16 转 GBK
  - `Utf8ToAnsi()` - UTF-8 转 GBK（组合函数）
  - `LoadFromFolder()` - 使用转换后的 GBK 路径

### DLL 实现
- `ConsoleDllTest/DllDicom/DicomApi.cpp`
  - `Dicom_ReadFile()` - GBK → UTF-16 → UTF-8
  - `Dicom_ReadDirectory()` - 使用 GBK 路径进行文件遍历
  - `Dicom_Volume_LoadFromDicomSeries()` - 文件收集和路径转换

## 测试案例

### 成功的路径
- 纯英文路径：`D:\Projects\DicomData\Series1`
- 带空格路径：`D:\DICOM Files\Test Series`

### 失败的路径（未转换时）
- 中文路径：`D:\Scripts\Example\股骨\0.372`
- 日文路径：`D:\データ\テスト`

### 成功的转换流程
```
"D:\\Scripts\\Example\\股骨\\0.372" (UTF-8 in JavaScript)
  → Utf8ToAnsi() in Native Addon
  → "D:\\Scripts\\Example\\股骨\\0.372" (GBK in DLL)
  → fs::path().wstring() in DicomApi
  → L"D:\\Scripts\\Example\\股骨\\0.372" (UTF-16)
  → fs::path().u8string()
  → "D:\\Scripts\\Example\\股骨\\0.372" (UTF-8 for GDCM)
  → 成功加载
```

## 注意事项

1. **不要混用编译选项**
   - 不要在部分项目使用 `/utf-8`，部分使用默认 GBK
   - 链接时会出现符号不匹配

2. **字符串字面量**
   - 代码中的中文字符串需要注意源文件编码
   - 建议使用 UTF-8 with BOM 或 GB2312

3. **调试输出**
   - 使用 `std::cout` 输出中文可能乱码
   - 使用 `OutputDebugStringW()` 输出宽字符

4. **路径分隔符**
   - Windows 接受 `/` 和 `\\`
   - `fs::path` 会自动转换为平台分隔符

## 参考资料

- [Microsoft Docs - MultiByteToWideChar](https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar)
- [Microsoft Docs - WideCharToMultiByte](https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte)
- [C++20 filesystem UTF-8 support](https://en.cppreference.com/w/cpp/filesystem/path/path)
- [MSVC UTF-8 source files and executables](https://learn.microsoft.com/en-us/cpp/build/reference/utf-8-set-source-and-executable-character-sets-to-utf-8)

## 更新日志

- 2025-01-XX：初次记录，尝试 UTF-8 编译选项失败
- 2025-01-XX：尝试 fs::u8path 方案失败
- 2025-01-XX：恢复 UTF-8 → GBK 转换方案，测试成功
