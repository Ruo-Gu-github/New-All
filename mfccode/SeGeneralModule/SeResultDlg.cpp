// SeResultDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SeGeneralModule.h"
#include "GeneralSwapData.h"
#include "SeResultDlg.h"
#include "afxdialogex.h"
#include "SeMPRView.h"
#include "SePicture.h"

class CDcmPic;

namespace
{
	CStringA ToUtf8(const CString& text)
	{
		CStringA utf8;
#ifdef UNICODE
		// text 是宽字符
		LPCWSTR pwsz = text.GetString();
#else
		// text 是窄字符，先转成宽字符
		CStringW wtext(text);
		LPCWSTR pwsz = wtext.GetString();
#endif
		int required = WideCharToMultiByte(CP_UTF8, 0, pwsz, -1, NULL, 0, NULL, NULL);
		if (required <= 0)
			return utf8;
		LPSTR buffer = utf8.GetBufferSetLength(required - 1);
		WideCharToMultiByte(CP_UTF8, 0, pwsz, -1, buffer, required, NULL, NULL);
		utf8.ReleaseBuffer();
		return utf8;
	}
}


// SeResultDlg 对话框

IMPLEMENT_DYNAMIC(CSeResultDlg, CDialogEx)

CSeResultDlg::CSeResultDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSeResultDlg::IDD, pParent)
{

}

CSeResultDlg::~CSeResultDlg()
{
}

void CSeResultDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_MEASURE, m_ResultList);
}


void CSeResultDlg::UpdateTable(vector<CMeasurement*>& vecResult, int nPos)
{	
// 	m_ResultList.DeleteAllItems();
// 
// 	for(int i=0; i<vecResult.size(); i++) {
// 		CString str = "";
// 		if (vecResult[i]->m_nPlaneNumber == 1)
// 			str = "正视图";
// 		else if(vecResult[i]->m_nPlaneNumber == 2)
// 			str = "侧视图";
// 		else if(vecResult[i]->m_nPlaneNumber == 3)
// 			str = "俯视图";
// 		m_ResultList.InsertItem(i, str);
// 		str = "";
// 		str.Format("%d", vecResult[i]->m_nSliceNumber);
// 		m_ResultList.SetItemText(i, 1, str);
// 		m_ResultList.SetItemText(i, 2, vecResult[i]->m_strToolName);
// 		for(int j=0; j<vecResult[i]->m_vecResults.size(); j++) {
// 			CString title = vecResult[i]->m_vecResults[j].first;
// 			CString result;
// 			result.Format("%.4f", vecResult[i]->m_vecResults[j].second);
// 			if(title == "面积(mm^2)")
// 				m_ResultList.SetItemText(i, 5, result);
// 			else if(title == "平均CT值(HU)" || title == "CT值(HU)")
// 				m_ResultList.SetItemText(i, 6, result);
// 			else if(title == "方差(HU)")
// 				m_ResultList.SetItemText(i, 7, result);
// 			else if(title == "角度(°)")
// 				m_ResultList.SetItemText(i, 4, result);
// 			else if(title == "长度(mm)")
// 				m_ResultList.SetItemText(i, 3, result);
// 		}
// 	}

	for(int j=0; j<vecResult[nPos]->m_vecResults.size(); j++) {
		CString title = vecResult[nPos]->m_vecResults[j].first;
		CString result;
		result.Format("%.4f", vecResult[nPos]->m_vecResults[j].second);
		if(title == "面积(mm^2)")
			m_ResultList.SetItemText(nPos, 5, result);
		else if(title == "最大值(HU)")
			m_ResultList.SetItemText(nPos, 6, result);
		else if(title == "最小值(HU)")
			m_ResultList.SetItemText(nPos, 7, result);
		else if(title == "平均值(HU)" || title == "平均CT值(HU)" || title == "CT值(HU)")
			m_ResultList.SetItemText(nPos, 8, result);
		else if(title == "标准差(HU)" || title == "方差(HU)")
			m_ResultList.SetItemText(nPos, 9, result);
		else if(title == "角度(°)")
			m_ResultList.SetItemText(nPos, 4, result);
		else if(title == "长度(mm)")
			m_ResultList.SetItemText(nPos, 3, result);
	}
}

