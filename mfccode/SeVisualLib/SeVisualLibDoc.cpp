// ImageViewerMPRDoc.cpp : implementation file
//

#include "stdafx.h"
#include "SeVisualLibDoc.h"
#include "LibSwapData.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// SeVisualLibDoc

IMPLEMENT_DYNCREATE(SeVisualLibDoc, CImageDisplayDoc)

	SeVisualLibDoc::SeVisualLibDoc()
{

}

BOOL SeVisualLibDoc::OnNewDocument()
{
	if (!CImageViewerDoc::OnNewDocument())
		return FALSE;

	theLibSwapData.m_MPROberverHost.AddChain(this);

	return TRUE;
}

SeVisualLibDoc::~SeVisualLibDoc()
{
}


BEGIN_MESSAGE_MAP(SeVisualLibDoc, CDocument)
	//{{AFX_MSG_MAP(SeVisualLibDoc)
	// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// SeVisualLibDoc diagnostics

#ifdef _DEBUG
void SeVisualLibDoc::AssertValid() const
{
	CImageDisplayDoc::AssertValid();
}

void SeVisualLibDoc::Dump(CDumpContext& dc) const
{
	CImageDisplayDoc::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// SeVisualLibDoc serialization

void SeVisualLibDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

void SeVisualLibDoc::OnProcessAllFinish( img_process ip,CSize cs )
{
	MPRParameter	P;
	P.nType			= ip;
	P.szDelta		= cs;

	theLibSwapData.m_szDelta = cs;
	if (ip == img_adjustwin)
	{
		theLibSwapData.m_nWinWidth = cs.cy;
		theLibSwapData.m_nWinCenter = cs.cx;
	}

	else if (ip == img_sharpen)
	{
		for (int i=0; i<m_imgSetMgr.GetSize(); i++)
		{
			m_imgSetMgr.GetImage(i)->SetSharpenDepth(double(cs.cx)/100);
		}
		theLibSwapData.m_dbSharpen = double(cs.cx)/100;
		if (m_pView != NULL)
			m_pView->Invalidate(FALSE);
	}
	
	theLibSwapData.m_ip = ip;

	theLibSwapData.m_MPROberverHost.FireEvent(P,this);

}

void SeVisualLibDoc::OnImageProcessing( MPRParameter& mp )
{
	if (mp.nType == img_adjustwin)
	{
		CImageDisplayDoc::OnImageProcessing(mp);
	}
	else if (mp.nType == img_zoom)
	{
		CSize sz = mp.szDelta;
		CImageBase* pImg = (CImageBase*)sz.cx;
		for (int i=0; i<m_imgSetMgr.GetSize(); i++)
		{
			double dbAdjustZoom = (pImg->GetDispScale())*(pImg->GetDisplayRatio())/m_imgSetMgr.GetImage(i)->GetDisplayRatio();
			m_imgSetMgr.GetImage(i)->SetDispScale(dbAdjustZoom);
		}
	}
	else if (mp.nType == img_move)
	{
		for (int i=0; i<m_imgSetMgr.GetSize(); i++)
		{
			m_imgSetMgr.GetImage(i)->Move(mp.szDelta.cx, mp.szDelta.cy);
		}
	}
	m_pView->Invalidate(FALSE);
}

void SeVisualLibDoc::AddImage(CImageBase* pDcmpic,DWORD dwSeriesID, BOOL bNeedDel /*=true */)
{
	pDcmpic->SetSharpenDepth(theLibSwapData.m_dbSharpen);

	CImageViewerDoc::AddImage(pDcmpic, dwSeriesID, bNeedDel);
}

/////////////////////////////////////////////////////////////////////////////
// SeVisualLibDoc commands
