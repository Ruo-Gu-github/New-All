#include "StdAfx.h"
#include "OpenGLShader.h"


COpenGLShader::COpenGLShader(void)
{
	m_frontProgramHandle = 0;
	m_backProgramHandle = 0;

	m_bfVertHandle = 0;
	m_bfFragHandle = 0;
	m_rcVertHandle = 0;
	m_rcFragHandle = 0;
}


COpenGLShader::~COpenGLShader(void)
{
}

GLboolean COpenGLShader::InitShader()
{
	CString strPath = CSeToolKit::GetWorkPath();
	strPath.Replace("\\", "/");
	// vertex shader object for first pass
	m_bfVertHandle = InitShaderObj(strPath + "/GLSL/backface.vert", GL_VERTEX_SHADER);
	// fragment shader object for first pass
	m_bfFragHandle = InitShaderObj(strPath + "/GLSL/backface.frag", GL_FRAGMENT_SHADER);
	// vertex shader object for second pass
	m_rcVertHandle = InitShaderObj(strPath + "/GLSL/raycasting.vert", GL_VERTEX_SHADER);
	// fragment shader object for second pass
	m_rcFragHandle = InitShaderObj(strPath + "/GLSL/raycasting.frag", GL_FRAGMENT_SHADER);
	// vertex shader object for volume render
	m_vrVertHandle = InitShaderObj(strPath + "/GLSL/volumerender.vert", GL_VERTEX_SHADER);
	// fragment shader object for volume render
	m_vrFragHandle = InitShaderObj(strPath + "/GLSL/volumerender.frag", GL_FRAGMENT_SHADER);
	// vertex shader object for free cut 
	m_fcVertHandle = InitShaderObj(strPath + "/GLSL/freecut.vert", GL_VERTEX_SHADER);
	// fragmetn shader object for free cut
	m_fcFragHandle = InitShaderObj(strPath + "/GLSL/freecut.frag", GL_FRAGMENT_SHADER);

	m_txVertHandle = InitShaderObj(strPath + "/GLSL/text.vert", GL_VERTEX_SHADER);

	m_txFragHandle = InitShaderObj(strPath + "/GLSL/text.frag", GL_FRAGMENT_SHADER);

	// create the shader program , use it in an appropriate time
	m_backProgramHandle = CreateShaderProgram(m_bfVertHandle, m_bfFragHandle);
	// 获得由着色器编译器分配的索引(可选)
	m_frontProgramHandle = CreateShaderProgram(m_rcVertHandle, m_rcFragHandle);

	m_VolumeRenderProgramHandle = CreateShaderProgram(m_vrVertHandle, m_vrFragHandle);

	m_freecutProgramHandle = CreateShaderProgram(m_fcVertHandle, m_fcFragHandle);

	m_textProgramHandle = CreateShaderProgram(m_txVertHandle, m_txFragHandle);

	LinkShader(m_backProgramHandle, m_bfVertHandle, m_bfFragHandle);
	LinkShader(m_frontProgramHandle, m_rcVertHandle, m_rcFragHandle);
	LinkShader(m_VolumeRenderProgramHandle, m_vrVertHandle, m_vrFragHandle);
	LinkShader(m_freecutProgramHandle, m_fcVertHandle, m_fcFragHandle);
	LinkShader(m_textProgramHandle, m_txVertHandle, m_txFragHandle);

	return true;
}

GLboolean COpenGLShader::RefleshShader()
{
	CString strPath = CSeToolKit::GetWorkPath();
	strPath.Replace("\\", "/");

	glDetachShader(m_VolumeRenderProgramHandle, m_vrVertHandle);
	glDetachShader(m_VolumeRenderProgramHandle, m_vrFragHandle);
	glDeleteProgram(m_VolumeRenderProgramHandle);

	m_vrVertHandle = InitShaderObj(strPath + "/GLSL/volumerender.vert", GL_VERTEX_SHADER);
	// fragment shader object for volume render
	m_vrFragHandle = InitShaderObj(strPath + "/GLSL/volumerender.frag", GL_FRAGMENT_SHADER);

	m_VolumeRenderProgramHandle = CreateShaderProgram(m_vrVertHandle, m_vrFragHandle);

	LinkShader(m_VolumeRenderProgramHandle, m_vrVertHandle, m_vrFragHandle);

	return true;
}

GLboolean COpenGLShader::LinkShader(GLuint ProgramHandle, GLuint vertHandle, GLuint fragHandle)
{
	const GLsizei maxCount = 2;
	GLsizei count;
	GLuint shaders[maxCount];
	glGetAttachedShaders(ProgramHandle, maxCount, &count, shaders);
	for (int i = 0; i < count; i++) {
		glDetachShader(ProgramHandle, shaders[i]);
	}
	// Bind index 0 to the shader input variable "VerPos"
	glBindAttribLocation(ProgramHandle, 0, "VerPos");
	// Bind index 1 to the shader input variable "VerClr"
	glBindAttribLocation(ProgramHandle, 1, "VerClr");
	glAttachShader(ProgramHandle, vertHandle);
	glAttachShader(ProgramHandle, fragHandle);
	glLinkProgram(ProgramHandle);
	if (GL_FALSE == CheckShaderLinkStatus(ProgramHandle))
	{
		AfxMessageBox(_T("Failed to link shader program!"));
		exit(EXIT_FAILURE);
	}
	return true;
}

GLuint COpenGLShader::CreateShaderProgram(GLuint vertHandle, GLuint fragHandle)
{
	// Create the shader program
	GLuint programHandle = glCreateProgram();
	if (0 == programHandle)
	{
		AfxMessageBox(_T("Error create shader program"));
		exit(EXIT_FAILURE);
	}
	glAttachShader(programHandle, vertHandle);
	glAttachShader(programHandle, fragHandle);
	glLinkProgram(programHandle);
	return programHandle;
}

GLuint COpenGLShader::InitShaderObj(CString srcfile, GLenum shaderType)
{
	CFile file;
	file.Open(srcfile, CFile::modeRead);
	LONG len = file.GetLength();
	GLchar* shaderCode = new GLchar[len + 1];
	shaderCode[len] = 0;
	file.Read(shaderCode, len);
	file.Close();
	// create the shader Object
	GLuint shader = glCreateShader(shaderType);
	const GLchar* codeArray[] = {shaderCode};
	glShaderSource(shader, 1, codeArray, NULL);
	delete []shaderCode;
	// compile the shader
	glCompileShader(shader);
	if (GL_FALSE == CompileCheck(shader))
	{
		AfxMessageBox("Compile Failed.");
	}
	return shader;
}

GLboolean COpenGLShader::CheckShaderLinkStatus(GLuint ProgramHandle)
{
	GLint status;
	glGetProgramiv(ProgramHandle, GL_LINK_STATUS, &status);
	if (GL_FALSE == status)
	{
		GLint logLen;
		glGetProgramiv(ProgramHandle, GL_INFO_LOG_LENGTH, &logLen);
		if (logLen > 0)
		{
			GLchar * log = (GLchar *)malloc(logLen);
			GLsizei written;
			glGetProgramInfoLog(ProgramHandle, logLen, &written, log);
			int a = 10;
		}
	}
	return status;
}

GLboolean COpenGLShader::CompileCheck(GLuint shader)
{
	GLint err;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &err);

	if (GL_FALSE == err)
	{
		GLint logLen;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
		if (logLen > 0)
		{
			char* log = (char *)malloc(logLen);
			GLsizei written;
			glGetShaderInfoLog(shader, logLen, &written, log);
			free(log);		
		}
	}
	return err;
}
