#include "StdAfx.h"
#include "OpenGLDataMgr.h"

COpenGLDataMgr::COpenGLDataMgr(void)
{
	m_bfTexObj = 0;
	m_frameBuffer = 0;
	m_transFuncObj = 0;
	m_volumeTexture = 0;
	m_VAO = 0;
	m_freeCutVAO = 0;
	m_textVAO = 0;
	m_textVBO = 0;
	m_nImageWidth = 0;
	m_nImageHeight = 0;
	m_nImageLength = 0;

	m_vecVolTexObjList.clear();
	m_vecVolTexObjList2.clear();

	m_volTexObjColorList[0] = glm::vec3(0.0f, 1.0f, 0.0f);
	m_volTexObjColorList[1] = glm::vec3(1.0f, 0.0f, 0.0f);
	m_volTexObjColorList[2] = glm::vec3(0.0f, 0.0f, 1.0f);
	m_volTexObjColorList[3] = glm::vec3(1.0f, 1.0f, 0.0f);
	m_volTexObjColorList[4] = glm::vec3(1.0f, 0.0f, 1.0f);
	m_volTexObjColorList[5] = glm::vec3(0.0f, 1.0f, 1.0f);
	m_volTexObjColorList[6] = glm::vec3(0.5f, 0.0f, 1.0f);
	m_volTexObjColorList[7] = glm::vec3(1.0f, 0.5f, 0.0f);

	m_fvRange[0] = 0.0f;
	m_fvRange[1] = 1.0f;
	m_fvRange[2] = 0.0f;
	m_fvRange[3] = 1.0f;
	m_fvRange[4] = 0.0f;
	m_fvRange[5] = 1.0f;

	m_pTransFuncData = NULL;
	m_fPixelScale = 1.0f;
}


COpenGLDataMgr::~COpenGLDataMgr(void)
{
	DeleteAllVolTexObj();
	if (m_frameBuffer != 0)
		glDeleteFramebuffers(1, &m_frameBuffer);
	if (m_frameBufferFront != 0)
		glDeleteFramebuffers(1, &m_frameBufferFront);
	if (m_frameBufferBack != 0)
		glDeleteFramebuffers(1, &m_frameBufferBack);
	if (m_bfTexObj != 0)
		glDeleteTextures(1, &m_bfTexObj);
// 	if (m_fcbfFrontTexObj != 0)
// 		glDeleteTextures(1, &m_fcbfFrontTexObj);
// 	if (m_fcbfBackTexObj != 0)
// 		glDeleteTextures(1, &m_fcbfBackTexObj);
	if (m_VAO != 0)
		glDeleteVertexArrays(1, &m_VAO);
	if (m_textVAO != 0)
		glDeleteVertexArrays(1, &m_textVAO);
	if (m_freeCutVAO != 0)
		glDeleteVertexArrays(1, &m_freeCutVAO);
	if (m_volumeTexture != 0)
		glDeleteTextures(1, &m_volumeTexture);
	if (m_transFuncObj != 0)
		glDeleteTextures(1, &m_transFuncObj);
// 	map <GLchar, Character> :: iterator iter;
// 	while(iter != Characters.end())
// 	{
// 		glDeleteTextures(1, &(iter->second.TextureID));
// 		iter++;
// 	}

	if (m_pTransFuncData != NULL)
	{
		delete m_pTransFuncData;
		m_pTransFuncData = NULL;
	}
}

void COpenGLDataMgr::InitVAO()
{
	GLfloat vertices[24] = {
		0.0, 0.0, 0.0,
		0.0, 0.0, 1.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 1.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 1.0,
		1.0, 1.0, 0.0,
		1.0, 1.0, 1.0
	};
	// draw the six faces of the boundbox by drawwing triangles
	// draw it contra-clockwise
	// front: 1 5 7 3
	// back: 0 2 6 4
	// left：0 1 3 2
	// right:7 5 4 6    
	// up: 2 3 7 6
	// down: 1 0 4 5


	GLuint indices[36] = {
		1,5,7,
		7,3,1,
		0,2,6,
		6,4,0,
		0,1,3,
		3,2,0,
		7,5,4,
		4,6,7,
		2,3,7,
		7,6,2,
		1,0,4,
		4,5,1
	};
	GLuint gbo[2];

	glGenBuffers(2, gbo);
	GLuint vertexdat = gbo[0];
	GLuint veridxdat = gbo[1];
	glBindBuffer(GL_ARRAY_BUFFER, vertexdat);
	glBufferData(GL_ARRAY_BUFFER, 24*sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	// used in glDrawElement()
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, veridxdat);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36*sizeof(GLuint), indices, GL_STATIC_DRAW);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	// vao like a closure binding 3 buffer object: verlocdat vercoldat and veridxdat
	glBindVertexArray(vao);
	glEnableVertexAttribArray(0); // for vertexloc
	glEnableVertexAttribArray(1); // for vertexcol

	// the vertex location is the same as the vertex color
	glBindBuffer(GL_ARRAY_BUFFER, vertexdat);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLfloat *)NULL);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLfloat *)NULL);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, veridxdat);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	m_VAO = vao;
}

