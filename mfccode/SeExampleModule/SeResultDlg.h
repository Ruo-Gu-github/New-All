#pragma once
#include "Resource.h"

// CSeResultDlg 锟皆伙拷锟斤拷

struct MaskStatResult {
	int voxelCount;         // 体素数
	double volume;          // 体积 mm^3
	double area;            // 表面积（简单近似，统计边界像素数*面积）
	double mean;            // 平均值
	double max;             // 最大值
	double min;             // 最小值
	double variance;        // 方差
	double mass;            // 质量
	CString name;
};

MaskStatResult CalcMaskStats(
	const BYTE* mask,
	const short* image,
	int width, int height, int depth,
	double xySpacing,
	double zSpacing,
	double density
	);

class CSeResultDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSeResultDlg)

public:
	CSeResultDlg(CWnd* pParent = NULL);   
	virtual ~CSeResultDlg();


	enum { IDD = IDD_DIALOG_RESULT };

	void AddMaskStat(
		const BYTE* mask,
		const short* image,
		int width, int height, int depth,
		double xySpacing, double zSpacing, double density,
		const CString& name);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 
	std::vector<MaskStatResult> m_statResults; // 保存所有结果
	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
	afx_msg void OnLvnItemchangedListResult(NMHDR *pNMHDR, LRESULT *pResult);
};
