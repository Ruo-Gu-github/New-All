// SeBoneParamSelection.cpp : 实现文件
//

#include "stdafx.h"
#include "afxdialogex.h"
#include "SeBoneParamSelectionDlg.h"
#include "SeSetDensityParamDlg.h"
// SeBoneParamSelection 对话框

IMPLEMENT_DYNAMIC(SeBoneParamSelection, CDialog)

SeBoneParamSelection::SeBoneParamSelection(CWnd* pParent /*=NULL*/)
	: CDialog(SeBoneParamSelection::IDD, pParent)
{
	m_brush.CreateSolidBrush(RGB(106,112,128));
	m_pParent = pParent;

	m_nSmall = 0;
	m_nBig = 0;
	m_fSamllDensity = 0;
	m_fBigDensity = 0;
}

SeBoneParamSelection::~SeBoneParamSelection()
{
	m_pParent = NULL;
}

void SeBoneParamSelection::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(SeBoneParamSelection, CDialog)
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BONE_THICK, &SeBoneParamSelection::OnBnClickedBoneThick)
	ON_BN_CLICKED(IDC_BONE_SPACE, &SeBoneParamSelection::OnBnClickedBoneSpace)
	ON_BN_CLICKED(IDC_BONE_NUMBER, &SeBoneParamSelection::OnBnClickedBoneNumber)
	ON_BN_CLICKED(IDC_BONE_CONNECT, &SeBoneParamSelection::OnBnClickedBoneConnect)
	ON_BN_CLICKED(IDC_BONE_SMI, &SeBoneParamSelection::OnBnClickedBoneSmi)
	ON_BN_CLICKED(IDC_BONE_TBPF, &SeBoneParamSelection::OnBnClickedBoneTbpf)
	ON_BN_CLICKED(IDC_BONE_DA, &SeBoneParamSelection::OnBnClickedBoneDa)
	ON_BN_CLICKED(IDOK, &SeBoneParamSelection::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BONE_DENSITY, &SeBoneParamSelection::OnBnClickedBoneDensity)
	ON_BN_CLICKED(IDC_BONE_CORTICAL_AREA, &SeBoneParamSelection::OnBnClickedBoneCorticalArea)
	ON_BN_CLICKED(IDC_BONE_CORTICAL_THICKNESS, &SeBoneParamSelection::OnBnClickedBoneCorticalThickness)
	ON_BN_CLICKED(IDC_BONE_CORTICAL_FRACTION, &SeBoneParamSelection::OnBnClickedBoneCorticalFraction)
	ON_BN_CLICKED(IDC_BONE_STRUCTURE_AREA, &SeBoneParamSelection::OnBnClickedBoneStructureArea)
	ON_BN_CLICKED(IDC_BONE_CORTICAL_DENSITY, &SeBoneParamSelection::OnBnClickedBoneCorticalDensity)
END_MESSAGE_MAP()


BOOL SeBoneParamSelection::OnInitDialog()
{
	CDialog::OnInitDialog();

	return TRUE; 
}


BOOL SeBoneParamSelection::OnEraseBkgnd(CDC* pDC)
{
	CRect rect;
	GetClientRect(&rect);
	CZKMemDC memDC(pDC);
	memDC.FillSolidRect(rect,RGB(106,112,128));
	return TRUE;
	//return CDialog::OnEraseBkgnd(pDC);
}


HBRUSH SeBoneParamSelection::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	switch(nCtlColor)  
	{  
	case CTLCOLOR_STATIC: 
		{  
			pDC->SetTextColor(RGB(0,0,0)); 
			pDC->SetBkMode(TRANSPARENT);
			return (HBRUSH)m_brush.GetSafeHandle();
			break;  
		}  
	case CTLCOLOR_BTN: 
		{
			return (HBRUSH)m_brush.GetSafeHandle();
			break;  
		}

	default:  
		break;  
	}  
	return hbr;
}


void SeBoneParamSelection::OnBnClickedBoneThick()
{
	// TODO: 在此添加控件通知处理程序代码
	ParamSelection.b_BoneThick = !ParamSelection.b_BoneThick;
}


void SeBoneParamSelection::OnBnClickedBoneSpace()
{
	// TODO: 在此添加控件通知处理程序代码
	ParamSelection.b_SpaceThick = !ParamSelection.b_SpaceThick;
}


void SeBoneParamSelection::OnBnClickedBoneNumber()
{
	// TODO: 在此添加控件通知处理程序代码
	ParamSelection.b_BoneNumber = !ParamSelection.b_BoneNumber;
}


void SeBoneParamSelection::OnBnClickedBoneConnect()
{
	// TODO: 在此添加控件通知处理程序代码
	ParamSelection.b_Connectivity = !ParamSelection.b_Connectivity;
}


