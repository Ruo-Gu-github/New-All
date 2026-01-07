#pragma once

class COpenGLCamera;
class COpenGLDataMgr;
class COpenGLCamera;
class COpenGLShader;


class AFX_EXT_CLASS COpenGLMain
{
public:
	COpenGLMain(void);
	~COpenGLMain(void);

	GLboolean Initialize(HDC hContext, COpenGLDataMgr* pGLDataMgr, COpenGLCamera* pGLCamera, COpenGLShader* pGLShader);
	void Render(GLuint nWinWidth, GLuint nWinHeight);
	void SetRulerRenderInfo(std::string text, GLfloat fScale);
	void SetLineRenderInfo(std::string text, GLfloat fXS, GLfloat fXE, GLfloat fYS, GLfloat fYE, GLboolean bMoving);
	void SetLineShow(GLboolean b){m_bLine = b;}
	void SetRenderMode(BOOL b){m_bRayCasting = b;}
	void Resize(GLuint nWidth, GLuint nHeight);
	void ShowBorder(BOOL b){m_bShowBorder = b;}
	
private:
	void DrawBox(GLenum glFaces);
	void RenderRayCasting(GLuint nWinWidth, GLuint nWinHeight);
	void RenderVolumeRender(GLuint nWinWidth, GLuint nWinHeight);
	void RenderBorder(GLuint nWinWidth, GLuint nWinHeight);
	void RenderObject(GLenum cullFace);
	void RenderRuler(GLuint nWinWidth, GLuint nWinHeight);
	void RenderScale(GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color);
	void RenderText(std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color);
	void SetUniforms(GLuint ProgramHandle, GLuint nWinWidth, GLuint nWinHeight, GLboolean bFront);
	void RenderLine(GLuint nWinWidth, GLuint nWinHeight, GLfloat xs, GLfloat xe, GLfloat ys, GLfloat ye, glm::vec3 color);
	
	GLboolean GetUniformLocation(GLuint ProgramHandle, GLchar* chUniformName, GLint& nLoc);
	GLboolean InErrorList(CString str);

	void SetUniformForVolumeRender(GLuint ProgramHandle, GLuint nWinWidth, GLuint nWinHeight);
	void SetUniformForFreecut(GLuint ProgramHandle, GLuint nWinWidth, GLuint nWinHeight);
	void SetUniformForFreecut(GLuint ProgramHandle, GLuint nWinWidth, GLuint nWinHeight, GLboolean bFront);

private:
	CStringArray m_csaUniformError;
	HGLRC m_hRC;
	COpenGLShader* m_pGLShader;
	COpenGLCamera* m_pGLCamera;
	COpenGLDataMgr* m_pGLDataMgr;
	int m_bRayCasting;
	GLboolean m_bShowBorder;


	// ruler
	std::string m_text;
	GLfloat m_fScale;

	// line
	GLboolean m_bLine;
	std::string m_linetext;
	vector<GLfloat> m_vecPoints;
	vector<std::string> m_vecTexts;
	GLfloat m_fXS;
	GLfloat m_fXE;
	GLfloat m_fYS;
	GLfloat m_fYE;
};

