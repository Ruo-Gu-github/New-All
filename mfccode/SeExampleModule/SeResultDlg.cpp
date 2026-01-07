// SeResultDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SeExampleModule.h"
#include "SeResultDlg.h"
#include "afxdialogex.h"


// CSeResultDlg 对话框

IMPLEMENT_DYNAMIC(CSeResultDlg, CDialogEx)

CSeResultDlg::CSeResultDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSeResultDlg::IDD, pParent)
{

}

CSeResultDlg::~CSeResultDlg()
{
}

void CSeResultDlg::AddMaskStat(const BYTE* mask, const short* image, int width, int height, int depth, double xySpacing, double zSpacing, double density, const CString& name)
{
	MaskStatResult stat = CalcMaskStats(mask, image, width, height, depth, xySpacing, zSpacing, density);
	stat.name = name;
	m_statResults.push_back(stat);
}

void CSeResultDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSeResultDlg, CDialogEx)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_RESULT, &CSeResultDlg::OnLvnItemchangedListResult)
END_MESSAGE_MAP()


// CSeResultDlg 消息处理程序

MaskStatResult CalcMaskStats(const BYTE* mask, const short* image, int width, int height, int depth, double xySpacing, double zSpacing, double density)
{
	MaskStatResult result = {0};
	double sum = 0.0, sum2 = 0.0;
	double minVal = 1e9, maxVal = -1e9;
	int voxelCount = 0;
	int areaCount = 0;

	int sliceSize = width * height;
	for (int k = 0; k < depth; ++k) {
		for (int j = 0; j < height; ++j) {
			for (int i = 0; i < width; ++i) {
				int idx = k * sliceSize + j * width + i;
				if (mask[idx] == 255) {
					short val = image[idx];
					sum += val;
					sum2 += val * val;
					if (val < minVal) minVal = val;
					if (val > maxVal) maxVal = val;
					++voxelCount;

					// 统计边界像素（面积近似）
					bool isEdge = false;
					for (int dz = -1; dz <= 1 && !isEdge; ++dz) {
						for (int dy = -1; dy <= 1 && !isEdge; ++dy) {
							for (int dx = -1; dx <= 1 && !isEdge; ++dx) {
								if (dx == 0 && dy == 0 && dz == 0) continue;
								int ni = i + dx, nj = j + dy, nk = k + dz;
								if (ni < 0 || ni >= width || nj < 0 || nj >= height || nk < 0 || nk >= depth) {
									isEdge = true;
								} else {
									int nidx = nk * sliceSize + nj * width + ni;
									if (mask[nidx] != 255) isEdge = true;
								}
							}
						}
					}
					if (isEdge) ++areaCount;
				}
			}
		}
	}

	result.voxelCount = voxelCount;
	result.volume = voxelCount * xySpacing * xySpacing * zSpacing;
	result.area = areaCount * xySpacing * xySpacing;
	result.mean = voxelCount > 0 ? sum / voxelCount : 0;
	result.max = voxelCount > 0 ? maxVal : 0;
	result.min = voxelCount > 0 ? minVal : 0;
	result.variance = voxelCount > 0 ? sqrt(sum2 / voxelCount - result.mean * result.mean) : 0;
	result.mass = (density > 0.0) ? (result.volume * density) : 0.0;

	return result;
}


BOOL CSeResultDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_RESULT);
	if (pList)
	{
		pList->ModifyStyle(0, LVS_REPORT); // 强制设置为报表模式
		pList->InsertColumn(0, _T("名称"), LVCFMT_LEFT, 120);
		pList->InsertColumn(1, _T("体素数"), LVCFMT_LEFT, 80);
		pList->InsertColumn(2, _T("体积(mm³)"), LVCFMT_LEFT, 100);
		pList->InsertColumn(3, _T("面积(mm²)"), LVCFMT_LEFT, 100);
		pList->InsertColumn(4, _T("平均值(HU)"), LVCFMT_LEFT, 80);
		pList->InsertColumn(5, _T("最大值(HU)"), LVCFMT_LEFT, 80);
		pList->InsertColumn(6, _T("最小值(HU)"), LVCFMT_LEFT, 80);
		pList->InsertColumn(7, _T("方差(HU)"), LVCFMT_LEFT, 80);

		for (size_t i = 0; i < m_statResults.size(); ++i) {
			int row = pList->InsertItem(i, m_statResults[i].name);
			CString str;
			str.Format(_T("%d"), m_statResults[i].voxelCount);
			pList->SetItemText(row, 1, str);

			str.Format(_T("%.2f"), m_statResults[i].volume);
			pList->SetItemText(row, 2, str);

			str.Format(_T("%.2f"), m_statResults[i].area);
			pList->SetItemText(row, 3, str);

			str.Format(_T("%.2f"), m_statResults[i].mean);
			pList->SetItemText(row, 4, str);

			str.Format(_T("%.2f"), m_statResults[i].max);
			pList->SetItemText(row, 5, str);

			str.Format(_T("%.2f"), m_statResults[i].min);
			pList->SetItemText(row, 6, str);

			str.Format(_T("%.2f"), m_statResults[i].variance);
			pList->SetItemText(row, 7, str);

		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CSeResultDlg::OnLvnItemchangedListResult(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}
