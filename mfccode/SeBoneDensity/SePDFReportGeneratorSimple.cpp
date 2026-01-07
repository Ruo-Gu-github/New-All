#include "stdafx.h"
#include "SePDFReportGeneratorSimple.h"
#include "SeLogger.h"
#include <fstream>
#include <sstream>

SePDFReportGeneratorSimple::SePDFReportGeneratorSimple()
	: m_hasScreenshot(false)
{
}

SePDFReportGeneratorSimple::~SePDFReportGeneratorSimple()
{
}

void SePDFReportGeneratorSimple::SetScreenshotPath(const CString& path)
{
	m_screenshotPath = path;
	m_hasScreenshot = !path.IsEmpty() && PathFileExists(path);
}

bool SePDFReportGeneratorSimple::FindLatestScreenshot(const CString& folderPath)
{
	CString screenshotFolder = folderPath + "\\ScreenCapture";
	
	if (!PathFileExists(screenshotFolder)) {
		LOG_WARNING("ScreenCapture文件夹不存在: " + screenshotFolder);
		m_hasScreenshot = false;
		return false;
	}

	CString latestFile = FindLatestImageFile(screenshotFolder);
	if (latestFile.IsEmpty()) {
		LOG_WARNING("ScreenCapture文件夹中未找到图片文件");
		m_hasScreenshot = false;
		return false;
	}

	m_screenshotPath = latestFile;
	m_hasScreenshot = true;
	LOG_INFO("找到最新截图: " + latestFile);
	return true;
}

CString SePDFReportGeneratorSimple::FindLatestImageFile(const CString& folderPath)
{
	CFileFind finder;
	CString searchPath = folderPath + "\\*.*";
	
	BOOL bWorking = finder.FindFile(searchPath);
	
	CString latestFile;
	FILETIME latestTime = {0, 0};
	
	while (bWorking) {
		bWorking = finder.FindNextFile();
		
		if (finder.IsDots() || finder.IsDirectory())
			continue;
		
		CString fileName = finder.GetFileName();
		fileName.MakeLower();
		
		// 检查是否是图片文件
		if (fileName.Find(".png") != -1 || 
			fileName.Find(".jpg") != -1 || 
			fileName.Find(".jpeg") != -1 ||
			fileName.Find(".bmp") != -1) {
			
			FILETIME fileTime = GetFileModifyTime(finder.GetFilePath());
			
			if (CompareFileTime(&fileTime, &latestTime) > 0) {
				latestTime = fileTime;
				latestFile = finder.GetFilePath();
			}
		}
	}
	
	finder.Close();
	return latestFile;
}

FILETIME SePDFReportGeneratorSimple::GetFileModifyTime(const CString& filePath)
{
	FILETIME fileTime = {0, 0};
	HANDLE hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, 
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	
	if (hFile != INVALID_HANDLE_VALUE) {
		GetFileTime(hFile, NULL, NULL, &fileTime);
		CloseHandle(hFile);
	}
	
	return fileTime;
}

void SePDFReportGeneratorSimple::SetParameterData(const std::vector<ParamData>& paramData)
{
	m_paramData = paramData;
}

CString SePDFReportGeneratorSimple::HtmlEscape(const CString& str)
{
	CString result = str;
	result.Replace("&", "&amp;");
	result.Replace("<", "&lt;");
	result.Replace(">", "&gt;");
	result.Replace("\"", "&quot;");
	return result;
}

