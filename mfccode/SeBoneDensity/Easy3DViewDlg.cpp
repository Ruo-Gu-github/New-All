// Easy3DViewDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SeBoneDensity.h"
#include "Easy3DViewDlg.h"
#include "afxdialogex.h"

// 添加MessageBoxTimeout支持
extern "C"
{
	int WINAPI MessageBoxTimeoutA(IN HWND hWnd, IN LPCSTR lpText, IN LPCSTR lpCaption, IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);
	int WINAPI MessageBoxTimeoutW(IN HWND hWnd, IN LPCWSTR lpText, IN LPCWSTR lpCaption, IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);
};
#ifdef UNICODE
#define MessageBoxTimeout MessageBoxTimeoutW
#else
#define MessageBoxTimeout MessageBoxTimeoutA
#endif
// CEasy3DViewDlg 对话框

IMPLEMENT_DYNAMIC(CEasy3DViewDlg, CDialog)

CEasy3DViewDlg::CEasy3DViewDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEasy3DViewDlg::IDD, pParent)
{
	m_pGLDataMgr = new COpenGLDataMgr;
	m_pGLCamera = new COpenGLCamera;
	m_pGLShader = new COpenGLShader;
	m_bOpened = FALSE;
	m_pDc = NULL;
	m_pData = NULL;
}

CEasy3DViewDlg::~CEasy3DViewDlg()
{
	if (m_pGLDataMgr != NULL)
	{
		delete m_pGLDataMgr;
		m_pGLDataMgr = NULL;
	}
	if (m_pGLCamera != NULL)
	{
		delete m_pGLCamera;
		m_pGLCamera = NULL;
	}
	if (m_pGLShader != NULL)
	{
		delete m_pGLShader;
		m_pGLShader = NULL;
	}
	Safe_Delete(m_pData);
	Safe_Delete(m_pDc);
}

void CEasy3DViewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CEasy3DViewDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()


// CEasy3DViewDlg 消息处理程序


void CEasy3DViewDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: 在此处添加消息处理程序代码
	// 不为绘图消息调用 CDialog::OnPaint()
}


BOOL CEasy3DViewDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CEasy3DViewDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_ptOri = point;
	CDialog::OnLButtonDown(nFlags, point);
}


void CEasy3DViewDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CDialog::OnLButtonUp(nFlags, point);
}


//void CEasy3DViewDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
//{
//	// 此功能要求 Windows Vista 或更高版本。
//	// _WIN32_WINNT 符号必须 >= 0x0600。
//	// TODO: 在此添加消息处理程序代码和/或调用默认值
//	m_pGLCamera->Scale(zDelta);
//	CDialog::OnMouseWheel(nFlags, zDelta, pt);
//}


void CEasy3DViewDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if(nFlags & MK_LBUTTON)
	{
		m_pGLCamera->Move(m_ptOri.x-point.x, m_ptOri.y-point.y, 0);
		m_ptOri = point;
	}
	if (nFlags & MK_RBUTTON)
	{
		m_pGLCamera->Rotate(m_ptRightOri.x-point.x, m_ptRightOri.y-point.y, 0);
		m_ptRightOri = point;
	}
	CDialog::OnMouseMove(nFlags, point);
}


void CEasy3DViewDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_ptRightOri = point;
	CDialog::OnRButtonDown(nFlags, point);
}


void CEasy3DViewDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialog::OnRButtonUp(nFlags, point);
}


void CEasy3DViewDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ( !IsIconic())
	{
		CRect rect;
		GetClientRect(&rect);
		if (rect.Width() == 0 || rect.Height() == 0)
			return;
		m_glMainHandle.Render(rect.Width(), rect.Height());
		SwapBuffers(wglGetCurrentDC());
	}
	CDialog::OnTimer(nIDEvent);
}


BOOL CEasy3DViewDlg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	return TRUE;
	//return CDialog::OnEraseBkgnd(pDC);
}


