# PowerShell脚本：将项目中所有源文件转换为UTF-8 with BOM编码
# 并修改所有.vcxproj文件添加UTF-8编译选项

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "开始转换项目文件为UTF-8编码..." -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# 获取脚本所在目录（项目根目录）
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

# 统计信息
$convertedFiles = 0
$skippedFiles = 0
$errorFiles = 0

# 第一步：转换所有C++源文件为UTF-8 with BOM
Write-Host "`n[步骤 1/2] 转换源文件编码..." -ForegroundColor Yellow

# 需要转换的文件扩展名
$extensions = @("*.cpp", "*.h", "*.c", "*.hpp", "*.cc", "*.cxx")

foreach ($ext in $extensions) {
    $files = Get-ChildItem -Path $projectRoot -Filter $ext -Recurse -File | Where-Object {
        # 排除vcpkg_installed目录和x64目录
        $_.FullName -notmatch "\\vcpkg_installed\\" -and 
        $_.FullName -notmatch "\\x64\\"
    }
    
    foreach ($file in $files) {
        try {
            # 读取文件内容（自动检测编码）
            $content = Get-Content -Path $file.FullName -Raw -Encoding Default
            
            if ($null -ne $content) {
                # 写入为UTF-8 with BOM
                $utf8Bom = New-Object System.Text.UTF8Encoding $true
                [System.IO.File]::WriteAllText($file.FullName, $content, $utf8Bom)
                
                Write-Host "  ✓ $($file.FullName.Replace($projectRoot, '.'))" -ForegroundColor Green
                $convertedFiles++
            } else {
                Write-Host "  ⊘ $($file.FullName.Replace($projectRoot, '.')) (空文件)" -ForegroundColor Gray
                $skippedFiles++
            }
        }
        catch {
            Write-Host "  ✗ $($file.FullName.Replace($projectRoot, '.')) - 错误: $($_.Exception.Message)" -ForegroundColor Red
            $errorFiles++
        }
    }
}

# 第二步：修改所有.vcxproj文件添加UTF-8编译选项
Write-Host "`n[步骤 2/2] 修改项目文件添加UTF-8编译选项..." -ForegroundColor Yellow

$vcxprojFiles = Get-ChildItem -Path $projectRoot -Filter "*.vcxproj" -Recurse -File | Where-Object {
    $_.FullName -notmatch "\\vcpkg_installed\\" -and 
    $_.FullName -notmatch "\\x64\\"
}

foreach ($vcxproj in $vcxprojFiles) {
    try {
        # 读取项目文件内容
        [xml]$xmlContent = Get-Content -Path $vcxproj.FullName -Encoding UTF8
        
        $modified = $false
        $namespace = "http://schemas.microsoft.com/developer/msbuild/2003"
        $nsmgr = New-Object System.Xml.XmlNamespaceManager($xmlContent.NameTable)
        $nsmgr.AddNamespace("ms", $namespace)
        
        # 查找所有 ItemDefinitionGroup/ClCompile 节点
        $clCompileNodes = $xmlContent.SelectNodes("//ms:ItemDefinitionGroup/ms:ClCompile", $nsmgr)
        
        foreach ($clCompile in $clCompileNodes) {
            # 检查是否已经有 AdditionalOptions
            $additionalOptions = $clCompile.SelectSingleNode("ms:AdditionalOptions", $nsmgr)
            
            if ($null -eq $additionalOptions) {
                # 创建新的 AdditionalOptions 节点
                $additionalOptions = $xmlContent.CreateElement("AdditionalOptions", $namespace)
                $additionalOptions.InnerText = "/utf-8 %(AdditionalOptions)"
                $clCompile.AppendChild($additionalOptions) | Out-Null
                $modified = $true
            }
            elseif ($additionalOptions.InnerText -notmatch "/utf-8") {
                # 如果已存在但没有/utf-8，则添加
                if ($additionalOptions.InnerText -match "%(AdditionalOptions)") {
                    $additionalOptions.InnerText = "/utf-8 " + $additionalOptions.InnerText
                }
                else {
                    $additionalOptions.InnerText = "/utf-8 " + $additionalOptions.InnerText + " %(AdditionalOptions)"
                }
                $modified = $true
            }
        }
        
        if ($modified) {
            # 保存修改后的文件
            $xmlContent.Save($vcxproj.FullName)
            Write-Host "  ✓ $($vcxproj.FullName.Replace($projectRoot, '.'))" -ForegroundColor Green
        }
        else {
            Write-Host "  ⊘ $($vcxproj.FullName.Replace($projectRoot, '.')) (已有UTF-8选项)" -ForegroundColor Gray
        }
    }
    catch {
        Write-Host "  ✗ $($vcxproj.FullName.Replace($projectRoot, '.')) - 错误: $($_.Exception.Message)" -ForegroundColor Red
        $errorFiles++
    }
}

# 输出统计信息
Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "转换完成！" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "成功转换: $convertedFiles 个文件" -ForegroundColor Green
Write-Host "跳过文件: $skippedFiles 个文件" -ForegroundColor Gray
Write-Host "失败文件: $errorFiles 个文件" -ForegroundColor $(if ($errorFiles -gt 0) { "Red" } else { "Gray" })
Write-Host "`n提示：" -ForegroundColor Yellow
Write-Host "1. 所有源文件已转换为UTF-8 with BOM编码" -ForegroundColor White
Write-Host "2. 所有.vcxproj项目文件已添加 /utf-8 编译选项" -ForegroundColor White
Write-Host "3. 建议在Visual Studio中重新生成解决方案以确保更改生效" -ForegroundColor White
Write-Host "`n按任意键退出..." -ForegroundColor Gray
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