void CSeResultDlg::AddTable(CMeasurement* Result)
{
	int nPos = m_ResultList.GetItemCount();

	CString str = "";
	if (Result->m_nPlaneNumber == 1)
		str = "正视图";
	else if(Result->m_nPlaneNumber == 2)
		str = "侧视图";
	else if(Result->m_nPlaneNumber == 3)
		str = "俯视图";
	m_ResultList.InsertItem(nPos, str);
	str = "";
	str.Format("%d", Result->m_nSliceNumber);
	m_ResultList.SetItemText(nPos, 1, str);
	m_ResultList.SetItemText(nPos, 2, Result->m_strToolName);
	for(int j=0; j<Result->m_vecResults.size(); j++) {
		CString title = Result->m_vecResults[j].first;
		CString result;
		result.Format("%.4f", Result->m_vecResults[j].second);
		if(title == "面积(mm^2)")
			m_ResultList.SetItemText(nPos, 5, result);
		else if(title == "最大值(HU)")
			m_ResultList.SetItemText(nPos, 6, result);
		else if(title == "最小值(HU)")
			m_ResultList.SetItemText(nPos, 7, result);
		else if(title == "平均值(HU)" || title == "平均CT值(HU)" || title == "CT值(HU)")
			m_ResultList.SetItemText(nPos, 8, result);
		else if(title == "标准差(HU)" || title == "方差(HU)")
			m_ResultList.SetItemText(nPos, 9, result);
		else if(title == "角度(°)")
			m_ResultList.SetItemText(nPos, 4, result);
		else if(title == "长度(mm)")
			m_ResultList.SetItemText(nPos, 3, result);
	}
}

void CSeResultDlg::RefreshTable(vector<CMeasurement*>& vecResult)
{
	m_ResultList.DeleteAllItems();

	for(int i=0; i<vecResult.size(); i++) {
		CString str = "";
		if (vecResult[i]->m_nPlaneNumber == 1)
			str = "正视图";
		else if(vecResult[i]->m_nPlaneNumber == 2)
			str = "侧视图";
		else if(vecResult[i]->m_nPlaneNumber == 3)
			str = "俯视图";
		m_ResultList.InsertItem(i, str);
		str = "";
		str.Format("%d", vecResult[i]->m_nSliceNumber);
		m_ResultList.SetItemText(i, 1, str);
		m_ResultList.SetItemText(i, 2, vecResult[i]->m_strToolName);
		for(int j=0; j<vecResult[i]->m_vecResults.size(); j++) {
			CString title = vecResult[i]->m_vecResults[j].first;
			CString result;
			result.Format("%.4f", vecResult[i]->m_vecResults[j].second);
			if(title == "面积(mm^2)")
				m_ResultList.SetItemText(i, 5, result);
			else if(title == "最大值(HU)")
				m_ResultList.SetItemText(i, 6, result);
			else if(title == "最小值(HU)")
				m_ResultList.SetItemText(i, 7, result);
			else if(title == "平均值(HU)" || title == "平均CT值(HU)" || title == "CT值(HU)")
				m_ResultList.SetItemText(i, 8, result);
			else if(title == "标准差(HU)" || title == "方差(HU)")
				m_ResultList.SetItemText(i, 9, result);
			else if(title == "角度(°)")
				m_ResultList.SetItemText(i, 4, result);
			else if(title == "长度(mm)")
				m_ResultList.SetItemText(i, 3, result);
		}
	}
}

LRESULT CSeResultDlg::OnUpdateResult(WPARAM wParam, LPARAM lParam)
{ 
	vector<CMeasurement*>* vecResult = (vector<CMeasurement *>*)wParam;
	int                    nPos = (int)lParam;
	UpdateTable(*vecResult, nPos);
	return  0;
}

LRESULT CSeResultDlg::OnAddResult(WPARAM wParam, LPARAM lParam)
{
	CMeasurement* oneResult = (CMeasurement*)wParam;
	AddTable(oneResult);
	return 0;
}

LRESULT CSeResultDlg::OnRefreshResult(WPARAM wParam, LPARAM lParam)
{
	vector<CMeasurement*>* vecResult = (vector<CMeasurement *>*)wParam;
	RefreshTable(*vecResult);
	return 0;
}

