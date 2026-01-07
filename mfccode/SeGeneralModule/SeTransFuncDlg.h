#pragma once
#include "Resource.h"

// CSeTransFuncDlg 对话框

struct TransFuncColorCtlPoint
{
	float		pos;
	COLORREF	color;
	BOOL		hit;
	TransFuncColorCtlPoint(float f, COLORREF c, BOOL b)
	{
		pos = f;
		color = c;
		hit = b;
	}
};

struct TransFuncLightnessCtlPoint
{
	float       ptX;
	float       ptY;
	BOOL        hit;
	TransFuncLightnessCtlPoint(float f1, float f2, BOOL b)
	{
		ptX = f1; 
		ptY = f2;
		hit = b;
	}
};

struct TransFuncSetting
{
	CString csSettingName;
	CString csSetttingPoints;
	CString csSettingColors;
	TransFuncSetting(CString str1, CString str2, CString str3)
	{
		csSettingName = str1;
		csSetttingPoints = str2;
		csSettingColors = str3;
	}
};


// 本对话框布局
// 本对话框分为上下两个部分
// 上面一个对话框距离上下左右边框距离分别为 15， 75， 15， 15
// 下面一个对话框距离上下左右距离为 75 - 15 - 5， 25， 15， 15


class CSeTransFuncDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSeTransFuncDlg)

public:
	CSeTransFuncDlg(CWnd* pParent = NULL);   // 标准构造函数
	CSeTransFuncDlg(LONG* pHistogram, LONG lMaxNumber, int nMax = 4096, int nMin = -1000, CWnd* pParent = NULL);
	virtual ~CSeTransFuncDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_TRANSFUNC };
	enum MouseStateInPlane {NOTHIT = 0, HITLINE, HITPOINT, HITCOLORLINE, HITCOLOR};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	CWnd* m_pParent;
	int m_nMin;
	int m_nMax;
	LONG* m_pHistogram;
	LONG m_lMaxNumber;
	vector <TransFuncColorCtlPoint> m_vecColorCtlPoints;
	vector <TransFuncLightnessCtlPoint> m_vecLightnessCtlPoints;

	BOOL m_bLBtnDown;
	MouseStateInPlane m_state;
	int               m_hitIndex;
	vector <TransFuncSetting> m_vecSetting;
	CRect  m_rtUpPlane;
	CRect  m_rtDownPlane;

private:
	COLORREF GetMixColor(TransFuncColorCtlPoint start, TransFuncColorCtlPoint end, int pos);
	void     LoadTransFuncSetting(CString csFile);
	void	 SetNewTransFuncSetting(CString strSettingName);
	void     SaveTransFuncSetting(CString csFile, CString strSettingName);
	void     RefreshTransFuncSetting();
	

public:
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnDeleteLinePoint();
	afx_msg void OnDeleteColorPoint();
	afx_msg void OnChangmeColor();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButtonSave();
	afx_msg void OnBnClickedButtonLoad();	
	afx_msg void OnEnChangeEditTransfuncName();
};
