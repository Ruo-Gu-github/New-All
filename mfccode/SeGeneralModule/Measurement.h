#pragma once

#include <vector>

class CDcmPic;

class CMeasurement
{
public:
	CMeasurement(void);
	virtual ~CMeasurement(void);

public:
	int m_nPlaneNumber;
	int m_nSliceNumber;
	BOOL m_nSelected;
	double m_dPixelSize;
	CString m_strToolName;
	vector<pair<CString, double>> m_vecResults;
	virtual void GetMeasurement();
	virtual void UpdateRect(CPoint ptMove, CRect rtValid);
	virtual BOOL GetProfileData(std::vector<double>& xAxis, std::vector<double>& values) const;
	virtual BOOL GetSampleValues(std::vector<int>& samples) const;
};

class CMeaLine : public CMeasurement
{
public:
	CMeaLine();
	virtual ~CMeaLine();
	CPoint m_ptStart;
	CPoint m_ptEnd;
	CDcmPic* pDcm;
	std::vector<double> m_profileAxis;
	std::vector<double> m_profileValues;
	virtual void GetMeasurement();
	virtual void UpdateRect(CPoint ptMove, CRect rtValid);
	virtual BOOL GetProfileData(std::vector<double>& xAxis, std::vector<double>& values) const;
	virtual BOOL GetSampleValues(std::vector<int>& samples) const;
};

class CMeaAngle : public CMeasurement
{
public:
	CMeaAngle();
	virtual ~CMeaAngle();
	CPoint m_ptStart;
	CPoint m_ptAngle;
	CPoint m_ptEnd;
	virtual void GetMeasurement();
	virtual void UpdateRect(CPoint ptMove, CRect rtValid);
};

class CMeaShape : public CMeasurement
{
public:
	CMeaShape();
	virtual ~CMeaShape();
	CPoint m_ptTopLeft;
	CPoint m_ptBottonRight;
	CDcmPic* pDcm;
	std::vector<int> m_samples;
	virtual void GetMeasurement();
	virtual void UpdateRect(CPoint ptMove, CRect rtValid);
	virtual BOOL GetSampleValues(std::vector<int>& samples) const;
};

class CMeaCT : public CMeasurement
{
public:
	CMeaCT();
	virtual ~CMeaCT();
	CPoint m_ptPostion;
	CDcmPic *pDcm;
	virtual void GetMeasurement();
	virtual void UpdateRect(CPoint ptMove, CRect rtValide);
};

class CMeaArea : public CMeasurement
{
public:
	CMeaArea();
	virtual ~CMeaArea();
	CDcmPic *pDcm;
	vector<CPoint> m_vecPoints;
	std::vector<int> m_samples;
	virtual void GetMeasurement();
	virtual void UpdateRect(CPoint ptMove, CRect rtValid);
	virtual BOOL GetSampleValues(std::vector<int>& samples) const;
};


class CMeaEllipse : public CMeasurement
{
public:
	CMeaEllipse();
	virtual ~CMeaEllipse();

	CPoint m_ptStart;
	CPoint m_ptEnd;
	BOOL   m_bIsCircle; // ?????
	CDcmPic *pDcm;
	std::vector<int> m_samples;
	virtual void GetMeasurement();
	virtual void UpdateRect(CPoint ptMove, CRect rtValid);
	virtual BOOL GetSampleValues(std::vector<int>& samples) const;
};