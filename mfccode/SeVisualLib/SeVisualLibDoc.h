#pragma once

class SEV_EXT_CLASS SeVisualLibDoc : public CImageDisplayDoc
{
protected:
	SeVisualLibDoc();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(SeVisualLibDoc)

	// Attributes
public:

	// Operations
public:

	void		AddImage(CImageBase* pDcmpic,DWORD dwSeriesID, BOOL bNeedDel =true );
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SeVisualLibDoc)
public:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
protected:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL
	virtual void OnProcessAllFinish(img_process ip,CSize cs);
	virtual void OnImageProcessing(MPRParameter& mp);

	// Implementation
public:
	virtual ~SeVisualLibDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(SeVisualLibDoc)
	// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

//#endif // !defined(AFX_IMAGEVIEWERMPRDOC_H__00A5C459_F1D1_4AEA_9BBF_8FC4B7BC333D__INCLUDED_)
