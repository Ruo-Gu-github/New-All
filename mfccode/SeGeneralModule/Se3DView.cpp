// Se3DView.cpp : 实现文件
//

#include "stdafx.h"
#include "SeGeneralModule.h"
#include "Se3DView.h"
#include "GeneralSwapData.h"

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

// CSe3DView

IMPLEMENT_DYNCREATE(Se3DView, CView)

Se3DView::Se3DView()
{
	m_pGLDataMgr = new COpenGLDataMgr;
	m_pGLCamera = new COpenGLCamera;
	m_pGLShader = new COpenGLShader;
 	m_bOpened = FALSE;
	m_pDc = NULL;
}

Se3DView::~Se3DView()
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
	Safe_Delete(m_pDc);
}

BEGIN_MESSAGE_MAP(Se3DView, CView)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_CREATE()
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()


// CSe3DView 绘图

void Se3DView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: 在此添加绘制代码
	CRect rect;
	GetClientRect(&rect);


	// 		//CDC* pDC = GetDC();
	// 		CDC  MemDC;
	// 		CBitmap MemBitmap;
	// 		MemDC.CreateCompatibleDC(NULL);
	// 		MemBitmap.CreateCompatibleBitmap(pDC, rect.Width(), rect.Height());
	// 		CBitmap* pOldBit = MemDC.SelectObject(&MemBitmap);
	// 		//MemDC.FillSolidRect(0,0,rect.Width(), rect.Height(), RGB(0,0,0));
	// 		MemDC.BitBlt(0,0,rect.Width(), rect.Height(), pDC, 0, 0, SRCCOPY);
	// 		DrawRuler(&MemDC);
	// 
	// 		pDC->BitBlt(0,0,rect.Width(), rect.Height(), &MemDC, 0, 0, SRCCOPY);
	// 		MemBitmap.DeleteObject();

}


// CSe3DView 诊断

#ifdef _DEBUG
void Se3DView::AssertValid() const
{
	CView::AssertValid();
}

#ifndef _WIN32_WCE
void Se3DView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif
#endif //_DEBUG


// CSe3DView 消息处理程序


BOOL Se3DView::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	return TRUE;
}


BOOL Se3DView::PreTranslateMessage(MSG* pMsg)
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
 	return CView::PreTranslateMessage(pMsg);

}


void Se3DView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	// TODO: 在此处添加消息处理程序代码
	m_glMainHandle.Resize(cx, cy);
}





void Se3DView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_ptOri = point;
	m_ptStart = point;
	if((nFlags & MK_CONTROL) == MK_CONTROL)
	{
		m_bCtrlDown = TRUE;
		m_glMainHandle.SetLineShow(TRUE);
	}
	else 
	{
		m_bCtrlDown = FALSE;
		m_glMainHandle.SetLineShow(FALSE);
	}
	CView::OnLButtonDown(nFlags, point);
}


void Se3DView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_bCtrlDown = FALSE;
	if((nFlags & MK_CONTROL) == MK_CONTROL)
	{
		SendLineInfo(m_ptStart, point, FALSE);
		m_glMainHandle.SetLineShow(TRUE);
	}
	CView::OnLButtonUp(nFlags, point);
}


BOOL Se3DView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_pGLCamera->Scale(zDelta);
	m_glMainHandle.SetLineShow(FALSE);
 	return CView::OnMouseWheel(nFlags, zDelta, pt);
}


void Se3DView::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if(nFlags & MK_LBUTTON)
	{
		if (m_bCtrlDown)
			SendLineInfo(m_ptStart, point, TRUE);
		else
			m_pGLCamera->Move(m_ptOri.x-point.x, m_ptOri.y-point.y, 0, TRUE);
		m_ptOri = point;
	}
	if (nFlags & MK_RBUTTON)
	{
		if (m_bShiftDown)
			m_pGLCamera->Rotate(m_ptRightOri.x-point.x, m_ptRightOri.y-point.y, 0, TRUE);
		else 
			m_pGLCamera->Rotate(m_ptRightOri.x-point.x, m_ptRightOri.y-point.y, 0, FALSE);
		m_ptRightOri = point;
	}
	CView::OnMouseMove(nFlags, point);
}