void COpenGLDataMgr::InitFont()
{
	CString strPath = CSeToolKit::GetWorkPath();
	strPath.Replace("\\", "/");

	FT_Library ft;
	if (FT_Init_FreeType(&ft))
	{
		AfxMessageBox("font init fail");
		return;
	}

	FT_Face face;
	if (FT_New_Face(ft, strPath + "/Config/arial.ttf", 0, &face))
	{
		AfxMessageBox("font load fail");
		return;
	}
	// Set size to load glyphs as
	FT_Set_Pixel_Sizes(face, 0, 48);

	// Disable byte-alignment restriction
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 

	// Load first 128 characters of ASCII set
	for (GLubyte c = 0; c < 128; c++)
	{
		// Load character glyph 
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			AfxMessageBox("Glyph load fail");
			continue;
		}
		// Generate texture
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
			);
		// Set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// Now store character for later use
		Character character = {
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			face->glyph->advance.x
		};
		Characters.insert(std::pair<GLchar, Character>(c, character));
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	// Destroy FreeType once we're finished
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	// Configure VAO/VBO for texture quads
	GLuint VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	 
	m_textVAO = VAO;
	m_textVBO = VBO;
}

void COpenGLDataMgr::InitRulerTex()
{
	CString strPath = CSeToolKit::GetWorkPath();
	strPath.Replace("\\", "/");

	// Load and create a texture for ruler
	GLuint texture1;
	// ====================
	// Texture 1
	// ====================
	glGenTextures(1, &texture1);
	glBindTexture(GL_TEXTURE_2D, texture1); // All upcoming GL_TEXTURE_2D operations now have effect on our texture object
	// Set our texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// Set texture wrapping to GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// Set texture filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Load, create texture and generate mipmaps
	int width, height;
	unsigned char* image = SOIL_load_image(strPath + "/Config/ruler.png", &width, &height, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess up our texture.
	m_ftTexObj = texture1;
}

void COpenGLDataMgr::InitFace2DTex(GLuint nWidth, GLuint nHeight)
{
	glDeleteTextures(1, &m_bfTexObj);
	glGenTextures(1, &m_bfTexObj);
	glBindTexture(GL_TEXTURE_2D, m_bfTexObj);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, nWidth, nHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void COpenGLDataMgr::InitFrameBuffer(GLuint nWidth, GLuint nHeight)
{
	// attach the texture and the depth buffer to the framebuffer
	glDeleteFramebuffers(1, &m_frameBuffer);
	glGenFramebuffers(1, &m_frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_bfTexObj, 0);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, nWidth, nHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
	CheckFrameBuffferStatus();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);  
}

void COpenGLDataMgr::CheckFrameBuffferStatus()
{
	GLenum complete = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (complete != GL_FRAMEBUFFER_COMPLETE)
	{
		AfxMessageBox(_T("framebuffer is not complete"));
		exit(EXIT_FAILURE);
	}
}

void COpenGLDataMgr::DeleteVolTexObj(GLuint nTexId)
{
	std::vector <VolTexObjInfo>::iterator it;

	for (it=m_vecVolTexObjList.begin(); it!=m_vecVolTexObjList.end(); )
	{
		if ((*it).id  == nTexId) //相当于指针
		{
			glDeleteTextures(1, &(*it).id);
			it = m_vecVolTexObjList.erase(it);   
			break;
		}
		else
			++it;    //指向下一个位置
	}
}

void COpenGLDataMgr::DeleteVolTexObj2(GLuint nTexId)
{
	std::vector <VolTexObjInfo>::iterator it;

	for (it=m_vecVolTexObjList2.begin(); it!=m_vecVolTexObjList2.end(); )
	{
		if ((*it).id  == nTexId) //相当于指针
		{
			glDeleteTextures(1, &(*it).id);
			it = m_vecVolTexObjList2.erase(it);   
			break;
		}
		else
			++it;    //指向下一个位置
	}
}

void COpenGLDataMgr::DeleteAllVolTexObj()
{
	for (int i=0; i<m_vecVolTexObjList.size(); i++)
	{
		glDeleteTextures(1, &m_vecVolTexObjList[i].id);
	}
	m_vecVolTexObjList.clear();

}

void COpenGLDataMgr::DeleteAllVolTexObj2()
{
	for (int i=0; i<m_vecVolTexObjList2.size(); i++)
	{
		glDeleteTextures(1, &m_vecVolTexObjList2[i].id);
	}
	m_vecVolTexObjList2.clear();
}

void COpenGLDataMgr::ChangeVolTexObjColor(GLuint nTexId, COLORREF color)
{
	GLuint r = (BYTE)(color);
	GLuint g = (BYTE)(color >> 8);
	GLuint b = (BYTE)(color >> 16);
	for (int i=0 ; i<m_vecVolTexObjList.size(); i++)
	{
		if (m_vecVolTexObjList[i].id == nTexId)
		{
			m_vecVolTexObjList[i].color = glm::vec3((GLfloat)r/256.0f, (GLfloat)g/256.0f, (GLfloat)b/256.0f);
			m_vecVolTexObjList[i].RGBColor = color;
		}
	}
}

void COpenGLDataMgr::ChangeVolTexObjColor2(GLuint nTexId, COLORREF color)
{
	GLuint r = (BYTE)(color);
	GLuint g = (BYTE)(color >> 8);
	GLuint b = (BYTE)(color >> 16);
	for (int i=0 ; i<m_vecVolTexObjList2.size(); i++)
	{
		if (m_vecVolTexObjList2[i].id == nTexId)
		{
			m_vecVolTexObjList2[i].color = glm::vec3((GLfloat)r/256.0f, (GLfloat)g/256.0f, (GLfloat)b/256.0f);
			m_vecVolTexObjList2[i].RGBColor = color;
		}
	}
}

BOOL COpenGLDataMgr::AddVol3DTex( COLORREF color, BOOL show, GLfloat translate/* = 1.0f*/)
{


	CStringArray csaImages;
	if (!GetFilesName(csaImages))
		return FALSE;
	CDcmPic* pDcm = new CDcmPic;
	pDcm->SetDataInMem(true);
	pDcm->LoadFromDcmFile(csaImages[0]);

	int nWidth = pDcm->GetWidth();
	int nHeight = pDcm->GetHeight();
	int nLength = csaImages.GetSize();
	float pixelSize = pDcm->GetMMPerpixel();
	float pixelSpacing = pDcm->GetSliceSpace();
	unsigned short* pData = new unsigned short[(LONGLONG)nWidth * (LONGLONG)nHeight * (LONGLONG)nLength];
	memset(pData, 0, sizeof(unsigned short) * (LONGLONG)nWidth * (LONGLONG)nHeight * (LONGLONG)nLength);
	unsigned short* pHead = pData;
	pDcm->ReleaseBuffer();
	Safe_Delete(pDcm);
	theAppIVConfig.m_pILog->ProgressInit(csaImages.GetSize());
	for (int i=0; i<nLength; i++)
	{
		CDcmPic *pTempDcm = new CDcmPic;
		pTempDcm->SetDataInMem(true);
		pTempDcm->LoadFromDcmFile(csaImages[i]);
		short* pTmp = (short*)pTempDcm->GetData();
		for (LONGLONG j=0; j<nWidth*nHeight; j++)
		{
			int x = *pTmp++ + 32768;
			*pHead++ = x;
		}
		pTempDcm->ReleaseBuffer();
		Safe_Delete(pTempDcm);
		theAppIVConfig.m_pILog->ProgressStepIt();
	}
	SetPixelScale(pixelSpacing/ pixelSize);
	// InitVolumeTexture(nWidth, nHeight, nLength, pData, GL_LUMINANCE, GL_SHORT);

	GLuint volTexObj;
	glGenTextures(1, &volTexObj);

	//glGenTextures(1, &m_volumeTexture);
	glBindTexture( GL_TEXTURE_3D, volTexObj);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// 字节对齐 1
	glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
	//glTexImage3D(GL_TEXTURE_3D, 0, GL_INTENSITY16, nWidth, nHeight, nLength, 0, GL_LUMINANCE16, GL_SHORT, pData);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_INTENSITY, nWidth, nHeight, nLength, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, pData);
	glBindTexture( GL_TEXTURE_3D, 0);
	theAppIVConfig.m_pILog->ProgressClose();
	//m_pGLDataMgr->InitVolumeTextureTest(nWidth, nHeight, nLength, p, GL_LUMINANCE, GL_SHORT);
	// delete pData;

	//CDcmPic* pDcm = new CDcmPic;
	//pDcm->SetDataInMem(true);
	//pDcm->LoadFromDcmFile(csaImages[0]);
	//m_nImageWidth = pDcm->GetWidth();
	//m_nImageHeight = pDcm->GetHeight();
	//m_nImageLength = csaImages.GetSize();

	//unsigned short* pData = new unsigned short[m_nImageWidth * m_nImageHeight * m_nImageLength];
	//memset(pData, 0, sizeof(unsigned short) * m_nImageWidth * m_nImageHeight * m_nImageLength);
	//unsigned short* pDataTmp = pData;
	//theAppIVConfig.m_pILog->ProgressInit(csaImages.GetSize());
	//for (int i=0; i<csaImages.GetSize(); i++)
	//{
	//	
	//	pDcm->ReleaseBuffer();
	//	pDcm->LoadFromDcmFile(csaImages[i]);
	//	memcpy(pDataTmp, pDcm->GetData(), sizeof(unsigned short) * m_nImageWidth * m_nImageHeight);
	//	pDataTmp += m_nImageWidth * m_nImageHeight;
	//}
	//pDcm->ReleaseBuffer();
	//Safe_Delete(pDcm);
	//GLuint volTexObj;
	//glGenTextures(1, &volTexObj);
	//glBindTexture( GL_TEXTURE_3D, volTexObj);
	//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//// 字节对齐 2
	//glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
	//glTexImage3D(GL_TEXTURE_3D, 0, GL_LUMINANCE16, m_nImageWidth, m_nImageHeight , m_nImageLength, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, pData);
	//glBindTexture( GL_TEXTURE_3D, 0);

	//GLuint volTexObj;
	//glGenTextures(1, &volTexObj);
	//glBindTexture( GL_TEXTURE_3D, volTexObj);
	//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//// 字节对齐 1
	//glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
	////glTexImage3D(GL_TEXTURE_3D, 0, GL_INTENSITY16, nWidth, nHeight, nLength, 0, GL_LUMINANCE16, GL_SHORT, pData);
	//glTexImage3D(GL_TEXTURE_3D, 0, GL_INTENSITY, nWidth, nHeight, nLength, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, pData);
	//glBindTexture( GL_TEXTURE_3D, 0);

	//theAppIVConfig.m_pILog->ProgressClose();
	//delete[] pData;

	VolTexObjInfo info;
	info.id = volTexObj;
	info.RGBColor = color;
	info.color = TransColor(color);
	info.translate = translate;
	info.show = show;
	m_vecVolTexObjList2.push_back(info);
	Safe_Delete(pData);
	return TRUE;
}

void COpenGLDataMgr::AddVol3DTex(GLuint nWidth, GLuint nHeight, GLuint nLength, GLbyte* pData, GLenum format, GLenum type, COLORREF color, BOOL show, GLfloat translate)
{
	m_nImageWidth = nWidth;
	m_nImageHeight = nHeight;
	m_nImageLength = nLength;
	//static GLuint nColorNum = 0;
	VolTexObjInfo info;
	GLuint volTexObj;
	glGenTextures(1, &volTexObj);
	glBindTexture( GL_TEXTURE_3D, volTexObj);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	// 字节对齐 1
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_INTENSITY, nWidth, nHeight , nLength, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pData);
	glBindTexture( GL_TEXTURE_3D, 0);



	info.id = volTexObj;
	info.RGBColor = color;
	info.color = TransColor(color);
	info.translate = translate;
	info.show = show;
	//nColorNum = (nColorNum + 1) % 8;;
	m_vecVolTexObjList.push_back(info);

}

