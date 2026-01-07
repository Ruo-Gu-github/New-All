#pragma once


// SeBinaryView 视图

class SeBinaryView : public CImageViewerView
{
	DECLARE_DYNCREATE(SeBinaryView)

protected:
	SeBinaryView();           // 动态创建所使用的受保护的构造函数
	virtual ~SeBinaryView();

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
	void            SetDcmArray(CDcmPicArray* pDcmArray);
	const  BOOL     ShowNewMask(){return m_bNewMask;}
	const  void     SetShowNewMask(BOOL b){m_bNewMask = b;}
	const  void     SetInfo(int n1, int n2, DWORD n3, int n4){m_nMin = n1; m_nMax = n2; m_color = n3, m_alpha = n4;}
	CDcmPicArray*	GetDcmArray(){return m_pDcmPicArray;}
	const  int      GetMin(){return m_nMin;}
	const  int      GetMax(){return m_nMax;}
	const  void     Reset();
	void			OnExportImage();

private:
	Bitmap*			GreatePng(CImageBase* pImg, int nMin, int nMax, COLORREF color, DWORD alpha);

private:
	CDcmPicArray*       m_pDcmPicArray;
	BOOL                m_bNewMask;
	BYTE*               m_pData;
	int                 m_nMin;
	int                 m_nMax;
	DWORD               m_color;
	int                 m_alpha;

};