BOOL CEasy3DViewDlg::PreTranslateMessage(MSG* pMsg)
{
	// 	// TODO: 在此添加专用代码和/或调用基类
	// 	// 字符不能用 switch case
	// 	//"w" 的ACSLL码 87
	// 	//"a" 的ACSLL码 65
	// 	//"s" 的ACSLL码 83
	// 	//"d" 的ACSLL码 68
	// 	//"q" 的ACSLL码 81
	// 	//"e" 的ACSLL码 69
	if (pMsg->message == WM_KEYDOWN)
		m_pGLCamera->SetKey(pMsg->wParam, true);
	if (pMsg->message == WM_KEYUP)
		m_pGLCamera->SetKey(pMsg->wParam, false);
	return CDialog::PreTranslateMessage(pMsg);
}


void CEasy3DViewDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码
	m_glMainHandle.Resize(cx, cy);
}


int CEasy3DViewDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码
	m_pDc = new CClientDC(this);
	mhContext = m_pDc->GetSafeHdc();
	if( !m_glMainHandle.Initialize( mhContext, m_pGLDataMgr, m_pGLCamera, m_pGLShader ))
	{
		::MessageBox( 0, _T( "Failed to initialze the renderer !"), _T( "Renderer" ), MB_OK );
		exit(0);
	}
	SetTimer(0, 100 ,NULL);
	return 0;
}

void CEasy3DViewDlg::InitData(CDcmPicArray* pDcmArray, int nMin, int nMax, CRect rt)
{
	if (pDcmArray == NULL || pDcmArray->GetDcmArray().size() == 0)
		return;
	int nWidth = pDcmArray->GetDcmArray()[0]->GetWidth();
	int nHeight = pDcmArray->GetDcmArray()[0]->GetHeight();
	int nSize = (int)pDcmArray->GetDcmArray().size();
	// 上下左右前后各多一层， 用来计算法向量的余量
	m_pData = new BYTE[(nWidth + 2) * (nHeight + 2) * (nSize + 2)];
	memset(m_pData, 0, sizeof(BYTE) * (nWidth + 2) * (nHeight + 2) * (nSize + 2));
	BYTE* pHead = m_pData;
	for (int i=0; i<nSize; i++)
	{
		short* pData = (short*)pDcmArray->GetDcmArray()[i]->GetData();
		for (int j=0; j<nHeight; j++)
		{
			for (int k=0; k<nWidth; k++)
			{
				if (pData[j*nWidth + k] > nMin && pData[j*nWidth + k] < nMax)
					pHead[(i+1) * (nWidth+2) * (nHeight+2) + (j+1) * (nWidth+2) + k + 1] = 255;
				else
					pHead[(i+1) * (nWidth+2) * (nHeight+2) + (j+1) * (nWidth+2) + k +1] = 0;
			}
		}
	}
	// 最后一层本来就置 0 了，不用管

// 	CRect rect;
// 	GetClientRect(&rect);
	m_pGLDataMgr->InitFace2DTex(rt.Width(), rt.Height());
	//m_pGLDataMgr->InitFace2DTexForFreeCut(rt.Width(), rt.Height());
	m_pGLDataMgr->InitFrameBuffer(rt.Width(), rt.Height());
	//m_pGLDataMgr->InitFrameBufferForFreeCut(rt.Width(), rt.Height());
	m_pGLDataMgr->AddVol3DTex(nWidth+2, nHeight+2, nSize+2, (GLbyte*)m_pData, GL_LUMINANCE, GL_UNSIGNED_BYTE, RGB(255, 255, 255), TRUE);
}

void CEasy3DViewDlg::Reset()
{
	m_pGLDataMgr->DeleteAllVolTexObj();
	m_pGLCamera->ResetAll();
}