void Se3DView::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_ptRightOri = point;
	if((nFlags & MK_SHIFT) == MK_SHIFT)
	{
		m_bShiftDown = TRUE;
	}
	else 
	{
		m_bShiftDown = FALSE;
	}
	m_glMainHandle.SetLineShow(FALSE);
	CView::OnRButtonDown(nFlags, point);
}


void Se3DView::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_bShiftDown = FALSE;
	CView::OnRButtonUp(nFlags, point);
}


int Se3DView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码
	m_pDc = new CClientDC(this);
	mhContext = m_pDc->GetSafeHdc();
	if( !m_glMainHandle.Initialize( mhContext, m_pGLDataMgr, m_pGLCamera, m_pGLShader ))
	{
		::MessageBox( 0, _T( "Failed to initialze the renderer !"), _T( "Renderer" ), MB_OK );
		exit(0);
	}
	SetTimer(0, 100, NULL);
	return 0;
}




void Se3DView::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ( !IsIconic())
	{
		CRect rect;
		GetClientRect(&rect);
		if (rect.Width() == 0 || rect.Height() == 0)
			return;
		// send info for draw ruler
		SendRulerInfo();

		m_glMainHandle.Render(rect.Width(), rect.Height());
		SwapBuffers(wglGetCurrentDC());

		if (commands.size() != 0)
		{
			ACTION_INFO info = commands.front();
			RunCommand(info);
			commands.pop();
		}

		Invalidate(FALSE);
		m_pGLCamera->MoveOrRotate();
	}

	CView::OnTimer(nIDEvent);
}


void Se3DView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	// TODO: 在此添加专用代码和/或调用基类
}

void Se3DView::InitData()
{
	CRect rect;
	GetClientRect(&rect);
	m_pGLDataMgr->InitFace2DTex(rect.Width(), rect.Height());
	//m_pGLDataMgr->InitFace2DTexForFreeCut(rect.Width(), rect.Height());
	m_pGLDataMgr->InitFrameBuffer(rect.Width(), rect.Height());
	//m_pGLDataMgr->InitFrameBufferForFreeCut(rect.Width(), rect.Height());
}

void Se3DView::AddVolTex(int nWidth, int nHeight, int nLength, BYTE* pData, COLORREF color)
{
	CRect rect;
	GetClientRect(&rect);
	m_pGLDataMgr->InitFace2DTex(rect.Width(), rect.Height());
	m_pGLDataMgr->InitFrameBuffer(rect.Width(), rect.Height());
	BYTE* pDeliteData = new BYTE[(ULONG)(nWidth+2) * (ULONG)(nHeight+2) * (ULONG)(nLength+2)];
	memset(pDeliteData, 0, NULL);
	DeliteData(pData, pDeliteData, nWidth, nHeight, nLength);
	m_pGLDataMgr->AddVol3DTex((GLuint)nWidth + 2, (GLuint)nHeight + 2, (GLuint)nLength + 2, (GLbyte*)pDeliteData, GL_LUMINANCE, GL_UNSIGNED_BYTE, color, TRUE);
	delete pDeliteData;
}

BOOL Se3DView::AddVolTex(COLORREF color)
{
	CRect rect;
	GetClientRect(&rect);
	m_pGLDataMgr->InitFace2DTex(rect.Width(), rect.Height());
	m_pGLDataMgr->InitFrameBuffer(rect.Width(), rect.Height());
	return m_pGLDataMgr->AddVol3DTex(color, TRUE);

}

void Se3DView::RemoveVolTex(int nPos, bool bRayCasting/* = TRUE*/)
{
	if (bRayCasting)
	{
		m_pGLDataMgr->DeleteVolTexObj(m_pGLDataMgr->GetVolTexList()[nPos].id);
	}
	else
	{
		m_pGLDataMgr->DeleteVolTexObj2(m_pGLDataMgr->GetVolTexList2()[nPos].id);
	}
	
}

void Se3DView::ChangeColor(int nRow, COLORREF color, bool bRayCasting/* = TRUE*/)
{
	if (bRayCasting)
	{
		UINT id = m_pGLDataMgr->GetVolTexList()[nRow].id; 
		m_pGLDataMgr->ChangeVolTexObjColor(id, color);
	}
	else
	{
		UINT id = m_pGLDataMgr->GetVolTexList2()[nRow].id; 
		m_pGLDataMgr->ChangeVolTexObjColor2(id, color);
	}
}

