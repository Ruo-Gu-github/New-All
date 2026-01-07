#pragma once


class SeExampleModuleDlg;
class SeExampleModuleCtrlDlg;

struct ImageLoadInfo
{
	CString csImageName;
	int     Index;
	ImageLoadInfo(CString cs, int n)
	{
		csImageName = cs;
		Index = n;
	}
};

struct ThreadInfo
{
	queue<ImageLoadInfo> info;
	vector<CDcmPic*>	 imgList;
	int                 nAliveThread;
	ThreadInfo(queue<ImageLoadInfo> queueInfo,vector<CDcmPic*> vecImgs, int n)
	{
		info = queueInfo;
		imgList = vecImgs;
		nAliveThread = n;
	}
};

class SeExampleModule : public ISeProcessModuleEx
{
public:
	SeExampleModule(void);
	~SeExampleModule(void);

	//初始化模块
	virtual BOOL		Initialize() ;
	//模块退出
	virtual void		ExitInstance() ;
	//模块删除
	virtual void		Release() ;
	//模块复位，内存清除
	virtual void		Reset() ;
	//获取模块名称
	virtual CString		GetCaption() ;
	//创建模块主窗口，并返回主窗口指针
	virtual CWnd*		CreateUI(CWnd* pParent) ;
	//获取模块窗口指针
	virtual CWnd*		GetUI()	;
	//获取模块图标
	virtual HICON		GetIcon() ;
	//判断模块是否有处理某个子模块的功能
	virtual BOOL		CanProcess(ProcessModule emModule) ;
	//初始化子模块
	virtual BOOL		InitProcess(ProcessModule emModule) ;
	//传递全局数据
	virtual void		AssignProcessData(const vector<CDcmPic*>& vecDcmArr) ;
	//传递全局数据
	virtual void		AssignProcessData(CStringArray& csaDcmFiles) ;

	virtual void		AssignProcessData(CDcmPicArray* pDcmPicArray);
	//传递框架指针
	virtual void		AssignInterface(ISeProcessMainFrame* pMainFrame) ;

	virtual CWnd*		CreateCtrlUI(CWnd* pParent) ;

	virtual CWnd*		GetCtrlUI(CWnd* pParent);




	static UINT         __LoadImage(LPVOID pParam);



	SeExampleModuleDlg*		m_pExampleModuleDlg;
	SeExampleModuleCtrlDlg*	m_pExampleModuleCtrlDlg;
	ISeProcessMainFrame*	m_pMainFrame;


	CDcmPicArray            m_ExampleArray;
	CDcmPicArray            m_SliceArray;

};

extern	SeExampleModule*	g_pExampleModule;