#pragma once
#include <string>
#include <vector>
#include <atlstr.h>
#include <windows.h>

// 基于wkhtmltopdf的PDF报告生成器
// 下载地址：https://wkhtmltopdf.org/downloads.html
// 只需将 wkhtmltopdf.exe 放到程序目录或系统PATH中即可

class SePDFReportGeneratorSimple {
public:
	struct ParamData {
		CString strDescription;
		CString strDescription2;
		CString strAbbreviation;
		double dValue;
		CString strUnit;
	};

	SePDFReportGeneratorSimple();
	~SePDFReportGeneratorSimple();

	// 设置3D截图路径
	void SetScreenshotPath(const CString& path);
	
	// 自动查找最新截图
	bool FindLatestScreenshot(const CString& folderPath);
	
	// 设置参数数据
	void SetParameterData(const std::vector<ParamData>& paramData);
	
	// 生成PDF报告
	bool GeneratePDF(const CString& outputPath);

private:
	CString m_screenshotPath;
	std::vector<ParamData> m_paramData;
	bool m_hasScreenshot;

	// 生成HTML报告
	bool GenerateHTML(const CString& htmlPath);
	
	// 使用wkhtmltopdf转换HTML为PDF
	bool ConvertHTMLToPDF(const CString& htmlPath, const CString& pdfPath);
	
	// 查找文件夹中最新的图片文件
	CString FindLatestImageFile(const CString& folderPath);
	
	// 获取文件最后修改时间
	FILETIME GetFileModifyTime(const CString& filePath);
	
	// HTML转义
	CString HtmlEscape(const CString& str);
};
