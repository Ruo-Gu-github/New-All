#pragma once
#include <atlstr.h>
#include <vector>
#include <windows.h>
#include <gdiplus.h>
#include "hpdf.h"

#pragma comment(lib, "libhpdfd.lib")
#pragma comment(lib, "gdiplus.lib")

// PDF报告生成器（使用libharu，支持中文，紧凑布局）
class SePDFReportGenerator
{
public:
	// 生成PDF报告
	static bool GenerateReport(
		const CString& imageFolderPath,
		const std::vector<CString>& params,
		const CString& outputPath = _T(""))
	{
		// 1. 确定输出路径
		CString finalOutputPath = outputPath;
		if (finalOutputPath.IsEmpty()) {
			finalOutputPath = imageFolderPath + _T("\\骨密度分析报告.pdf");
		}

		// 2. 创建PDF文档
		HPDF_Doc pdf = HPDF_New(NULL, NULL);
		if (!pdf) {
			return false;
		}

		HPDF_UseUTFEncodings(pdf);
		HPDF_SetCompressionMode(pdf, HPDF_COMP_ALL);

		// 3. 加载中文字体
		CString fontPath = _T("SimSun.ttf");
		const char* fontName = HPDF_LoadTTFontFromFile(pdf, "SimSun.ttf", HPDF_TRUE);
		if (!fontName) {
			fontName = HPDF_LoadTTFontFromFile(pdf, "Bin\\SimSun.ttf", HPDF_TRUE);
			if (!fontName) {
				HPDF_Free(pdf);
				return false;
			}
		}

		// 4. 创建A4页面
		HPDF_Page page = HPDF_AddPage(pdf);
		HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);

		HPDF_Font font = HPDF_GetFont(pdf, fontName, "UTF-8");

		// 5. 设置黑字白底
		HPDF_Page_SetRGBFill(page, 0, 0, 0);
		HPDF_Page_SetRGBStroke(page, 0, 0, 0);

		// 6. 页面布局参数（按照你的图片样式）
		float pageWidth = HPDF_Page_GetWidth(page);
		float pageHeight = HPDF_Page_GetHeight(page);
		float margin = 50;
		float currentY = pageHeight - 50;

