#include "StdAfx.h"
#include "OpenGLCamera.h"


COpenGLCamera::COpenGLCamera(void)
{
	m_matRotation = glm::mat4(1.0f);
	m_matRotationForFreeCut = glm::mat4(1.0f);
	m_vec4Move = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	m_vec4MoveForFreeCut = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	m_vec3RotateCenter = glm::vec3(-0.5f);
	m_fScale = 1.0f;
	m_fViewAngle = 45.0f;
	m_fMoveSpeed = 4.0f;
	m_fRotateSpeed = 4.0f;
	m_fScaleSpeed = 4.0f;
	m_fPixelSize = 0.0f;
	m_vec3CameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	m_vec3LightPos = glm::vec3(2.0f, 2.0f, 3.0f);
	m_vec3BackgroundColor = glm::vec3(0.0f, 0.0f, 0.0f);

	m_vec3LightColor = glm::vec3(1.0f);
	m_vec3MaterialColor = glm::vec3(0.5f);
	m_vec4LightMat = glm::vec4(0.5f, 0.6f, 0.3f, 32.0f);
	m_bLight = GL_FALSE;
	m_bShadow = GL_FALSE;

	m_bMoveLeft = false;
	m_bMoveRight = false;
	m_bMoveUp = false;
	m_bMoveDown = false;
	m_bRotateClockWise = false;
	m_bRotateAntiClockWise = false;
}


COpenGLCamera::~COpenGLCamera(void)
{
}

void COpenGLCamera::Scale(GLint nZoomDirection)
{
// 	if (nZoomDirection > 0 && m_fViewAngle <= 90.0f)
// 		m_fViewAngle += m_fScaleSpeed/5.0f;
// 	else if (nZoomDirection < 0 && m_fViewAngle >= 0.0f)
// 		m_fViewAngle -= m_fScaleSpeed/5.0f;

	if (m_fScale > 10.0f && nZoomDirection > 0)
		return;
	if (m_fScale < 0.1f && nZoomDirection < 0)
		return;
	if (nZoomDirection > 0)
		m_fScale *= (1.0f + (0.01f * m_fScaleSpeed));
	else if (nZoomDirection < 0)
		m_fScale *= (1.0f - (0.01f * m_fScaleSpeed));
}

void COpenGLCamera::Move(float fx, float fy, float fz)
{
	glm::vec4 translate(-1.0f * fx/4000.0f, fy/4000.0f, fz/4000.0f, 1.0f);
	translate *= m_fMoveSpeed;
	translate = glm::inverse(m_matRotation) * translate;
	m_vec4Move += translate;
}

void COpenGLCamera::Move(GLfloat fx, GLfloat fy, GLfloat fz, GLboolean bFreeCut)
{
	glm::vec4 translate(-1.0f * fx/4000.0f, fy/4000.0f, fz/4000.0f, 1.0f);
	translate *= m_fMoveSpeed;
	translate = glm::inverse(m_matRotation) * translate;
	m_vec4Move += translate;

	glm::vec4 translate2(-1.0f * fx/4000.0f, fy/4000.0f, fz/4000.0f, 1.0f);
	translate2 *= m_fMoveSpeed;
	translate2 = glm::inverse(m_matRotationForFreeCut) * translate2;
	m_vec4MoveForFreeCut += translate2;
}

