#pragma once


class SEV_EXT_CLASS SeVisualRotate
{
public:
	SeVisualRotate();
	~SeVisualRotate();

public:
	HGLRC				m_hRC;

	double				m_dXALLTrans;
	double				m_dYALLTrans;
	double				m_dZALLTrans;



private:
	MprRotateInfo       m_MprRotateInfo;
	V3dRotateInfo		m_V3dRotateInfo;

	// Operations
public:
	MprRotateInfo	ChangeRotateAngle_Current(bool bCube, double dAngleX, double dAngleY, MprRotateInfo CurrentInfo);
	MprRotateInfo	ChangeRotateAngle_CurrentZ(bool bCube, double dAngleZ, MprRotateInfo CurrentInfo);
	MprRotateInfo   ChangeRotateAngle_World(bool bCube, int nAxisNum, double dAngle,MprRotateInfo CurrentInfo);
	V3dRotateInfo	ChangeRotateAngle_V3D( double dAngleX, double dAngleY,V3dRotateInfo CurrentInfo);
	//V3dRotateInfo	GetCurrent(double &dMatrixTemp[6]);

private:            
	////////////////////////////////////////////////////////////////////////////////////////
//	void			InitOpenGL();
//	BOOL			SetUpPixelFormat();
	////////////////////////////////////////////////////////////////////////////////////////

	void			UpdataTransX( bool bWorldorCurrent, bool bRotateCube, int nAxisNum, double dAngle );
	void			UpdataTransY( bool bWorldorCurrent, bool bRotateCube, int nAxisNum, double dAngle );
	void			UpdataTransZ( bool bWorldorCurrent, bool bRotateCube, int nAxisNum, double dAngle );
	void			UpdateXYZAxis( double dCurRotateAxisX, double dCurRotateAxisY, double dCurRotateAxisZ, double dAngle, double* dRotateAccumu, double* dCurRotateInvMatrix, double dOriX, double dOriY, double dOriZ, double& dNewX, double& dNewY, double& dNewZ );

	////////////////////////////////////////////////////////////////////////////////////////

	void			UpdataV3DWorldTransY( double dAngleX );
	void			UpdataV3DWorldTransX( double dAngleY );
};

