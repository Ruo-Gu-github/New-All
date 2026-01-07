#pragma once

// SeExampleView 视图

class SeExampleView : public CImageViewerView
{
	DECLARE_DYNCREATE(SeExampleView)

protected:
	SeExampleView();           // 动态创建所使用的受保护的构造函数
	virtual ~SeExampleView();

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
	const  void     Reset();
	const  void     SetSelect(int n) { m_nSelect = n;}
	void            SetInfo(int n1, int n2, DWORD n3, int n4, int n5);
private:
	Bitmap*				GreatePng(CImageBase* pImg, int nMin, int nMax, COLORREF color, DWORD alpha);

	CDcmPicArray*       m_pDcmPicArray;
	int                 m_nSelect;
	int                 m_nFatMin;
	int                 m_nFatMax;
	int                 m_nLungMin;
	int                 m_nLungMax;
	int                 m_nBoneMin;
	int                 m_nBoneMax;
	int                 m_nNowPos;
};


