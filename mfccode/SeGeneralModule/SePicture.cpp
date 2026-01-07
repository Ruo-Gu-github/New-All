// SePicture.cpp : 实现文件
//

#include "stdafx.h"
#include "SeGeneralModule.h"
#include "SePicture.h"
#include "afxdialogex.h"

#include <algorithm>
#include <cmath>

// CSePicture 对话框

IMPLEMENT_DYNAMIC(CSePicture, CDialogEx)

CSePicture::CSePicture(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSePicture::IDD, pParent)
	, m_pMeasurement(NULL)
	, m_bHistogram(false)
	, m_bDataReady(false)
	, m_nHistMaxCount(0)
	, m_histMinValue(0.0)
	, m_histMaxValue(0.0)
{

}

CSePicture::~CSePicture()
{
}

void CSePicture::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSePicture, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSePicture::OnBnClickedOk)
	ON_WM_PAINT()
END_MESSAGE_MAP()


// CSePicture 消息处理程序


void CSePicture::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
}

BOOL CSePicture::SetMeasurement(CMeasurement* pMeasurement)
{
	m_pMeasurement = pMeasurement;
	m_profileAxis.clear();
	m_profileValues.clear();
	m_histBins.clear();
	m_histCounts.clear();
	m_nHistMaxCount = 0;
	m_histMinValue = 0.0;
	m_histMaxValue = 0.0;
	m_bHistogram = false;
	m_bDataReady = false;
	m_strChartTitle.Empty();

	if (m_pMeasurement == NULL)
	{
		return FALSE;
	}

	CString strTool = m_pMeasurement->m_strToolName;
	if (strTool == _T("Line"))
	{
		std::vector<double> axis;
		std::vector<double> values;
		if (!m_pMeasurement->GetProfileData(axis, values))
		{
			return FALSE;
		}
		m_profileAxis.assign(axis.begin(), axis.end());
		m_profileValues.assign(values.begin(), values.end());
		m_bHistogram = false;
		m_bDataReady = !m_profileAxis.empty();
		m_strChartTitle.Format(_T("%s Profile 曲线"), strTool); 
	}
	else if (strTool == _T("Shape") || strTool == _T("Area") || strTool == _T("Ellipse"))
	{
		std::vector<int> samples;
		if (!m_pMeasurement->GetSampleValues(samples))
		{
			return FALSE;
		}
		BuildHistogram(samples);
		m_bHistogram = true;
		m_bDataReady = !m_histCounts.empty();
		m_strChartTitle.Format(_T("%s CT值直方图"), strTool);
	}
	else
	{
		return FALSE;
	}

	return m_bDataReady ? TRUE : FALSE;
}

BOOL CSePicture::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	if (!m_strChartTitle.IsEmpty())
	{
		SetWindowText(m_strChartTitle);
	}

	return TRUE;
}

