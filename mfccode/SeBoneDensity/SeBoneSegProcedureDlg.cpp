// CSeBoneSegProcedureDlg.cpp : 实现文件

#include "stdafx.h"
#include "SeBoneSegProcedureDlg.h"
#include "SeBoneSegProcedureChangeDlg.h"
#include "afxdialogex.h"
#include "cJSON.h"
#include <iostream>
#include <fstream>
#include <sstream>


// CSeNewMaskDlg 对话框

IMPLEMENT_DYNAMIC(CSeBoneSegProcedureDlg, CDialogEx)

	CSeBoneSegProcedureDlg::CSeBoneSegProcedureDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSeBoneSegProcedureDlg::IDD, pParent)
{
	m_pParent = pParent;
}

CSeBoneSegProcedureDlg::CSeBoneSegProcedureDlg(vector<FuncSingle> *pvecFuncs, map<CString, int> *pmapFunc, CWnd* pParent /*= NULL*/)
	: CDialogEx(CSeBoneSegProcedureDlg::IDD, pParent)
{
	m_pvecFuncList = pvecFuncs;
	m_pParent = pParent;
	m_pmapFunc = pmapFunc;
}

CSeBoneSegProcedureDlg::~CSeBoneSegProcedureDlg()
{
}

void CSeBoneSegProcedureDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_BONE_CAL_PROCEDURE, m_ListCtrBoneSeg);
	DDX_Control(pDX, IDC_COMBO_FUNCNAME, m_cbFuncName);
	DDX_Control(pDX, IDC_COMBO_KERNELTYPE, m_cbKernelType);
	DDX_Control(pDX, IDC_EDIT_KERNELSIZE, m_edKernelSize);
	//DDX_Control(pDX, IDC_COMBO_SEGNAME, m_cbSegName);
	DDX_Control(pDX, IDC_COMBO_COLOR, m_cbColor);
	DDX_Control(pDX, IDC_COMBO_SHOWCOLOR_INFER, m_cbShowColorInferColor);
	DDX_Control(pDX, IDC_EDIT_SEGNAME, m_edSegName);
}

BEGIN_MESSAGE_MAP(CSeBoneSegProcedureDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSeBoneSegProcedureDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CSeBoneSegProcedureDlg::OnBnClickedCancel)
	ON_NOTIFY(NM_CLICK, IDC_LIST_BONE_CAL_PROCEDURE, &CSeBoneSegProcedureDlg::OnClickListBoneCalProcedure)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_BONE_CAL_PROCEDURE, &CSeBoneSegProcedureDlg::OnDblclkListBoneCalProcedure)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CSeBoneSegProcedureDlg::OnBnClickedButtonAdd)
	ON_CBN_SELCHANGE(IDC_COMBO_FUNCNAME, &CSeBoneSegProcedureDlg::OnSelchangeComboFuncname)
	ON_CBN_SELCHANGE(IDC_COMBO_KERNELTYPE, &CSeBoneSegProcedureDlg::OnSelchangeComboKerneltype)
	ON_BN_CLICKED(IDC_BUTTON_RUN, &CSeBoneSegProcedureDlg::OnBnClickedButtonRun)
	ON_BN_CLICKED(IDC_BUTTON_RESET, &CSeBoneSegProcedureDlg::OnBnClickedButtonReset)
	ON_BN_CLICKED(IDC_BUTTON_LOAD, &CSeBoneSegProcedureDlg::OnBnClickedButtonLoad)
	ON_BN_CLICKED(IDC_BUTTON_SAVE, &CSeBoneSegProcedureDlg::OnBnClickedButtonSave)

	// 自订消息
	ON_MESSAGE(WM_BONE_CALCHANGE, &CSeBoneSegProcedureDlg::OnDblckChange)


	ON_CBN_SELCHANGE(IDC_COMBO_SHOWCOLOR_INFER, &CSeBoneSegProcedureDlg::OnSelchangeComboShowcolorInfer)
END_MESSAGE_MAP()

// CSeBoneSegProcedureDlg 消息处理程序

BOOL CSeBoneSegProcedureDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	pmyListCtrl = (CListCtrl*)GetDlgItem(IDC_LIST_BONE_CAL_PROCEDURE);

	//pmyListCtrl->SetExtendedStyle(LVS_EX_CHECKBOXES);			//设置列表控件使用复选框

	int nIndex = 0;
	m_ListCtrBoneSeg.InsertColumn(nIndex++, _T("操作对象"), LVCFMT_CENTER, 100);
	m_ListCtrBoneSeg.InsertColumn(nIndex++, _T("操作名称"), LVCFMT_CENTER, 130);
	m_ListCtrBoneSeg.InsertColumn(nIndex++, _T("核类型"), LVCFMT_CENTER,100);
	m_ListCtrBoneSeg.InsertColumn(nIndex++, _T("核大小"), LVCFMT_CENTER,100);

	for (int i=0;i<m_pvecFuncList->size();i++)
	{
		CString strKernelSize;
		if (m_pvecFuncList->at(i).nKernelSize == 0)
		{
			strKernelSize = "";
		}
		else
		{
			strKernelSize.Format("%d",m_pvecFuncList->at(i).nKernelSize);
		}
		pmyListCtrl->InsertItem(i, m_pvecFuncList->at(i).strColor);
		pmyListCtrl->SetItemText(i, 1, m_pvecFuncList->at(i).strFuncName);
		pmyListCtrl->SetItemText(i, 2, m_pvecFuncList->at(i).strKernelType);
		pmyListCtrl->SetItemText(i, 3, strKernelSize);
	}


	m_cbColor.InsertString(0,"red");
	m_cbColor.InsertString(0,"green");
	m_cbColor.SetCurSel(0);

	m_cbFuncName.InsertString(0,"reverse");
	m_cbFuncName.InsertString(1,"gethole");
	m_cbFuncName.InsertString(2,"fillhole");
	m_cbFuncName.InsertString(3,"corrosion");
	m_cbFuncName.InsertString(4,"dilate");
	m_cbFuncName.InsertString(5,"open");
	m_cbFuncName.InsertString(6,"close");
	m_cbFuncName.SetCurSel(0);
			
	m_cbKernelType.InsertString(0,"circle");
	m_cbKernelType.InsertString(1,"square");
	m_cbKernelType.InsertString(2,"none");
	m_cbKernelType.SetCurSel(2);
 			
 	m_edKernelSize.SetWindowTextA("");


// 	m_cbSegName.InsertString(0,"trabecular");
// 	m_cbSegName.InsertString(1,"corticalbone");
// 	m_cbSegName.SetCurSel(1);

	COLORREF colorR = RGB(255, 0, 0);
	COLORREF colorG = RGB(0, 255, 0);
	COLORREF colorY = RGB(255, 255, 0);

	m_cbShowColorInferColor.AddItem("",colorY);
	m_cbShowColorInferColor.AddItem("",colorG);
	m_cbShowColorInferColor.AddItem("",colorR);
	m_cbShowColorInferColor.SetCurSel(2);
// 	m_cbShowColorInfer.InsertString(0,"red");
// 	m_cbShowColorInfer.InsertString(1,"green");
// 	m_cbShowColorInfer.InsertString(2,"both");
	
	
	//存储
// 	m_mapFunc.insert(pair<CString, int>("反白",0));
// 	m_mapFunc.insert(pair<CString, int>("最大连通区域",1));
// 	m_mapFunc.insert(pair<CString, int>("腐蚀",2));
// 	m_mapFunc.insert(pair<CString, int>("膨胀",3));
// 	m_mapFunc.insert(pair<CString, int>("开操作",4));
// 	m_mapFunc.insert(pair<CString, int>("闭操作",5));
// 	m_mapFunc.insert(pair<CString, int>("圆盘",0));
// 	m_mapFunc.insert(pair<CString, int>("方块",1));
// 	m_mapFunc.insert(pair<CString, int>("无",2));

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CSeBoneSegProcedureDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码=
	//完成了显示两个部分
	//m_pParent->SendMessage(WM_BONE_SHOWTWOPARTS,1,1);
	CString csName;
	m_edSegName.GetWindowTextA(csName);
	//m_cbSegName.GetLBText(m_cbSegName.GetCurSel(),csName);
	theBoneDensitySwapData.m_csSegPartName = csName;
	m_pParent->SendMessage(WM_BONE_FUNCFINISHED,1,1);
	CDialogEx::OnOK();
	return;
}


void CSeBoneSegProcedureDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	//m_pParent->SendMessage(WM_MASK_ITEM, 0, 0);
	m_pParent->SendMessage(WM_BONE_FUNCFINISHED,1,1);
	CDialogEx::OnCancel();
}

void CSeBoneSegProcedureDlg::OnClickListBoneCalProcedure(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	DWORD dwPos = GetMessagePos(); //返回表示屏幕坐标下光标位置的长整数值
	CPoint point( LOWORD(dwPos), HIWORD(dwPos) );
	m_ListCtrBoneSeg.ScreenToClient(&point); //把屏幕上指定点的屏幕坐标转换成用户坐标
	LVHITTESTINFO lvinfo;
	lvinfo.pt = point;
	lvinfo.flags = LVHT_ABOVE;
	int nItem = m_ListCtrBoneSeg.SubItemHitTest(&lvinfo);
	if (lvinfo.iItem != -1)
	{
		m_nSelRow = lvinfo.iItem;
		m_nSelCol = lvinfo.iSubItem;

// 		传数据到cbbox
// 				m_strSelFuncName = m_ListCtrBoneSeg.GetItemText(m_nSelRow,0);
// 				m_strSelKernelType = m_ListCtrBoneSeg.GetItemText(m_nSelRow,1);
// 				m_strSelKernelSize = m_ListCtrBoneSeg.GetItemText(m_nSelRow,2);
// 		
// 				m_cbFuncName.SetCurSel(m_pmapFunc->at(m_strSelFuncName)); 
// 				m_cbKernelType.SetCurSel(m_pmapFunc->at(m_strSelKernelType));
// 				m_edKernelSize.SetWindowTextA(m_strSelKernelSize);
	}
	*pResult = 0;
}

BOOL CSeBoneSegProcedureDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 在此添加专用代码和/或调用基类
	// 字符不能用 switch case
	//"w" 的ACSLL码 87
	//"a" 的ACSLL码 65
	//"s" 的ACSLL码 83
	//"d" 的ACSLL码 68
	//"q" 的ACSLL码 81
	//"e" 的ACSLL码 69
	if (pMsg->wParam == VK_DELETE)
	{//选中删除
		bool bchoosed = 0;
		for ( int iItem = m_ListCtrBoneSeg.GetItemCount()-1; iItem>=0; iItem--)
		{
			if ( LVIS_SELECTED == m_ListCtrBoneSeg.GetItemState(iItem,LVIS_SELECTED))     //发现选中行
			{
				m_ListCtrBoneSeg.DeleteItem(iItem);
				bchoosed = 1;
			}
		}
	}
	if (pMsg->wParam == VK_UP)
	{
		for ( int iItem = 1; iItem < (m_ListCtrBoneSeg.GetItemCount()); iItem++)
		{
			if ( LVIS_SELECTED == m_ListCtrBoneSeg.GetItemState(iItem,LVIS_SELECTED))     //发现选中行
			{
				CString strColor = m_ListCtrBoneSeg.GetItemText(iItem,0);
				CString strFuncName = m_ListCtrBoneSeg.GetItemText(iItem,1);
				CString strKernelType = m_ListCtrBoneSeg.GetItemText(iItem,2);
				CString strKernelSize = m_ListCtrBoneSeg.GetItemText(iItem,3);

				m_ListCtrBoneSeg.DeleteItem(iItem);

				m_ListCtrBoneSeg.InsertItem(iItem-1,strColor);
				m_ListCtrBoneSeg.SetItemText(iItem-1,1,strFuncName);
				m_ListCtrBoneSeg.SetItemText(iItem-1,2,strKernelType);
				m_ListCtrBoneSeg.SetItemText(iItem-1,3,strKernelSize);

			}
		}
		return 1;
	}
	if (pMsg->wParam == VK_DOWN)
	{
		for ( int iItem = 0; iItem < (m_ListCtrBoneSeg.GetItemCount()-1); iItem++)
		{
			if ( LVIS_SELECTED == m_ListCtrBoneSeg.GetItemState(iItem,LVIS_SELECTED))     //发现选中行
			{
				CString strColor = m_ListCtrBoneSeg.GetItemText(iItem,0);
				CString strFuncName = m_ListCtrBoneSeg.GetItemText(iItem,1);
				CString strKernelType = m_ListCtrBoneSeg.GetItemText(iItem,2);
				CString strKernelSize = m_ListCtrBoneSeg.GetItemText(iItem,3);

				m_ListCtrBoneSeg.DeleteItem(iItem);

				m_ListCtrBoneSeg.InsertItem(iItem+1,strColor);
				m_ListCtrBoneSeg.SetItemText(iItem+1,1,strFuncName);
				m_ListCtrBoneSeg.SetItemText(iItem+1,2,strKernelType);
				m_ListCtrBoneSeg.SetItemText(iItem+1,3,strKernelSize);
			}
		}
		return 1;
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

LRESULT CSeBoneSegProcedureDlg::OnDblckChange(WPARAM wParam, LPARAM lParam)
{
	pmyListCtrl->SetItemText(m_nSelRow, 0, strChangeSingleFun[0]);
	pmyListCtrl->SetItemText(m_nSelRow, 1, strChangeSingleFun[1]);
	pmyListCtrl->SetItemText(m_nSelRow, 2, strChangeSingleFun[2]);
	pmyListCtrl->SetItemText(m_nSelRow, 3, strChangeSingleFun[3]);
	return 1;
}

void CSeBoneSegProcedureDlg::OnBnClickedButtonAdd()
{
	// TODO: 增加一条（如果没有选中的就在最下面加一条，如果选中，就在选中的后面加）
	//当前的列表为空
	m_cbColor.GetLBText(m_cbColor.GetCurSel(),m_strSelColor);
	m_cbFuncName.GetLBText(m_cbFuncName.GetCurSel(),m_strSelFuncName);
	m_cbKernelType.GetLBText(m_cbKernelType.GetCurSel(),m_strSelKernelType);
	m_edKernelSize.GetWindowTextA(m_strSelKernelSize);

	if (m_cbFuncName.GetCurSel() == 0 || m_cbFuncName.GetCurSel() == 1 || m_cbFuncName.GetCurSel() == 2)
	{
		if (m_strSelKernelSize!="")
		{
			return;
		}
		if (m_cbKernelType.GetCurSel() != 2)
		{
			return;
		}
	}
	else{
		if (m_strSelKernelSize=="")
		{
			return;
		}
		if (m_cbKernelType.GetCurSel() == 2)
		{
			return;
		}
	}

	if (m_ListCtrBoneSeg.GetItemCount() == 0)
	{
		pmyListCtrl->InsertItem(0, m_strSelColor);
		pmyListCtrl->SetItemText(0, 1, m_strSelFuncName);
		pmyListCtrl->SetItemText(0, 2, m_strSelKernelType);
		pmyListCtrl->SetItemText(0, 3, m_strSelKernelSize);
		return;
	}

	int nSumSelect = 0;
	int nPosSelect = m_ListCtrBoneSeg.GetItemCount()-1;
	for ( int iItem = 0; iItem < (m_ListCtrBoneSeg.GetItemCount()); iItem++)
	{
		if ( LVIS_SELECTED == m_ListCtrBoneSeg.GetItemState(iItem,LVIS_SELECTED))     //发现选中行
		{
			nSumSelect++;
			nPosSelect = iItem;
		}
	}
	if (nSumSelect>1)
	{
		return;
	}

	pmyListCtrl->InsertItem(nPosSelect+1, m_strSelColor);
	pmyListCtrl->SetItemText(nPosSelect+1, 1, m_strSelFuncName);
	pmyListCtrl->SetItemText(nPosSelect+1, 2, m_strSelKernelType);
	pmyListCtrl->SetItemText(nPosSelect+1, 3, m_strSelKernelSize);

	return;

// 	if (m_cbFuncName.GetCurSel() == 0 || m_cbFuncName.GetCurSel() == 1)
// 	{
// 		if (m_strSelKernelSize!="")
// 		{
// 			return;
// 		}
// 		if (m_cbKernelType.GetCurSel() != 2)
// 		{
// 			return;
// 		}
// 	}
// 	else{
// 		if (m_strSelKernelSize=="")
// 		{
// 			return;
// 		}
// 		if (m_cbKernelType.GetCurSel() == 2)
// 		{
// 			return;
// 		}
// 	}
// 
// 
// 	pmyListCtrl->SetItemText(m_nSelRow, 0, m_strSelFuncName);
// 	pmyListCtrl->SetItemText(m_nSelRow, 1, m_strSelKernelType);
// 	pmyListCtrl->SetItemText(m_nSelRow, 2, m_strSelKernelSize);
// 
// 
// 
// 
// 
// 
// 
// 	if (!m_bChange)
// 	{
// 		return;
// 	}
// 
// 	int m = m_cbFuncName.GetCurSel();
// 	int n = m_cbKernelType.GetCurSel();
// 	m_cbFuncName.GetLBText(m_cbFuncName.GetCurSel(),m_strSelFuncName);
// 	m_cbKernelType.GetLBText(m_cbKernelType.GetCurSel(),m_strSelKernelType);
// 	m_edKernelSize.GetWindowTextA(m_strSelKernelSize);
// 
// 	if (m_cbFuncName.GetCurSel() == 0 || m_cbFuncName.GetCurSel() == 1)
// 	{
// 		if (m_strSelKernelSize!="")
// 		{
// 			return;
// 		}
// 		if (m_cbKernelType.GetCurSel() != 2)
// 		{
// 			return;
// 		}
// 	}
// 	else{
// 		if (m_strSelKernelSize=="")
// 		{
// 			return;
// 		}
// 		if (m_cbKernelType.GetCurSel() == 2)
// 		{
// 			return;
// 		}
// 	}
// 
// 	//
// 	//pmyListCtrl->InsertItem(m_nSelRow, _T("最大联通区域"));
// 
// 	pmyListCtrl->SetItemText(m_nSelRow, 0, m_strSelFuncName);
// 	pmyListCtrl->SetItemText(m_nSelRow, 1, m_strSelKernelType);
// 	pmyListCtrl->SetItemText(m_nSelRow, 2, m_strSelKernelSize);
// 
// 	//m_bChange = 0;
/*	return;*/
}
// 
// 
void CSeBoneSegProcedureDlg::OnSelchangeComboFuncname()
{
	// TODO: 在此添加控件通知处理程序代码
	//
	int nselcur = m_cbFuncName.GetCurSel();
	if (nselcur == 0 || nselcur == 1 || nselcur == 2)
	{
		//后面两个没有
		m_cbKernelType.SetCurSel(2);
		m_edKernelSize.SetWindowTextA("");
		m_bChangeLastTwo = 0;
	}
	else
	{
		m_cbKernelType.SetCurSel(0);
		m_edKernelSize.SetWindowTextA("3");
		m_bChangeLastTwo = 1;
	}
}


void CSeBoneSegProcedureDlg::OnSelchangeComboKerneltype()
{
	// TODO: 在此添加控件通知处理程序代码

	if (!m_bChange)
	{
		return;
	}

	if (!m_bChangeLastTwo)
	{
		return;
	}

	return;
}





void CSeBoneSegProcedureDlg::OnBnClickedButtonRun()
{
	// TODO: 在此添加控件通知处理程序代码
	int nSum = m_ListCtrBoneSeg.GetItemCount();
	if (nSum == 0)
	{
		return;
		AfxMessageBox("函数列表为空！");
	}

	//从头开始
	m_pParent->SendMessage(WM_BONE_FUNCRESET,1,1);

	//写入vec中
	m_pvecFuncList->clear();
	for (int i=0;i<nSum;i++)
	{
		FuncSingle temp;

		temp.strColor = pmyListCtrl->GetItemText(i,0);
		temp.strFuncName = pmyListCtrl->GetItemText(i,1);
		temp.strKernelType = pmyListCtrl->GetItemText(i,2);

		//check是否包含
		auto iiterN = m_pmapFunc->find(temp.strFuncName);
		if (iiterN == m_pmapFunc->end())
		{
			return;
		}
		auto iiterC = m_pmapFunc->find(temp.strColor);
		if (iiterC == m_pmapFunc->end())
		{
			return;
		}

		//
		if (temp.strFuncName == "reverse" || temp.strFuncName == "gethole" || temp.strFuncName == "fillhole")
		{
			temp.strKernelType = "none";
			temp.nKernelSize = 0;
		}
		else
		{
			temp.strKernelType = pmyListCtrl->GetItemText(i,2);
			temp.nKernelSize = _ttoi(pmyListCtrl->GetItemText(i,3));
		}
		m_pvecFuncList->push_back(temp);
	}

	//发消息进行操作
	m_pParent->SendMessage(WM_BONE_FUNCLIST,1,1);

	//要显示的
	int nChoose = m_cbShowColorInferColor.GetCurSel();
	bool nRed = 0;
	bool nGreen = 0;
	if (nChoose == 0)
	{
		nRed = 1;
	}
	else if (nChoose == 1)
	{
		nGreen = 1;
	}
	else{
		nRed = 1;
		nGreen = 1;
	}

	m_pParent->SendMessage(WM_BONE_FUNCSHOWMASKSET,nRed,nGreen);
	//g_pBoneDensityModule->GetUI()->SendMessage(WM_SIZE);
	return;
}


void CSeBoneSegProcedureDlg::OnBnClickedButtonReset()
{
	// TODO: 返回二值化的状态
	m_pParent->SendMessage(WM_BONE_FUNCRESET,1,1);
	return;
}

void CSeBoneSegProcedureDlg::OnDblclkListBoneCalProcedure(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	DWORD dwPos = GetMessagePos(); //返回表示屏幕坐标下光标位置的长整数值
	CPoint point( LOWORD(dwPos), HIWORD(dwPos) );
	m_ListCtrBoneSeg.ScreenToClient(&point); //把屏幕上指定点的屏幕坐标转换成用户坐标
	LVHITTESTINFO lvinfo;
	lvinfo.pt = point;
	lvinfo.flags = LVHT_ABOVE;
	int nItem = m_ListCtrBoneSeg.SubItemHitTest(&lvinfo);
	if (lvinfo.iItem != -1)
	{
		m_nSelRow = lvinfo.iItem;
		m_nSelCol = lvinfo.iSubItem;

		strChangeSingleFun[0] = m_ListCtrBoneSeg.GetItemText(m_nSelRow,0);
		strChangeSingleFun[1] = m_ListCtrBoneSeg.GetItemText(m_nSelRow,1);
		strChangeSingleFun[2] = m_ListCtrBoneSeg.GetItemText(m_nSelRow,2);
		strChangeSingleFun[3] = m_ListCtrBoneSeg.GetItemText(m_nSelRow,3);



		CSeBoneSegProcedureChangeDlg dlg(m_pmapFunc,strChangeSingleFun,this);
		dlg.DoModal();

	}

	*pResult = 0;
}

void CSeBoneSegProcedureDlg::ReadFile(const char *pFileName, char *pFileContent)
{
	ifstream inFile(pFileName);
	string contents("");
	if (inFile.is_open())
	{
		std::stringstream buffer;
		buffer << inFile.rdbuf();
		contents.append(buffer.str());	
	}

	inFile.close();

	strcpy(pFileContent, contents.c_str());

}


void CSeBoneSegProcedureDlg::OnBnClickedButtonLoad()
{
	//文件路径
	CString cspath;
	GetModuleFileName(NULL,cspath.GetBufferSetLength(MAX_PATH+1),MAX_PATH);
	cspath.ReleaseBuffer();
	int pos = cspath.ReverseFind('\\');
	CString cspathBin = cspath.Left(pos);
	CString cspathSeg = cspathBin + "\\SeBoneDensity";
	auto m = PathIsDirectory(cspathSeg);
	if (!PathIsDirectory(cspathSeg))
	{
		CreateDirectory(cspathSeg,NULL);
	}
	//加载配置文件
	LPCTSTR lpszFilter = "JSON Files(*.json)|*.json|";
	m_pvecFuncList->clear();
	m_ListCtrBoneSeg.DeleteAllItems();
	CFileDialog ccFileDlg(TRUE, lpszFilter, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, lpszFilter);//, NULL);
	//打开固定路径 
	ccFileDlg.m_ofn.lpstrInitialDir = cspathSeg;
	if (ccFileDlg.DoModal() == IDOK)
	{
		CString strPathName = ccFileDlg.GetPathName();
		char *pFileContent = new char[3000];
		memset(pFileContent, 0, 3000);
		ReadFile(strPathName, pFileContent);
		cJSON* root = cJSON_Parse(pFileContent);

		delete []pFileContent;

		int m = cJSON_GetArraySize(root);
		if (m != 2)
		{
			return;
		}
		cJSON * item_name = cJSON_GetArrayItem(root, 0);
		CString csname = item_name->string;
		int csnameType = item_name->type;
		if (csname != "name" || csnameType != 16)
		{
			return;
		}
		CString cssegname = item_name->valuestring;
		m_edSegName.SetWindowTextA(cssegname);
//		int nindex;
// 		m_cbSegName.FindString(nindex,cssegname);
// 		if(nindex == -1)
// 		{
// 			return;
// 		}
//		m_cbSegName.SetCurSel(nindex);

		cJSON * item_funs = cJSON_GetArrayItem(root, 1);
		csname = item_funs->string;
		if (csname != "functions")
		{
			return;
		}

		for (int i=0;i<cJSON_GetArraySize(item_funs);i++)
		{
			cJSON * item_fun = cJSON_GetArrayItem(item_funs, i);
			if (cJSON_GetArraySize(item_fun) != 3)
			{
				return;
			}
			FuncSingle fstemp;
			fstemp.strFuncName = item_fun->string;
			auto iiter = m_pmapFunc->find(fstemp.strFuncName);
			if (iiter == m_pmapFunc->end())
			{
				return;
			}
			cJSON * fun_0 = cJSON_GetArrayItem(item_fun, 0);
			CString temp0 =  fun_0->string;
			int ntempTyle0 = fun_0->type;
			if (temp0 != "color" || ntempTyle0 != 16)
			{
				return;
			}
			fstemp.strColor = fun_0->valuestring;

			cJSON * fun_1 = cJSON_GetArrayItem(item_fun, 1);
			CString temp1 =  fun_1->string;
			int ntempTyle1 = fun_1->type;
			if (temp1 != "kerneltype" || ntempTyle1 != 16)
			{
				return;
			}
			fstemp.strKernelType = fun_1->valuestring;

			cJSON* fun_2 = cJSON_GetArrayItem(item_fun, 2);
			CString temp2 =  fun_2->string;
			int ntempTyle2 = fun_2->type;
			if (temp2 != "kernelsize" || ntempTyle2 != 8)
			{
				return;
			}
			fstemp.nKernelSize = fun_2->valueint;

			m_pvecFuncList->push_back(fstemp);
		}
	}
	else{
		return;
	}

	//写入list中
	for (int i=0;i<m_pvecFuncList->size();i++)
	{
		CString strKernelSize;
		if (m_pvecFuncList->at(i).nKernelSize == 0)
		{
			strKernelSize = "";
		}
		else
		{
			strKernelSize.Format("%d",m_pvecFuncList->at(i).nKernelSize);
		}
		pmyListCtrl->InsertItem(i, m_pvecFuncList->at(i).strColor);
		pmyListCtrl->SetItemText(i, 1, m_pvecFuncList->at(i).strFuncName);
		pmyListCtrl->SetItemText(i, 2, m_pvecFuncList->at(i).strKernelType);
		pmyListCtrl->SetItemText(i, 3, strKernelSize);
	}

	//重置
	m_pParent->SendMessage(WM_BONE_FUNCRESET,1,1);

	//要显示的
	int nChoose = m_cbShowColorInferColor.GetCurSel();
	bool nRed = 0;
	bool nGreen = 0;
	if (nChoose == 0)
	{
		nRed = 1;
	}
	if (nChoose == 1)
	{
		nGreen = 1;
	}
	else{
		nRed = 1;
		nGreen = 1;
	}

	m_pParent->SendMessage(WM_BONE_FUNCSHOWMASKSET,nRed,nGreen);
}


void CSeBoneSegProcedureDlg::OnBnClickedButtonSave()
{
	// TODO: 在此添加控件通知处理程序代码

	//保存配置文件
	//文件路径
	CString cspath;
	GetModuleFileName(NULL,cspath.GetBufferSetLength(MAX_PATH+1),MAX_PATH);
	cspath.ReleaseBuffer();
	int pos = cspath.ReverseFind('\\');
	CString cspathBin = cspath.Left(pos);
	CString cspathSeg = cspathBin + "\\SeBoneDensity";
	auto m = PathIsDirectory(cspathSeg);
	if (!PathIsDirectory(cspathSeg))
	{
		CreateDirectory(cspathSeg,NULL);
	}

	//写入json中
	int nSum = m_ListCtrBoneSeg.GetItemCount();
	if (nSum == 0)
	{
		return;
		AfxMessageBox("函数列表为空！");
	}

	LPCTSTR lpszFilter = "JSON Files(*.json)|*.json|";
	CFileDialog ccFileDlg(TRUE, lpszFilter, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT , lpszFilter);//, NULL);
	ccFileDlg.m_ofn.lpstrInitialDir = cspathSeg;
	if (ccFileDlg.DoModal() == IDOK)
	{
		CString strPathName = ccFileDlg.GetPathName();

		
		m_pvecFuncList->clear();
		for (int i=0;i<nSum;i++)
		{
			FuncSingle temp;
			temp.strFuncName = pmyListCtrl->GetItemText(i,1);
			auto iiterN = m_pmapFunc->find(temp.strFuncName);
			if (iiterN == m_pmapFunc->end())
			{
				return;
			}

			temp.strColor = pmyListCtrl->GetItemText(i,0);
			auto iiterC = m_pmapFunc->find(temp.strColor);
			if (iiterC == m_pmapFunc->end())
			{
				return;
			}

			if (temp.strFuncName == "reverse" || temp.strFuncName == "gethole" || temp.strFuncName == "fillhole")
			{
				temp.strKernelType = "none";
				temp.nKernelSize = 0;
			}
			else
			{
				temp.strKernelType = pmyListCtrl->GetItemText(i,2);
				temp.nKernelSize = _ttoi(pmyListCtrl->GetItemText(i,3));
			}
			m_pvecFuncList->push_back(temp);
		}

		//写成json
		CString csName;
		m_edSegName.GetWindowTextA(csName);
		//m_cbSegName.GetLBText(m_cbSegName.GetCurSel(),csName);

		cJSON *root = cJSON_CreateObject();
		cJSON_AddItemToObject(root, "name", cJSON_CreateString(csName));
		cJSON *fmt;
		cJSON_AddItemToObject(root, "functions", fmt = cJSON_CreateObject());

		for (int i=0;i<m_pvecFuncList->size();i++)
		{
			cJSON *fmt_;
			cJSON_AddItemToObject(fmt, m_pvecFuncList->at(i).strFuncName, fmt_ = cJSON_CreateObject());
			cJSON_AddStringToObject(fmt_, "color", m_pvecFuncList->at(i).strColor);
			cJSON_AddStringToObject(fmt_, "kerneltype", m_pvecFuncList->at(i).strKernelType);
			cJSON_AddNumberToObject(fmt_, "kernelsize", m_pvecFuncList->at(i).nKernelSize);
		}

		char *pFileContent = new char[3000];
		memset(pFileContent, 0, 3000);
		pFileContent = cJSON_Print(root);

		std::ofstream file_writer(strPathName, std::ios_base::out);
		file_writer << pFileContent;
		file_writer.close();
		delete []pFileContent;
	}
}


void CSeBoneSegProcedureDlg::OnSelchangeComboShowcolorInfer()
{
	// TODO: 在此添加控件通知处理程序代码
	//
	int nChoose = m_cbShowColorInferColor.GetCurSel();
	bool nRed = 0;
	bool nGreen = 0;
	if (nChoose == 0)
	{
		nRed = 1;
	}
	else if (nChoose == 1)
	{
		nGreen = 1;
	}
	else{
		nRed = 1;
		nGreen = 1;
	}

	m_pParent->SendMessage(WM_BONE_FUNCSHOWMASKSET,nRed,nGreen);

	return;
}


