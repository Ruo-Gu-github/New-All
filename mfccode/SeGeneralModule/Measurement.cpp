#include "StdAfx.h"
#include "Measurement.h"

#include <algorithm>
#include <numeric>
#include <cmath>


CMeasurement::CMeasurement(void)
{
}


CMeasurement::~CMeasurement(void)
{
}

void CMeasurement::GetMeasurement()
{

}

void CMeasurement::UpdateRect(CPoint ptMove, CRect rtValid)
{

}

BOOL CMeasurement::GetProfileData(std::vector<double>& xAxis, std::vector<double>& values) const
{
	xAxis.clear();
	values.clear();
	return FALSE;
}

BOOL CMeasurement::GetSampleValues(std::vector<int>& samples) const
{
	samples.clear();
	return FALSE;
}

CMeaArea::CMeaArea()
{
	m_strToolName = "Area";
	pDcm = NULL;
}

CMeaArea::~CMeaArea()
{

}

void CMeaArea::GetMeasurement()
{
	m_vecResults.clear();
	m_samples.clear();
	if (pDcm == NULL || m_vecPoints.size() < 3)
		return;

	const int nCount = static_cast<int>(m_vecPoints.size());
	std::vector<PointF> pts(nCount);
	for (int i = 0; i < nCount; ++i)
	{
		pts[i].X = static_cast<REAL>(m_vecPoints[i].x);
		pts[i].Y = static_cast<REAL>(m_vecPoints[i].y);
	}

	GraphicsPath path;
	path.AddPolygon(&pts[0], nCount);
	path.CloseFigure();

	Region intersectRegion(&path);
	Matrix matrix;
	int rectCount = intersectRegion.GetRegionScansCount(&matrix);
	if (rectCount <= 0)
		return;

	std::vector<Rect> rects(rectCount);
	intersectRegion.GetRegionScans(&matrix, &rects[0], &rectCount);

	std::vector<int> samples;
	samples.reserve(rectCount * 32);

	for (int idx = 0; idx < rectCount; ++idx)
	{
		const Rect& rc = rects[idx];
		const int xEnd = rc.X + rc.Width;
		const int yEnd = rc.Y + rc.Height;
		for (int x = rc.X; x < xEnd; ++x)
		{
			if (x < 0 || x >= pDcm->GetWidth())
				continue;
			for (int y = rc.Y; y < yEnd; ++y)
			{
				if (y < 0 || y >= pDcm->GetHeight())
					continue;
				samples.push_back(pDcm->GetPixelData(x, y));
			}
		}
	}

	if (samples.empty())
		return;

	m_samples = samples;

	const double area = static_cast<double>(samples.size()) * m_dPixelSize * m_dPixelSize;

	double minValue = static_cast<double>(samples[0]);
	double maxValue = static_cast<double>(samples[0]);
	const int sampleCount = static_cast<int>(samples.size());
	for (int i = 1; i < sampleCount; ++i)
	{
		if (samples[i] < minValue)
			minValue = static_cast<double>(samples[i]);
		if (samples[i] > maxValue)
			maxValue = static_cast<double>(samples[i]);
	}

	const double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
	const double meanValue = sum / samples.size();

	double sqSum = 0.0;
	for (int i = 0; i < sampleCount; ++i)
	{
		const double diff = static_cast<double>(samples[i]) - meanValue;
		sqSum += diff * diff;
	}
	const double stdDev = sqrt(sqSum / samples.size());

	m_vecResults.push_back(make_pair<CString, double>("面积(mm^2)", area));
	m_vecResults.push_back(make_pair<CString, double>("最大值(HU)", maxValue));
	m_vecResults.push_back(make_pair<CString, double>("最小值(HU)", minValue));
	m_vecResults.push_back(make_pair<CString, double>("平均值(HU)", meanValue));
	m_vecResults.push_back(make_pair<CString, double>("标准差(HU)", stdDev));
}

void CMeaArea::UpdateRect(CPoint ptMove, CRect rtValid)
{

}

BOOL CMeaArea::GetSampleValues(std::vector<int>& samples) const
{
	samples = m_samples;
	return !samples.empty();
}