BOOL COpenGLDataMgr::GetFilesName(CStringArray& csaFilesName)
{
	csaFilesName.RemoveAll();
	CFileDialog			filedlg(TRUE, "dcm", "*.*", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_ALLOWMULTISELECT , 
		"Dicom (*.dcm)|*.dcm|All Files(*.*)|*.*|");
	// 默认的filedialog文件名储存空间为512B，空间不足，扩充为1024*1024B
	filedlg.m_ofn.nMaxFile = 1024*1024;
	filedlg.m_ofn.lpstrFile = new char[1024*1024+1];
	memset(filedlg.m_ofn.lpstrFile, 0, 1024*1024+1);
	if(filedlg.DoModal() == IDCANCEL)
	{
		delete []filedlg.m_ofn.lpstrFile ;
		return FALSE;
	}

	POSITION	pos = filedlg.GetStartPosition();
	while (pos != NULL)
	{
		CString	cs = filedlg.GetNextPathName(pos);
		csaFilesName.Add(cs);
	}
	delete []filedlg.m_ofn.lpstrFile;
	return TRUE;
}


COLORREF COpenGLDataMgr::GetColor(GLuint nTexId)
{
	for (int i=0 ; i<m_vecVolTexObjList.size(); i++)
	{
		if (m_vecVolTexObjList[i].id == nTexId)
			return m_vecVolTexObjList[i].RGBColor;
	}
	return RGB(0,0,0);
}

