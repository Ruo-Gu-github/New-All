// SeHTMLReportGenerator.h
// 超轻量级报告生成器 - 纯HTML,0外部依赖
// 用户可直接用浏览器打开,按Ctrl+P打印为PDF
#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <windows.h>
#include <gdiplus.h>
#include <iostream>
#include <algorithm>

#pragma comment(lib, "gdiplus.lib")

// 使用Base64编码将图片嵌入HTML,实现真正的单文件报告
class Base64Encoder {
public:
    static std::string Encode(const std::vector<BYTE>& data) {
        static const char* base64_chars = 
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

        std::string ret;
        int i = 0;
        int j = 0;
        BYTE char_array_3[3];
        BYTE char_array_4[4];
        size_t in_len = data.size();
        const BYTE* bytes_to_encode = data.data();

        while (in_len--) {
            char_array_3[i++] = *(bytes_to_encode++);
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for(i = 0; (i < 4); i++)
                    ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }

        if (i) {
            for(j = i; j < 3; j++)
                char_array_3[j] = '\0';

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

            for (j = 0; (j < i + 1); j++)
                ret += base64_chars[char_array_4[j]];

            while((i++ < 3))
                ret += '=';
        }

        return ret;
    }

    static std::string EncodeFile(const std::wstring& filePath) {
        // 读取文件内容
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) return "";

        std::vector<BYTE> buffer((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
        file.close();

        return Encode(buffer);
    }
};

class SeHTMLReportGenerator {
public:
    // 生成HTML报告
    static bool GenerateReport(
        const std::wstring& imageFolderPath,      // 图像文件夹路径
        const std::vector<std::wstring>& params,  // 参数列表
        const std::wstring& outputPath = L"")     // 输出路径(可选)
    {
        // 1. 查找最新截图
        std::wstring screenshotPath = FindLatestScreenshot(imageFolderPath);
        
        // 2. 生成HTML内容
        std::string html = GenerateHTML(screenshotPath, params);
        
        // 3. 确定输出路径
        std::wstring finalOutputPath = outputPath;
        if (finalOutputPath.empty()) {
            finalOutputPath = imageFolderPath + L"\\骨密度分析报告.html";
        }
        
        // 4. 保存HTML文件
        std::ofstream outFile(finalOutputPath, std::ios::binary);
        if (!outFile.is_open()) {
            return false;
        }
        
        // 写入UTF-8 BOM
        outFile.write("\xEF\xBB\xBF", 3);
        outFile << html;
        outFile.close();
        
        // 5. 在默认浏览器中打开
        ShellExecuteW(NULL, L"open", finalOutputPath.c_str(), NULL, NULL, SW_SHOW);
        
        return true;
    }

private:
    static std::wstring FindLatestScreenshot(const std::wstring& imageFolderPath) {
        std::wstring screenshotFolder = imageFolderPath + L"\\ScreenCapture";
        std::wstring pattern = screenshotFolder + L"\\*.png";
        
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW(pattern.c_str(), &findData);
        
        if (hFind == INVALID_HANDLE_VALUE) {
            return L"";
        }
        
        std::wstring latestFile;
        FILETIME latestTime = {0, 0};
        
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                if (CompareFileTime(&findData.ftLastWriteTime, &latestTime) > 0) {
                    latestTime = findData.ftLastWriteTime;
                    latestFile = screenshotFolder + L"\\" + findData.cFileName;
                }
            }
        } while (FindNextFileW(hFind, &findData));
        
