// SeROIEditDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SeGeneralModule.h"
#include "SeROIEditDlg.h"
#include "afxdialogex.h"


// CSeROIEditDlg 对话框

IMPLEMENT_DYNAMIC(CSeROIEditDlg, CDialogEx)

CSeROIEditDlg::CSeROIEditDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSeROIEditDlg::IDD, pParent)
	, m_nOperation(0)
	, m_nDirection(0)
	, m_nShape(0)
{
	m_pParent = pParent;
}

CSeROIEditDlg::~CSeROIEditDlg()
{
}

void CSeROIEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO_ROI_ADD, m_nOperation);
	DDV_MinMaxInt(pDX, m_nOperation, 0, 2);
	DDX_Radio(pDX, IDC_RADIO_FRONT, m_nDirection);
	DDV_MinMaxInt(pDX, m_nDirection, 0, 2);
	DDX_Radio(pDX, IDC_RADIO_ROI_ANY, m_nShape);
	DDV_MinMaxInt(pDX, m_nShape, 0, 2);
}


BEGIN_MESSAGE_MAP(CSeROIEditDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSeROIEditDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_ROI_CLEAR, &CSeROIEditDlg::OnBnClickedButtonRoiClear)
	ON_BN_CLICKED(IDC_BUTTON_MID_LAYER, &CSeROIEditDlg::OnBnClickedButtonMidLayer)
	ON_BN_CLICKED(IDC_RADIO_FRONT, &CSeROIEditDlg::OnBnClickedRadioFront)
	ON_BN_CLICKED(IDC_RADIO_LEFT, &CSeROIEditDlg::OnBnClickedRadioFront)
	ON_BN_CLICKED(IDC_RADIO_TOP, &CSeROIEditDlg::OnBnClickedRadioFront)
	ON_BN_CLICKED(IDC_RADIO_ROI_ADD, &CSeROIEditDlg::OnBnClickedRadioRoiAdd)
	ON_BN_CLICKED(IDC_RADIO_ROI_REMOVE, &CSeROIEditDlg::OnBnClickedRadioRoiAdd)
	ON_BN_CLICKED(IDC_RADIO_ROI_ANY, &CSeROIEditDlg::OnBnClickedRadioRoiShape)
	ON_BN_CLICKED(IDC_RADIO_ROI_SQUARE, &CSeROIEditDlg::OnBnClickedRadioRoiShape)
	ON_BN_CLICKED(IDC_RADIO_ROI_ROUND, &CSeROIEditDlg::OnBnClickedRadioRoiShape)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CSeROIEditDlg 消息处理程序


void CSeROIEditDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	m_pParent->SendMessage(WM_EXECUTE_ROI);
	//CDialogEx::OnOK();
}


void CSeROIEditDlg::OnBnClickedButtonRoiClear()
{
	// TODO: 在此添加控件通知处理程序代码
	m_pParent->SendMessage(WM_DELETE_ROI);
}


void CSeROIEditDlg::OnBnClickedButtonMidLayer()
{
	// TODO: 在此添加控件通知处理程序代码
	m_pParent->SendMessage(WM_MID_LAYER);
}


void CSeROIEditDlg::OnBnClickedRadioFront()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_pParent->SendMessage(WM_CHANGE_PLANE_NUMBER);	
}


void CSeROIEditDlg::OnBnClickedRadioRoiAdd()
{
	// TODO: 在此添加控件通知处理程序代
	UpdateData(TRUE);
}

void CSeROIEditDlg::OnBnClickedRadioRoiShape()
{
	int nOldShape = m_nShape;
	if (!UpdateData(TRUE))
		return;
	if (m_pParent != NULL && nOldShape != m_nShape)
	{
		m_pParent->SendMessage(WM_ROI_SHAPE_CHANGED, static_cast<WPARAM>(m_nShape), static_cast<LPARAM>(nOldShape));
		m_pParent->SendMessage(WM_DELETE_ROI);
	}
}

const int CSeROIEditDlg::GetPlaneNumNow()
{
	UpdateData(TRUE);
	return m_nDirection + 1;
}

const int CSeROIEditDlg::GetOperation()
{
	UpdateData(TRUE);
	return m_nOperation;
}


void CSeROIEditDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_pParent->SendMessage(WM_SET_MOUSE_TOOL, MT_MPR, 0);
	CDialogEx::OnClose();
}