COLORREF COpenGLDataMgr::GetColor2(GLuint nTexId)
{
	for (int i=0 ; i<m_vecVolTexObjList2.size(); i++)
	{
		if (m_vecVolTexObjList2[i].id == nTexId)
			return m_vecVolTexObjList2[i].RGBColor;
	}
	return RGB(0,0,0);
}

glm::vec3 COpenGLDataMgr::TransColor(COLORREF color)
{
	GLuint r = (BYTE)(color);
	GLuint g = (BYTE)(color >> 8);
	GLuint b = (BYTE)(color >> 16);

	return glm::vec3((GLfloat)r/256.0f, (GLfloat)g/256.0f, (GLfloat)b/256.0f);
}



int COpenGLDataMgr::GetShowVolTex()
{
	int nCount = 0;
	for (int i=0; i<m_vecVolTexObjList.size(); i++)
	{
		if (m_vecVolTexObjList[i].show = TRUE)
			nCount++;
	}
	return nCount;
} 

int COpenGLDataMgr::GetShowVolTex2()
{
	int nCount = 0;
	for (int i=0; i<m_vecVolTexObjList2.size(); i++)
	{
		if (m_vecVolTexObjList2[i].show = TRUE)
			nCount++;
	}
	return nCount;
}

void COpenGLDataMgr::ChangeVolTexObjVisible(GLuint nTexId, BOOL show)
{
	for (int i=0 ; i<m_vecVolTexObjList.size(); i++)
	{
		if (m_vecVolTexObjList[i].id == nTexId)
		{
			m_vecVolTexObjList[i].show = show;
		}
	}
}