COLORREF Se3DView::GetColor(int nRow, bool bRayCasting/* = TRUE*/)
{
	if (bRayCasting)
	{
		UINT id = m_pGLDataMgr->GetVolTexList()[nRow].id; 
		return m_pGLDataMgr->GetColor(id);
	}
	else
	{
		UINT id = m_pGLDataMgr->GetVolTexList2()[nRow].id; 
		return m_pGLDataMgr->GetColor2(id);
		//return RGB(0,0,0);
	}

}




void Se3DView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	static BOOL bShowAll = TRUE;
	if (bShowAll)
	{
		GetParent()->GetParent()->SendMessage(WM_3D_STATE, SHOW_3D);
	}
	else
	{
		GetParent()->GetParent()->SendMessage(WM_3D_STATE, SHOW_ALL);
	}
	bShowAll = !bShowAll;
	CView::OnLButtonDblClk(nFlags, point);
}

void Se3DView::DeliteData(BYTE* pSrc, BYTE* pRst, int nWidth, int nHeight, int nLength)
{
	BYTE* pHead = pRst;
	BYTE* pSrcHead = pSrc;
	for (int i=0; i<nLength; i++)
	{
		for (int j=0; j<nHeight; j++)
		{
			for (int k=0; k<nWidth; k++)
			{
				pHead[(ULONG)(nWidth+2) * (ULONG)(nHeight + 2) * (ULONG)(i+1) + (ULONG)(nWidth + 2) * (ULONG)(j+1) + (ULONG)k+1] = pSrcHead[(ULONG)nWidth * (ULONG)nHeight * (ULONG)i + (ULONG)nWidth * (ULONG)j + (ULONG)k];
			}
		}
	}
}

void Se3DView::Reset()
{
	m_pGLDataMgr->DeleteAllVolTexObj();
	m_pGLDataMgr->DeleteAllVolTexObj2();
	m_pGLCamera->ResetAll();
}

void Se3DView::ChangeVisible(int nRow, BOOL show, bool bRayCasting/* = TRUE*/)
{
	if (bRayCasting)
	{
		UINT id = m_pGLDataMgr->GetVolTexList()[nRow].id; 
		m_pGLDataMgr->ChangeVolTexObjVisible(id, show);
	}
	else
	{
		UINT id = m_pGLDataMgr->GetVolTexList2()[nRow].id; 
		m_pGLDataMgr->ChangeVolTexObjVisible2(id, show);
	}

}

void Se3DView::SetLightPos(float fX, float fY, float fZ)
{
	m_pGLCamera->SetLightPos(fX, fY, fZ);
}

void Se3DView::SetShadow(float shadow)
{
	m_pGLCamera->SetShadow(shadow);
}

void Se3DView::NeedLight(bool needed)
{
	m_pGLCamera->NeedLight(needed);
}

void Se3DView::NeedShadow(bool needed)
{
	m_pGLCamera->NeedShadow(needed);
}

void Se3DView::SetLightColor(COLORREF color)
{
	m_pGLCamera->SetLightColor(color);
}

void Se3DView::SetMaterialColor(COLORREF color)
{
	m_pGLCamera->SetMaterialColor(color);
}

void Se3DView::SetLightMat(float emission, float diffuse, float reflect, float specular)
{
	m_pGLCamera->SetLightMat(COpenGLCamera::EMISSION, emission);
	m_pGLCamera->SetLightMat(COpenGLCamera::DIFFUSE, diffuse);
	m_pGLCamera->SetLightMat(COpenGLCamera::REFLECT, reflect);
	m_pGLCamera->SetLightMat(COpenGLCamera::SPECULAR, specular);
}

void Se3DView::SetViewPos(float fX, float fY, float fZ)
{
	m_pGLCamera->SetCameraPos(fX, fY, fZ);
}

void Se3DView::SetSpeed(int nSpeed, SPEED_TYPE type)
{
	m_pGLCamera->SetSpeed((GLuint)type, nSpeed);
}

