#pragma once

class AFX_EXT_CLASS COpenGLShader
{
public:
	COpenGLShader(void);
	~COpenGLShader(void);

	// 外部调用
	GLboolean InitShader();

	GLboolean RefleshShader();

	// gl_Main 调用
	const GLuint GetFrontProgramHandle(){return m_frontProgramHandle;}
	const GLuint GetBackProgramHandle(){return m_backProgramHandle;}
	const GLuint GetVolumeRenderProgramHandle(){return m_VolumeRenderProgramHandle;}
	const GLuint GetFreeCutProgramHandle(){return m_freecutProgramHandle;}
	const GLuint GetTextProgramHandle(){return m_textProgramHandle;}

private:
	GLboolean LinkShader(GLuint ProgramHandle, GLuint vertHandle, GLuint fragHandle);
	GLuint CreateShaderProgram(GLuint vertHandle, GLuint fragHandle);
	GLuint InitShaderObj(CString srcfile, GLenum shaderType);
	GLboolean CheckShaderLinkStatus(GLuint ProgramHandle);
	GLboolean CompileCheck(GLuint shader);

private:
	GLuint m_frontProgramHandle;
	GLuint m_backProgramHandle;
	GLuint m_VolumeRenderProgramHandle;
	GLuint m_freecutProgramHandle;
	GLuint m_textProgramHandle;

	GLuint m_bfVertHandle;
	GLuint m_bfFragHandle;
	GLuint m_rcVertHandle;
	GLuint m_rcFragHandle;
	GLuint m_vrVertHandle;
	GLuint m_vrFragHandle;
	GLuint m_fcVertHandle;
	GLuint m_fcFragHandle;
	GLuint m_txVertHandle;
	GLuint m_txFragHandle;
};