void COpenGLDataMgr::ChangeVolTexObjVisible2(GLuint nTexId, BOOL show)
{
	for (int i=0 ; i<m_vecVolTexObjList2.size(); i++)
	{
		if (m_vecVolTexObjList2[i].id == nTexId)
		{
			m_vecVolTexObjList2[i].show = show;
		}
	}
}

void COpenGLDataMgr::AdjustTransFunc(vector <float>& vecStrengthPos, vector <float>& vecStrengthValue, vector <float>& vecColorPos, vector<COLORREF>& vecColorValue, int nMinValue, int nMaxValue, float fHeight, int nPos)
{
	int nSize = nMaxValue - nMinValue + 1;
	// 不知道为什么，new nSize 大小删除会出错
	COLORREF* pfColorList = new COLORREF[nSize + 100];
	GLfloat* pfStrengthList = new GLfloat[nSize + 100];
	memset(pfStrengthList, 0, sizeof(GLfloat) * (nSize + 100));
	memset(pfColorList, 0, sizeof(COLORREF) * (nSize + 100));

	GLfloat fLength = vecColorPos[vecColorPos.size() - 1] - vecColorPos[0];
/*	GLfloat fHeight =  vecStrengthValue[0] - vecStrengthValue[vecStrengthValue.size() - 1];*/
// 	GLfloat fHeight;
	GLfloat fBiggest = 0.0;
	GLfloat fMiniest = 65535.0;
	for (int i=0; i<vecStrengthValue.size(); i++)
	{
		fBiggest = fBiggest >= vecStrengthValue[i] ? fBiggest : vecStrengthValue[i];
		fMiniest = fMiniest <= vecStrengthValue[i] ? fMiniest : vecStrengthValue[i];

	}
// 	fHeight = fBiggest - fMiniest;

	GLfloat fStep = fLength / nSize;
	GLfloat fStart = vecColorPos[0];
 	int nIndex = 0;
	assert(vecColorPos.size() == vecColorValue.size());
	for (int i=1; i<vecColorPos.size(); i++)
	{
		while (fStart < vecColorPos[i])
		{
			fStart += fStep;
			int R = (float)(GetRValue(vecColorValue[i]) - GetRValue(vecColorValue[i - 1])) * (fStart - vecColorPos[i-1]) / (vecColorPos[i] - vecColorPos[i -1]) + (float)GetRValue(vecColorValue[i - 1]); 
			int G = (float)(GetGValue(vecColorValue[i]) - GetGValue(vecColorValue[i - 1])) * (fStart - vecColorPos[i-1]) / (vecColorPos[i] - vecColorPos[i -1]) + (float)GetGValue(vecColorValue[i - 1]);
			int B = (float)(GetBValue(vecColorValue[i]) - GetBValue(vecColorValue[i - 1])) * (fStart - vecColorPos[i-1]) / (vecColorPos[i] - vecColorPos[i -1]) + (float)GetBValue(vecColorValue[i - 1]);
			pfColorList[nIndex++] = RGB(R, G, B); 
		}
	}

	fStart = vecStrengthPos[0];
	nIndex = 0;
	assert(vecStrengthPos.size() == vecStrengthValue.size());
	for (int i=1; i<vecStrengthPos.size(); i++)
	{
		while (fStart < vecStrengthPos[i])
		{
			fStart += fStep;
			pfStrengthList[nIndex++] = (fBiggest - (vecStrengthValue[i] - vecStrengthValue[i - 1]) * (fStart - vecStrengthPos[i-1]) / (vecStrengthPos[i] - vecStrengthPos[i -1]) - vecStrengthValue[i - 1]) / fHeight;	
		}
	}
	
	int nStart = 0;
	if (nPos != -1)
	{
		nStart = 65535 * 4 * (nPos + 1);
	}
	memset(m_pTransFuncData + nStart, 0, sizeof(GLubyte) * 65536 * 4);
	for (int i=0; i<nSize; i++)
	{
		m_pTransFuncData[nStart + (i + nMinValue + 32768) * 4] = GetRValue(pfColorList[i])/* * pfStrengthList[i]*/;
		m_pTransFuncData[nStart + (i + nMinValue + 32768) * 4 + 1] = GetGValue(pfColorList[i])/* * pfStrengthList[i]*/;
		m_pTransFuncData[nStart + (i + nMinValue + 32768) * 4 + 2] = GetBValue(pfColorList[i])/* * pfStrengthList[i]*/;
		m_pTransFuncData[nStart + (i + nMinValue + 32768) * 4 + 3] = 255 * pfStrengthList[i];
	}
// 	for (int i= 310+32768; i<2240+32768; i++)
// 	{
// 		m_pTransFuncData[i * 4] = 255;
// 		m_pTransFuncData[i * 4 + 1] = 255;
// 		m_pTransFuncData[i * 4 + 2] = 255;
// 		m_pTransFuncData[i * 4 + 3] = 255;
// 	}
// 	GLubyte* pH = m_pTransFuncData;
// 	for (int i=0; i<65536; i++)
// 	{
// 		if (i<32768)
// 		{
// 			*pH++ = 0;
// 			*pH++ = 0;
// 			*pH++ = 0;
// 		}
// 		else
// 		{
// 			*pH++ = 255;
// 			*pH++ = 255;
// 			*pH++ = 255;
// 		}
// 		*pH++ = 255;
// 	}
	delete [] pfColorList;
	delete [] pfStrengthList;
	// 更改传递函数

 	glBindTexture(GL_TEXTURE_2D, m_transFuncObj);
 	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 2304, GL_RGBA, GL_UNSIGNED_BYTE, m_pTransFuncData);
 	glBindTexture(GL_TEXTURE_2D, 0);
}

