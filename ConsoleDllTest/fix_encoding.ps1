$file = "DllVisualization\OffscreenRenderingApi.h"
$text = [System.IO.File]::ReadAllText($file, [System.Text.Encoding]::UTF8)

# Replace Chinese with English
$replacements = @{
    '会话ID' = 'Session ID'
    '成功返回 NATIVE_OK' = 'Returns NATIVE_OK on success'
    '获取最新的渲染结果（双缓冲，前台读后台写）' = 'Get latest render result (double buffering)'
    '输出渲染结果指针' = 'Output render result pointer'
    '成功返回 NATIVE_OK，如果没有新数据返回 NATIVE_E_NO_DATA' = 'Returns NATIVE_OK on success, NATIVE_E_NO_DATA if no new data'
    '检查是否有新的渲染结果可用' = 'Check if new render result is available'
    '释放渲染结果的像素数据（由 GetRenderResult 返回的 pixelData）' = 'Free pixel data from render result'
    '多视图链接' = 'Multi-View Linking'
    '链接多个离屏视图的中心点（用于 Axial/Coronal/Sagittal 三联动）' = 'Link center points of multiple offscreen views'
    '取消链接' = 'Unlink centers'
}

foreach ($pair in $replacements.GetEnumerator()) {
    $text = $text.Replace($pair.Key, $pair.Value)
}

# Write back as UTF-8 without BOM
$utf8NoBom = New-Object System.Text.UTF8Encoding $false
[System.IO.File]::WriteAllText($file, $text, $utf8NoBom)

Write-Host "File updated successfully"