void SeBoneParamSelection::OnBnClickedBoneSmi()
{
	// TODO: 在此添加控件通知处理程序代码
	ParamSelection.b_SMI = !ParamSelection.b_SMI;
}


void SeBoneParamSelection::OnBnClickedBoneTbpf()
{
	// TODO: 在此添加控件通知处理程序代码
	ParamSelection.b_TBPf = !ParamSelection.b_TBPf;
}


void SeBoneParamSelection::OnBnClickedBoneDa()
{
	// TODO: 在此添加控件通知处理程序代码
	ParamSelection.b_DA = !ParamSelection.b_DA;
}


void SeBoneParamSelection::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码

	CDialog::OnOK();
}


void SeBoneParamSelection::OnBnClickedBoneDensity()
{
	ParamSelection.b_Density = !ParamSelection.b_Density;
	// TODO: 在此添加控件通知处理程序代码
	if (ParamSelection.b_Density)
	{
		CSeSetDensityParamDlg dlg;
		if (m_nSmall != 0 || m_nBig != 0 || m_fSamllDensity > 0.0 || m_fBigDensity > 0.0)
			dlg.SetInfo(m_nSmall, m_nBig, m_fSamllDensity, m_fBigDensity);
		if (IDOK == dlg.DoModal())
		{
			ParamSelection.densityInfo = Density_Info(
				dlg.GetSmallValue(),
				dlg.GetBigValue(),
				dlg.GetSmallDensity(),
				dlg.GetBigDensity());
		}
		else
		{
			ParamSelection.b_Density = FALSE;
			((CButton*)GetDlgItem(IDC_BONE_DENSITY))->SetCheck(FALSE);
			ParamSelection.densityInfo = Density_Info(0, 0, 0.0, 0.0);
		}

		m_nSmall = dlg.GetSmallValue();
		m_nBig = dlg.GetBigValue();
		m_fSamllDensity = dlg.GetSmallDensity();
		m_fBigDensity = dlg.GetBigDensity();
	}
}

void SeBoneParamSelection::OnBnClickedBoneCorticalArea()
{
	// TODO: 在此添加控件通知处理程序代码
	ParamSelection.b_CorticalArea = !ParamSelection.b_CorticalArea;
}


void SeBoneParamSelection::OnBnClickedBoneCorticalThickness()
{
	// TODO: 在此添加控件通知处理程序代码
	ParamSelection.b_CorticalThick = !ParamSelection.b_CorticalThick;
}


void SeBoneParamSelection::OnBnClickedBoneCorticalFraction()
{
	// TODO: 在此添加控件通知处理程序代码
	ParamSelection.b_CorticalFraction = !ParamSelection.b_CorticalFraction;
}


void SeBoneParamSelection::OnBnClickedBoneStructureArea()
{
	// TODO: 在此添加控件通知处理程序代码
	ParamSelection.b_MedullaryArea = !ParamSelection.b_MedullaryArea;
}


void SeBoneParamSelection::OnBnClickedBoneCorticalDensity()
{
	// TODO: 在此添加控件通知处理程序代码
	ParamSelection.b_CorticalDensity = !ParamSelection.b_CorticalDensity;

	if (ParamSelection.b_CorticalDensity)
	{
		CSeSetDensityParamDlg dlg;
		if (m_nSmall != 0 || m_nBig != 0 || m_fSamllDensity > 0.0 || m_fBigDensity > 0.0)
			dlg.SetInfo(m_nSmall, m_nBig, m_fSamllDensity, m_fBigDensity);
		if (IDOK == dlg.DoModal())
		{
			ParamSelection.densityInfo = Density_Info(
				dlg.GetSmallValue(),
				dlg.GetBigValue(),
				dlg.GetSmallDensity(),
				dlg.GetBigDensity());
		}
		else
		{
			ParamSelection.b_CorticalDensity = FALSE;
			((CButton*)GetDlgItem(IDC_BONE_DENSITY))->SetCheck(FALSE);
			ParamSelection.densityInfo = Density_Info(0, 0, 0.0, 0.0);
		}

		m_nSmall = dlg.GetSmallValue();
		m_nBig = dlg.GetBigValue();
		m_fSamllDensity = dlg.GetSmallDensity();
		m_fBigDensity = dlg.GetBigDensity();
	}
}

void SeBoneParamSelection::SetInfo(int nSmall, int nBig, float fSmallDensity, float fBigDensity)
{
	m_nSmall = nSmall;
	m_nBig = nBig;
	m_fSamllDensity = fSmallDensity;
	m_fBigDensity =fBigDensity;
}

void SeBoneParamSelection::GetInfo(int& nSmall, int& nBig, float& fSmallDensity, float& fBigDensity)
{
	nSmall = m_nSmall;
	nBig = m_nBig;
	fSmallDensity = m_fSamllDensity;
	fBigDensity = m_fBigDensity;
}


