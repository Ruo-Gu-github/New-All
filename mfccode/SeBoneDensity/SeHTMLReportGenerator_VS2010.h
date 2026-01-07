// SeHTMLReportGenerator_VS2010.h
// VS2010兼容版本 - 超轻量级HTML报告生成器
#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <fstream>
#include <sstream>
#include <vector>

#pragma comment(lib, "gdiplus.lib")

// Base64编码器 - VS2010兼容版本
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
		const BYTE* bytes_to_encode = &data[0];

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

	static std::string EncodeFile(const CString& filePath) {
		// 读取文件
		std::ifstream file(CStringA(filePath), std::ios::binary);
		if (!file.is_open()) return "";

		// 读取到vector
		std::vector<BYTE> buffer;
		file.seekg(0, std::ios::end);
		size_t size = (size_t)file.tellg();
		file.seekg(0, std::ios::beg);
		
		buffer.resize(size);
		file.read((char*)&buffer[0], size);
		file.close();

		return Encode(buffer);
	}
};

// HTML报告生成器 - VS2010兼容版本
class SeHTMLReportGenerator {
public:
	// 生成HTML报告
	static bool GenerateReport(
		const CString& imageFolderPath,
		const std::vector<CString>& params,
		const CString& outputPath = _T(""))
	{
		// 1. 查找最新截图
		CString screenshotPath = FindLatestScreenshot(imageFolderPath);
		
		// 2. 生成HTML内容
		std::string html = GenerateHTML(screenshotPath, params);
		
		// 3. 确定输出路径
		CString finalOutputPath = outputPath;
		if (finalOutputPath.IsEmpty()) {
			finalOutputPath = imageFolderPath + "\\骨密度分析报告.html";
		}
		
		// 4. 保存HTML文件
		std::ofstream outFile(CString(finalOutputPath), std::ios::binary);
		if (!outFile.is_open()) {
			return false;
		}
		
		// 写入UTF-8 BOM
		outFile.write("\xEF\xBB\xBF", 3);
		outFile << html;
		outFile.close();
		
		// 5. 在默认浏览器中打开
		ShellExecute(NULL, _T("open"), finalOutputPath, NULL, NULL, SW_SHOW);
		
		return true;
	}

private:
	static CString FindLatestScreenshot(const CString& imageFolderPath) {
		CString screenshotFolder = imageFolderPath + _T("\\ScreenCapture");
		CString pattern = screenshotFolder + _T("\\*.png");
		
		WIN32_FIND_DATA findData;
		HANDLE hFind = FindFirstFile(pattern, &findData);
		
		if (hFind == INVALID_HANDLE_VALUE) {
			return _T("");
		}
		
		CString latestFile;
		FILETIME latestTime;
		ZeroMemory(&latestTime, sizeof(FILETIME));
		
		do {
			if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				if (CompareFileTime(&findData.ftLastWriteTime, &latestTime) > 0) {
					latestTime = findData.ftLastWriteTime;
					latestFile = screenshotFolder + _T("\\") + findData.cFileName;
				}
			}
		} while (FindNextFile(hFind, &findData));
		