BEGIN_MESSAGE_MAP(CSeResultDlg, CDialogEx)
	ON_MESSAGE(WM_UPDATE_RESULT, &CSeResultDlg::OnUpdateResult)
	ON_MESSAGE(WM_ADD_RESULT, &CSeResultDlg::OnAddResult)
	ON_MESSAGE(WM_REFRESH_RESULT, &CSeResultDlg::OnRefreshResult)
	ON_NOTIFY(HDN_ITEMDBLCLICK, 0, &CSeResultDlg::OnHdnItemdblclickListMeasure)
	ON_WM_KEYDOWN()
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_MEASURE, &CSeResultDlg::OnNMDblclkListMeasure)
	ON_BN_CLICKED(IDC_BUTTON_SHOW, &CSeResultDlg::OnBnClickedButtonShow)
	ON_BN_CLICKED(IDC_BUTTON_OUTPUT, &CSeResultDlg::OnBnClickedButtonOutput)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_MARK, &CSeResultDlg::OnBnClickedButtonSaveMark)
	ON_BN_CLICKED(IDC_BUTTON_LOAD_MARK, &CSeResultDlg::OnBnClickedButtonLoadMark)
END_MESSAGE_MAP()


// SeResultDlg 消息处理程序

BOOL CSeResultDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化

	DWORD dwStyle = m_ResultList.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;							//选中某行使整行高亮（只适用与report风格的listctrl）
	dwStyle |= LVS_EX_GRIDLINES;								//网格线（只适用与report风格的listctrl）						//item前生成checkbox控件
	m_ResultList.SetExtendedStyle(dwStyle);				//设置扩展风格


	m_ResultList.InsertColumn(0, "视图", LVCFMT_LEFT , 60);
	m_ResultList.InsertColumn(1, "层数", LVCFMT_LEFT , 60);
	m_ResultList.InsertColumn(2, "工具", LVCFMT_LEFT , 60);
	m_ResultList.InsertColumn(3, "长度(mm)", LVCFMT_LEFT , 90);
	m_ResultList.InsertColumn(4, "角度(°)", LVCFMT_LEFT , 90);
	m_ResultList.InsertColumn(5, "面积(mm^2)", LVCFMT_LEFT , 90);
	m_ResultList.InsertColumn(6, "最大值(HU)", LVCFMT_LEFT , 90);
	m_ResultList.InsertColumn(7, "最小值(HU)", LVCFMT_LEFT , 90);
	m_ResultList.InsertColumn(8, "平均值(HU)", LVCFMT_LEFT , 90);
	m_ResultList.InsertColumn(9, "标准差(HU)", LVCFMT_LEFT , 90);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CSeResultDlg::OnHdnItemdblclickListMeasure(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}


void CSeResultDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnKeyDown(nChar, nRepCnt, nFlags);
}


BOOL CSeResultDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 在此添加专用代码和/或调用基类
	vector<int> vecSelected;
	if(pMsg->message == WM_KEYDOWN  && pMsg->wParam == VK_DELETE) {
		const int nCount = m_ResultList.GetItemCount();
		vector<CMeasurement*>& vecRst = SeMPRView::GetResult();
		vector<CMeasurement*> vecTmp;
// 		for (int i=0; i<nCount; i++)
// 		{
// 			BOOL bSelected = m_ResultList.GetSelectedColumn(i);
// 			if (!bSelected)
// 				vecTmp.push_back(vecRst[i]);
// 			else
// 			{
// 				if (vecRst[i] != NULL)
// 					delete vecRst[i];
// 			}
// 		}

		CString str;
		POSITION pos = m_ResultList.GetFirstSelectedItemPosition (); //pos 选中的首行位置
		if (pos != NULL)
		{
			while (pos)   // 如果你选择多行
			{
				int nIdx=-1;
				nIdx= m_ResultList.GetNextSelectedItem(pos);

				if(nIdx >=0 && nIdx<nCount)
				{
					if (vecRst[nIdx] != NULL)
					{
						delete vecRst[nIdx];
						vecRst[nIdx] = NULL;
					}
				}
			}
		}
		for(int i=0; i<vecRst.size(); i++) {
			if (vecRst[i] != NULL)
				vecTmp.push_back(vecRst[i]);
		}

		vecRst = vecTmp;
		RefreshTable(SeMPRView::GetResult());
		theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
		theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
		theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}