		// 7. GBK转UTF8辅助函数
		auto GbkToUtf8 = [](const CString& gbkText) -> char* {
			int wideLen = MultiByteToWideChar(CP_ACP, 0, gbkText, -1, NULL, 0);
			wchar_t* wideText = new wchar_t[wideLen];
			MultiByteToWideChar(CP_ACP, 0, gbkText, -1, wideText, wideLen);
			int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideText, -1, NULL, 0, NULL, NULL);
			char* utf8Text = new char[utf8Len];
			WideCharToMultiByte(CP_UTF8, 0, wideText, -1, utf8Text, utf8Len, NULL, NULL);
			delete[] wideText;
			return utf8Text;
		};

		// 8. 写入标题
		HPDF_Page_BeginText(page);
		HPDF_Page_SetFontAndSize(page, font, 16);
		char* titleUtf8 = GbkToUtf8(_T("骨密度分析报告"));
		float titleWidth = HPDF_Page_TextWidth(page, titleUtf8);
		HPDF_Page_TextOut(page, (pageWidth - titleWidth) / 2, currentY, titleUtf8);
		delete[] titleUtf8;
		HPDF_Page_EndText(page);
		currentY -= 40;

		// 9. 查找并插入3D截图（BMP转JPEG后加载）
		CString screenshotPath = FindLatestScreenshot(imageFolderPath);
		HPDF_Image image = NULL;
		BYTE* jpegData = NULL;
		DWORD jpegSize = 0;

		if (!screenshotPath.IsEmpty()) {
			// 将BMP转换为JPEG内存数据
			if (ConvertBmpToJpegMemory(screenshotPath, &jpegData, &jpegSize)) {
				image = HPDF_LoadJpegImageFromMem(pdf, jpegData, jpegSize);
			}
		}

		if (image) {
				// 图片区域：宽度占页面80%，高度约250
				float imgWidth = pageWidth - 2 * margin;
				float imgHeight = 250;
				HPDF_Page_DrawImage(page, image, margin, currentY - imgHeight, imgWidth, imgHeight);
				currentY -= imgHeight + 20;
			
		} else {
			// 如果没有截图，绘制占位框
			HPDF_Page_SetLineWidth(page, 1);
			HPDF_Page_Rectangle(page, margin, currentY - 250, pageWidth - 2 * margin, 250);
			HPDF_Page_Stroke(page);
			HPDF_Page_BeginText(page);
			HPDF_Page_SetFontAndSize(page, font, 12);
			char* placeholderUtf8 = GbkToUtf8(_T("screenCapture图像"));
			HPDF_Page_TextOut(page, pageWidth / 2 - 50, currentY - 125, placeholderUtf8);
			delete[] placeholderUtf8;
			HPDF_Page_EndText(page);
			currentY -= 270;
		}

		// 10. 绘制参数表格（每行两个参数，增加行高）
		float tableX = margin;
		float tableWidth = pageWidth - 2 * margin;
		float rowHeight = 22;  // 增加行高以容纳中文
		float halfWidth = tableWidth / 2;
		
		// 左侧参数列宽
		float leftCol1Width = 80;   // 参数描述列
		float leftCol2Width = 50;   // 参数缩写列
		float leftCol3Width = 60;   // 数值列
		float leftCol4Width = halfWidth - leftCol1Width - leftCol2Width - leftCol3Width; // 单位列
		
		// 右侧参数列宽（与左侧相同）
		float rightCol1Width = 80;
		float rightCol2Width = 50;
		float rightCol3Width = 60;
		float rightCol4Width = halfWidth - rightCol1Width - rightCol2Width - rightCol3Width;

		float tableStartY = currentY;

		// 表格顶部线
		HPDF_Page_SetLineWidth(page, 1);
		HPDF_Page_MoveTo(page, tableX, currentY);
		HPDF_Page_LineTo(page, tableX + tableWidth, currentY);
		HPDF_Page_Stroke(page);

		HPDF_Page_BeginText(page);
		HPDF_Page_SetFontAndSize(page, font, 9);

		// 绘制参数（每行两个）
		for (size_t i = 0; i < params.size(); i += 2) {
			currentY -= rowHeight;
			
			// 左侧参数（第i个）
			if (i < params.size()) {
				CString param = params[i];
				int pos1 = param.Find(_T(" ("));
				int pos2 = param.Find(_T("): "));
				
				if (pos1 > 0 && pos2 > pos1) {
					CString desc = param.Left(pos1);
					CString abbr = param.Mid(pos1 + 2, pos2 - pos1 - 2);
					CString valueUnit = param.Mid(pos2 + 3);
					
					int spacePos = valueUnit.ReverseFind(_T(' '));
					CString value = (spacePos > 0) ? valueUnit.Left(spacePos) : valueUnit;
					CString unit = (spacePos > 0) ? valueUnit.Mid(spacePos + 1) : _T("");

					// 使用GBK->Wide->UTF8方法转换中文
					char* descUtf8 = GbkToUtf8(desc);
					char* abbrUtf8 = GbkToUtf8(abbr);
					char* valueUtf8 = GbkToUtf8(value);
					char* unitUtf8 = GbkToUtf8(unit);

					// 输出到表格左侧
					HPDF_Page_TextOut(page, tableX + 3, currentY + 5, descUtf8);
					HPDF_Page_TextOut(page, tableX + leftCol1Width + 3, currentY + 5, abbrUtf8);
					HPDF_Page_TextOut(page, tableX + leftCol1Width + leftCol2Width + 3, currentY + 5, valueUtf8);
					HPDF_Page_TextOut(page, tableX + leftCol1Width + leftCol2Width + leftCol3Width + 3, currentY + 5, unitUtf8);
					
					delete[] descUtf8;
					delete[] abbrUtf8;
					delete[] valueUtf8;
					delete[] unitUtf8;
				}
			}
			
			// 右侧参数（第i+1个）
			if (i + 1 < params.size()) {
				CString param = params[i + 1];
				int pos1 = param.Find(_T(" ("));
				int pos2 = param.Find(_T("): "));
				
				if (pos1 > 0 && pos2 > pos1) {
					CString desc = param.Left(pos1);
					CString abbr = param.Mid(pos1 + 2, pos2 - pos1 - 2);
					CString valueUnit = param.Mid(pos2 + 3);
					
					int spacePos = valueUnit.ReverseFind(_T(' '));
					CString value = (spacePos > 0) ? valueUnit.Left(spacePos) : valueUnit;
					CString unit = (spacePos > 0) ? valueUnit.Mid(spacePos + 1) : _T("");

					// 使用GBK->Wide->UTF8方法转换中文
					char* descUtf8 = GbkToUtf8(desc);
					char* abbrUtf8 = GbkToUtf8(abbr);
					char* valueUtf8 = GbkToUtf8(value);
					char* unitUtf8 = GbkToUtf8(unit);

					// 输出到表格右侧
					float rightX = tableX + halfWidth;
					HPDF_Page_TextOut(page, rightX + 3, currentY + 5, descUtf8);
					HPDF_Page_TextOut(page, rightX + rightCol1Width + 3, currentY + 5, abbrUtf8);
					HPDF_Page_TextOut(page, rightX + rightCol1Width + rightCol2Width + 3, currentY + 5, valueUtf8);
					HPDF_Page_TextOut(page, rightX + rightCol1Width + rightCol2Width + rightCol3Width + 3, currentY + 5, unitUtf8);
					
					delete[] descUtf8;
					delete[] abbrUtf8;
					delete[] valueUtf8;
					delete[] unitUtf8;
				}
			}

			// 行底线
			HPDF_Page_EndText(page);
			HPDF_Page_SetLineWidth(page, 0.5f);
			HPDF_Page_MoveTo(page, tableX, currentY);
			HPDF_Page_LineTo(page, tableX + tableWidth, currentY);
			HPDF_Page_Stroke(page);
			
			// 中间分隔线（左右两列之间）
			HPDF_Page_MoveTo(page, tableX + halfWidth, currentY);
			HPDF_Page_LineTo(page, tableX + halfWidth, currentY + rowHeight);
			HPDF_Page_Stroke(page);
			
			HPDF_Page_BeginText(page);
			HPDF_Page_SetFontAndSize(page, font, 9);
		}

		HPDF_Page_EndText(page);

		// 表格左右边框
		HPDF_Page_SetLineWidth(page, 1);
		HPDF_Page_MoveTo(page, tableX, tableStartY);
		HPDF_Page_LineTo(page, tableX, currentY);
		HPDF_Page_Stroke(page);
		HPDF_Page_MoveTo(page, tableX + tableWidth, tableStartY);
		HPDF_Page_LineTo(page, tableX + tableWidth, currentY);
		HPDF_Page_Stroke(page);

		// 11. 保存PDF文件
		CStringA pdfPathA(finalOutputPath);
		HPDF_STATUS status = HPDF_SaveToFile(pdf, pdfPathA.GetString());
		HPDF_Free(pdf);

		// 释放JPEG内存
		if (jpegData) {
			delete[] jpegData;
		}

		return (status == HPDF_OK);
	}

