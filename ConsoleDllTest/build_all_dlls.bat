@echo off
REM 编译所有DLL项目

cd /d "%~dp0"

echo ========================================
echo 开始编译所有DLL项目
echo ========================================

set "MSBUILD="
for /f "usebackq delims=" %%i in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0find_msbuild.ps1"`) do set "MSBUILD=%%i"
if "%MSBUILD%"=="" (
    echo [错误] 找不到 MSBuild.exe。
    echo        可设置环境变量 MSBUILD_EXE=MSBuild.exe 的完整路径。
    pause
    exit /b 1
)
if not exist "%MSBUILD%" (
    echo [错误] MSBuild.exe 路径无效: %MSBUILD%
    pause
    exit /b 1
)
echo 使用 MSBuild: %MSBUILD%

set "SOLUTION_DIR=%~dp0"
set "SOLUTION_DIR_RAW=%SOLUTION_DIR%"
if "%SOLUTION_DIR_RAW:~-1%"=="\" set "SOLUTION_DIR_RAW=%SOLUTION_DIR_RAW:~0,-1%"
set "SOLUTION_DIR_ARG=%SOLUTION_DIR_RAW%\\"
echo 使用 SolutionDir: %SOLUTION_DIR_ARG%

echo.
echo [1/8] 编译 DllCore...
"%MSBUILD%" DllCore\DllCore.vcxproj /p:Configuration=Release /p:Platform=x64 /p:SolutionDir="%SOLUTION_DIR_ARG%" /t:Rebuild /v:minimal
if %ERRORLEVEL% neq 0 (
    echo [错误] DllCore 编译失败！
    pause
    exit /b 1
)

echo.
echo [2/8] 编译 DllDicom...
"%MSBUILD%" DllDicom\DllDicom.vcxproj /p:Configuration=Release /p:Platform=x64 /p:SolutionDir="%SOLUTION_DIR_ARG%" /t:Rebuild /v:minimal
if %ERRORLEVEL% neq 0 (
    echo [错误] DllDicom 编译失败！
    pause
    exit /b 1
)

echo.
echo [3/8] 编译 DllBoneAnalysis...
"%MSBUILD%" DllBoneAnalysis\DllBoneAnalysis.vcxproj /p:Configuration=Release /p:Platform=x64 /p:SolutionDir="%SOLUTION_DIR_ARG%" /t:Rebuild /v:minimal
if %ERRORLEVEL% neq 0 (
    echo [错误] DllBoneAnalysis 编译失败！
    pause
    exit /b 1
)

echo.
echo [4/8] 编译 DllFatAnalysis...
"%MSBUILD%" DllFatAnalysis\DllFatAnalysis.vcxproj /p:Configuration=Release /p:Platform=x64 /p:SolutionDir="%SOLUTION_DIR_ARG%" /t:Rebuild /v:minimal
if %ERRORLEVEL% neq 0 (
    echo [错误] DllFatAnalysis 编译失败！
    pause
    exit /b 1
)

echo.
echo [5/8] 编译 DllLungAnalysis...
"%MSBUILD%" DllLungAnalysis\DllLungAnalysis.vcxproj /p:Configuration=Release /p:Platform=x64 /p:SolutionDir="%SOLUTION_DIR_ARG%" /t:Rebuild /v:minimal
if %ERRORLEVEL% neq 0 (
    echo [错误] DllLungAnalysis 编译失败！
    pause
    exit /b 1
)

echo.
echo [6/8] 编译 DllAnalysisBase...
"%MSBUILD%" DllAnalysisBase\DllAnalysisBase.vcxproj /p:Configuration=Release /p:Platform=x64 /p:SolutionDir="%SOLUTION_DIR_ARG%" /t:Rebuild /v:minimal
if %ERRORLEVEL% neq 0 (
    echo [错误] DllAnalysisBase 编译失败！
    pause
    exit /b 1
)

echo.
echo [7/8] 编译 DllImageProcessing...
"%MSBUILD%" DllImageProcessing\DllImageProcessing.vcxproj /p:Configuration=Release /p:Platform=x64 /p:SolutionDir="%SOLUTION_DIR_ARG%" /t:Rebuild /v:minimal
if %ERRORLEVEL% neq 0 (
    echo [错误] DllImageProcessing 编译失败！
    pause
    exit /b 1
)

echo.
echo [8/8] 编译 DllVisualization...
"%MSBUILD%" DllVisualization\DllVisualization.vcxproj /p:Configuration=Release /p:Platform=x64 /p:SolutionDir="%SOLUTION_DIR_ARG%" /t:Rebuild /v:minimal
if %ERRORLEVEL% neq 0 (
    echo [错误] DllVisualization 编译失败！
    pause
    exit /b 1
)

echo.
echo ========================================
echo 编译完成！
echo ========================================
echo.
echo 检查编译结果：
dir Dlls\bin\*.dll /b

echo.
echo ========================================
echo 同步DLL到hiscan-analyzer项目
echo ========================================
echo.

REM 调用 sync_dlls.bat 同步所有DLL
if exist "%~dp0sync_dlls.bat" (
    set "NO_PAUSE=1"
    call "%~dp0sync_dlls.bat"
    if %ERRORLEVEL% NEQ 0 (
        echo [错误] DLL同步失败
        pause
        exit /b 1
    )
) else (
    echo [警告] 找不到 sync_dlls.bat，跳过DLL同步
)

echo.
echo ========================================
echo 所有操作完成！
echo ========================================

if "%NO_PAUSE%"=="1" exit /b 0
pause
