#pragma once
#include "resource.h"
#include "afxwin.h"
#include "SeBoneSegProcedureDlg.h"

// SeBoneDensityCtrlDlg 对话框
class SeBoneParamCaculate;
class CEasy3DViewThread;

class SeBoneDensityCtrlDlg : public CSeDialogBase
{
	DECLARE_DYNAMIC(SeBoneDensityCtrlDlg)
public:
	SeBoneDensityCtrlDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~SeBoneDensityCtrlDlg();
	
// 对话框数据
	enum { IDD = IDD_SEBONEDENSITYCTRLDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL			OnEraseBkgnd(CDC* pDC);
	afx_msg HBRUSH			OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void			OnSize(UINT nType, int cx, int cy);
	afx_msg int				OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void			OnTimer(UINT_PTR nIDEvent);
	afx_msg void			OnIdok();
	afx_msg void			OnBnClickedBoneChiptool();//选择区域
	afx_msg void			OnBnClickedBoneCreateseries();//继续圈定区域
	afx_msg void			OnBnClickedBoneDeleteSet();//裁剪内部或者裁剪外部
	afx_msg void			OnBnClickedBoneDrawedge();//圈定区域
	afx_msg void			OnBnClickedBoneClearedge();//重新圈定
	afx_msg void			OnBnClickedBoneGetedge();//绘制中间层曲线 
	afx_msg void			OnBnClickedBoneConfirmregion();//确定圈定区域
	afx_msg void			OnBnClickedBonePartbinary();//二值化
	afx_msg void			OnBnClickedBoneParametercalcu();//参数计算
	afx_msg void			OnBnClickedBoneDensity();//骨密度分析
	afx_msg void			OnBnClickedBoneLoadmodel();//读取模体
	afx_msg void			OnBnClickedBoneDensitycalcu();//骨密度计算
	afx_msg void			OnBnClickedBoneLaststep();//上一步
	afx_msg void			OnBnClickedBonePrintreport();//打印报告
	afx_msg void			OnBnClickedBoneParamSelect();//骨小梁分析
	afx_msg void			OnBnClickedBoneRectconfirm();//参数计算
	afx_msg void			OnBnClickedBoneStartdivision();//开始裁剪
	afx_msg void			OnBnClickedBoneClose();//闭运算
	afx_msg void			OnBnClickedExportExcel();//输出excel
	afx_msg void			OnBnClickedBoneCalculate();//计算
	afx_msg void			OnBnClickedBoneExportFile();//导出图像
	afx_msg void			OnBnClickedBoneShow3d();//显示3D
	afx_msg void			OnBnClickedBonePrint3d();//3D截图

	//分割松质骨
	afx_msg void			OnBnClickedBoneThre();//二值化
	afx_msg void			OnBnClickedBoneSegFunc();//皮质骨松质骨分割
	afx_msg void			OnBnClickedBoneConfirmregion2();//转到计算松质骨


	afx_msg void			OnEnChangeEditXs();
	afx_msg void			OnEnChangeEditXe();
	afx_msg void			OnEnChangeEditYs();
	afx_msg void			OnEnChangeEditYe();
	afx_msg void			OnEnChangeEditZs();
	afx_msg void			OnEnChangeEditZe();
	afx_msg void			OnEnKillfocusXs();
	afx_msg void			OnEnKillfocusXe();
	afx_msg void			OnEnKillfocusYs();
	afx_msg void			OnEnKillfocusYe();
	afx_msg void			OnEnKillfocusZs();
	afx_msg void			OnEnKillfocusZe();
	afx_msg void			OnEnSetfocusXs();
	afx_msg void			OnEnSetfocusXe();
	afx_msg void			OnEnSetfocusYs();
	afx_msg void			OnEnSetfocusYe();
	afx_msg void			OnEnSetfocusZs();
	afx_msg void			OnEnSetfocusZe();

// 自订消息
	afx_msg LRESULT         OnAddMask(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT			OnBoneSegmentation(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT			OnBoneSegReset(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT			OnBoneSegShowALL(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT			OnBoneSegFinished(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT			OnBoneSetShowMaskInfer(WPARAM wParam, LPARAM lParam);


private: 
	HBRUSH                  m_brush;
	CGroupControl           m_GroupSlice;
	int						m_nDeleteOutside;
	BOOL                    m_bSquare;
	BOOL                    m_bGetEage;
	BOOL                    m_bCaculateDirect;
	BOOL                    m_bBianry;
	int                     m_nEditID;
	BOOL                    m_bSetValue;
	BOOL					m_bSegFuncsUsed;
	SeBoneParamCaculate*    m_pResult;
	CEasy3DViewThread*      m_pThread1;
	CEasy3DViewThread*      m_pThread2;  
	
public:

	void					SetHighCTLevel(int nLevel);
	void					SetLowCTLevel(int nLevel);
	void					Reset();
	void					numOnly(int nID);
	void					GetValue(int nID, int nValue);
	int						SetValue(int nID);
	int						GetEditValue(int nID);
	void					SetLWH();
	void                    Corrosion(short *pData, int nWidth, int nHeight, int nCore);
	void                    Inflation(short *pData, int nWidth, int nHeight, int nCore);
	void                    ReturnToLastStep();
	int						Otsu(short *pData, int nWidth, int nHeight);
	
	afx_msg void OnBnClickedBoneExportFileEx();
	afx_msg void OnBnClickedExportPdf();
};
