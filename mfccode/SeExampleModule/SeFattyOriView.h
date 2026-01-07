#pragma once


// CSeFattyOriView 视图

class CSeFattyOriView : public CImageViewerView
{
	DECLARE_DYNCREATE(CSeFattyOriView)

protected:
	CSeFattyOriView();           // 动态创建所使用的受保护的构造函数
	virtual ~CSeFattyOriView();

public:
	CDcmPicArray*	m_pDcmPicArray;
	BOOL            m_bDrawWindowName;

public:
	void	SetDcmArray(CDcmPicArray* pDcmPicArray);
	const void Reset();
	void    DrawWindowName(CDC* pDC);
	HBITMAP		GetScreenImage(LPRECT lpRect);
public:
	virtual void OnDraw(CDC* pDC);      // 重写以绘制该视图
#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif

protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual void OnInitialUpdate();
};


