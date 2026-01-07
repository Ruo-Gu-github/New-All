@echo off
chcp 65001 >nul
echo ========================================
echo   编译 DICOM 缩略图测试程序
echo ========================================
echo.

REM 设置 Visual Studio 环境
if not defined VSCMD_VER (
    echo 正在设置 Visual Studio 环境...
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    if errorlevel 1 (
        echo 错误: 无法设置 Visual Studio 环境
        pause
        exit /b 1
    )
)

echo 【步骤 1】编译测试程序...
cl /EHsc /std:c++17 /I. /Fe:test_dicom_thumbnail.exe test_dicom_thumbnail.cpp shell32.lib
if errorlevel 1 (
    echo 编译失败！
    pause
    exit /b 1
)
echo ✓ 编译成功
echo.

echo 【步骤 2】复制依赖的 DLL...
if exist "DllDicom\x64\Release\DllDicom.dll" (
    copy /Y "DllDicom\x64\Release\DllDicom.dll" .
    echo ✓ 已复制 DllDicom.dll
) else if exist "DllDicom\x64\Debug\DllDicom.dll" (
    copy /Y "DllDicom\x64\Debug\DllDicom.dll" .
    echo ✓ 已复制 DllDicom.dll (Debug)
) else if exist "x64\Release\DllDicom.dll" (
    copy /Y "x64\Release\DllDicom.dll" .
    echo ✓ 已复制 DllDicom.dll
) else if exist "x64\Debug\DllDicom.dll" (
    copy /Y "x64\Debug\DllDicom.dll" .
    echo ✓ 已复制 DllDicom.dll (Debug)
) else (
    echo 警告: 找不到 DllDicom.dll，请确保已编译 DllDicom 项目
)
echo.

echo ========================================
echo   编译完成！
echo ========================================
echo.
echo 用法示例:
echo   test_dicom_thumbnail.exe "path\to\your\file.dcm"
echo   test_dicom_thumbnail.exe "path\to\your\file.dcm" 512 512
echo.
pause
