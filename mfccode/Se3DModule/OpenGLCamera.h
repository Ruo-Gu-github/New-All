#pragma once


class AFX_EXT_CLASS COpenGLCamera
{
public:
	COpenGLCamera(void);
	~COpenGLCamera(void);
	enum LIGHTMAT {EMISSION = 0, DIFFUSE, REFLECT, SPECULAR}; 
	
	// 外部调用
	void Scale(GLint nZoomDirection);
	void Move(GLfloat fx, GLfloat fy, GLfloat fz);
	void Move(GLfloat fx, GLfloat fy, GLfloat fz, GLboolean bFreeCut);
	void Rotate(GLfloat fx, GLfloat fy, GLfloat fz);
	void Rotate(GLfloat fx, GLfloat fy, GLfloat fz, GLboolean bFreecut);
	void MoveOrRotate();
	void ResetRotation(GLuint nDirect = XDIRECTION);
	void AutoRotate(GLuint nDirect = XDIRECTION);
	void SetSpeed(GLuint nType, GLuint nSpeed);
	void ChangeRotateCenterPoint(GLfloat fx, GLfloat fy, GLfloat fz);
	void ResetRotateCenterPoint();
	void ResetAll();
	// perspective 投影矩阵使用，现关闭
	void SetViewAngle(GLfloat fViewAngle);
	void SetScale(GLfloat fScale);
	void SetRotate(GLfloat fx, GLfloat fy, GLfloat fz);
	void SetBackgroundColor(COLORREF color);
	void SetCameraPos(GLfloat fx, GLfloat fy, GLfloat fz);
	void SetRotateCenter(GLfloat fx, GLfloat fy, GLfloat fz);
	void SetKey(GLuint keyFlag, GLboolean bStatus);
	void SetStepScale(GLfloat fStepScale);
	void SetOffsetScale(GLfloat fOffsetScale);

	// light
	void SetLightColor(COLORREF color);
	void SetMaterialColor(COLORREF color);
	void SetLightMat(LIGHTMAT type, GLfloat value);
	void SetLightPos(GLfloat fx, GLfloat fy, GLfloat fz);
	void NeedLight(GLboolean needed);
	void NeedShadow(GLboolean needed);
	void SetShadow(GLfloat shadow);

	// glMain 调用
	const glm::mat4 GetMatrix(){return m_matRotation;}
	const glm::mat4 GetFreeCutMatrix(){return m_matRotationForFreeCut;}
	const glm::vec4 GetMove(){return m_vec4Move;}
	const glm::vec4 GetFreeCutMove(){return m_vec4MoveForFreeCut;}
	// perspective 投影矩阵使用，现关闭
	const GLfloat   GetViewAngle(){return m_fViewAngle;}
	const GLfloat   GetScale(){return m_fScale;}
	const glm::vec3 GetBackgroundColor(){return m_vec3BackgroundColor;}
	const glm::vec3 GetCameraPos(){return m_vec3CameraPos;}
	const glm::vec3 GetRotateCenter(){return m_vec3RotateCenter;}
	const GLfloat   GetStepScale(){return m_fStepScale;}
	const GLfloat   GetOffsetScale(){return m_fOffsetScale;}
	const GLboolean IsRotate(){return m_bRotate;}
	const void      ResetRotate(){m_bRotate = false;}
	

	// light
	const glm::vec3 GetLightColor(){return m_vec3LightColor;}
	const glm::vec3 GetMaterialColor(){return m_vec3MaterialColor;}
	const glm::vec4 GetLightMat(){return m_vec4LightMat;}
	const glm::vec3 GetLightPos(){return m_vec3LightPos;}
	const GLboolean NeedLight(){return m_bLight;}
	const GLboolean NeedShadow(){return m_bShadow;}
	const GLfloat   GetShadow(){return m_fShadow;}

private:
	glm::mat4 m_matRotation;
	glm::mat4 m_matRotationForFreeCut;
	glm::vec4 m_vec4Move;
	glm::vec4 m_vec4MoveForFreeCut;
	glm::vec3 m_vec3RotateCenter;
	glm::vec3 m_vec3CameraPos;


	glm::vec3 m_vec3BackgroundColor;
	GLfloat m_fViewAngle;
	GLfloat m_fScale;
	GLfloat m_fMoveSpeed;
	GLfloat m_fRotateSpeed;
	GLfloat m_fScaleSpeed;
	GLfloat m_fPixelSize;
	GLfloat m_fStepScale;
	GLfloat m_fOffsetScale;

	// light
	glm::vec4 m_vec4LightMat;
	glm::vec3 m_vec3LightColor;
	glm::vec3 m_vec3MaterialColor;
	glm::vec3 m_vec3LightPos;
	GLboolean m_bLight;
	GLboolean m_bShadow;
	GLfloat   m_fShadow;
	
	// keyboard move action
	GLboolean m_bMoveLeft;
	GLboolean m_bMoveRight;
	GLboolean m_bMoveUp;
	GLboolean m_bMoveDown;
	GLboolean m_bRotateClockWise;
	GLboolean m_bRotateAntiClockWise;
	GLboolean m_bRotate;

	// is rotate
	
};