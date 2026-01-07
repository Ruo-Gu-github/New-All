# 精准更新.vcxproj项目文件，添加UTF-8编译选项
# 针对Visual Studio 2022项目结构优化

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "精准更新项目文件的UTF-8编译配置" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$rootPath = "d:\2025-09-25 新系列\ConsoleDllTest"
$vcxprojFiles = Get-ChildItem -Path $rootPath -Filter "*.vcxproj" -Recurse -File | Where-Object {
    $_.FullName -notlike "*\x64\*" -and 
    $_.FullName -notlike "*\Debug\*" -and 
    $_.FullName -notlike "*\Release\*"
}

$totalProjects = $vcxprojFiles.Count
$updatedProjects = 0
$skippedProjects = 0

Write-Host "找到 $totalProjects 个项目文件" -ForegroundColor Yellow
Write-Host ""

foreach ($vcxproj in $vcxprojFiles) {
    try {
        $projectName = $vcxproj.BaseName
        Write-Host "处理项目: $projectName" -ForegroundColor Cyan
        
        # 读取为XML
        [xml]$xmlDoc = Get-Content -Path $vcxproj.FullName -Encoding UTF8
        
        $modified = $false
        $namespace = $xmlDoc.DocumentElement.NamespaceURI
        $nsManager = New-Object System.Xml.XmlNamespaceManager($xmlDoc.NameTable)
        $nsManager.AddNamespace("ms", $namespace)
        
        # 查找所有 ItemDefinitionGroup 节点
        $itemDefGroups = $xmlDoc.SelectNodes("//ms:ItemDefinitionGroup", $nsManager)
        
        foreach ($group in $itemDefGroups) {
            $clCompile = $group.SelectSingleNode("ms:ClCompile", $nsManager)
            
            if ($clCompile -ne $null) {
                # 检查是否已有 AdditionalOptions
                $additionalOptions = $clCompile.SelectSingleNode("ms:AdditionalOptions", $nsManager)
                
                if ($additionalOptions -eq $null) {
                    # 创建新的 AdditionalOptions 节点
                    $additionalOptions = $xmlDoc.CreateElement("AdditionalOptions", $namespace)
                    $additionalOptions.InnerText = "/utf-8 %(AdditionalOptions)"
                    $clCompile.AppendChild($additionalOptions) | Out-Null
                    $modified = $true
                    Write-Host "  [+] 添加 UTF-8 选项到配置: $($group.Condition)" -ForegroundColor Green
                }
                elseif ($additionalOptions.InnerText -notmatch "/utf-8") {
                    # 已有选项，但没有 /utf-8，添加之
                    $additionalOptions.InnerText = "/utf-8 " + $additionalOptions.InnerText
                    $modified = $true
                    Write-Host "  [+] 追加 UTF-8 选项到配置: $($group.Condition)" -ForegroundColor Green
                }
                else {
                    Write-Host "  [√] 已包含 UTF-8 选项: $($group.Condition)" -ForegroundColor DarkGray
                }
            }
        }
        
        if ($modified) {
            # 保存修改
            $settings = New-Object System.Xml.XmlWriterSettings
            $settings.Indent = $true
            $settings.IndentChars = "  "
            $settings.NewLineChars = "`r`n"
            $settings.Encoding = [System.Text.Encoding]::UTF8
            
            $writer = [System.Xml.XmlWriter]::Create($vcxproj.FullName, $settings)
            $xmlDoc.Save($writer)
            $writer.Close()
            
            Write-Host "[✓ 已更新] $projectName" -ForegroundColor Green
            $updatedProjects++
        }
        else {
            Write-Host "[已配置] $projectName" -ForegroundColor Yellow
            $skippedProjects++
        }
        
        Write-Host ""
    }
    catch {
        Write-Host "[✗ 错误] $($vcxproj.BaseName) - $($_.Exception.Message)" -ForegroundColor Red
        Write-Host ""
    }
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "项目配置更新完成！" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "总项目数: $totalProjects" -ForegroundColor White
Write-Host "已更新: $updatedProjects" -ForegroundColor Green
Write-Host "已配置: $skippedProjects" -ForegroundColor Yellow
Write-Host ""

if ($updatedProjects -gt 0) {
    Write-Host "✓ 配置已更新！" -ForegroundColor Green
    Write-Host ""
    Write-Host "下一步操作：" -ForegroundColor Yellow
    Write-Host "1. 在Visual Studio中重新加载解决方案" -ForegroundColor White
    Write-Host "2. 或运行编译脚本: .\build_all_dlls.bat" -ForegroundColor White
}
