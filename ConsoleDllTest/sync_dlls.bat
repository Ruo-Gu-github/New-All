@echo off
chcp 65001 > nul
REM 迁移所有编译好的DLL到hiscan-analyzer项目
REM 用法: sync_dlls.bat

cd /d "%~dp0"

echo ========================================
echo 开始迁移DLL文件
echo ========================================
echo.

set "SOURCE_DIR=%~dp0Dlls\bin"
set "TARGET_DIR=%~dp0..\hiscan-analyzer\native"
set "TARGET_DIR2=%~dp0..\hiscan-analyzer\native\console-dll\build\Release"

REM 检查源目录是否存在
if not exist "%SOURCE_DIR%" (
    echo [错误] 源目录不存在: %SOURCE_DIR%
    echo        请先运行 build_all_dlls.bat 编译DLL
    pause
    exit /b 1
)

REM 创建目标目录（如果不存在）
if not exist "%TARGET_DIR%" (
    echo [创建] 目标目录: %TARGET_DIR%
    mkdir "%TARGET_DIR%"
)
if not exist "%TARGET_DIR2%" (
    echo [创建] 目标目录2: %TARGET_DIR2%
    mkdir "%TARGET_DIR2%"
)

echo 源目录: %SOURCE_DIR%
echo 目标目录1: %TARGET_DIR%
echo 目标目录2: %TARGET_DIR2%
echo.

set "SYNC_FAILED=0"

REM 定义要复制的DLL列表（项目DLL）
set "PROJECT_DLLS=DllCore DllDicom DllBoneAnalysis DllFatAnalysis DllLungAnalysis DllAnalysisBase DllImageProcessing DllVisualization"

REM 复制项目DLL
echo [1] 复制项目DLL到TARGET_DIR...
for %%D in (%PROJECT_DLLS%) do (
    if exist "%SOURCE_DIR%\%%D.dll" (
        copy /Y "%SOURCE_DIR%\%%D.dll" "%TARGET_DIR%\" > nul
        if errorlevel 1 (
            echo   X %%D.dll [失败]
            if /I "%%D"=="DllVisualization" set "SYNC_FAILED=1"
        ) else (
            echo   OK %%D.dll
        )
    ) else (
        echo   - %%D.dll [未找到]
        if /I "%%D"=="DllVisualization" set "SYNC_FAILED=1"
    )
)

echo.
echo [1.5] 复制DllVisualization到TARGET_DIR2（Node.js加载路径）...
if exist "%SOURCE_DIR%\DllVisualization.dll" (
    copy /Y "%SOURCE_DIR%\DllVisualization.dll" "%TARGET_DIR2%\" > nul
    if errorlevel 1 (
        echo   X DllVisualization.dll [失败]
        set "SYNC_FAILED=1"
    ) else (
        echo   OK DllVisualization.dll
    )
) else (
    echo   - DllVisualization.dll [未找到]
    set "SYNC_FAILED=1"
)

echo.
echo [2] 复制依赖DLL...

REM 定义依赖DLL列表
set "DEP_DLLS=glew32 glfw3 fmt zlib1 libexpat openjp2 socketxx"
set "GDCM_DLLS=gdcmcharls gdcmCommon gdcmDICT gdcmDSED gdcmgetopt gdcmIOD gdcmjpeg8 gdcmjpeg12 gdcmjpeg16 gdcmMEXD gdcmMSFF"

REM 复制常规依赖
for %%D in (%DEP_DLLS%) do (
    if exist "%SOURCE_DIR%\%%D.dll" (
        copy /Y "%SOURCE_DIR%\%%D.dll" "%TARGET_DIR%\" > nul
        if errorlevel 1 (
            echo   X %%D.dll [失败]
        ) else (
            echo   OK %%D.dll
        )
    )
)

REM 复制GDCM相关DLL
for %%D in (%GDCM_DLLS%) do (
    if exist "%SOURCE_DIR%\%%D.dll" (
        copy /Y "%SOURCE_DIR%\%%D.dll" "%TARGET_DIR%\" > nul
        if errorlevel 1 (
            echo   X %%D.dll [失败]
        ) else (
            echo   OK %%D.dll
        )
    )
)

echo.
echo ========================================
echo 迁移完成！
echo ========================================
echo.

if "%SYNC_FAILED%"=="1" (
    echo [错误] DLL迁移失败：请确认程序已关闭（DLL未被占用）且源文件存在。
    if "%NO_PAUSE%"=="1" exit /b 1
    pause
    exit /b 1
)

REM 显示统计信息
echo 目标目录中的DLL文件：
dir /b "%TARGET_DIR%\*.dll" 2>nul | find /c /v "" 

if "%NO_PAUSE%"=="1" exit /b 0
pause