void COpenGLCamera::Rotate(float fx, float fy, float fz)
{
	glm::mat4 matrixTmp = glm::mat4(1.0f);
	matrixTmp = glm::rotate(matrixTmp, glm::radians(fx * m_fRotateSpeed / 4.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	matrixTmp = glm::rotate(matrixTmp, glm::radians(fy * m_fRotateSpeed / 4.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	matrixTmp = glm::rotate(matrixTmp, glm::radians(fz * m_fRotateSpeed / 4.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//matrixTmp *= (GLfloat)m_nRotateSpeed;
	m_matRotation = matrixTmp * m_matRotation;

}

void COpenGLCamera::Rotate(GLfloat fx, GLfloat fy, GLfloat fz, GLboolean bFreecut)
{
	m_bRotate = true;
	glm::mat4 matrixTmp = glm::mat4(1.0f);
	matrixTmp = glm::rotate(matrixTmp, glm::radians(fx * m_fRotateSpeed / 4.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	matrixTmp = glm::rotate(matrixTmp, glm::radians(fy * m_fRotateSpeed / 4.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	matrixTmp = glm::rotate(matrixTmp, glm::radians(fz * m_fRotateSpeed / 4.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//matrixTmp *= (GLfloat)m_nRotateSpeed;
	if (!bFreecut)
		m_matRotation = matrixTmp * m_matRotation;
	m_matRotationForFreeCut = matrixTmp * m_matRotationForFreeCut;
}

void COpenGLCamera::SetSpeed(GLuint nType, GLuint nSpeed)
{
	switch(nType)
	{
	case MOVE:
		m_fMoveSpeed = (GLfloat)nSpeed/25.0f;
		break;
	case ROTATE:
		m_fRotateSpeed = (GLfloat)nSpeed/25.0f;
		break;
	case SCALE:
		m_fScaleSpeed = (GLfloat)nSpeed/25.0f;
		break;
	default:
		break;
	}
}

void COpenGLCamera::AutoRotate(GLuint nDirect /*= XDIRECTION*/)
{
	switch (nDirect)
	{
	case XDIRECTION:
		//m_matRotation = glm::rotate(m_matRotation, glm::radians(m_nRotateSpeed), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case YDIRECTION:
		//m_matRotation = glm::rotate(m_matRotation, glm::radians(m_nRotateSpeed), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case ZDIRECTION:
		//m_matRotation = glm::rotate(m_matRotation, glm::radians(m_nRotateSpeed), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	default:
		break;
	}
}

void COpenGLCamera::ChangeRotateCenterPoint(float fx, float fy, float fz)
{
	m_vec3RotateCenter = glm::vec3(fx, fy, fz);
}

void COpenGLCamera::ResetRotateCenterPoint()
{
	m_vec3RotateCenter = glm::vec3(-0.5f);
}

void COpenGLCamera::ResetAll()
{
	m_matRotation = glm::mat4(1.0f);
	m_matRotationForFreeCut = glm::mat4(1.0f);
	m_vec4Move = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	m_vec4MoveForFreeCut = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	m_vec3RotateCenter = glm::vec3(-0.5f);
	m_fViewAngle = 45.0f;
	m_fScale = 1.0f;
	m_fMoveSpeed = 4;
	m_fRotateSpeed = 4;
	m_fScaleSpeed = 4;

}



void COpenGLCamera::ResetRotation(GLuint nDirect /*= XDIRECTION*/)
{
	switch (nDirect)
	{
	case XDIRECTION:
		m_matRotation = glm::mat4(1.0f);
		break;
	case YDIRECTION:
		m_matRotation = glm::mat4(1.0f);
		break;
	case ZDIRECTION:
		m_matRotation = glm::mat4(1.0f);
		break;
	default:
		break;
	}
}

void COpenGLCamera::SetViewAngle(GLfloat fViewAngle)
{
	m_fViewAngle = fViewAngle;
}

void COpenGLCamera::SetScale(GLfloat fScale)
{
	m_fScale = 1.0/fScale;
}

void COpenGLCamera::SetRotate(GLfloat fx, GLfloat fy, GLfloat fz)
{
	m_bRotate = true;
	glm::mat4 matrixTmp = glm::mat4(1.0f);
	matrixTmp = glm::rotate(matrixTmp, glm::radians(fx), glm::vec3(0.0f, -1.0f, 0.0f));
	matrixTmp = glm::rotate(matrixTmp, glm::radians(fy), glm::vec3(-1.0f, 0.0f, 0.0f));
	matrixTmp = glm::rotate(matrixTmp, glm::radians(fz), glm::vec3(0.0f, 0.0f, 1.0f));
	m_matRotation = matrixTmp;
	m_matRotationForFreeCut = matrixTmp;
}

void COpenGLCamera::SetLightColor(COLORREF color)
{
	GLuint r = (BYTE)(color);
	GLuint g = (BYTE)(color >> 8);
	GLuint b = (BYTE)(color >> 16);
	m_vec3LightColor = glm::vec3((GLfloat)r/256.0f, (GLfloat)g/256.0f, (GLfloat)b/256.0f);
}

void COpenGLCamera::SetMaterialColor(COLORREF color)
{
	GLuint r = (BYTE)(color);
	GLuint g = (BYTE)(color >> 8);
	GLuint b = (BYTE)(color >> 16);
	m_vec3MaterialColor = glm::vec3((GLfloat)r/256.0f, (GLfloat)g/256.0f, (GLfloat)b/256.0f);
}

void COpenGLCamera::SetLightMat(LIGHTMAT type, GLfloat value)
{
	switch(type)
	{
	case EMISSION:
		{
			m_vec4LightMat.x = value;
			break;
		}
	case DIFFUSE:
		{
			m_vec4LightMat.y = value;
			break;
		}
	case REFLECT:
		{
			m_vec4LightMat.z = value;
			break;
		}
	case SPECULAR:
		{
			m_vec4LightMat.w = value;
			break;
		}
	default:
		break;
	}
}

void COpenGLCamera::SetLightPos(GLfloat fx, GLfloat fy, GLfloat fz)
{
	m_vec3LightPos = glm::vec3(fx, fy, fz);
}

void COpenGLCamera::NeedLight(GLboolean needed)
{
	m_bLight = needed;
}

void COpenGLCamera::NeedShadow(GLboolean needed)
{
	m_bShadow = needed;
}

void COpenGLCamera::SetShadow(GLfloat shadow)
{
	m_fShadow = shadow;
}

void COpenGLCamera::SetCameraPos(GLfloat fx, GLfloat fy, GLfloat fz)
{
	m_vec3CameraPos = glm::vec3(fx, fy, fz);
}

void COpenGLCamera::MoveOrRotate()
{
	//glm::mat4 matrixTmp = glm::mat4(1.0f);
	//glm::vec4 translate(0.0f, 0.0f, 0.0f, 1.0f);

	//if (m_bMoveUp)
	//	translate = glm::vec4(0.0f, 0.01f, 0.0f, 1.0f);
	//if (m_bMoveDown)
	//	translate = glm::vec4(0.0f, -0.01f, 0.0f, 1.0f);
	//if (m_bMoveLeft)
	//	translate =  glm::vec4(-0.01f, 0.0f, 0.0f, 1.0f);
	//if (m_bMoveRight)
	//	translate =  glm::vec4(0.01f, 0.0f, 0.0f, 1.0f);
	//translate *= m_fMoveSpeed;

	//translate = inverse(m_matRotation) * translate;
	//m_vec4Move += translate;

	//
	//matrixTmp = glm::mat4(1.0f);
	//GLfloat rotateAngle = 0.0f;
	//if (m_bRotateClockWise)
	//	rotateAngle += 1.0f;
	//if (m_bRotateAntiClockWise)
	//	rotateAngle -= 1.0f;
	//rotateAngle *= m_fRotateSpeed;
	//matrixTmp = glm::rotate(matrixTmp, glm::radians(rotateAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	//	
	//m_matRotation = matrixTmp * m_matRotation;
	
	if (m_bMoveUp)
		Move(0.0f, 15.0f, 0.0f, true);
	if (m_bMoveDown)
		Move(0.0f, -15.0f, 0.0f, true);
	if (m_bMoveLeft)
		Move(15.0f, 0.0f, 0.0f, true);
	if (m_bMoveRight)
		Move(-15.0f, 0.0f, 0.0f, true);

	if (m_bRotateClockWise)
		Rotate(10.0f, 0.0f, 0.0f, false);
	if (m_bRotateAntiClockWise)
		Rotate(-10.0f, 0.0f, 0.0f, false);
}

void COpenGLCamera::SetKey(GLuint keyFlag, GLboolean bStatus)
{
	//"w" 的ACSLL码 87
	//"a" 的ACSLL码 65
	//"s" 的ACSLL码 83
	//"d" 的ACSLL码 68
	//"q" 的ACSLL码 81
	//"e" 的ACSLL码 69
	switch (keyFlag)
	{
	case 87:
		m_bMoveUp = bStatus;
		break;
	case 65:
		m_bMoveLeft = bStatus;
		break;
	case 83:
		m_bMoveDown = bStatus;
		break;
	case 68:
		m_bMoveRight = bStatus;
		break;
	case 81:
		m_bRotateAntiClockWise = bStatus;
		break;
	case 69:
		m_bRotateClockWise = bStatus;
		break;
	default:
		break;
	}
}

void COpenGLCamera::SetStepScale(GLfloat fStepScale)
{
// 	if (fStepScale <= 50)
// 	{
// 		m_fStepScale = (1.0f - 1.0f / fStepScale);
// 	}
// 	else
// 	{
// 		m_fStepScale = 1.0f + (fStepScale - 50) / 5.0f;
// 	}
	m_fStepScale = (fStepScale - 50.0f) / 33.3f;

}

void COpenGLCamera::SetOffsetScale(GLfloat fOffsetScale)
{
// 	if (fOffsetScale <= 50)
// 	{
// 		m_fOffsetScale = (1.0f - 1.0f / fOffsetScale);
// 	}
// 	else
// 	{
// 		m_fOffsetScale = 1.0f + (fOffsetScale - 50) / 5.0f;
// 	}
	m_fOffsetScale = (fOffsetScale - 50.0f) / 33.3f;
}

void COpenGLCamera::SetBackgroundColor(COLORREF color)
{
	GLuint r = (BYTE)(color);
	GLuint g = (BYTE)(color >> 8);
	GLuint b = (BYTE)(color >> 16);
	m_vec3BackgroundColor = glm::vec3((GLfloat)r/256.0f, (GLfloat)g/256.0f, (GLfloat)b/256.0f);
}

void COpenGLCamera::SetRotateCenter(GLfloat fx, GLfloat fy, GLfloat fz)
{

}