void CSeResultDlg::OnNMDblclkListMeasure(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	POSITION pos= m_ResultList.GetFirstSelectedItemPosition();
	if( pos != NULL )
	{
		int nPos = -1;
		nPos = m_ResultList.GetNextSelectedItem( pos );        
		if(nPos >= 0 && nPos < m_ResultList.GetItemCount())
		{
			SeMPRView::MoveToToolPos(nPos);
		}
	}
	*pResult = 0;
}


void CSeResultDlg::OnBnClickedButtonShow()
{
	POSITION pos = m_ResultList.GetFirstSelectedItemPosition();
	if (pos == NULL)
	{
		AfxMessageBox(_T("请先选择一条测量记录。"));
		return;
	}

	int nIndex = m_ResultList.GetNextSelectedItem(pos);
	vector<CMeasurement*>& vecResult = SeMPRView::GetResult();
	if (nIndex < 0 || nIndex >= static_cast<int>(vecResult.size()))
		return;

	CMeasurement* pMeasurement = vecResult[nIndex];
	if (pMeasurement == NULL)
		return;

	CString strTool = pMeasurement->m_strToolName;
	if (strTool == _T("Angle"))
	{
		AfxMessageBox(_T("角度测量不支持统计图。"));
		return;
	}

	AttachCurrentDcm(pMeasurement);

	CSePicture dlg(this);
	if (!dlg.SetMeasurement(pMeasurement))
	{
		AfxMessageBox(_T("当前测量没有可用的数据。"));
		return;
	}
	dlg.DoModal();
}


