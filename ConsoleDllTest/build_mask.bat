@echo off
REM Mask功能测试编译脚本

echo ===================================
echo 编译 DllImageProcessing (Mask模块)
echo ===================================

cd /d "%~dp0"

set "MSBUILD="
for /f "usebackq delims=" %%i in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0find_msbuild.ps1"`) do set "MSBUILD=%%i"
if "%MSBUILD%"=="" (
    echo.
    echo [错误] 找不到 MSBuild.exe。
    echo        可设置环境变量 MSBUILD_EXE=MSBuild.exe 的完整路径。
    pause
    exit /b 1
)
if not exist "%MSBUILD%" (
    echo.
    echo [错误] MSBuild.exe 路径无效: %MSBUILD%
    pause
    exit /b 1
)
echo 使用 MSBuild: %MSBUILD%

set "SOLUTION_DIR=%~dp0"
set "SOLUTION_DIR_RAW=%SOLUTION_DIR%"
if "%SOLUTION_DIR_RAW:~-1%"=="\" set "SOLUTION_DIR_RAW=%SOLUTION_DIR_RAW:~0,-1%"
set "SOLUTION_DIR_ARG=%SOLUTION_DIR_RAW%\\"

REM 清理并编译DllImageProcessing
"%MSBUILD%" DllImageProcessing\DllImageProcessing.vcxproj /p:Configuration=Release /p:Platform=x64 /p:SolutionDir="%SOLUTION_DIR_ARG%" /t:Clean,Build /m

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [错误] DllImageProcessing 编译失败!
    pause
    exit /b 1
)

echo.
echo [成功] DllImageProcessing 编译完成
echo.

REM 编译主测试程序
echo ===================================
echo 编译 ConsoleDllTest (主程序)
echo ===================================

"%MSBUILD%" ConsoleDllTest\ConsoleDllTest.vcxproj /p:Configuration=Release /p:Platform=x64 /p:SolutionDir="%SOLUTION_DIR_ARG%" /t:Clean,Build /m

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [错误] ConsoleDllTest 编译失败!
    pause
    exit /b 1
)

echo.
echo [成功] ConsoleDllTest 编译完成
echo.
echo ===================================
echo 所有模块编译成功!
echo ===================================
echo.
echo 运行测试:
echo   1. 进入 Dlls\bin 目录
echo   2. 运行 ConsoleDllTest.exe
echo   3. 输入命令: mask-mpr
echo.
pause