void CEasy3DViewDlg::GetPrintScreen(CString strPath)
{
	CRect rect;
	GetWindowRect(&rect);
	HBITMAP bitmap = GetScreenImage(rect);
	CString strFile;

	CreateFolder(strPath + "\\ScreenCapture");

	CTime tm = CTime::GetCurrentTime();
	CString csTime = tm.Format(_T("%Y%m%d%H%M%S"));


	strFile.Format("%s\\ScreenCapture\\%s.bmp",strPath , csTime);

	BOOL bCaptured = SaveImageToFile(bitmap, strFile.operator LPCTSTR());
	if(bCaptured)
	{
		/*		AfxMessageBox("已截图！");*/
		MessageBoxTimeout(NULL, "            已截图！        ", "提示", MB_ICONINFORMATION, 0, 1000);
	}
	else
	{
		/*		AfxMessageBox("截图失败！");*/
		MessageBoxTimeout(NULL, "           截图失败！       ", "提示", MB_ICONINFORMATION, 0, 1000);
	}
}

HBITMAP CEasy3DViewDlg::GetScreenImage(LPRECT lpRect)
{
	HDC hScrDC, hMemDC;             // 屏幕和内存设备描述表  
	HBITMAP hBitmap, hOldBitmap;    // 位图句柄  
	int nX, nY, nX2, nY2;           // 选定区域坐标  
	int nWidth, nHeight;            // 位图宽度和高度  
	int xScrn, yScrn;               // 屏幕分辨率  

	if (IsRectEmpty(lpRect))  
		return NULL;  

	hScrDC = CreateDC("DISPLAY", NULL, NULL, NULL);     // 为屏幕创建设备描述表  

	hMemDC = CreateCompatibleDC(hScrDC);                // 为屏幕设备描述表创建兼容的内存设备描述表  

	nX = lpRect->left;  
	nY = lpRect->top;  
	nX2 = lpRect->right;  
	nY2 = lpRect->bottom;  

	xScrn = GetDeviceCaps(hScrDC, HORZRES); // 获得屏幕水平分辨率  
	yScrn = GetDeviceCaps(hScrDC, VERTRES);  

	if (nX < 0)  
		nX = 0;  
	if (nY < 0)  
		nY = 0;  
	if (nX2 > xScrn)  
		nX2 = xScrn;  
	if (nY2 > yScrn)  
		nY2 = yScrn;  
	nWidth = nX2 - nX;  
	nHeight = nY2 - nY;  

	hBitmap = CreateCompatibleBitmap(hScrDC, nWidth, nHeight);      // 创建一个与屏幕设备描述表兼容的位图  
	hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);            // 把新位图选到内存设备描述表中  
	BitBlt(hMemDC, 0, 0, nWidth, nHeight, hScrDC, nX, nY, SRCCOPY); // 把屏幕设备描述表拷贝到内存设备描述表中  
	hBitmap = (HBITMAP)SelectObject(hMemDC, hOldBitmap);            // 得到屏幕位图的句柄  

	DeleteDC(hScrDC);  
	DeleteDC(hMemDC);  

	return hBitmap;
}

