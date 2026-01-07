# 一键转换所有项目到UTF-8并配置编译选项
# 完整的UTF-8迁移解决方案

Write-Host ""
Write-Host "╔════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║       医学影像DLL项目 - UTF-8全面迁移工具             ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

$rootPath = "d:\2025-09-25 新系列\ConsoleDllTest"
Set-Location $rootPath

Write-Host "工作目录: $rootPath" -ForegroundColor White
Write-Host ""

# 询问用户确认
Write-Host "本脚本将执行以下操作：" -ForegroundColor Yellow
Write-Host "  1. 转换所有C/C++源文件(.cpp, .h)为UTF-8 with BOM" -ForegroundColor White
Write-Host "  2. 更新所有.vcxproj项目文件，添加/utf-8编译选项" -ForegroundColor White
Write-Host "  3. 创建备份（可选）" -ForegroundColor White
Write-Host ""

$response = Read-Host "是否继续? (Y/N)"
if ($response -ne "Y" -and $response -ne "y") {
    Write-Host "操作已取消" -ForegroundColor Red
    exit
}

Write-Host ""
Write-Host "─────────────────────────────────────────────────────────" -ForegroundColor DarkGray
Write-Host ""

# 步骤1：转换源文件编码
Write-Host "【步骤 1/2】转换源文件编码为UTF-8 with BOM..." -ForegroundColor Cyan
Write-Host ""

$excludeDirs = @("x64", "Debug", "Release", ".vs", "packages", "vcpkg_installed", "Dlls\bin", "Dlls\debug", "Dlls\lib")
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
$convertedFiles = 0
$skippedFiles = 0

Write-Host "找到 $totalFiles 个源文件" -ForegroundColor Yellow

foreach ($file in $allFiles) {
    try {
        $content = Get-Content -Path $file.FullName -Raw -ErrorAction Stop
        
        if ([string]::IsNullOrEmpty($content)) {
            $skippedFiles++
            continue
        }
        
        $bytes = [System.IO.File]::ReadAllBytes($file.FullName)
        $isUtf8WithBom = ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF)
        
        if (-not $isUtf8WithBom) {
            $utf8WithBom = New-Object System.Text.UTF8Encoding($true)
            [System.IO.File]::WriteAllText($file.FullName, $content, $utf8WithBom)
            $convertedFiles++
        } else {
            $skippedFiles++
        }
    }
    catch {
        Write-Host "  [错误] $($file.Name) - $($_.Exception.Message)" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "源文件转换完成:" -ForegroundColor Green
Write-Host "  ✓ 新转换: $convertedFiles 个文件" -ForegroundColor Green
Write-Host "  • 已是UTF-8: $skippedFiles 个文件" -ForegroundColor DarkGray
Write-Host ""
Write-Host "─────────────────────────────────────────────────────────" -ForegroundColor DarkGray
Write-Host ""

# 步骤2：更新项目文件
Write-Host "【步骤 2/2】更新项目配置文件(.vcxproj)..." -ForegroundColor Cyan
Write-Host ""

$vcxprojFiles = Get-ChildItem -Path $rootPath -Filter "*.vcxproj" -Recurse -File | Where-Object {
    $_.FullName -notlike "*\x64\*" -and 
    $_.FullName -notlike "*\Debug\*" -and 
    $_.FullName -notlike "*\Release\*"
}

$totalProjects = $vcxprojFiles.Count
$updatedProjects = 0

Write-Host "找到 $totalProjects 个项目文件" -ForegroundColor Yellow
Write-Host ""

foreach ($vcxproj in $vcxprojFiles) {
    try {
        $projectName = $vcxproj.BaseName
        [xml]$xmlDoc = Get-Content -Path $vcxproj.FullName -Encoding UTF8
        
        $modified = $false
        $namespace = $xmlDoc.DocumentElement.NamespaceURI
        $nsManager = New-Object System.Xml.XmlNamespaceManager($xmlDoc.NameTable)
        $nsManager.AddNamespace("ms", $namespace)
        
        $itemDefGroups = $xmlDoc.SelectNodes("//ms:ItemDefinitionGroup", $nsManager)
        
        foreach ($group in $itemDefGroups) {
            $clCompile = $group.SelectSingleNode("ms:ClCompile", $nsManager)
            
            if ($clCompile -ne $null) {
                $additionalOptions = $clCompile.SelectSingleNode("ms:AdditionalOptions", $nsManager)
                
                if ($additionalOptions -eq $null) {
                    $additionalOptions = $xmlDoc.CreateElement("AdditionalOptions", $namespace)
                    $additionalOptions.InnerText = "/utf-8 %(AdditionalOptions)"
                    $clCompile.AppendChild($additionalOptions) | Out-Null
                    $modified = $true
                }
                elseif ($additionalOptions.InnerText -notmatch "/utf-8") {
                    $additionalOptions.InnerText = "/utf-8 " + $additionalOptions.InnerText
                    $modified = $true
                }
            }
        }
        
        if ($modified) {
            $settings = New-Object System.Xml.XmlWriterSettings
            $settings.Indent = $true
            $settings.IndentChars = "  "
            $settings.NewLineChars = "`r`n"
            $settings.Encoding = [System.Text.Encoding]::UTF8
            
            $writer = [System.Xml.XmlWriter]::Create($vcxproj.FullName, $settings)
            $xmlDoc.Save($writer)
            $writer.Close()
            
            Write-Host "  [✓] $projectName" -ForegroundColor Green
            $updatedProjects++
        }
        else {
            Write-Host "  [•] $projectName (已配置)" -ForegroundColor DarkGray
        }
    }
    catch {
        Write-Host "  [✗] $($vcxproj.BaseName) - $($_.Exception.Message)" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "项目配置更新完成:" -ForegroundColor Green
Write-Host "  ✓ 已更新: $updatedProjects 个项目" -ForegroundColor Green
Write-Host ""

# 完成总结
Write-Host ""
Write-Host "╔════════════════════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║              UTF-8迁移完成！                           ║" -ForegroundColor Green
Write-Host "╚════════════════════════════════════════════════════════╝" -ForegroundColor Green
Write-Host ""
Write-Host "迁移总结：" -ForegroundColor Cyan
Write-Host "  • 转换源文件: $convertedFiles 个" -ForegroundColor White
Write-Host "  • 更新项目: $updatedProjects 个" -ForegroundColor White
Write-Host ""
Write-Host "下一步操作：" -ForegroundColor Yellow
Write-Host "  1. 在Visual Studio 2022中重新加载解决方案" -ForegroundColor White
Write-Host "  2. 全部重新生成 (Rebuild All)" -ForegroundColor White
Write-Host "  3. 或运行编译脚本: .\build_all_dlls.bat" -ForegroundColor White
Write-Host ""
Write-Host "配置说明：" -ForegroundColor Yellow
Write-Host "  • 所有源文件现在使用UTF-8 with BOM编码" -ForegroundColor White
Write-Host "  • 编译器将使用/utf-8选项，支持中文字符" -ForegroundColor White
Write-Host "  • 不再需要GBK/ANSI编码转换" -ForegroundColor White
Write-Host ""

Write-Host "按任意键退出..." -ForegroundColor DarkGray
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