void Se3DView::SetBkGndColor(COLORREF color)
{
	m_pGLCamera->SetBackgroundColor(color);
}

HBITMAP Se3DView::GetScreenImage(LPRECT lpRect)
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

BOOL Se3DView::SaveImageToFile(HBITMAP hBitmap, LPCTSTR lpFileName)
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

void Se3DView::CreateFolder(CString csPath)
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

void Se3DView::SendRulerInfo()
{
	float fScale = m_pGLCamera->GetScale();
	float fmmPerPixel = theGeneralSwapData.m_dbXYSliceSpace;
   	CRect rect;
	GetClientRect(&rect);
	//float fRulerScale = (float)theGeneralSwapData.m_nWidth / (float)rect.Height() * 2.0f * fScale;
	float fWidth = theGeneralSwapData.m_nWidth > theGeneralSwapData.m_nHeight ? (float)theGeneralSwapData.m_nWidth : (float)theGeneralSwapData.m_nHeight;
	float fOrders = fmmPerPixel * 800.0f * fScale * fWidth / ((float)rect.Width() / 2);
	CString str;
	float fScale2;
	if (fOrders <= 10.0f)
	{
		int nOrder = static_cast<int>(fOrders);
		fScale2 = nOrder * 1.0f / fOrders;
		str.Format("%d00um", nOrder);
		
	}
	else if (fOrders > 10.0f && fOrders <= 100.0f)
	{
		int nOrder = static_cast<int>(fOrders / 10.0f);
		fScale2 = nOrder * 10.0f / fOrders;
		str.Format("%dmm", nOrder);
	}
	else if (fOrders > 100.0f)
	{
		int nOrder = static_cast<int>(fOrders / 100.0f);
		fScale2 = nOrder * 100.0f / fOrders;
		str.Format("%dcm", nOrder);
	}
	else
	{
		str = "";
		fScale2 = 1.0f;
	}
	m_glMainHandle.SetRulerRenderInfo(str.GetBuffer(0), fScale2);
}

void Se3DView::SendLineInfo(CPoint ptStart, CPoint ptEnd, BOOL Moving/* = TRUE*/)
{
	float fPixelLength = sqrt(static_cast<float>((ptEnd.y - ptStart.y) * (ptEnd.y - ptStart.y) + (ptEnd.x - ptStart.x) * (ptEnd.x - ptStart.x)));
	float fScale = m_pGLCamera->GetScale();
	float fmmPerPixel = theGeneralSwapData.m_dbXYSliceSpace;
	float fWidth = theGeneralSwapData.m_nWidth > theGeneralSwapData.m_nHeight ? (float)theGeneralSwapData.m_nWidth : (float)theGeneralSwapData.m_nHeight;
	CRect rect;
	GetClientRect(&rect);
	float fLength = fmmPerPixel * fPixelLength * fScale * fWidth / ((float)rect.Width() / 2);
	CString strLength = CSeToolKit::float2CStr(fLength) + "mm";
	m_glMainHandle.SetLineRenderInfo(strLength.GetBuffer(0), 
		static_cast<float>(ptStart.x),
		static_cast<float>(ptEnd.x),
		static_cast<float>(ptStart.y),
		static_cast<float>(ptEnd.y), Moving);
}

void Se3DView::GetPrintScreen(CString strPath)
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

void Se3DView::ChangeShowState(SHOW_STATE state)
{
	GetParent()->GetParent()->SendMessage(WM_3D_STATE, state);
}

void Se3DView::SetRenderMode(BOOL bRayCasting /*= TRUE*/)
{
	m_glMainHandle.SetRenderMode(bRayCasting);
}