private:
	// BMP转JPEG到内存（使用GDI+）
	static bool ConvertBmpToJpegMemory(const CString& bmpPath, BYTE** jpegData, DWORD* jpegSize)
	{
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		ULONG_PTR gdiplusToken;
		if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Gdiplus::Ok) {
			return false;
		}

		bool success = false;
		int wideLen = MultiByteToWideChar(CP_ACP, 0, bmpPath, -1, NULL, 0);
		wchar_t* widePath = new wchar_t[wideLen];
		MultiByteToWideChar(CP_ACP, 0, bmpPath, -1, widePath, wideLen);

		Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(widePath);
		delete[] widePath;

		if (bitmap && bitmap->GetLastStatus() == Gdiplus::Ok) {
			CLSID jpegClsid;
			UINT num = 0, size = 0;
			Gdiplus::GetImageEncodersSize(&num, &size);
			if (size > 0) {
				Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)malloc(size);
				if (pImageCodecInfo) {
					Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
					for (UINT i = 0; i < num; ++i) {
						if (wcscmp(pImageCodecInfo[i].MimeType, L"image/jpeg") == 0) {
							jpegClsid = pImageCodecInfo[i].Clsid;
							IStream* pStream = NULL;
							if (CreateStreamOnHGlobal(NULL, TRUE, &pStream) == S_OK) {
								if (bitmap->Save(pStream, &jpegClsid, NULL) == Gdiplus::Ok) {
									STATSTG stats;
									if (pStream->Stat(&stats, STATFLAG_DEFAULT) == S_OK) {
										*jpegSize = (DWORD)stats.cbSize.QuadPart;
										*jpegData = new BYTE[*jpegSize];
										LARGE_INTEGER li = {0};
										pStream->Seek(li, STREAM_SEEK_SET, NULL);
										ULONG bytesRead = 0;
										pStream->Read(*jpegData, *jpegSize, &bytesRead);
										success = (bytesRead == *jpegSize);
									}
								}
								pStream->Release();
							}
							break;
						}
					}
					free(pImageCodecInfo);
				}
			}
			delete bitmap;
		}

		Gdiplus::GdiplusShutdown(gdiplusToken);
		return success;
	}
	// 查找最新的3D截图
	static CString FindLatestScreenshot(const CString& imageFolderPath)
	{
		CString screenshotFolder = imageFolderPath + _T("\\ScreenCapture");
		CString searchPath = screenshotFolder + _T("\\*.bmp");
		
		WIN32_FIND_DATA findData;
		HANDLE hFind = FindFirstFile(searchPath, &findData);
		
		if (hFind == INVALID_HANDLE_VALUE) {
			return _T("");
		}

		CString latestFile;
		FILETIME latestTime = {0, 0};

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
};
