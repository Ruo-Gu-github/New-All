@echo off
chcp 65001
echo 正在撤销 GBK 转换，恢复为 UTF-8...
powershell -ExecutionPolicy Bypass -Command "Get-ChildItem -Path . -Include *.h,*.cpp -Recurse | Where-Object { $_.DirectoryName -notmatch 'vcpkg_installed|x64|Debug|Release' } | ForEach-Object { try { $content = Get-Content $_.FullName -Raw -Encoding Default; if ($content) { $utf8 = New-Object System.Text.UTF8Encoding $false; [System.IO.File]::WriteAllText($_.FullName, $content, $utf8); Write-Host \"Converted: $($_.Name)\" -ForegroundColor Green } } catch { Write-Host \"Failed: $($_.Name)\" -ForegroundColor Red } }"
echo.
echo 恢复完成！
pause
