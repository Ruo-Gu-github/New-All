$file = "D:\2025-09-25 新系列\ConsoleDllTest\DllVisualization\ImageBrowserOrthogonal3DRenderer.cpp"
$content = [System.IO.File]::ReadAllText($file)

Write-Host "Original file length: $($content.Length)"

# 1. Remove the debug stage block (from "// ====== 分阶段矩阵排查" to the closing brace before "// ========== 3D正交渲染")
$startMarker = "// ====== 分阶段矩阵排查"
$endMarker = "// ========== 3D正交渲染"
$startIdx = $content.IndexOf($startMarker)
$endIdx = $content.IndexOf($endMarker)

if ($startIdx -gt 0 -and $endIdx -gt $startIdx) {
    $content = $content.Substring(0, $startIdx) + $content.Substring($endIdx)
    Write-Host "Removed debug stage block from position $startIdx to $endIdx"
}

# 2. Remove all fopen_s(...3d_render_debug.log...) blocks
# Pattern: fopen_s(&logFile, "D:\\3d_render_debug.log", "a");\r?\n\s*if \(logFile\) \{\r?\n[\s\S]*?fclose\(logFile\);\r?\n\s*\}
$pattern = 'fopen_s\(&logFile, "D:\\\\3d_render_debug\.log", "a"\);\s*\r?\n\s*if \(logFile\) \{\s*\r?\n[\s\S]*?fclose\(logFile\);\s*\r?\n\s*\}'
$content = [regex]::Replace($content, $pattern, '')
Write-Host "Removed fopen_s debug blocks"

# Also the single-statement version
$pattern2 = 'FILE\* logFile = nullptr;\s*\r?\n\s*fopen_s\(&logFile, "D:\\\\3d_render_debug\.log", "a"\);\s*\r?\n\s*if \(logFile\) \{\s*\r?\n[\s\S]*?fclose\(logFile\);\s*\r?\n\s*\}'
$content = [regex]::Replace($content, $pattern2, '')

# 3. Remove printf("[3D]...") statements
$content = [regex]::Replace($content, 'printf\("\[3D\][^"]*"[^)]*\);\s*\r?\n', '')
Write-Host "Removed printf statements"

# 4. Remove standalone FILE* logFile declarations if any remain
$content = [regex]::Replace($content, '\s*FILE\* logFile = nullptr;\s*\r?\n', "`n")

# 5. Clean up the Render function entry - replace with clean version
$oldEntry = @'
) {
    // 写日志到文件，确保能看到
    FILE* logFile = nullptr;
'@
$newEntry = @'
) {
'@
$content = $content.Replace($oldEntry, $newEntry)

# 6. Remove the #if 0 block containing old rendering code
$if0Start = "// ========== 以下是原来的复杂渲染代码，暂时跳过 =========="
$if0End = "#endif  // #if 0 关闭复杂渲染"
$if0StartIdx = $content.IndexOf($if0Start)
$if0EndIdx = $content.IndexOf($if0End)

if ($if0StartIdx -gt 0 -and $if0EndIdx -gt $if0StartIdx) {
    $endOfLine = $content.IndexOf("`n", $if0EndIdx)
    if ($endOfLine -gt 0) {
        $content = $content.Substring(0, $if0StartIdx) + $content.Substring($endOfLine + 1)
        Write-Host "Removed #if 0 block"
    }
}

# 7. Remove final debug log after the #if 0 end
$finalDebug = @'
    fopen_s(&logFile, "D:\\3d_render_debug.log", "a");
    if (logFile) {
        fprintf(logFile, "========== [3D Render End - SUCCESS] ==========\n\n");
        fclose(logFile);
    }
'@
$content = $content.Replace($finalDebug, '')

# 8. Clean up extra blank lines
$content = [regex]::Replace($content, '\r?\n\r?\n\r?\n+', "`r`n`r`n")

Write-Host "New file length: $($content.Length)"
[System.IO.File]::WriteAllText($file, $content)
Write-Host "Cleanup complete!"
