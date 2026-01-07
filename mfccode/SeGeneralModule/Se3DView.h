#pragma once
#include "SeMPRView.h"


// CSe3DView 视图

class Se3DView : public CView
{
	DECLARE_DYNCREATE(Se3DView)

protected:
	Se3DView();           // 动态创建所使用的受保护的构造函数
	virtual ~Se3DView();

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
	afx_msg int	OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);

	void InitData();
	void AddVolTex(int nWidth, int nHeight, int nLength, BYTE* pData, COLORREF color);
	BOOL AddVolTex(COLORREF color);
	void RemoveVolTex(int nPos, bool bRayCasting = TRUE);
	void Reset();
	void ChangeColor(int nRow, COLORREF color, bool bRayCasting = TRUE);
	void ChangeVisible(int nRow, BOOL show, bool bRayCasting = TRUE);
	void ChangeShowState(SHOW_STATE state);
	COLORREF GetColor(int nRow, bool bRayCasting = TRUE);
	int GetTranslate(int nRow, bool bRayCasting = TRUE);
	void SetViewPos(float fX, float fY, float fZ);
	void SetSpeed(int nSpeed, SPEED_TYPE type);
	void SetBkGndColor(COLORREF color);
	void SetTranslate(int nRow, int nTranslate, bool bRayCasting = TRUE);

	// light
	void SetLightColor(COLORREF color);
	void SetMaterialColor(COLORREF color);
	void SetLightMat(float emission, float diffuse, float reflect, float specular);
	void SetLightPos(float fx, float fy, float fz);
	void SetShadow(float shadow);
	void NeedLight(bool needed);
	void NeedShadow(bool needed);

	// 直接体绘制相关
	void SetRenderMode(BOOL bRayCasting = TRUE);
	void InitVolumeData(CDcmPicArray* pDcmArray);
	void ReleaseVolumeData();

	// FreeCut 相关
	void ShowBorder(BOOL bShowBorder = FALSE);

	// 录屏 相关
	void RunCommand(ACTION_INFO info);
	void PushRotateCommand(int xstart, int xend, int ystart, int yend, int zstart, int zend, int during);
	void PushScaleCommand(double start, double end, int during);
	void PushCutCommand(float xstart, float xend, float ystart, float yend, float zstart, float zend, int during);
private:
	queue<ACTION_INFO> commands;

	// 截图相关
public:
	void                GetPrintScreen(CString strPath);
private:
	HBITMAP				GetScreenImage(LPRECT lpRect);
	BOOL				SaveImageToFile( HBITMAP hBitmap, LPCTSTR lpFileName );
	void				CreateFolder(CString csPath );
	void                SendRulerInfo();
	void                SendLineInfo(CPoint ptStart, CPoint ptEnd, BOOL Moving = TRUE);

private:
	void DeliteData(BYTE* pSrc, BYTE* pRst, int nWidth, int nHeight, int nLength);

	// 标尺相关 not useful anymore, it cause a flash problem
private:
	void DrawRuler(CDC *pDC);

	//// 体绘制 多物体渲染 相关
	//void AddVolTex2(int nWidth, int nHeight, int nLength, unsigned short* pData, COLORREF color);
	//void RemoveVolTex2(int nPos);
public:


public:
	HDC				mhContext;
	COpenGLMain		m_glMainHandle;
	COpenGLShader*  m_pGLShader;
	COpenGLDataMgr* m_pGLDataMgr;
	COpenGLCamera*	m_pGLCamera;
	CPoint			m_ptOri;
	CPoint          m_ptRightOri;
	CPoint          m_ptStart;
	BOOL			m_bOpened;
	BOOL            m_bShiftDown;
	BOOL            m_bCtrlDown;

private:
	CClientDC*     m_pDc;
};


