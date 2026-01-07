# 批量替换DllDicom/DicomApi.cpp中的编码转换代码

$filePath = "d:\2025-09-25 新系列\ConsoleDllTest\DllDicom\DicomApi.cpp"
$content = Get-Content $filePath -Raw -Encoding UTF8

# 替换模式1: file.c_str()
$pattern1 = @'
            // 使用 MultiByteToWideChar 将 GBK 转为 UTF-16
            int wlen = MultiByteToWideChar\(936, 0, file\.c_str\(\), -1, NULL, 0\);  // 936 = GBK
            if \(wlen <= 0\) \{
                continue;
            \}
            std::wstring wpath\(wlen, 0\);
            MultiByteToWideChar\(936, 0, file\.c_str\(\), -1, &wpath\[0\], wlen\);
            if \(!wpath\.empty\(\) && wpath\.back\(\) == L'\\0'\) \{
                wpath\.pop_back\(\);
            \}
            std::string utf8path = fs::path\(wpath\)\.u8string\(\);
'@

$replacement1 = @'
            // 使用统一的编码工具将 GBK 转为 UTF-8
            std::string utf8path = EncodingUtils::GetUtf8Path(file.c_str());
'@

$content = $content -replace $pattern1, $replacement1

# 替换模式2: dicomFiles[i].c_str()
$pattern2 = @'
            int wlen = MultiByteToWideChar\(936, 0, dicomFiles\[i\]\.c_str\(\), -1, NULL, 0\);
            if \(wlen <= 0\) continue;
            
            std::wstring wpath\(wlen, 0\);
            MultiByteToWideChar\(936, 0, dicomFiles\[i\]\.c_str\(\), -1, &wpath\[0\], wlen\);
            if \(!wpath\.empty\(\) && wpath\.back\(\) == L'\\0'\) \{
                wpath\.pop_back\(\);
            \}
            
            std::string utf8path = fs::path\(wpath\)\.u8string\(\);
'@

$replacement2 = @'
            // 使用统一的编码工具将 GBK 转为 UTF-8
            std::string utf8path = EncodingUtils::GetUtf8Path(dicomFiles[i].c_str());
'@

$content = $content -replace $pattern2, $replacement2

# 替换模式3: dicomFiles[0].c_str()
$pattern3 = @'
        int wlen = MultiByteToWideChar\(936, 0, dicomFiles\[0\]\.c_str\(\), -1, NULL, 0\);
        std::wstring wpath\(wlen, 0\);
        MultiByteToWideChar\(936, 0, dicomFiles\[0\]\.c_str\(\), -1, &wpath\[0\], wlen\);
        if \(!wpath\.empty\(\) && wpath\.back\(\) == L'\\0'\) \{
            wpath\.pop_back\(\);
        \}
        std::string utf8path = fs::path\(wpath\)\.u8string\(\);
'@

$replacement3 = @'
        // 使用统一的编码工具将 GBK 转为 UTF-8
        std::string utf8path = EncodingUtils::GetUtf8Path(dicomFiles[0].c_str());
'@

$content = $content -replace $pattern3, $replacement3

# 替换模式4: ctx->filePaths[0].c_str()
$pattern4 = @'
        int wlen = MultiByteToWideChar\(936, 0, ctx->filePaths\[0\]\.c_str\(\), -1, NULL, 0\);
        std::wstring wpath\(wlen, 0\);
        MultiByteToWideChar\(936, 0, ctx->filePaths\[0\]\.c_str\(\), -1, &wpath\[0\], wlen\);
        if \(!wpath\.empty\(\) && wpath\.back\(\) == L'\\0'\) \{
            wpath\.pop_back\(\);
        \}
        std::string utf8path = fs::path\(wpath\)\.u8string\(\);
'@

$replacement4 = @'
        // 使用统一的编码工具将 GBK 转为 UTF-8
        std::string utf8path = EncodingUtils::GetUtf8Path(ctx->filePaths[0].c_str());
'@

$content = $content -replace $pattern4, $replacement4

# 保存文件
$content | Set-Content $filePath -Encoding UTF8 -NoNewline

Write-Host "已完成DllDicom/DicomApi.cpp的编码转换代码重构" -ForegroundColor Green
