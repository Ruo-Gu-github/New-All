// Easy3DViewThread.cpp : 实现文件
//

#include "stdafx.h"
#include "SeBoneDensity.h"
#include "Easy3DViewThread.h"
#include "Easy3DViewDlg.h"


// CEasy3DViewThread

IMPLEMENT_DYNCREATE(CEasy3DViewThread, CWinThread)

CEasy3DViewThread::CEasy3DViewThread()
{
	m_pInfo = NULL;
	m_pDlg = NULL;


}

CEasy3DViewThread::~CEasy3DViewThread()
{
	m_pInfo = NULL;
	// 如果非模态对话框已经创建则删除它   
	m_pDlg = NULL;
}

BOOL CEasy3DViewThread::InitInstance()
{
	// TODO: 在此执行任意逐线程初始化
	m_pDlg = new CEasy3DViewDlg();
	m_pDlg->Create(IDD_EASY3DVIEWDLG, NULL);
	m_pDlg->MoveWindow(m_pInfo->rtDisPlay.left, m_pInfo->rtDisPlay.top, m_pInfo->rtDisPlay.Width(), m_pInfo->rtDisPlay.Height());
	CRect rect;
	m_pDlg->GetClientRect(&rect);
	m_pDlg->InitData(m_pInfo->pDcmArray, m_pInfo->nMin, m_pInfo->nMax, rect);
	m_pDlg->ShowWindow(SW_SHOW);
	m_pDlg->RunModalLoop();//调用这个方法,把他变成一个模态对话框形式的
	return FALSE;//这里需要返回false，不然线程就不会关闭的, False 表示 init 之后直接 exit；
}

int CEasy3DViewThread::ExitInstance()
{
	// TODO: 在此执行任意逐线程清理
	if (NULL != m_pDlg)   
	{   
		// 删除非模态对话框对象   
		delete m_pDlg;
		m_pDlg = NULL;
	} 
	if(m_pInfo->pParent != NULL)
		m_pInfo->pParent->SendMessage(WM_SIZE);
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CEasy3DViewThread, CWinThread)
	ON_THREAD_MESSAGE(WM_PRINT_3D, &CEasy3DViewThread::OnGetPrintScreen) 
END_MESSAGE_MAP()

void CEasy3DViewThread::OnGetPrintScreen(WPARAM wParam, LPARAM lParam)
{
	// 自订消息使用 CString
	// 发送消息
	// CString csWPARAM=_T("ZHPC 连接服务器成功 ");
	// CString csLPARAM=_T("ZHPC 收到登录请求，收到公钥 随机数 ");
	// PostMessage(WM_PostMessage, (WPARAM)csWPARAM.AllocSysString(), (LPARAM)csLPARAM.AllocSysString());、

	//  消息响应函数内
	// 	CString csWPARAM=(CString)((BSTR)wParam);
	// 	CString csLPARAM=(CString)((BSTR)lParam);
	// 	SysFreeString((BSTR)wParam);
	// 	SysFreeString((BSTR)lParam);

	CString str = (CString)((BSTR)wParam);
	SysFreeString((BSTR)wParam);
	m_pDlg->GetPrintScreen(str);
}


// CEasy3DViewThread 消息处理程序
