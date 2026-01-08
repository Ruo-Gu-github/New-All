@echo off
setlocal enabledelayedexpansion

echo ========================================
echo 自动编译 DLL 和 Node Addon
echo ========================================

set "PROJECT_ROOT=%~dp0"
cd /d "%PROJECT_ROOT%"

echo.
echo [1/2] 编译 C++ DLL...
echo ----------------------------------------
cd ConsoleDllTest
call build_all_dlls.bat
if errorlevel 1 (
    echo [ERROR] DLL 编译失败！
    pause
    exit /b 1
)

echo.
echo [2/2] 编译 Node Addon...
echo ----------------------------------------
cd "%PROJECT_ROOT%\hiscan-analyzer\native\console-dll"
call npm run build
if errorlevel 1 (
    echo [ERROR] Node Addon 编译失败！
    pause
    exit /b 1
)

echo.
echo ========================================
echo 编译完成！
echo ========================================
echo.

cd "%PROJECT_ROOT%"