void COpenGLDataMgr::InitTransFunc()
{
	m_pTransFuncData = new GLubyte[65536 * 36];
	memset(m_pTransFuncData, 0, sizeof(GLubyte) * 65536 * 36);
// 	GLubyte* pH = m_pTransFuncData;
// 	for (int i=0; i<65536; i++)
// 	{
// 		*pH++ = (GLubyte)(i / 256);
// 		*pH++ = (GLubyte)(i / 256);
// 		*pH++ = (GLubyte)(i / 256);
// 		*pH++ = (GLubyte)(i / 256);
// 	}

	glDeleteTextures(1, &m_transFuncObj);
	glGenTextures(1, &m_transFuncObj);
	glBindTexture(GL_TEXTURE_2D, m_transFuncObj);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 2304, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pTransFuncData);
	glBindTexture(GL_TEXTURE_1D, 0);


// 	// read in the user defined data of transfer function
// 	const char* filename = "C:\\Users\\Ruogu\\Desktop\\ProcessPro_x64_Ex\\Se3DModule\\tff.dat";
// 	ifstream inFile(filename, ifstream::in);
// 	if (!inFile)
// 	{
// 		cerr << "Error openning file: " << filename << endl;
// 		exit(EXIT_FAILURE);
// 	}
// 
// 	const int MAX_CNT = 1000000;
// 	GLubyte *tff = (GLubyte *) calloc(MAX_CNT, sizeof(GLubyte));
// 	inFile.read(reinterpret_cast<char *>(tff), MAX_CNT);
// 	if (inFile.eof())
// 	{
// 		size_t bytecnt = inFile.gcount();
// 		*(tff + bytecnt) = '\0';
// 		cout << "bytecnt " << bytecnt << endl;
// 	}
// 	else if(inFile.fail())
// 	{
// 		cout << filename << "read failed " << endl;
// 	}
// 	else
// 	{
// 		cout << filename << "is too large" << endl;
// 	}   
// 	GLubyte* p = tff;
// 	for(int i=0; i<65536; i++)
// 	{
// 		int n = i / 256;
// 		*p++ = n;
// 		*p++ = n;
// 		*p++ = n;
// 		*p++ = n;
// 	}
// 
// 	GLuint tff1DTex;
// 	glGenTextures(1, &tff1DTex);
// 	glBindTexture(GL_TEXTURE_2D, tff1DTex);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
// 	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
// 	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, tff);
// 	free(tff);    
// 	m_transFuncObj = tff1DTex;
}

