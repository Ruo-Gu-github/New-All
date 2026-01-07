# 更新所有.vcxproj项目文件，添加UTF-8编译选项
# 适用于Visual Studio 2022

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "更新所有项目文件的UTF-8编译配置" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$rootPath = "d:\2025-09-25 新系列\ConsoleDllTest"
$vcxprojFiles = Get-ChildItem -Path $rootPath -Filter "*.vcxproj" -Recurse -File

$totalProjects = $vcxprojFiles.Count
$updatedProjects = 0
$skippedProjects = 0

Write-Host "找到 $totalProjects 个项目文件" -ForegroundColor Yellow
Write-Host ""

foreach ($vcxproj in $vcxprojFiles) {
    try {
        $projectName = $vcxproj.BaseName
        $content = Get-Content -Path $vcxproj.FullName -Raw -Encoding UTF8
        
        $modified = $false
        
        # 检查是否已经包含UTF-8配置
        if ($content -notmatch "/utf-8") {
            # 为每个配置添加UTF-8编译选项
            # 查找 <ClCompile> 部分并添加 <AdditionalOptions>
            
            # 匹配所有 ItemDefinitionGroup 中的 ClCompile 节点
            $pattern = '(<ClCompile>(?:(?!<\/ClCompile>).)*)'
            
            if ($content -match $pattern) {
                # 在每个 ClCompile 节点中添加 UTF-8 选项
                $newContent = $content -replace '(<ClCompile>)', ('$1' + "`r`n      <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>")
                
                # 避免重复添加
                $finalContent = $newContent -replace '(<AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>\s*){2,}', '$1'
                
                if ($finalContent -ne $content) {
                    [System.IO.File]::WriteAllText($vcxproj.FullName, $finalContent, [System.Text.Encoding]::UTF8)
                    $modified = $true
                }
            }
        }
        
        if ($modified) {
            Write-Host "[✓ 更新] $projectName" -ForegroundColor Green
            $updatedProjects++
        } else {
            Write-Host "[已配置] $projectName" -ForegroundColor DarkGray
            $skippedProjects++
        }
    }
    catch {
        Write-Host "[✗ 错误] $($vcxproj.BaseName) - $($_.Exception.Message)" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "项目配置更新完成！" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "总项目数: $totalProjects" -ForegroundColor White
Write-Host "已更新: $updatedProjects" -ForegroundColor Green
Write-Host "已配置: $skippedProjects" -ForegroundColor Yellow
Write-Host ""

if ($updatedProjects -gt 0) {
    Write-Host "配置已更新，现在可以重新编译项目" -ForegroundColor Green
    Write-Host "运行: .\build_all_dlls.bat" -ForegroundColor White
}
