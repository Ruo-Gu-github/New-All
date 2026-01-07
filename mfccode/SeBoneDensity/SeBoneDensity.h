#pragma once


class SeBoneDensityDlg;
class SeBoneDensityCtrlDlg;

enum SHAPE_TYPE {ROUND = 0, SQUARE};

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

struct ImageCropInfo
{
	int Index;
	int nPos;
	ImageCropInfo(int n1, int n2)
	{
		Index = n1;
		nPos = n2;
	}
};

struct ThreadInfoForCrop
{
	queue <ImageCropInfo>		 info;
	vector<CDcmPic*>     imgList;
#ifdef CUDA_AVAILABLE
	SeVisualAPR_with_CUDA*         pAPR;
#else
	SeVisualAPR* pAPR;
#endif // CUDA_AVAILABLE

	MprRotateInfo        rotateInfo;
	int                  nAliveThread;
	int                  nXStart;
	int                  nXEnd;
	int                  nYStart;
	int                  nYEnd;
	int                  nMinValue;
	SHAPE_TYPE           shape;

#ifdef CUDA_AVAILABLE
	ThreadInfoForCrop(queue<ImageCropInfo> queueInfo, vector<CDcmPic*> vecImgs, SeVisualAPR_with_CUDA* p, MprRotateInfo r, int n1, int n2, int n3, int n4, int n5, int n6,SHAPE_TYPE t)
#else
	ThreadInfoForCrop(queue<ImageCropInfo> queueInfo, vector<CDcmPic*> vecImgs, SeVisualAPR* p, MprRotateInfo r, int n1, int n2, int n3, int n4, int n5, int n6,SHAPE_TYPE t)
#endif // CUDA_AVAILABLE
	
	{
		info         = queueInfo;
		imgList      = vecImgs;
		pAPR         = p;
		rotateInfo   = r;
		nAliveThread = n1;
		nXStart      = n2;
		nXEnd        = n3;
		nYStart      = n4;
		nYEnd        = n5;
		nMinValue    = n6;
		shape        = t;
	}
};

struct ImageROIInfo
{
	vector <CPoint> vecPts;
	vector <CPoint> vecPtsInside;
	int             nPos;
	ImageROIInfo(vector <CPoint> vecPts1, vector <CPoint> vecPts2, int n)
	{
		vecPts = vecPts1;
		vecPtsInside = vecPts2;
		nPos = n;
	}
};

struct ThreadInfoForROI
{
	queue <ImageROIInfo> info;
	vector <CDcmPic*>     imgList;
	int                  nAliveThread;
	int                  nImageWidth;
	int                  nImageHeight;
	int                  nMinValue;
	ThreadInfoForROI(queue <ImageROIInfo> queueInfo, vector <CDcmPic*> vecImgs, int n1, int n2, int n3, int n4)
	{
		info = queueInfo;
		imgList = vecImgs;
		nAliveThread = n1;
		nImageWidth = n2;
		nImageHeight = n3;
		nMinValue = n4;
	}
};

struct ThreadInfoForSaveDcm
{
	CString csFullPath;
	CDcmPicArray* pDcmArray;
	ThreadInfoForSaveDcm(CString str, CDcmPicArray* p)
	{
		csFullPath = str;
		pDcmArray = p;
	}
};

class SeBoneDensity : public ISeProcessModuleEx
{
public:
	SeBoneDensity(void);
	~SeBoneDensity(void);
	
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

	void                MotifyData(CDcmPicArray dcmaryIn, CDcmPicArray dcmaryOut, MOTIFY_OPERATION op);



	static UINT         __LoadImage(LPVOID pParam);
	static UINT         __CropImage(LPVOID pParam);
	static UINT         __ROIImage(LPVOID pParam);
	static UINT         __SaveDcmFile(LPVOID pParam);

	             
	SeBoneDensityDlg*		m_pBoneDensityDlg;
	SeBoneDensityCtrlDlg*	m_pBoneDensityCtrlDlg;
	ISeProcessMainFrame*	m_pMainFrame;


	CDcmPicArray			m_OriDcmArray;
	CDcmPicArray			m_SliceArray;
	CDcmPicArray			m_MaskArray;
	CDcmPicArray			m_MaskOutArray;
	CDcmPicArray            m_ROIArray;
	CDcmPicArray            m_BinaryArray;
	CString                 m_strInfoFile;

	const CDcmPicArray*		GetOriDcmArray(){return &m_OriDcmArray;}
	const CDcmPicArray*		GetSliceDcmArray(){return &m_SliceArray;}
	const CDcmPicArray*		GetMaskDcmArray(){return &m_MaskArray;}
	const CDcmPicArray*		GetMaskOutDcmArray(){return &m_MaskOutArray;}
	const CDcmPicArray*		GetROIDcmArray(){return &m_ROIArray;}
	const CDcmPicArray*		GetBinaryDcmArray(){return &m_BinaryArray;}
	const void				ExportDcmArray(CString strFolderName, CDcmPicArray* pDcmArray);
	const void              CreateFolder( CString csPath );

	const void				CutImage(int nXStart, int nXEnd, int nYStart, int nYEnd, int nZStart, int nZEnd, SHAPE_TYPE shape);
	const void				CutImage(vector<CPoint>* pVecEdge, vector<CPoint>* pVecEdgeInside);

	const void				GetBoneSegOuter();
	const void				GetBoneSegInner();

	const static BOOL		PtInEclipse(int nPosX, int nPosY, int nXS, int nXE, int nYS, int nYE);
	
};

extern	SeBoneDensity*	g_pBoneDensityModule;