		FindClose(hFind);
		return latestFile;
	}

	static std::string WStringToUTF8(const CString& wstr) {
		if (wstr.IsEmpty()) return std::string();
		
		// 先转换为Unicode
		CStringW wide = CStringW(wstr);
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
		std::string strTo(size_needed - 1, 0);
		WideCharToMultiByte(CP_UTF8, 0, wide, -1, &strTo[0], size_needed, NULL, NULL);
		return strTo;
	}

	static std::string GenerateHTML(const CString& screenshotPath, 
		const std::vector<CString>& params) 
	{
		std::stringstream ss;
		
		// HTML头部
		ss << "<!DOCTYPE html>\n";
		ss << "<html lang=\"zh-CN\">\n";
		ss << "<head>\n";
		ss << "    <meta charset=\"UTF-8\">\n";
		ss << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
		ss << "    <title>骨密度分析报告</title>\n";
		ss << "    <style>\n";
		ss << "        @media print {\n";
		ss << "            @page { margin: 1cm; }\n";
		ss << "            body { margin: 0; }\n";
		ss << "            .no-print { display: none !important; }\n";
		ss << "        }\n";
		ss << "        * { margin: 0; padding: 0; box-sizing: border-box; }\n";
		ss << "        body {\n";
		ss << "            font-family: 'Microsoft YaHei', 'SimSun', sans-serif;\n";
		ss << "            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n";
		ss << "            padding: 20px;\n";
		ss << "            min-height: 100vh;\n";
		ss << "        }\n";
		ss << "        .container {\n";
		ss << "            max-width: 1200px;\n";
		ss << "            margin: 0 auto;\n";
		ss << "            background: white;\n";
		ss << "            border-radius: 20px;\n";
		ss << "            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);\n";
		ss << "            overflow: hidden;\n";
		ss << "        }\n";
		ss << "        .header {\n";
		ss << "            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n";
		ss << "            color: white;\n";
		ss << "            padding: 40px;\n";
		ss << "            text-align: center;\n";
		ss << "        }\n";
		ss << "        .header h1 {\n";
		ss << "            font-size: 36px;\n";
		ss << "            margin-bottom: 10px;\n";
		ss << "            text-shadow: 2px 2px 4px rgba(0,0,0,0.2);\n";
		ss << "        }\n";
		ss << "        .header .subtitle { font-size: 16px; opacity: 0.9; }\n";
		ss << "        .content { padding: 40px; }\n";
		ss << "        .section { margin-bottom: 40px; }\n";
		ss << "        .section-title {\n";
		ss << "            font-size: 24px;\n";
		ss << "            color: #667eea;\n";
		ss << "            margin-bottom: 20px;\n";
		ss << "            padding-bottom: 10px;\n";
		ss << "            border-bottom: 3px solid #667eea;\n";
		ss << "            display: flex;\n";
		ss << "            align-items: center;\n";
		ss << "        }\n";
		ss << "        .section-title::before {\n";
		ss << "            content: '';\n";
		ss << "            width: 8px;\n";
		ss << "            height: 24px;\n";
		ss << "            background: #667eea;\n";
		ss << "            margin-right: 12px;\n";
		ss << "            border-radius: 4px;\n";
		ss << "        }\n";
		ss << "        .screenshot-container {\n";
		ss << "            text-align: center;\n";
		ss << "            margin: 30px 0;\n";
		ss << "            padding: 20px;\n";
		ss << "            background: #f8f9fa;\n";
		ss << "            border-radius: 15px;\n";
		ss << "            box-shadow: inset 0 2px 8px rgba(0,0,0,0.05);\n";
		ss << "        }\n";
		ss << "        .screenshot {\n";
		ss << "            max-width: 100%;\n";
		ss << "            height: auto;\n";
		ss << "            border-radius: 10px;\n";
		ss << "            box-shadow: 0 10px 30px rgba(0,0,0,0.15);\n";
		ss << "            border: 4px solid white;\n";
		ss << "        }\n";
		ss << "        .params-grid {\n";
		ss << "            display: -ms-grid;\n";
		ss << "            -ms-grid-columns: 1fr 20px 1fr 20px 1fr;\n";
		ss << "        }\n";
		ss << "        .param-card {\n";
		ss << "            background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);\n";
		ss << "            padding: 20px;\n";
		ss << "            border-radius: 12px;\n";
		ss << "            box-shadow: 0 4px 12px rgba(0,0,0,0.08);\n";
		ss << "            margin-bottom: 20px;\n";
		ss << "        }\n";
		ss << "        .param-name {\n";
		ss << "            font-weight: bold;\n";
		ss << "            color: #667eea;\n";
		ss << "            font-size: 16px;\n";
		ss << "            margin-bottom: 8px;\n";
		ss << "        }\n";
		ss << "        .param-value {\n";
		ss << "            font-size: 20px;\n";
		ss << "            color: #2d3748;\n";
		ss << "            font-weight: 600;\n";
		ss << "        }\n";
		ss << "        .footer {\n";
		ss << "            background: #f8f9fa;\n";
		ss << "            padding: 30px 40px;\n";
		ss << "            text-align: center;\n";
		ss << "            color: #666;\n";
		ss << "            font-size: 14px;\n";
		ss << "        }\n";
		ss << "        .print-button {\n";
		ss << "            position: fixed;\n";
		ss << "            bottom: 30px;\n";
		ss << "            right: 30px;\n";
		ss << "            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n";
		ss << "            color: white;\n";
		ss << "            border: none;\n";
		ss << "            padding: 15px 30px;\n";
		ss << "            font-size: 16px;\n";
		ss << "            border-radius: 50px;\n";
		ss << "            cursor: pointer;\n";
		ss << "            box-shadow: 0 8px 20px rgba(102, 126, 234, 0.4);\n";
		ss << "            z-index: 1000;\n";
		ss << "        }\n";
		ss << "        .timestamp { color: #999; font-size: 14px; margin-top: 10px; }\n";
		ss << "        .info-banner {\n";
		ss << "            background: #e3f2fd;\n";
		ss << "            border-left: 4px solid #2196f3;\n";
		ss << "            padding: 15px 20px;\n";
		ss << "            margin-bottom: 30px;\n";
		ss << "            border-radius: 8px;\n";
		ss << "            color: #1565c0;\n";
		ss << "            font-size: 14px;\n";
		ss << "        }\n";
		ss << "    </style>\n";
		ss << "</head>\n";
		ss << "<body>\n";
		ss << "    <button class=\"print-button no-print\" onclick=\"window.print()\">\n";
		ss << "        &#128424; 打印 / 保存为PDF\n";
		ss << "    </button>\n";
		ss << "    <div class=\"container\">\n";
		ss << "        <div class=\"header\">\n";
		ss << "            <h1>骨密度分析报告</h1>\n";
		ss << "            <div class=\"subtitle\">Bone Density Analysis Report</div>\n";
		ss << "            <div class=\"timestamp\" id=\"timestamp\"></div>\n";
		ss << "        </div>\n";
		ss << "        <div class=\"content\">\n";
		ss << "            <div class=\"info-banner no-print\">\n";
		ss << "                &#128161; 提示: 点击右下角按钮或按 <strong>Ctrl+P</strong> 打印此报告为PDF文件\n";
		ss << "            </div>\n";

		// 3D截图部分
		if (!screenshotPath.IsEmpty()) {
			ss << "            <div class=\"section\">\n";
			ss << "                <div class=\"section-title\">三维可视化</div>\n";
			ss << "                <div class=\"screenshot-container\">\n";
			
			std::string base64Image = Base64Encoder::EncodeFile(screenshotPath);
			if (!base64Image.empty()) {
				ss << "                    <img class=\"screenshot\" src=\"data:image/png;base64," 
				   << base64Image << "\" alt=\"3D截图\">\n";
			} else {
				ss << "                    <p style=\"color: #999; padding: 40px;\">未找到3D截图</p>\n";
			}
			
			ss << "                </div>\n";
			ss << "            </div>\n";
		}

		// 参数部分
		ss << "            <div class=\"section\">\n";
		ss << "                <div class=\"section-title\">分析参数</div>\n";
		ss << "                <div class=\"params-grid\">\n";

		for (size_t i = 0; i < params.size(); i++) {
			CString param = params[i];
			int pos = param.Find(_T(": "));
			
			CString name, value;
			if (pos != -1) {
				name = param.Left(pos);
				value = param.Mid(pos + 2);
			} else {
				name.Format(_T("参数 %d"), i + 1);
				value = param;
			}
			
			ss << "                    <div class=\"param-card\">\n";
			ss << "                        <div class=\"param-name\">" 
			   << WStringToUTF8(name) << "</div>\n";
			ss << "                        <div class=\"param-value\">" 
			   << WStringToUTF8(value) << "</div>\n";
			ss << "                    </div>\n";
		}

		ss << "                </div>\n";
		ss << "            </div>\n";
		ss << "        </div>\n";
		ss << "        <div class=\"footer\">\n";
		ss << "            <p>此报告由骨密度分析系统自动生成</p>\n";
		ss << "            <p style=\"margin-top: 10px; font-size: 12px;\">\n";
		ss << "                Generated by Bone Density Analysis System\n";
		ss << "            </p>\n";
		ss << "        </div>\n";
		ss << "    </div>\n";
		ss << "    <script>\n";
		ss << "        document.getElementById('timestamp').textContent = \n";
		ss << "            '生成时间: ' + new Date().toLocaleString('zh-CN');\n";
		ss << "    </script>\n";
		ss << "</body>\n";
		ss << "</html>";

		return ss.str();
	}
};