BOOL CEasy3DViewDlg::SaveImageToFile(HBITMAP hBitmap, LPCTSTR lpFileName)
{
	HDC hDC;                        // 设备描述表  

	int iBits;                      // 当前显示分辨率下每个像素所占字节数  
	WORD wBitCount;                 // 位图中每个像素所占字节数  
	DWORD dwPaletteSize = 0, dwBmBitsSize, dwDIBSize, dwWritten;    // 调色板大小，位图数据大小，位图文件大小，写入文件字节数  
	BITMAP Bitmap;                  //位图属性结构  
	BITMAPFILEHEADER bmfHdr;        // 位图文件头  
	BITMAPINFOHEADER bi;            // 位图信息头  
	LPBITMAPINFOHEADER lpbi;        // 指向位图信息头结构  

	HANDLE fh, hDib;                // 定义文件，分配内存句柄  
	HPALETTE hPal, hOldPal=NULL;    // 调色板句柄  

	// 计算位图文件每个像素所占字节数  
	hDC = CreateDC("DISPLAY", NULL, NULL, NULL);  
	iBits = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);  
	DeleteDC(hDC);  
	if (iBits <= 1)  
		wBitCount = 1;  
	else if (iBits <= 4)  
		wBitCount = 4;  
	else if (iBits <= 8)  
		wBitCount = 8;  
	else if (iBits <= 24)  
		wBitCount = 24;  
	else  
		wBitCount = 32;  
	if (wBitCount <= 8)  
		dwPaletteSize = (1 << wBitCount) * sizeof(RGBQUAD);       // 计算调色板大小  

	// 设置位图信息头结构  
	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&Bitmap);  
	bi.biSize = sizeof(BITMAPINFOHEADER);  
	bi.biWidth = Bitmap.bmWidth;  
	bi.biHeight = Bitmap.bmHeight;  
	bi.biPlanes = 1;  
	bi.biBitCount = wBitCount;  
	bi.biCompression = BI_RGB;  
	bi.biSizeImage = 0;  
	bi.biXPelsPerMeter = 0;  
	bi.biYPelsPerMeter = 0;  
	bi.biClrUsed = 0;  
	bi.biClrImportant = 0;  
	dwBmBitsSize = ((Bitmap.bmWidth * wBitCount + 31) / 32) * 4 * Bitmap.bmHeight;  

	hDib = GlobalAlloc(GHND, dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER));  // 为位图内容分配内存  
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);  
	*lpbi = bi;  
	// 处理调色板  
	hPal = (HPALETTE)GetStockObject(DEFAULT_PALETTE);  
	if (hPal)  
	{  
		hDC = ::GetDC(NULL);  
		hOldPal = SelectPalette(hDC, hPal, FALSE);  
		RealizePalette(hDC);  
	}  
	// 获取该调色板下新的像素值  
	GetDIBits(hDC, hBitmap, 0, (UINT)Bitmap.bmHeight, (LPSTR)lpbi + sizeof(BITMAPINFOHEADER) + dwPaletteSize, (BITMAPINFO*)lpbi, DIB_RGB_COLORS);  

	if (hOldPal)                // 恢复调色板  
	{  
		SelectPalette(hDC, hOldPal, TRUE);  
		RealizePalette(hDC);  
		::ReleaseDC(NULL, hDC);
	}  
	// 创建位图文件   
	fh = CreateFile(lpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);  
	if (fh == INVALID_HANDLE_VALUE)  
		return FALSE;  

	// 设置位图文件头  
	bmfHdr.bfType = 0x4D42;     // 文件类型: "BM"  
	dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwPaletteSize + dwBmBitsSize;   
	bmfHdr.bfSize = dwDIBSize;  // 位图文件大小  
	bmfHdr.bfReserved1 = 0;  
	bmfHdr.bfReserved2 = 0;  
	bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize;  

	WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);  // 写入位图文件头  
	WriteFile(fh, (LPSTR)lpbi, dwDIBSize, &dwWritten, NULL);                    // 写入位图文件其余内容  

	GlobalUnlock(hDib);  
	GlobalFree(hDib);  
	CloseHandle(fh);  

	return TRUE;
}

void CEasy3DViewDlg::CreateFolder(CString csPath)
{
	if (!::CreateDirectoryA(csPath, NULL))
	{
		CFileFind   filefind;
		CString     csfilePath = csPath + "\\*.dcm";
		BOOL	bFind = filefind.FindFile(csfilePath); 
		while(bFind)
		{
			bFind = filefind.FindNextFile();
			if(filefind.IsDots())
				continue;
			else if(filefind.IsDirectory())
				continue;		
			else
			{
				CString   csfilename = filefind.GetFilePath();
				::DeleteFile(csfilename);
			}
		}
	}
}


void CEasy3DViewDlg::OnDestroy()
{
	CDialog::OnDestroy();

	// TODO: 在此处添加消息处理程序代码
}


BOOL CEasy3DViewDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_pGLCamera->Scale(zDelta);

	return CDialog::OnMouseWheel(nFlags, zDelta, pt);
}
