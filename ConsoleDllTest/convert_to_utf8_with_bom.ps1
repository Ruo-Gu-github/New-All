# UTF-8转换脚本 - 转换所有C++源文件为UTF-8 with BOM
# 适用于Visual Studio 2022

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "开始转换所有C++源文件为UTF-8 with BOM" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$rootPath = "d:\2025-09-25 新系列\ConsoleDllTest"
$excludeDirs = @("x64", "Debug", "Release", ".vs", "packages", "vcpkg_installed", "Dlls\bin", "Dlls\debug", "Dlls\lib")

# 统计变量
$totalFiles = 0
$convertedFiles = 0
$skippedFiles = 0
$errorFiles = 0

# 获取所有需要转换的文件
$filePatterns = @("*.cpp", "*.h", "*.hpp", "*.c")
$allFiles = @()

foreach ($pattern in $filePatterns) {
    $files = Get-ChildItem -Path $rootPath -Filter $pattern -Recurse -File | Where-Object {
        $relativePath = $_.FullName.Replace($rootPath, "")
        $shouldExclude = $false
        foreach ($excludeDir in $excludeDirs) {
            if ($relativePath -like "*\$excludeDir\*") {
                $shouldExclude = $true
                break
            }
        }
        -not $shouldExclude
    }
    $allFiles += $files
}

$totalFiles = $allFiles.Count
Write-Host "找到 $totalFiles 个需要检查的文件" -ForegroundColor Yellow
Write-Host ""

foreach ($file in $allFiles) {
    try {
        $relativePath = $file.FullName.Replace($rootPath + "\", "")
        
        # 读取文件内容（自动检测编码）
        $content = Get-Content -Path $file.FullName -Raw -ErrorAction Stop
        
        # 检查是否为空文件
        if ([string]::IsNullOrEmpty($content)) {
            Write-Host "[跳过] $relativePath (空文件)" -ForegroundColor DarkGray
            $skippedFiles++
            continue
        }
        
        # 检查当前编码
        $bytes = [System.IO.File]::ReadAllBytes($file.FullName)
        $isUtf8WithBom = ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF)
        
        if ($isUtf8WithBom) {
            Write-Host "[已是UTF-8] $relativePath" -ForegroundColor DarkGray
            $skippedFiles++
        } else {
            # 转换为UTF-8 with BOM
            $utf8WithBom = New-Object System.Text.UTF8Encoding($true)
            [System.IO.File]::WriteAllText($file.FullName, $content, $utf8WithBom)
            
            Write-Host "[✓ 转换] $relativePath" -ForegroundColor Green
            $convertedFiles++
        }
    }
    catch {
        Write-Host "[✗ 错误] $relativePath - $($_.Exception.Message)" -ForegroundColor Red
        $errorFiles++
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "转换完成！" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "总文件数: $totalFiles" -ForegroundColor White
Write-Host "已转换: $convertedFiles" -ForegroundColor Green
Write-Host "已是UTF-8: $skippedFiles" -ForegroundColor Yellow
Write-Host "错误: $errorFiles" -ForegroundColor Red
Write-Host ""

if ($convertedFiles -gt 0) {
    Write-Host "重要提示：" -ForegroundColor Yellow
    Write-Host "1. 文件编码已转换为UTF-8 with BOM" -ForegroundColor White
    Write-Host "2. 现在需要更新项目配置文件(.vcxproj)" -ForegroundColor White
    Write-Host "3. 运行 update_vcxproj_utf8.ps1 脚本来更新项目配置" -ForegroundColor White
}