void CSeResultDlg::OnBnClickedButtonOutput()
{
	if (m_ResultList.GetItemCount() == 0)
	{
		AfxMessageBox(_T("没有可导出的结果。"));
		return;
	}

	CFileDialog dlg(FALSE, _T("csv"), _T("MeasureResult.csv"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("CSV 文件 (*.csv)|*.csv||"), this);
	if (dlg.DoModal() != IDOK)
		return;

	CString strPath = dlg.GetPathName();
	if (strPath.Right(4).CompareNoCase(_T(".csv")) != 0)
	{
		strPath += _T(".csv");
	}

	if (ExportResultsToCsv(strPath))
	{
		AfxMessageBox(_T("结果已成功保存。"));
	}
	else
	{
		AfxMessageBox(_T("保存失败，请检查路径或文件权限。"));
	}
}


void CSeResultDlg::OnBnClickedButtonSaveMark()
{
	CFileDialog dlg(FALSE, _T("mark"), _T("MeasureMarks.mark"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("标记文件 (*.mark)|*.mark||"), this);
	if (dlg.DoModal() != IDOK)
		return;

	CString strPath = dlg.GetPathName();
	if (strPath.Right(5).CompareNoCase(_T(".mark")) != 0)
	{
		strPath += _T(".mark");
	}

	if (SaveMarksToFile(strPath))
	{
		AfxMessageBox(_T("标记已保存。"));
	}
	else
	{
		AfxMessageBox(_T("保存标记失败，请重试。"));
	}
}


void CSeResultDlg::OnBnClickedButtonLoadMark()
{
	CFileDialog dlg(TRUE, _T("mark"), NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
		_T("标记文件 (*.mark)|*.mark||"), this);
	if (dlg.DoModal() != IDOK)
		return;

	CString strPath = dlg.GetPathName();
	if (!LoadMarksFromFile(strPath))
	{
		AfxMessageBox(_T("加载标记失败，文件可能已损坏。"));
		return;
	}

	vector<CMeasurement*>& vecResult = SeMPRView::GetResult();
	RefreshTable(vecResult);
	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOYView->UpdateWindow();
	theGeneralSwapData.m_pXOZView->UpdateWindow();
	theGeneralSwapData.m_pYOZView->UpdateWindow();
}


BOOL CSeResultDlg::ExportResultsToCsv(const CString& strFilePath)
{
	CString content = BuildCsvContent();
	CFile file;
	if (!file.Open(strFilePath, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
	{
		return FALSE;
	}

	const BYTE bom[] = {0xEF, 0xBB, 0xBF};
	file.Write(bom, 3);
	CStringA utf8 = ToUtf8(content);
	if (!utf8.IsEmpty())
	{
		file.Write(utf8.GetString(), utf8.GetLength());
	}
	file.Close();
	return TRUE;
}


CString CSeResultDlg::BuildCsvContent() const
{
	CString content;
	content += _T("视图,层数,工具,长度(mm),角度(°),面积(mm^2),最大值(HU),最小值(HU),平均值(HU),标准差(HU)\r\n");
	int nRowCount = m_ResultList.GetItemCount();
	for (int row = 0; row < nRowCount; ++row)
	{
		CString line;
		for (int col = 0; col < 10; ++col)
		{
			CString cell = m_ResultList.GetItemText(row, col);
			cell.Replace(_T("\""), _T("\"\""));
			line += _T("\"") + cell + _T("\"");
			if (col != 9)
			{
				line += _T(",");
			}
		}
		line += _T("\r\n");
		content += line;
	}
	return content;
}


BOOL CSeResultDlg::SaveMarksToFile(const CString& strFilePath)
{
	CFile file;
	if (!file.Open(strFilePath, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
	{
		return FALSE;
	}

	try
	{
		CArchive ar(&file, CArchive::store);
		int nVersion = 1;
		ar << nVersion;

		vector<CMeasurement*>& vecResult = SeMPRView::GetResult();
		int nCount = 0;
		for (size_t i = 0; i < vecResult.size(); ++i)
		{
			if (vecResult[i] != NULL)
				nCount++;
		}
		ar << nCount;

		for (size_t i = 0; i < vecResult.size(); ++i)
		{
			CMeasurement* pMeasurement = vecResult[i];
			if (pMeasurement == NULL)
				continue;

			CString strTool = pMeasurement->m_strToolName;
			ar << strTool;
			ar << pMeasurement->m_nPlaneNumber;
			ar << pMeasurement->m_nSliceNumber;
			int nSelected = pMeasurement->m_nSelected ? 1 : 0;
			ar << nSelected;
			ar << pMeasurement->m_dPixelSize;

			int nResultCount = static_cast<int>(pMeasurement->m_vecResults.size());
			ar << nResultCount;
			for (int j = 0; j < nResultCount; ++j)
			{
				ar << pMeasurement->m_vecResults[j].first;
				ar << pMeasurement->m_vecResults[j].second;
			}

			if (strTool == _T("Line"))
			{
				CMeaLine* pLine = dynamic_cast<CMeaLine*>(pMeasurement);
				ar << pLine->m_ptStart.x << pLine->m_ptStart.y;
				ar << pLine->m_ptEnd.x << pLine->m_ptEnd.y;
				int nProfile = static_cast<int>(pLine->m_profileAxis.size());
				ar << nProfile;
				for (int k = 0; k < nProfile; ++k)
				{
					double axis = (k < static_cast<int>(pLine->m_profileAxis.size())) ? pLine->m_profileAxis[k] : 0.0;
					double value = (k < static_cast<int>(pLine->m_profileValues.size())) ? pLine->m_profileValues[k] : 0.0;
					ar << axis << value;
				}
			}
			else if (strTool == _T("Angle"))
			{
				CMeaAngle* pAngle = dynamic_cast<CMeaAngle*>(pMeasurement);
				ar << pAngle->m_ptStart.x << pAngle->m_ptStart.y;
				ar << pAngle->m_ptAngle.x << pAngle->m_ptAngle.y;
				ar << pAngle->m_ptEnd.x << pAngle->m_ptEnd.y;
			}
			else if (strTool == _T("Shape"))
			{
				CMeaShape* pShape = dynamic_cast<CMeaShape*>(pMeasurement);
				ar << pShape->m_ptTopLeft.x << pShape->m_ptTopLeft.y;
				ar << pShape->m_ptBottonRight.x << pShape->m_ptBottonRight.y;
				int nSamples = static_cast<int>(pShape->m_samples.size());
				ar << nSamples;
				for (int k = 0; k < nSamples; ++k)
				{
					ar << pShape->m_samples[k];
				}
			}
			else if (strTool == _T("CT"))
			{
				CMeaCT* pCT = dynamic_cast<CMeaCT*>(pMeasurement);
				ar << pCT->m_ptPostion.x << pCT->m_ptPostion.y;
			}
			else if (strTool == _T("Area"))
			{
				CMeaArea* pArea = dynamic_cast<CMeaArea*>(pMeasurement);
				int nPts = static_cast<int>(pArea->m_vecPoints.size());
				ar << nPts;
				for (int k = 0; k < nPts; ++k)
				{
					ar << pArea->m_vecPoints[k].x << pArea->m_vecPoints[k].y;
				}
				int nSamples = static_cast<int>(pArea->m_samples.size());
				ar << nSamples;
				for (int k = 0; k < nSamples; ++k)
				{
					ar << pArea->m_samples[k];
				}
			}
			else if (strTool == _T("Ellipse"))
			{
				CMeaEllipse* pEllipse = dynamic_cast<CMeaEllipse*>(pMeasurement);
				ar << pEllipse->m_ptStart.x << pEllipse->m_ptStart.y;
				ar << pEllipse->m_ptEnd.x << pEllipse->m_ptEnd.y;
				int nCircle = pEllipse->m_bIsCircle ? 1 : 0;
				ar << nCircle;
				int nSamples = static_cast<int>(pEllipse->m_samples.size());
				ar << nSamples;
				for (int k = 0; k < nSamples; ++k)
				{
					ar << pEllipse->m_samples[k];
				}
			}
		}
		ar.Flush();
	}
	catch (CException* e)
	{
		e->Delete();
		file.Close();
		return FALSE;
	}

	file.Close();
	return TRUE;
}


BOOL CSeResultDlg::LoadMarksFromFile(const CString& strFilePath)
{
	CFile file;
	if (!file.Open(strFilePath, CFile::modeRead | CFile::typeBinary))
	{
		return FALSE;
	}

	try
	{
		CArchive ar(&file, CArchive::load);
		int nVersion = 0;
		ar >> nVersion;
		if (nVersion != 1)
		{
			file.Close();
			return FALSE;
		}

		ClearAllMeasurements();
		vector<CMeasurement*>& vecResult = SeMPRView::GetResult();

		int nCount = 0;
		ar >> nCount;
		for (int i = 0; i < nCount; ++i)
		{
			CString strTool;
			ar >> strTool;

			int nPlane = 0;
			int nSlice = 0;
			int nSelected = 0;
			double dPixel = 0.0;
			ar >> nPlane;
			ar >> nSlice;
			ar >> nSelected;
			ar >> dPixel;

			int nResultCount = 0;
			ar >> nResultCount;
			vector<pair<CString, double>> vecResults;
			for (int j = 0; j < nResultCount; ++j)
			{
				CString title;
				double value = 0.0;
				ar >> title;
				ar >> value;
				vecResults.push_back(make_pair(title, value));
			}

			CMeasurement* pMeasurement = NULL;
			if (strTool == _T("Line"))
			{
				CMeaLine* pLine = new CMeaLine();
				ar >> pLine->m_ptStart.x >> pLine->m_ptStart.y;
				ar >> pLine->m_ptEnd.x >> pLine->m_ptEnd.y;
				int nProfile = 0;
				ar >> nProfile;
				pLine->m_profileAxis.clear();
				pLine->m_profileValues.clear();
				for (int k = 0; k < nProfile; ++k)
				{
					double axis = 0.0;
					double value = 0.0;
					ar >> axis >> value;
					pLine->m_profileAxis.push_back(axis);
					pLine->m_profileValues.push_back(value);
				}
				pMeasurement = pLine;
			}
			else if (strTool == _T("Angle"))
			{
				CMeaAngle* pAngle = new CMeaAngle();
				ar >> pAngle->m_ptStart.x >> pAngle->m_ptStart.y;
				ar >> pAngle->m_ptAngle.x >> pAngle->m_ptAngle.y;
				ar >> pAngle->m_ptEnd.x >> pAngle->m_ptEnd.y;
				pMeasurement = pAngle;
			}
			else if (strTool == _T("Shape"))
			{
				CMeaShape* pShape = new CMeaShape();
				ar >> pShape->m_ptTopLeft.x >> pShape->m_ptTopLeft.y;
				ar >> pShape->m_ptBottonRight.x >> pShape->m_ptBottonRight.y;
				int nSamples = 0;
				ar >> nSamples;
				pShape->m_samples.clear();
				for (int k = 0; k < nSamples; ++k)
				{
					int sampleValue = 0;
					ar >> sampleValue;
					pShape->m_samples.push_back(sampleValue);
				}
				pMeasurement = pShape;
			}
			else if (strTool == _T("CT"))
			{
				CMeaCT* pCT = new CMeaCT();
				ar >> pCT->m_ptPostion.x >> pCT->m_ptPostion.y;
				pMeasurement = pCT;
			}
			else if (strTool == _T("Area"))
			{
				CMeaArea* pArea = new CMeaArea();
				int nPts = 0;
				ar >> nPts;
				pArea->m_vecPoints.clear();
				for (int k = 0; k < nPts; ++k)
				{
					CPoint pt;
					ar >> pt.x >> pt.y;
					pArea->m_vecPoints.push_back(pt);
				}
				int nSamples = 0;
				ar >> nSamples;
				pArea->m_samples.clear();
				for (int k = 0; k < nSamples; ++k)
				{
					int sampleValue = 0;
					ar >> sampleValue;
					pArea->m_samples.push_back(sampleValue);
				}
				pMeasurement = pArea;
			}
			else if (strTool == _T("Ellipse"))
			{
				CMeaEllipse* pEllipse = new CMeaEllipse();
				ar >> pEllipse->m_ptStart.x >> pEllipse->m_ptStart.y;
				ar >> pEllipse->m_ptEnd.x >> pEllipse->m_ptEnd.y;
				int nCircle = 0;
				ar >> nCircle;
				pEllipse->m_bIsCircle = (nCircle != 0);
				int nSamples = 0;
				ar >> nSamples;
				pEllipse->m_samples.clear();
				for (int k = 0; k < nSamples; ++k)
				{
					int sampleValue = 0;
					ar >> sampleValue;
					pEllipse->m_samples.push_back(sampleValue);
				}
				pMeasurement = pEllipse;
			}

			if (pMeasurement == NULL)
				continue;

			pMeasurement->m_strToolName = strTool;
			pMeasurement->m_nPlaneNumber = nPlane;
			pMeasurement->m_nSliceNumber = nSlice;
			pMeasurement->m_nSelected = nSelected != 0;
			pMeasurement->m_dPixelSize = dPixel;
			pMeasurement->m_vecResults = vecResults;
			AttachCurrentDcm(pMeasurement);
			vecResult.push_back(pMeasurement);
		}
	}
	catch (CException* e)
	{
		e->Delete();
		file.Close();
		ClearAllMeasurements();
		return FALSE;
	}

	file.Close();
	return TRUE;
}


void CSeResultDlg::ClearAllMeasurements()
{
	vector<CMeasurement*>& vecResult = SeMPRView::GetResult();
	for (size_t i = 0; i < vecResult.size(); ++i)
	{
		CMeasurement* pMeasurement = vecResult[i];
		if (pMeasurement != NULL)
		{
			delete pMeasurement;
		}
	}
	vecResult.clear();
	m_ResultList.DeleteAllItems();
}


void CSeResultDlg::AttachCurrentDcm(CMeasurement* pMeasurement) const
{
	if (pMeasurement == NULL)
		return;

	SeMPRView* pView = NULL;
	switch (pMeasurement->m_nPlaneNumber)
	{
	case 1:
		pView = theGeneralSwapData.m_pXOYView;
		break;
	case 2:
		pView = theGeneralSwapData.m_pXOZView;
		break;
	case 3:
		pView = theGeneralSwapData.m_pYOZView;
		break;
	default:
		break;
	}

	if (pView == NULL)
		return;

	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm == NULL)
		return;

	CString strTool = pMeasurement->m_strToolName;
	if (strTool == _T("Line"))
	{
		CMeaLine* pLine = dynamic_cast<CMeaLine*>(pMeasurement);
		if (pLine != NULL)
			pLine->pDcm = pDcm;
	}
	else if (strTool == _T("Area"))
	{
		CMeaArea* pArea = dynamic_cast<CMeaArea*>(pMeasurement);
		if (pArea != NULL)
			pArea->pDcm = pDcm;
	}
	else if (strTool == _T("Ellipse"))
	{
		CMeaEllipse* pEllipse = dynamic_cast<CMeaEllipse*>(pMeasurement);
		if (pEllipse != NULL)
			pEllipse->pDcm = pDcm;
	}
	else if (strTool == _T("Shape"))
	{
		CMeaShape* pShape = dynamic_cast<CMeaShape*>(pMeasurement);
		if (pShape != NULL)
			pShape->pDcm = pDcm;
	}
	else if (strTool == _T("CT"))
	{
		CMeaCT* pCT = dynamic_cast<CMeaCT*>(pMeasurement);
		if (pCT != NULL)
			pCT->pDcm = pDcm;
	}
}
