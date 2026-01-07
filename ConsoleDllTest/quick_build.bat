@echo off
echo 清理并重新编译 DllImageProcessing...
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

REM 清理旧文件
"%MSBUILD%" DllImageProcessing\DllImageProcessing.vcxproj /p:Configuration=Debug /p:Platform=x64 /p:SolutionDir="%SOLUTION_DIR_ARG%" /t:Clean /nologo /v:minimal

REM 编译DLL
echo 编译 DllImageProcessing...
"%MSBUILD%" DllImageProcessing\DllImageProcessing.vcxproj /p:Configuration=Debug /p:Platform=x64 /p:SolutionDir="%SOLUTION_DIR_ARG%" /t:Build /nologo /v:minimal /m

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [成功] DllImageProcessing编译完成
    echo 检查生成的文件:
    dir Dlls\debug\bin\DllImageProcessing.dll 2>nul
    dir Dlls\debug\lib\DllImageProcessing.lib 2>nul
    echo.
    echo 编译 ConsoleDllTest...
    "%MSBUILD%" ConsoleDllTest\ConsoleDllTest.vcxproj /p:Configuration=Debug /p:Platform=x64 /p:SolutionDir="%SOLUTION_DIR_ARG%" /t:Build /nologo /v:minimal /m
    if %ERRORLEVEL% EQU 0 (
        echo.
        echo [成功] 所有项目编译完成!
        echo.
        echo 运行测试: Dlls\debug\bin\ConsoleDllTest.exe
        echo 输入命令: mask-mpr
    ) else (
        echo.
        echo [失败] ConsoleDllTest编译失败
    )
) else (
    echo.
    echo [失败] DllImageProcessing编译失败
)
pause