void Se3DView::InitVolumeData(CDcmPicArray* pDcmArray)
{
	int nWidth = pDcmArray->GetDcmArray()[0]->GetWidth();
	int nHeight = pDcmArray->GetDcmArray()[0]->GetHeight();
	int nLength = (int)pDcmArray->GetDcmArray().size();
	float pixelSize = pDcmArray->GetDcmArray()[0]->GetMMPerpixel();
	float pixelSpacing = pDcmArray->GetDcmArray()[0]->GetSliceSpace();
	unsigned short* pData = new unsigned short[(LONGLONG)nWidth * (LONGLONG)nHeight * (LONGLONG)nLength];
	memset(pData, 0, sizeof(unsigned short) * (LONGLONG)nWidth * (LONGLONG)nHeight * (LONGLONG)nLength);
	unsigned short* pHead = pData;
	for (int i=0; i<nLength; i++)
	{
		short* pTmp = (short*)pDcmArray->GetDcmArray()[i]->GetData();
		for (LONGLONG j=0; j<nWidth*nHeight; j++)
		{
			int x = *pTmp++ + 32768;
			*pHead++ = x;
		}
	}
	m_pGLDataMgr->SetPixelScale(pixelSpacing/ pixelSize);
	m_pGLDataMgr->InitVolumeTexture(nWidth, nHeight, nLength, pData, GL_LUMINANCE, GL_SHORT);
	
	//m_pGLDataMgr->InitVolumeTextureTest(nWidth, nHeight, nLength, p, GL_LUMINANCE, GL_SHORT);
	delete pData;
}

void Se3DView::ReleaseVolumeData()
{
	m_pGLDataMgr->DeleteVolumeTexture();
	m_pGLDataMgr->DeleteAllVolTexObj2();
}

void Se3DView::DrawRuler(CDC *pDC)
{
	CRect rect;
	GetClientRect(&rect);
	Graphics gc(pDC->GetSafeHdc());
	float fRulerScale = m_pGLCamera->GetScale();
	FontFamily fontFamily(L"幼圆");
	Gdiplus::Font font(&fontFamily, 24, FontStyleRegular, UnitPixel);
	LOGFONT lf;
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
	memcpy(lf.lfFaceName, "Arial", 5) ;
	lf.lfHeight = 25 ;
	Gdiplus::Font Gdi_font(mhContext, &lf);
	SolidBrush brush(Color(255, 255, 255, 255));
	

	float fScale = m_pGLCamera->GetScale();
	float fmmPerPixel = theGeneralSwapData.m_dbXYSliceSpace;
	//float fRulerScale = (float)theGeneralSwapData.m_nWidth / (float)rect.Height() * 2.0f * fScale;
	float fWidth = theGeneralSwapData.m_nWidth > theGeneralSwapData.m_nHeight ? (float)theGeneralSwapData.m_nWidth : (float)theGeneralSwapData.m_nHeight;
	int nOrders = (int)(fmmPerPixel * 1000.0f * fScale * (float)rect.Height() / fWidth);
	CStringW str;
	PointF ptRuler[4];
	if (nOrders <= 10)
	{
		int nOrder = nOrders;
		ptRuler[0] =  PointF(rect.right - 30.0f - nOrder * 100.0f / nOrders, rect.bottom - 35.0f);
		ptRuler[1] =  PointF(rect.right - 30.0f - nOrder * 100.0f / nOrders, rect.bottom - 30.0f);
		ptRuler[3] =  PointF(rect.right - 30.0f, rect.bottom - 35.0f);
		ptRuler[2] =  PointF(rect.right - 30.0f, rect.bottom - 30.0f);
		str.Format(L"%d00um", nOrder);
	}
	else if (nOrders > 10 && nOrders <= 100)
	{
		int nOrder = nOrders / 10;
		ptRuler[0] =  PointF(rect.right - 30.0f - nOrder * 1000.0f / nOrders, rect.bottom - 35.0f);
		ptRuler[1] =  PointF(rect.right - 30.0f - nOrder * 1000.0f / nOrders, rect.bottom - 30.0f);
		ptRuler[3] =  PointF(rect.right - 30.0f, rect.bottom - 35.0f);
		ptRuler[2] =  PointF(rect.right - 30.0f, rect.bottom - 30.0f);
		str.Format(L"  %dmm", nOrder);
	}
	else if (nOrders > 100)
	{
		int nOrder = nOrders / 100;
		ptRuler[0] =  PointF(rect.right - 30.0f - nOrder * 10000.0f / nOrders, rect.bottom - 35.0f);
		ptRuler[1] =  PointF(rect.right - 30.0f - nOrder * 10000.0f / nOrders, rect.bottom - 30.0f);
		ptRuler[3] =  PointF(rect.right - 30.0f, rect.bottom - 35.0f);
		ptRuler[2] =  PointF(rect.right - 30.0f, rect.bottom - 30.0f);
		str.Format(L"  %dcm", nOrder);
	}

	Pen pen(Color(255, 255, 255, 255), 1.0f);
	gc.DrawLines(&pen, &ptRuler[0], 4);

	gc.DrawString(str, -1, &font, PointF(rect.right - 100.0f, rect.bottom - 72.0f), &brush);
}

