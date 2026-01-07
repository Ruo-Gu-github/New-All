#pragma once

#define  COLOR_LIST_SIZE 8

struct VolTexObjInfo
{
	GLuint id;
	COLORREF RGBColor;
	glm::vec3 color;
	float     translate;
	BOOL      show;
};

struct Character {
	GLuint TextureID;
	glm::ivec2 Size;
	glm::ivec2 Bearing;
	GLuint Advance;
};



class AFX_EXT_CLASS COpenGLDataMgr
{
public:
	COpenGLDataMgr(void);
	~COpenGLDataMgr(void);

	// 外部调用
	void InitVAO();
	void InitFont();
	void InitRulerTex();
	void InitFace2DTex(GLuint nWidth, GLuint nHeight);
/*	void InitFace2DTexForFreeCut(GLuint nWidth, GLuint nHeight);*/
	BOOL AddVol3DTex( COLORREF color, BOOL show, GLfloat translate = 1.0f);
	void AddVol3DTex(GLuint nWidth, GLuint nHeight, GLuint nLength, GLbyte* pData, GLenum format, GLenum type, COLORREF color, BOOL show, GLfloat translate = 1.0f);
	void InitFrameBuffer(GLuint nWidth, GLuint nHeight);
/*	void InitFrameBufferForFreeCut(GLuint nWidth, GLuint nHeight);*/
	void DeleteVolTexObj(GLuint nTexId);
	void DeleteVolTexObj2(GLuint nTexId);
	void DeleteAllVolTexObj();
	void DeleteAllVolTexObj2();
	void ChangeVolTexObjColor(GLuint nTexId, COLORREF color);
	void ChangeVolTexObjColor2(GLuint nTexId, COLORREF color);
	void ChangeVolTexObjVisible(GLuint nTexId, BOOL show);
	void ChangeVolTexObjVisible2(GLuint nTexId, BOOL show);
	void ChangeVolTexObjTranslate(GLuint nTexID, GLfloat translate);
	void ChangeVolTexObjTranslate2(GLuint nTexID, GLfloat translate);
	COLORREF GetColor(GLuint nTexId);
	COLORREF GetColor2(GLuint nTexId);
	int GetTranslate(GLuint nTexId);
	int GetTranslate2(GLuint nTexId);
	int  GetShowVolTex();
	int  GetShowVolTex2();


	// 外部调用 直接体绘制相关
	void InitTransFunc();
	void AdjustTransFunc(vector <float>& vecStrengthPos, vector <float>& vecStrengthValue, vector <float>& vecColorPos,	vector<COLORREF>& vecColorValue, int nMinValue, int nMaxValue, float fHeight, int nPos);
	void InitVolumeTexture(GLuint nWidth, GLuint nHeight, GLuint nLength, GLushort* pData, GLenum format, GLenum type);
	void InitVolumeTextureTest(GLuint nWidth, GLuint nHeight, GLuint nLength, GLubyte* pData, GLenum format, GLenum type);
	void DeleteVolumeTexture();

	// 外部调用 自由切割相关
	void InitFreeCutVAO();
	void AdjustFreeCutVAO(GLfloat fXs, GLfloat fXe, GLfloat fYs, GLfloat fYe, GLfloat fZs, GLfloat fZe);
	

	// glMain 调用
	const GLuint GetBfTex(){return m_bfTexObj;}
// 	const GLuint GetBfFreeCutFrontTex(){return m_fcbfFrontTexObj;}
// 	const GLuint GetBfFreeCutBackTex(){return m_fcbfBackTexObj;}
	const GLuint GetWidth(){return m_nImageWidth;}
	const GLuint GetHeight(){return m_nImageHeight;}
	const GLuint GetLength(){return m_nImageLength;}
	const GLuint GetFrameBuffer(){return m_frameBuffer;}
	const GLuint GetFreeCutFrontFrameBuffer(){return m_frameBufferFront;}
	const GLuint GetFreeCutBackFrameBuffer(){return m_frameBufferBack;} 
	const GLuint GetVAO(){return m_VAO;}
	const GLuint GetFtVAO(){return m_textVAO;}
	const GLuint GetFtVBO(){return m_textVBO;}
	const map<GLchar, Character> GetTextMap(){return Characters;}
	const GLuint GetFtTex(){return m_ftTexObj;}
	const GLuint GetFreeCutVAO(){return m_freeCutVAO;}
	const std::vector <VolTexObjInfo> GetVolTexList(){return m_vecVolTexObjList;}
	const std::vector <VolTexObjInfo> GetVolTexList2(){return m_vecVolTexObjList2;}

	// glMain 调用 直接体绘制相关
	const GLuint GetTransFunc(){return m_transFuncObj;}
	// const std::vector GetTransFunc() {return NULL;}
	const GLuint GetVolumeTex(){return m_volumeTexture;}
	

	// glMain 调用 自由切割相关
	const GLfloat* GetRange(){return m_fvRange;}


	// for pixel spacing different from pixel size
	const GLfloat GetPixelScale(){return m_fPixelScale;}
	void SetPixelScale(GLfloat scale) {m_fPixelScale = scale;} 

private:
	BOOL GetFilesName(CStringArray& csaFilesName);
	void CheckFrameBuffferStatus(); 
	glm::vec3 TransColor(COLORREF color);


private:
	GLuint m_nImageWidth;
	GLuint m_nImageHeight;
	GLuint m_nImageLength;

	GLuint m_bfTexObj;
// 	GLuint m_fcbfBackTexObj;
// 	GLuint m_fcbfFrontTexObj;
//	GLuint m_ftTexObj;
	GLuint m_frameBuffer;
	GLuint m_frameBufferFront;
	GLuint m_frameBufferBack;
	GLuint m_transFuncObj;
	GLuint m_volumeTexture;
	GLuint m_VAO;
	GLuint m_freeCutVAO;
	GLuint m_freeCutVBO;
	GLuint m_textVAO;
	GLuint m_textVBO;
	GLuint m_ftTexObj;
	glm::vec3 m_volTexObjColorList[COLOR_LIST_SIZE];
	std::vector <VolTexObjInfo> m_vecVolTexObjList;
	std::vector <VolTexObjInfo> m_vecVolTexObjList2;

	GLubyte* m_pTransFuncData;

	// free cut range
	GLfloat m_fvRange[6];

	// for render text
	map <GLchar, Character> Characters;

	// for pixel spacing different from pixel size
	GLfloat m_fPixelScale;
};