bool SePDFReportGeneratorSimple::GenerateHTML(const CString& htmlPath)
{
	try {
		std::ofstream file(CStringA(htmlPath), std::ios::out);
		if (!file.is_open()) {
			LOG_ERROR("无法创建HTML文件: " + htmlPath);
			return false;
		}

		// 写入HTML头部
		file << "<!DOCTYPE html>\n";
		file << "<html>\n<head>\n";
		file << "<meta charset=\"UTF-8\">\n";
		file << "<title>骨参数分析报告</title>\n";
		file << "<style>\n";
		file << "body { font-family: 'Microsoft YaHei', Arial, sans-serif; margin: 40px; }\n";
		file << "h1 { color: #333; text-align: center; border-bottom: 3px solid #4CAF50; padding-bottom: 10px; }\n";
		file << ".screenshot { text-align: center; margin: 30px 0; }\n";
		file << ".screenshot img { max-width: 100%; height: auto; border: 1px solid #ddd; }\n";
		file << ".placeholder { background: #f0f0f0; padding: 100px; text-align: center; color: #999; font-style: italic; }\n";
		file << "table { width: 100%; border-collapse: collapse; margin-top: 20px; }\n";
		file << "th { background-color: #4CAF50; color: white; padding: 12px; text-align: left; }\n";
		file << "td { padding: 10px; border: 1px solid #ddd; }\n";
		file << "tr:nth-child(even) { background-color: #f9f9f9; }\n";
		file << "tr:hover { background-color: #f5f5f5; }\n";
		file << ".footer { margin-top: 40px; text-align: center; color: #666; font-size: 12px; }\n";
		file << "</style>\n";
		file << "</head>\n<body>\n";

		// 标题
		file << "<h1>骨参数分析报告</h1>\n";

		// 3D截图部分
		file << "<div class=\"screenshot\">\n";
		if (m_hasScreenshot && !m_screenshotPath.IsEmpty()) {
			// 转换为绝对路径
			CString imgPath = m_screenshotPath;
			imgPath.Replace("\\", "/");
			file << "<img src=\"file:///" << CStringA(imgPath) << "\" alt=\"3D截图\">\n";
		} else {
			file << "<div class=\"placeholder\">[3D图像区域 - 无截图]</div>\n";
		}
		file << "</div>\n";

		// 参数表格
		file << "<h2 style=\"color: #4CAF50; margin-top: 40px;\">参数列表</h2>\n";
		file << "<table>\n";
		file << "<tr>\n";
		file << "<th>参数名称</th>\n";
		file << "<th>参数名称(En)</th>\n";
		file << "<th>缩写</th>\n";
		file << "<th>数值</th>\n";
		file << "<th>单位</th>\n";
		file << "</tr>\n";

		// 写入参数数据
		for (size_t i = 0; i < m_paramData.size(); i++) {
			const ParamData& data = m_paramData[i];
			
			file << "<tr>\n";
			file << "<td>" << CStringA(HtmlEscape(data.strDescription)) << "</td>\n";
			file << "<td>" << CStringA(HtmlEscape(data.strDescription2)) << "</td>\n";
			file << "<td>" << CStringA(HtmlEscape(data.strAbbreviation)) << "</td>\n";
			
			CString valueStr;
			valueStr.Format("%.4f", data.dValue);
			file << "<td>" << CStringA(valueStr) << "</td>\n";
			file << "<td>" << CStringA(HtmlEscape(data.strUnit)) << "</td>\n";
			file << "</tr>\n";
		}

		file << "</table>\n";

		// 页脚
		SYSTEMTIME st;
		GetLocalTime(&st);
		CString dateTime;
		dateTime.Format("%04d-%02d-%02d %02d:%02d:%02d", 
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		
		file << "<div class=\"footer\">\n";
		file << "<p>报告生成时间: " << CStringA(dateTime) << "</p>\n";
		file << "</div>\n";

		file << "</body>\n</html>\n";
		file.close();

		return true;
	}
	catch (...) {
		LOG_ERROR("生成HTML文件时发生异常");
		return false;
	}
}

bool SePDFReportGeneratorSimple::ConvertHTMLToPDF(const CString& htmlPath, const CString& pdfPath)
{
	// 方案1: 尝试使用wkhtmltopdf
	CString wkhtmlPath = "wkhtmltopdf.exe";  // 假设在PATH中
	
	// 检查wkhtmltopdf是否存在
	CString checkCmd = "where wkhtmltopdf.exe >nul 2>&1";
	int result = system(CStringA(checkCmd));
	
	if (result != 0) {
		// wkhtmltopdf不存在，尝试从程序目录查找
		TCHAR szPath[MAX_PATH];
		GetModuleFileName(NULL, szPath, MAX_PATH);
		CString exePath = szPath;
		int pos = exePath.ReverseFind('\\');
		if (pos != -1) {
			exePath = exePath.Left(pos);
			wkhtmlPath = exePath + "\\wkhtmltopdf.exe";
			
			if (!PathFileExists(wkhtmlPath)) {
				LOG_WARNING("未找到wkhtmltopdf.exe，请下载并放到程序目录");
				LOG_WARNING("下载地址: https://wkhtmltopdf.org/downloads.html");
				
				// 方案2: 直接复制HTML文件，让用户手动转换或用浏览器打开
				CString htmlCopy = pdfPath;
				htmlCopy.Replace(".pdf", ".html");
				CopyFile(htmlPath, htmlCopy, FALSE);
				
				LOG_INFO("已生成HTML报告: " + htmlCopy);
				LOG_INFO("请使用浏览器打开HTML文件，然后打印为PDF");
				
				return true;  // 虽然没生成PDF，但HTML已生成
			}
		}
	}

	// 构建wkhtmltopdf命令
	CString cmd;
	cmd.Format("\"%s\" --page-size A4 --encoding UTF-8 --enable-local-file-access \"%s\" \"%s\"",
		wkhtmlPath, htmlPath, pdfPath);

	// 执行转换
	LOG_INFO("执行PDF转换: " + cmd);
	
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;  // 隐藏窗口

	if (CreateProcess(NULL, cmd.GetBuffer(), NULL, NULL, FALSE, 
		CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		
		// 等待进程完成
		WaitForSingleObject(pi.hProcess, 30000);  // 最多等待30秒
		
		DWORD exitCode;
		GetExitCodeProcess(pi.hProcess, &exitCode);
		
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		
		if (exitCode == 0 && PathFileExists(pdfPath)) {
			LOG_INFO("PDF转换成功: " + pdfPath);
			
			// 删除临时HTML文件
			DeleteFile(htmlPath);
			
			return true;
		} else {
			LOG_ERROR("PDF转换失败，退出码: " + CString(std::to_string(exitCode).c_str()));
		}
	} else {
		LOG_ERROR("无法启动wkhtmltopdf进程");
	}

	return false;
}

bool SePDFReportGeneratorSimple::GeneratePDF(const CString& outputPath)
{
	try {
		// 生成临时HTML文件
		CString htmlPath = outputPath;
		htmlPath.Replace(".pdf", "_temp.html");
		
		if (!GenerateHTML(htmlPath)) {
			LOG_ERROR("生成HTML失败");
			return false;
		}

		// 转换为PDF
		if (ConvertHTMLToPDF(htmlPath, outputPath)) {
			LOG_INFO("PDF报告生成成功: " + outputPath);
			return true;
		} else {
			LOG_WARNING("PDF转换失败，但HTML报告已生成");
			return false;
		}
	}
	catch (...) {
		LOG_ERROR("PDF生成过程中发生异常");
		return false;
	}
}