void COpenGLDataMgr::InitVolumeTexture(GLuint nWidth, GLuint nHeight, GLuint nLength, GLushort* pData, GLenum format, GLenum type)
{
	m_nImageWidth = nWidth;
	m_nImageHeight = nHeight;
	m_nImageLength = nLength;
	//static GLuint nColorNum = 0;
	glGenTextures(1, &m_volumeTexture);
	glBindTexture( GL_TEXTURE_3D, m_volumeTexture);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// 字节对齐 1
	glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
	//glTexImage3D(GL_TEXTURE_3D, 0, GL_INTENSITY16, nWidth, nHeight, nLength, 0, GL_LUMINANCE16, GL_SHORT, pData);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_INTENSITY, nWidth, nHeight, nLength, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, pData);
	glBindTexture( GL_TEXTURE_3D, 0);
}

void COpenGLDataMgr::DeleteVolumeTexture()
{
	if (m_volumeTexture != 0)
		glDeleteTextures(1, &m_volumeTexture);
}

void COpenGLDataMgr::InitVolumeTextureTest(GLuint nWidth, GLuint nHeight, GLuint nLength, GLubyte* pData, GLenum format, GLenum type)
{
	m_nImageWidth = nWidth;
	m_nImageHeight = nHeight;
	m_nImageLength = nLength;
	//static GLuint nColorNum = 0;
	glGenTextures(1, &m_volumeTexture);
	glBindTexture( GL_TEXTURE_3D, m_volumeTexture);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// 字节对齐 1
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//glTexImage3D(GL_TEXTURE_3D, 0, GL_INTENSITY16, nWidth, nHeight, nLength, 0, GL_LUMINANCE16, GL_SHORT, pData);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_INTENSITY, nWidth, nHeight , nLength, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pData);
	glBindTexture( GL_TEXTURE_3D, 0);
}

void COpenGLDataMgr::InitFreeCutVAO()
{
// 	GLfloat vertices[24] = {
// 		0.0, 0.0, 0.0,
// 		1.0, 0.0, 0.0,
// 		0.0, 1.0, 0.0,
// 		1.0, 1.0, 0.0,
// 		0.0, 0.0, 1.0,
// 		1.0, 0.0, 1.0,
// 		0.0, 1.0, 1.0,
// 		1.0, 1.0, 1.0
// 	};
	GLfloat vertices[24] = {
		-0.5, -0.5, 1,
		0.5, -0.5, 1,
		-0.5, 0.5, 1,
		0.5, 0.5, 1,
		-0.5, -0.5, 1,
		0.5, -0.5, 1,
		-0.5, 0.5, 1,
		0.5, 0.5, 1
	};

	static const GLuint indices[24] = 
	{ 
		0, 1,    
		2, 3,    
		4, 5,    
		6, 7,    
		0, 2,    
		1, 3,    
		4, 6,    
		5, 7,
		0, 4,
		1, 5,
		7, 3,
		2, 6
	}; 

	GLuint gbo[2];

	glGenBuffers(2, gbo);
	GLuint vertexdat = gbo[0];
	GLuint veridxdat = gbo[1];
	glBindBuffer(GL_ARRAY_BUFFER, vertexdat);
	glBufferData(GL_ARRAY_BUFFER, 24*sizeof(GLfloat), vertices, GL_DYNAMIC_DRAW);
	// used in glDrawElement()
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, veridxdat);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 24*sizeof(GLuint), indices, GL_STATIC_DRAW);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	// vao like a closure binding 3 buffer object: verlocdat vercoldat and veridxdat
	glBindVertexArray(vao);
	glEnableVertexAttribArray(0); // for vertexloc
	glEnableVertexAttribArray(1); // for vertexcol

	// the vertex location is the same as the vertex color
	glBindBuffer(GL_ARRAY_BUFFER, vertexdat);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLfloat *)NULL);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLfloat *)NULL);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, veridxdat);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	m_freeCutVBO = vertexdat;
	m_freeCutVAO = vao;
}