        FindClose(hFind);
        return latestFile;
    }

    static std::string WStringToUTF8(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }

    static std::string GenerateHTML(const std::wstring& screenshotPath, 
                                     const std::vector<std::wstring>& params) 
    {
        std::stringstream ss;
        
        // HTML头部 - 现代、精美的设计
        ss << R"(<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>骨密度分析报告</title>
    <style>
        @media print {
            @page { margin: 1cm; }
            body { margin: 0; }
            .no-print { display: none !important; }
        }
        
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Microsoft YaHei', 'SimSun', sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            padding: 20px;
            min-height: 100vh;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
            overflow: hidden;
        }
        
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 40px;
            text-align: center;
        }
        
        .header h1 {
            font-size: 36px;
            margin-bottom: 10px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.2);
        }
        
        .header .subtitle {
            font-size: 16px;
            opacity: 0.9;
        }
        
        .content {
            padding: 40px;
        }
        
        .section {
            margin-bottom: 40px;
        }
        
        .section-title {
            font-size: 24px;
            color: #667eea;
            margin-bottom: 20px;
            padding-bottom: 10px;
            border-bottom: 3px solid #667eea;
            display: flex;
            align-items: center;
        }
        
        .section-title::before {
            content: '';
            width: 8px;
            height: 24px;
            background: #667eea;
            margin-right: 12px;
            border-radius: 4px;
        }
        
        .screenshot-container {
            text-align: center;
            margin: 30px 0;
            padding: 20px;
            background: #f8f9fa;
            border-radius: 15px;
            box-shadow: inset 0 2px 8px rgba(0,0,0,0.05);
        }
        
        .screenshot {
            max-width: 100%;
            height: auto;
            border-radius: 10px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.15);
            border: 4px solid white;
        }
        
        .params-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
        }
        
        .param-card {
            background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
            padding: 20px;
            border-radius: 12px;
            box-shadow: 0 4px 12px rgba(0,0,0,0.08);
            transition: transform 0.3s ease, box-shadow 0.3s ease;
        }
        
        .param-card:hover {
            transform: translateY(-5px);
            box-shadow: 0 8px 20px rgba(0,0,0,0.15);
        }
        
        .param-name {
            font-weight: bold;
            color: #667eea;
            font-size: 16px;
            margin-bottom: 8px;
        }
        
        .param-value {
            font-size: 20px;
            color: #2d3748;
            font-weight: 600;
        }
        
        .footer {
            background: #f8f9fa;
            padding: 30px 40px;
            text-align: center;
            color: #666;
            font-size: 14px;
        }
        
        .print-button {
            position: fixed;
            bottom: 30px;
            right: 30px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 15px 30px;
            font-size: 16px;
            border-radius: 50px;
            cursor: pointer;
            box-shadow: 0 8px 20px rgba(102, 126, 234, 0.4);
            transition: all 0.3s ease;
            z-index: 1000;
        }
        
        .print-button:hover {
            transform: scale(1.05);
            box-shadow: 0 12px 30px rgba(102, 126, 234, 0.6);
        }
        
        .print-button:active {
            transform: scale(0.98);
        }
        
        .timestamp {
            color: #999;
            font-size: 14px;
            margin-top: 10px;
        }
        
        .info-banner {
            background: #e3f2fd;
            border-left: 4px solid #2196f3;
            padding: 15px 20px;
            margin-bottom: 30px;
            border-radius: 8px;
            color: #1565c0;
            font-size: 14px;
        }
    </style>
</head>
<body>
    <button class="print-button no-print" onclick="window.print()">
        🖨️ 打印 / 保存为PDF
    </button>
    
    <div class="container">
        <div class="header">
            <h1>骨密度分析报告</h1>
            <div class="subtitle">Bone Density Analysis Report</div>
            <div class="timestamp" id="timestamp"></div>
        </div>
        
        <div class="content">
            <div class="info-banner no-print">
                💡 提示: 点击右下角按钮或按 <strong>Ctrl+P</strong> 打印此报告为PDF文件
            </div>
)";

        // 3D截图部分
        if (!screenshotPath.empty()) {
            ss << R"(
            <div class="section">
                <div class="section-title">三维可视化</div>
                <div class="screenshot-container">
)";
            
            // 将图片转换为Base64并嵌入
            std::string base64Image = Base64Encoder::EncodeFile(screenshotPath);
            if (!base64Image.empty()) {
                ss << "                    <img class=\"screenshot\" src=\"data:image/png;base64," 
                   << base64Image << "\" alt=\"3D截图\">\n";
            } else {
                ss << "                    <p style=\"color: #999; padding: 40px;\">未找到3D截图</p>\n";
            }
            
            ss << R"(
                </div>
            </div>
)";
        }

        // 参数表格部分
        ss << R"(
            <div class="section">
                <div class="section-title">分析参数</div>
                <div class="params-grid">
)";

        // 添加参数卡片
        for (size_t i = 0; i < params.size(); i++) {
            std::wstring param = params[i];
            size_t pos = param.find(L": ");
            
            std::wstring name, value;
            if (pos != std::wstring::npos) {
                name = param.substr(0, pos);
                value = param.substr(pos + 2);
            } else {
                name = L"参数 " + std::to_wstring(i + 1);
                value = param;
            }
            
            ss << "                    <div class=\"param-card\">\n";
            ss << "                        <div class=\"param-name\">" 
               << WStringToUTF8(name) << "</div>\n";
            ss << "                        <div class=\"param-value\">" 
               << WStringToUTF8(value) << "</div>\n";
            ss << "                    </div>\n";
        }

        ss << R"(
                </div>
            </div>
        </div>
        
        <div class="footer">
            <p>此报告由骨密度分析系统自动生成</p>
            <p style="margin-top: 10px; font-size: 12px;">
                Generated by Bone Density Analysis System
            </p>
        </div>
    </div>
    
    <script>
        // 显示生成时间
        document.getElementById('timestamp').textContent = 
            '生成时间: ' + new Date().toLocaleString('zh-CN', {
                year: 'numeric',
                month: '2-digit',
                day: '2-digit',
                hour: '2-digit',
                minute: '2-digit',
                second: '2-digit'
            });
    </script>
</body>
</html>)";

        return ss.str();
    }
};