CMeaCT::CMeaCT()
{
	m_strToolName = "CT";
	pDcm = NULL;
}

CMeaCT::~CMeaCT()
{

}

void CMeaCT::GetMeasurement()
{
	if(pDcm == NULL) return;
	int nWidth = pDcm->GetWidth();
	int nHeight = pDcm->GetHeight();
	double result = pDcm->GetPixelData(m_ptPostion.x, m_ptPostion.y);
	if (m_ptPostion.x < 0 || m_ptPostion.y < 0 || m_ptPostion.x >= nWidth || m_ptPostion.y >= nHeight)
		result = 0.0f;	
	m_vecResults.clear();
	m_vecResults.push_back(make_pair<CString, double>("CT值(HU)", result));
}

void CMeaCT::UpdateRect(CPoint ptMove, CRect rtValid)
{

}

CMeaShape::CMeaShape()
{
	m_strToolName = "Shape";
	pDcm = NULL;
}

CMeaShape::~CMeaShape()
{

}

void CMeaShape::GetMeasurement()
{
	m_vecResults.clear();
	m_samples.clear();
	if (pDcm == NULL)
		return;

	const int left = min(m_ptTopLeft.x, m_ptBottonRight.x);
	const int top = min(m_ptTopLeft.y, m_ptBottonRight.y);
	const int right = max(m_ptTopLeft.x, m_ptBottonRight.x);
	const int bottom = max(m_ptTopLeft.y, m_ptBottonRight.y);

	const int widthPixels = right - left;
	const int heightPixels = bottom - top;
	if (widthPixels <= 0 || heightPixels <= 0)
		return;

	for (int y = top; y < bottom; ++y)
	{
		if (y < 0 || y >= pDcm->GetHeight())
			continue;
		for (int x = left; x < right; ++x)
		{
			if (x < 0 || x >= pDcm->GetWidth())
				continue;
			m_samples.push_back(pDcm->GetPixelData(x, y));
		}
	}

	if (m_samples.empty())
		return;

	const double area = static_cast<double>(widthPixels) * static_cast<double>(heightPixels) * m_dPixelSize * m_dPixelSize;

	double minValue = static_cast<double>(m_samples[0]);
	double maxValue = static_cast<double>(m_samples[0]);
	double sum = 0.0;
	const int sampleCount = static_cast<int>(m_samples.size());
	for (int i = 0; i < sampleCount; ++i)
	{
		const double v = static_cast<double>(m_samples[i]);
		sum += v;
		if (v < minValue)
			minValue = v;
		if (v > maxValue)
			maxValue = v;
	}
	const double meanValue = sum / static_cast<double>(sampleCount);

	double sqSum = 0.0;
	for (int i = 0; i < sampleCount; ++i)
	{
		const double diff = static_cast<double>(m_samples[i]) - meanValue;
		sqSum += diff * diff;
	}
	const double stdDev = sqrt(sqSum / static_cast<double>(sampleCount));

	m_vecResults.push_back(make_pair<CString, double>("面积(mm^2)", area));
	m_vecResults.push_back(make_pair<CString, double>("最大值(HU)", maxValue));
	m_vecResults.push_back(make_pair<CString, double>("最小值(HU)", minValue));
	m_vecResults.push_back(make_pair<CString, double>("平均值(HU)", meanValue));
	m_vecResults.push_back(make_pair<CString, double>("标准差(HU)", stdDev));
}

void CMeaShape::UpdateRect(CPoint ptMove, CRect rtValid)
{
	if(!rtValid.PtInRect(m_ptTopLeft + ptMove) || !rtValid.PtInRect(m_ptBottonRight + ptMove))
		return;
	m_ptTopLeft += ptMove;
	m_ptBottonRight += ptMove;
}

BOOL CMeaShape::GetSampleValues(std::vector<int>& samples) const
{
	samples = m_samples;
	return !samples.empty();
}

CMeaAngle::CMeaAngle()
{
	m_strToolName = "Angle";
}

CMeaAngle::~CMeaAngle()
{

}

