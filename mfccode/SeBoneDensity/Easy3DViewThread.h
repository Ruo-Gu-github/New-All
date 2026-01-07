#pragma 

// CEasy3DViewThread
struct ThreadInfoFor3D
{
	CWnd*         pParent;
	CDcmPicArray* pDcmArray;
	int           nMin;
	int           nMax;
	CRect         rtDisPlay;

	ThreadInfoFor3D(CWnd* pP, CDcmPicArray* p, int n1, int n2, CRect rt)
	{
		pParent = pP;
		pDcmArray = p;
		nMin = n1;
		nMax = n2;
		rtDisPlay = rt;
	}
};

class CEasy3DViewDlg;

class CEasy3DViewThread : public CWinThread
{
	DECLARE_DYNCREATE(CEasy3DViewThread)

protected:
	CEasy3DViewThread();           // 动态创建所使用的受保护的构造函数
	virtual ~CEasy3DViewThread();

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	const       void SetInfo(ThreadInfoFor3D* p){m_pInfo = p;}
	const       CWnd* GetDlgWnd(){return (CWnd*)m_pDlg;}
private:
	ThreadInfoFor3D* m_pInfo;
	CEasy3DViewDlg*  m_pDlg;

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnGetPrintScreen(WPARAM wParam, LPARAM lParam);
};


