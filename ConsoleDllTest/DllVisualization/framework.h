#pragma once

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
#define NOMINMAX                        // 防止 Windows.h 定义 min/max 宏，避免与 std::min/max 冲突
// Windows 头文件
#include <windows.h>
