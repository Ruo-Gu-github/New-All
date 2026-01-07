#pragma once
#include "Resource.h"
#include "afxwin.h"

// SeBoneParamSelection 对话框

struct Density_Info
{
	int nSmallValue;
	int nBigValue;
	float fSmallDensity;
	float fBigDensity;

	Density_Info (int n, int n2, float f, float f2)
	{
		nSmallValue = n;
		nBigValue = n2;
		fSmallDensity = f;
		fBigDensity = f2;
	}

	Density_Info()
	{
		nSmallValue = 0;
		nBigValue = 0;
		fSmallDensity = 0.0;
		fBigDensity = 0.0;
	}

	const void operator=(const Density_Info& d)
	{
		nSmallValue = d.nSmallValue;
		nBigValue = d.nBigValue;
		fSmallDensity = d.fSmallDensity;
		fBigDensity = d.fBigDensity;
		
	}
};

typedef struct BoneParameter
{
	BOOL b_BasicParameter;
	BOOL b_BoneThick;
	BOOL b_SpaceThick;
	BOOL b_BoneNumber;
	BOOL b_Connectivity;
	BOOL b_SMI;
	BOOL b_DA;
	BOOL b_TBPf;
	BOOL b_Density;
	BOOL b_CorticalArea;
	BOOL b_CorticalThick;
	BOOL b_CorticalFraction;
	BOOL b_MedullaryArea;
	BOOL b_CorticalDensity;
	Density_Info densityInfo;

	BoneParameter (
		BOOL BasicParameter = TRUE,
		BOOL BoneThick = FALSE,
		BOOL SpaceThick = FALSE,
		BOOL BoneNumber = FALSE,
		BOOL Connectivity = FALSE,
		BOOL SMI = FALSE,
		BOOL DA = FALSE,
		BOOL TBPf = FALSE,
		BOOL Density = FALSE,
		BOOL CorticalArea = FALSE,
		BOOL CorticalThick = FALSE,
		BOOL CorticalFraction = FALSE,
		BOOL MedullaryArea = FALSE,
		BOOL CorticalDensity = FALSE,
		Density_Info Info = Density_Info(0, 0, 0.0, 0.0))
	{
		b_BasicParameter = BasicParameter;
		b_BoneThick = BoneThick;
		b_SpaceThick = SpaceThick;
		b_BoneNumber = BoneNumber;
		b_Connectivity = Connectivity;
		b_SMI = SMI;
		b_DA = DA;
		b_TBPf = TBPf;
		b_Density = Density;
		b_CorticalArea = CorticalArea;
		b_CorticalThick = CorticalThick;
		b_CorticalFraction = CorticalFraction;
		b_MedullaryArea = MedullaryArea;
		b_CorticalDensity = CorticalDensity;
		densityInfo = Info;
	}
}Parameter;

class SeBoneParamSelection : public CDialog
{
	DECLARE_DYNAMIC(SeBoneParamSelection)

public:
	SeBoneParamSelection(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~SeBoneParamSelection();

// 对话框数据
	enum { IDD = IDD_BONEPARAMETERCALC };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedBoneThick();
	afx_msg void OnBnClickedBoneSpace();
	afx_msg void OnBnClickedBoneNumber();
	afx_msg void OnBnClickedBoneConnect();
	afx_msg void OnBnClickedBoneSmi();
	afx_msg void OnBnClickedBoneTbpf();
	afx_msg void OnBnClickedBoneDa();
	afx_msg void OnBnClickedBoneDensity();
	afx_msg void OnBnClickedBoneCorticalArea();
	afx_msg void OnBnClickedBoneCorticalThickness();
	afx_msg void OnBnClickedBoneCorticalFraction();
	afx_msg void OnBnClickedBoneStructureArea();
	afx_msg void OnBnClickedBoneCorticalDensity();
	afx_msg void OnBnClickedOk();

	const   void			SetParent(CWnd* pParent){m_pParent = pParent;}
	const   BoneParameter	GetParam(){return ParamSelection;}

	void                    SetInfo(int nSmall, int nBig, float fSmallDensity, float fBigDensity);
	void                    GetInfo(int& nSmall, int& nBig, float& fSmallDensity, float& fBigDensity);

private:
	BoneParameter ParamSelection;
	CWnd*         m_pParent;
	CBrush		  m_brush;

	int           m_nSmall;
	int           m_nBig;
	float         m_fSamllDensity;
	float         m_fBigDensity;
};