void COpenGLDataMgr::AdjustFreeCutVAO(GLfloat fXs, GLfloat fXe, GLfloat fYs, GLfloat fYe, GLfloat fZs, GLfloat fZe)
{
	GLfloat vertices[24] = {
		fXs, fYs, fZs,
		fXe, fYs, fZs,    
		fXs, fYe, fZs,
		fXe, fYe, fZs,
		fXs, fYs, fZe,
		fXe, fYs, fZe,
		fXs, fYe, fZe,
		fXe, fYe, fZe
	};
	glBindBuffer(GL_ARRAY_BUFFER, m_freeCutVBO);
	glBufferData(GL_ARRAY_BUFFER, 24*sizeof(GLfloat), vertices, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	m_fvRange[0] = fXs;
	m_fvRange[1] = fXe;
	m_fvRange[2] = fYs;
	m_fvRange[3] = fYe;
	m_fvRange[4] = fZs;
	m_fvRange[5] = fZe;
}

void COpenGLDataMgr::ChangeVolTexObjTranslate( GLuint nTexID, GLfloat translate )
{
	for (int i=0 ; i<m_vecVolTexObjList.size(); i++)
	{
		if (m_vecVolTexObjList[i].id == nTexID)
		{
			m_vecVolTexObjList[i].translate = translate;
		}
	}
}

void COpenGLDataMgr::ChangeVolTexObjTranslate2(GLuint nTexID, GLfloat translate)
{
	for (int i=0 ; i<m_vecVolTexObjList2.size(); i++)
	{
		if (m_vecVolTexObjList2[i].id == nTexID)
		{
			m_vecVolTexObjList2[i].translate = translate;
		}
	}
}

int COpenGLDataMgr::GetTranslate( GLuint nTexId )
{
	for (int i=0 ; i<m_vecVolTexObjList.size(); i++)
	{
		if (m_vecVolTexObjList[i].id == nTexId)
		{
			return static_cast<int>(m_vecVolTexObjList[i].translate * 100);
		}
	}
	return 100;
}

int COpenGLDataMgr::GetTranslate2(GLuint nTexId)
{
	for (int i=0 ; i<m_vecVolTexObjList2.size(); i++)
	{
		if (m_vecVolTexObjList2[i].id == nTexId)
		{
			return static_cast<int>(m_vecVolTexObjList2[i].translate * 100);
		}
	}
	return 100;
}

// void COpenGLDataMgr::InitFrameBufferForFreeCut(GLuint nWidth, GLuint nHeight)
// {
// 	// attach the texture and the depth buffer to the framebuffer
// 	// front
// 	glDeleteFramebuffers(1, &m_frameBufferFront);
// 	glGenFramebuffers(1, &m_frameBufferFront);
// 	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBufferFront);
// 	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fcbfFrontTexObj, 0);
// 	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, nWidth, nHeight);
// 	glBindRenderbuffer(GL_RENDERBUFFER, 0);
// 	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
// 	CheckFrameBuffferStatus();
// 	glBindFramebuffer(GL_FRAMEBUFFER, 0);
// 
// 	// back
// 	glDeleteFramebuffers(1, &m_frameBufferBack);
// 	glGenFramebuffers(1, &m_frameBufferBack);
// 	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBufferBack);
// 	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fcbfBackTexObj, 0);
// 	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, nWidth, nHeight);
// 	glBindRenderbuffer(GL_RENDERBUFFER, 0);
// 	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
// 	CheckFrameBuffferStatus();
// 	glBindFramebuffer(GL_FRAMEBUFFER, 0);
// }

// void COpenGLDataMgr::InitFace2DTexForFreeCut(GLuint nWidth, GLuint nHeight)
// {
// 	// front
// 	glDeleteTextures(1, &m_fcbfFrontTexObj);
// 	glGenTextures(1, &m_fcbfFrontTexObj);
// 	glBindTexture(GL_TEXTURE_2D, m_fcbfFrontTexObj);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
// 	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, nWidth, nHeight, 0, GL_RGBA, GL_FLOAT, NULL);
// 	glBindTexture(GL_TEXTURE_2D, 0);
// 
// 	//back
// 	glDeleteTextures(1, &m_fcbfBackTexObj);
// 	glGenTextures(1, &m_fcbfBackTexObj);
// 	glBindTexture(GL_TEXTURE_2D, m_fcbfBackTexObj);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
// 	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, nWidth, nHeight, 0, GL_RGBA, GL_FLOAT, NULL);
// 	glBindTexture(GL_TEXTURE_2D, 0);
// }