void CMeaAngle::GetMeasurement()
{
	double x1 = m_ptStart.x;
	double x2 = m_ptAngle.x;
	double x3 = m_ptEnd.x;
	double y1 = m_ptStart.y;
	double y2 = m_ptAngle.y;
	double y3 = m_ptEnd.y;
	double theta = atan2(x1 - x2, y1 - y2) - atan2(x2 - x3, y2 - y3);
	if (theta > M_PI)
		theta -= 2 * M_PI;
	if (theta < -M_PI)
		theta += 2 * M_PI;

	theta = 180.0 - abs(theta * 180.0 / M_PI);
	m_vecResults.push_back(make_pair<CString, double>("角度(°)", theta));
}

void CMeaAngle::UpdateRect(CPoint ptMove, CRect rtValid)
{
	if(!rtValid.PtInRect(m_ptStart + ptMove) || !rtValid.PtInRect(m_ptEnd + ptMove) || !rtValid.PtInRect(m_ptAngle + ptMove))
		return;
	m_ptStart += ptMove;
	m_ptAngle += ptMove;
	m_ptEnd += ptMove;
}

CMeaLine::CMeaLine()
{
	m_strToolName = "Line";
	pDcm = NULL;
}

CMeaLine::~CMeaLine()
{

}

void CMeaLine::GetMeasurement()
{
	double width = static_cast<double>(m_ptStart.x - m_ptEnd.x);
	double height = static_cast<double>(m_ptStart.y - m_ptEnd.y);
	double pixelLength = sqrt((width * width) + (height * height));
	double result = pixelLength * m_dPixelSize;

	m_vecResults.clear();
	m_vecResults.push_back(make_pair<CString, double>("长度(mm)", result));

	m_profileAxis.clear();
	m_profileValues.clear();

	if (pDcm == NULL)
		return;

	const int dx = m_ptEnd.x - m_ptStart.x;
	const int dy = m_ptEnd.y - m_ptStart.y;
	int steps = abs(dx);
	if (abs(dy) > steps)
		steps = abs(dy);
	if (steps <= 0)
	{
		if (m_ptStart.x >= 0 && m_ptStart.y >= 0 && m_ptStart.x < pDcm->GetWidth() && m_ptStart.y < pDcm->GetHeight())
		{
			int value = pDcm->GetPixelData(m_ptStart.x, m_ptStart.y);
			m_profileAxis.push_back(0.0);
			m_profileValues.push_back(static_cast<double>(value));
		}
		return;
	}

	for (int i = 0; i <= steps; ++i)
	{
		double t = static_cast<double>(i) / static_cast<double>(steps);
		double fx = static_cast<double>(m_ptStart.x) + t * static_cast<double>(dx);
		double fy = static_cast<double>(m_ptStart.y) + t * static_cast<double>(dy);
		int ix = static_cast<int>(floor(fx + 0.5));
		int iy = static_cast<int>(floor(fy + 0.5));
		if (ix < 0 || iy < 0 || ix >= pDcm->GetWidth() || iy >= pDcm->GetHeight())
			continue;
		int value = pDcm->GetPixelData(ix, iy);
		double distance = result * t;
		m_profileAxis.push_back(distance);
		m_profileValues.push_back(static_cast<double>(value));
	}
}

void CMeaLine::UpdateRect(CPoint ptMove, CRect rtValid)
{
	if(!rtValid.PtInRect(m_ptStart + ptMove) || !rtValid.PtInRect(m_ptEnd + ptMove))
		return;
	m_ptStart += ptMove;
	m_ptEnd += ptMove;
}

BOOL CMeaLine::GetProfileData(std::vector<double>& xAxis, std::vector<double>& values) const
{
	xAxis = m_profileAxis;
	values = m_profileValues;
	return !xAxis.empty() && xAxis.size() == values.size();
}

BOOL CMeaLine::GetSampleValues(std::vector<int>& samples) const
{
	samples.clear();
	if (m_profileValues.empty())
		return FALSE;
	samples.reserve(m_profileValues.size());
	for (size_t i = 0; i < m_profileValues.size(); ++i)
	{
		samples.push_back(static_cast<int>(m_profileValues[i]));
	}
	return !samples.empty();
}