void CSePicture::OnPaint()
{
	CPaintDC dc(this);
	CRect rcClient;
	GetClientRect(&rcClient);
	dc.FillSolidRect(rcClient, RGB(255, 255, 255));

	if (!m_strChartTitle.IsEmpty())
	{
		CRect rcTitle = rcClient;
		rcTitle.bottom = rcTitle.top + 30;
		dc.SetBkMode(TRANSPARENT);
		dc.SetTextColor(RGB(40, 40, 40));
		dc.SelectObject(GetStockObject(DEFAULT_GUI_FONT));
		dc.DrawText(m_strChartTitle, rcTitle, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}

	if (!m_bDataReady)
	{
		CRect rcMessage = rcClient;
		 dc.SetBkMode(TRANSPARENT);
		 dc.SetTextColor(RGB(120, 120, 120));
		 dc.DrawText(_T("没有可显示的数据"), rcMessage, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		return;
	}

	CRect rcPlot = rcClient;
	rcPlot.DeflateRect(70, 50, 40, 70);
	if (rcPlot.Width() <= 0 || rcPlot.Height() <= 0)
		return;

	CPen axisPen(PS_SOLID, 1, RGB(0, 0, 0));
	CPen* pOldPen = dc.SelectObject(&axisPen);
	dc.MoveTo(rcPlot.left, rcPlot.bottom);
	dc.LineTo(rcPlot.left, rcPlot.top);
	dc.MoveTo(rcPlot.left, rcPlot.bottom);
	dc.LineTo(rcPlot.right, rcPlot.bottom);
	dc.SelectObject(pOldPen);

	if (m_bHistogram)
	{
		DrawHistogram(&dc, rcPlot);
	}
	else
	{
		DrawProfile(&dc, rcPlot);
	}
}

void CSePicture::DrawProfile(CDC* pDC, const CRect& rcPlotArea)
{
	if (m_profileAxis.empty() || m_profileValues.empty())
		return;

	double xMin = m_profileAxis.front();
	double xMax = m_profileAxis.front();
	for (size_t i = 1; i < m_profileAxis.size(); ++i)
	{
		xMin = std::min(xMin, m_profileAxis[i]);
		xMax = std::max(xMax, m_profileAxis[i]);
	}
	if (fabs(xMax - xMin) < 1e-6)
	{
		xMax = xMin + 1.0;
	}

	double yMin = m_profileValues[0];
	double yMax = m_profileValues[0];
	for (size_t i = 1; i < m_profileValues.size(); ++i)
	{
		yMin = std::min(yMin, m_profileValues[i]);
		yMax = std::max(yMax, m_profileValues[i]);
	}
	if (fabs(yMax - yMin) < 1e-6)
	{
		yMax += 1.0;
		yMin -= 1.0;
	}

	CPen dataPen(PS_SOLID, 2, RGB(0, 114, 198));
	CPen* pOldPen = pDC->SelectObject(&dataPen);
	for (size_t i = 0; i < m_profileAxis.size(); ++i)
	{
		double fx = (m_profileAxis[i] - xMin) / (xMax - xMin);
		double fy = (m_profileValues[i] - yMin) / (yMax - yMin);
		int x = rcPlotArea.left + static_cast<int>(fx * rcPlotArea.Width());
		int y = rcPlotArea.bottom - static_cast<int>(fy * rcPlotArea.Height());
		if (i == 0)
		{
			pDC->MoveTo(x, y);
		}
		else
		{
			pDC->LineTo(x, y);
		}
	}
	pDC->SelectObject(pOldPen);

	pDC->SetBkMode(TRANSPARENT);
	pDC->SetTextColor(RGB(80, 80, 80));
	CString strText;
	strText.Format(_T("%.2f"), xMin);
	pDC->TextOut(rcPlotArea.left - 10, rcPlotArea.bottom + 8, strText);
	strText.Format(_T("%.2f"), xMax);
	pDC->TextOut(rcPlotArea.right - 50, rcPlotArea.bottom + 8, strText);
	strText.Format(_T("%.2f"), yMin);
	pDC->TextOut(rcPlotArea.left - 60, rcPlotArea.bottom - 10, strText);
	strText.Format(_T("%.2f"), yMax);
	pDC->TextOut(rcPlotArea.left - 60, rcPlotArea.top - 10, strText);

	CString strXLabel = _T("距离 (mm)");
	pDC->TextOut(rcPlotArea.CenterPoint().x - 30, rcPlotArea.bottom + 28, strXLabel);
	CString strYLabel = _T("CT值 (HU)");
	pDC->TextOut(rcPlotArea.left - 65, rcPlotArea.top - 30, strYLabel);
}

void CSePicture::DrawHistogram(CDC* pDC, const CRect& rcPlotArea)
{
	if (m_histCounts.empty())
		return;

	int binCount = static_cast<int>(m_histCounts.size());
	int maxCount = (m_nHistMaxCount > 0) ? m_nHistMaxCount : 1;
	double barWidth = static_cast<double>(rcPlotArea.Width()) / static_cast<double>(binCount);
	if (barWidth < 1.0)
		barWidth = 1.0;

	CPen pen(PS_SOLID, 1, RGB(0, 90, 180));
	CPen* pOldPen = pDC->SelectObject(&pen);
	CBrush brush(RGB(0, 140, 220));
	CBrush* pOldBrush = pDC->SelectObject(&brush);

	for (int i = 0; i < binCount; ++i)
	{
		double fraction = static_cast<double>(m_histCounts[i]) / static_cast<double>(maxCount);
		fraction = std::min(1.0, std::max(0.0, fraction));
		int barHeight = static_cast<int>(fraction * rcPlotArea.Height());
		int left = rcPlotArea.left + static_cast<int>(i * barWidth);
		int right = rcPlotArea.left + static_cast<int>((i + 1) * barWidth) - 1;
		if (right < left)
			right = left;
		CRect barRect(left, rcPlotArea.bottom - barHeight, right, rcPlotArea.bottom);
		pDC->Rectangle(barRect);
	}

	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);

	pDC->SetBkMode(TRANSPARENT);
	pDC->SetTextColor(RGB(80, 80, 80));
	CString strText;
	strText.Format(_T("%.0f"), m_histMinValue);
	pDC->TextOut(rcPlotArea.left - 10, rcPlotArea.bottom + 8, strText);
	strText.Format(_T("%.0f"), m_histMaxValue);
	pDC->TextOut(rcPlotArea.right - 50, rcPlotArea.bottom + 8, strText);
	strText.Format(_T("%d"), 0);
	pDC->TextOut(rcPlotArea.left - 40, rcPlotArea.bottom - 10, strText);
	strText.Format(_T("%d"), maxCount);
	pDC->TextOut(rcPlotArea.left - 40, rcPlotArea.top - 10, strText);

	CString strXLabel = _T("CT值 (HU)");
	pDC->TextOut(rcPlotArea.CenterPoint().x - 30, rcPlotArea.bottom + 28, strXLabel);
	CString strYLabel = _T("频次");
	pDC->TextOut(rcPlotArea.left - 40, rcPlotArea.top - 30, strYLabel);
}

void CSePicture::BuildHistogram(const std::vector<int>& samples)
{
	m_histBins.clear();
	m_histCounts.clear();
	m_nHistMaxCount = 0;
	m_histMinValue = 0.0;
	m_histMaxValue = 0.0;

	if (samples.empty())
	{
		return;
	}

	int minValue = samples[0];
	int maxValue = samples[0];
	for (size_t i = 1; i < samples.size(); ++i)
	{
		minValue = std::min(minValue, samples[i]);
		maxValue = std::max(maxValue, samples[i]);
	}

	m_histMinValue = static_cast<double>(minValue);
	m_histMaxValue = static_cast<double>(maxValue);

	int binCount = 64;
	int range = maxValue - minValue + 1;
	if (range <= 0)
	{
		binCount = 1;
	}
	else if (range < binCount)
	{
		binCount = range;
	}

	m_histCounts.assign(binCount, 0);
	m_histBins.resize(binCount, 0.0);

	double binSize = (range <= 0) ? 1.0 : static_cast<double>(range) / static_cast<double>(binCount);
	if (binSize <= 0.0)
	{
		binSize = 1.0;
	}

	for (size_t i = 0; i < samples.size(); ++i)
	{
		int index = static_cast<int>(floor((static_cast<double>(samples[i] - minValue)) / binSize));
		if (index >= binCount)
			index = binCount - 1;
		if (index < 0)
			index = 0;
		m_histCounts[index]++;
	}

	m_nHistMaxCount = 0;
	for (int i = 0; i < binCount; ++i)
	{
		m_histBins[i] = static_cast<double>(minValue) + (static_cast<double>(i) + 0.5) * binSize;
		if (m_histCounts[i] > m_nHistMaxCount)
		{
			m_nHistMaxCount = m_histCounts[i];
		}
	}
	if (m_nHistMaxCount == 0)
	{
		m_nHistMaxCount = 1;
	}
}