void Se3DView::ShowBorder(BOOL bShowBorder /*= FALSE*/)
{
	m_glMainHandle.ShowBorder(bShowBorder);
}



void Se3DView::RunCommand(ACTION_INFO info)
{
	CString type = info.type;
	if (type == "rotate")
	{
		m_pGLCamera->SetRotate(static_cast<GLfloat>(info.xpos), static_cast<GLfloat>(info.ypos), static_cast<GLfloat>(info.zpos));
	}
	else if (type == "scale")
	{
		m_pGLCamera->SetScale(info.scale);
	}
	else if (type == "cut")
	{
		m_pGLDataMgr->AdjustFreeCutVAO(
			-0.5f,
			static_cast<GLfloat>(info.fxpos),
			-0.5f,
			static_cast<GLfloat>(info.fypos),
			-0.5f,
			static_cast<GLfloat>(info.fzpos)
			);
	}
}

void Se3DView::PushRotateCommand(int xstart, int xend, int ystart, int yend, int zstart, int zend, int during)
{
	int step = during * 10;
	double xStep = (xend - xstart) / static_cast<double>(step);
	double yStep = (yend - ystart) / static_cast<double>(step);
	double zStep = (zend - zstart) / static_cast<double>(step);
	ACTION_INFO info;
	for(int i=0; i<step; i++)
	{	
		info.xpos = static_cast<int>(xStep * i) + xstart;
		info.ypos = static_cast<int>(yStep * i) + ystart;
		info.zpos = static_cast<int>(zStep * i) + zstart;
		info.type = "rotate";
		commands.push(info);
	}
}

void Se3DView::PushScaleCommand(double start, double end, int during)
{
	int step = during * 10;
	double stepsize = (end - start) / static_cast<double>(step);
	for(int i=0; i<step; i++)
	{
		ACTION_INFO info;
		info.scale = start + stepsize * i;
		info.type = "scale";
		commands.push(info);
	}
}

void Se3DView::PushCutCommand(float xstart, float xend, float ystart, float yend, float zstart, float zend, int during)
{
	int step = during * 10;
	double xStep = (xend - xstart) / static_cast<double>(step);
	double yStep = (yend - ystart) / static_cast<double>(step);
	double zStep = (zend - zstart) / static_cast<double>(step);
	for(int i=0; i<step; i++)
	{
		ACTION_INFO info;
		info.fxpos = xstart + static_cast<float>(xStep * i);
		info.fypos = ystart + static_cast<float>(yStep * i);
		info.fzpos = zstart + static_cast<float>(zStep * i);
		info.type = "cut";
		commands.push(info);
	}
}

void Se3DView::SetTranslate( int nRow, int nTranslate, bool bRayCasting/* = TRUE */)
{
	if (bRayCasting)
	{
		UINT id = m_pGLDataMgr->GetVolTexList()[nRow].id;
		m_pGLDataMgr->ChangeVolTexObjTranslate(id, static_cast<float>(nTranslate) / 100.0);
	}
	else
	{
		UINT id = m_pGLDataMgr->GetVolTexList2()[nRow].id;
		m_pGLDataMgr->ChangeVolTexObjTranslate2(id, static_cast<float>(nTranslate) / 100.0);
	}

}

int Se3DView::GetTranslate( int nRow, bool bRayCasting/* = TRUE */)
{
	if (bRayCasting)
	{
		UINT id = m_pGLDataMgr->GetVolTexList()[nRow].id; 
		return m_pGLDataMgr->GetTranslate(id);

	}
	else
	{
		UINT id = m_pGLDataMgr->GetVolTexList2()[nRow].id; 
		return m_pGLDataMgr->GetTranslate2(id);
	}

}