CMeaEllipse::CMeaEllipse()
{
	m_strToolName = "Ellipse";
}

CMeaEllipse::~CMeaEllipse()
{
}

void CMeaEllipse::GetMeasurement()
{
	m_vecResults.clear();
	m_samples.clear();
	if (pDcm == NULL)
		return;

	const int left = min(m_ptStart.x, m_ptEnd.x);
	const int top = min(m_ptStart.y, m_ptEnd.y);
	const int right = max(m_ptStart.x, m_ptEnd.x);
	const int bottom = max(m_ptStart.y, m_ptEnd.y);

	const int widthPixels = right - left;
	const int heightPixels = bottom - top;
	if (widthPixels <= 0 || heightPixels <= 0)
		return;

	const double a = static_cast<double>(widthPixels) * m_dPixelSize * 0.5;
	const double b = static_cast<double>(heightPixels) * m_dPixelSize * 0.5;
	const double area = M_PI * a * b;

	const double dx = (static_cast<double>(m_ptStart.x) - static_cast<double>(m_ptEnd.x)) * m_dPixelSize;
	const double dy = (static_cast<double>(m_ptStart.y) - static_cast<double>(m_ptEnd.y)) * m_dPixelSize;
	const double length = sqrt(dx * dx + dy * dy);

	const double cx = (static_cast<double>(left) + static_cast<double>(right)) * 0.5;
	const double cy = (static_cast<double>(top) + static_cast<double>(bottom)) * 0.5;
	const double rx = static_cast<double>(widthPixels) * 0.5;
	const double ry = static_cast<double>(heightPixels) * 0.5;

	std::vector<int> samples;
	samples.reserve(widthPixels * heightPixels);

	for (int y = top; y <= bottom; ++y)
	{
		if (y < 0 || y >= pDcm->GetHeight())
			continue;
		for (int x = left; x <= right; ++x)
		{
			if (x < 0 || x >= pDcm->GetWidth())
				continue;

			double nx = rx > 0.0 ? ((static_cast<double>(x) + 0.5) - cx) / rx : 0.0;
			double ny = ry > 0.0 ? ((static_cast<double>(y) + 0.5) - cy) / ry : 0.0;
			if ((nx * nx + ny * ny) <= 1.0)
			{
				samples.push_back(pDcm->GetPixelData(x, y));
			}
		}
	}

	if (samples.empty())
		return;

	m_samples = samples;

	double minValue = static_cast<double>(samples[0]);
	double maxValue = static_cast<double>(samples[0]);
	const int sampleCount = static_cast<int>(samples.size());
	for (int i = 1; i < sampleCount; ++i)
	{
		if (samples[i] < minValue)
			minValue = static_cast<double>(samples[i]);
		if (samples[i] > maxValue)
			maxValue = static_cast<double>(samples[i]);
	}

	const double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
	const double meanValue = sum / samples.size();

	double sqSum = 0.0;
	for (int i = 0; i < sampleCount; ++i)
	{
		const double diff = static_cast<double>(samples[i]) - meanValue;
		sqSum += diff * diff;
	}
	const double stdDev = sqrt(sqSum / samples.size());

	m_vecResults.push_back(make_pair<CString, double>("面积(mm^2)", area));
	m_vecResults.push_back(make_pair<CString, double>("长度(mm)", length));
	m_vecResults.push_back(make_pair<CString, double>("最大值(HU)", maxValue));
	m_vecResults.push_back(make_pair<CString, double>("最小值(HU)", minValue));
	m_vecResults.push_back(make_pair<CString, double>("平均值(HU)", meanValue));
	m_vecResults.push_back(make_pair<CString, double>("标准差(HU)", stdDev));
}
void CMeaEllipse::UpdateRect(CPoint ptMove, CRect rtValid)
{
	if(!rtValid.PtInRect(m_ptStart + ptMove) || !rtValid.PtInRect(m_ptEnd + ptMove))
		return;
	m_ptStart += ptMove;
	m_ptEnd += ptMove;
}

BOOL CMeaEllipse::GetSampleValues(std::vector<int>& samples) const
{
	samples = m_samples;
	return !samples.empty();
}
