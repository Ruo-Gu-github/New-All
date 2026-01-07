#include "stdafx.h"
#include "SeVisualRotate.h"


SeVisualRotate::SeVisualRotate()
{
	m_dXALLTrans = m_dYALLTrans = m_dZALLTrans = 0.0;
}


SeVisualRotate::~SeVisualRotate()
{
}

/*
void SeVisualRotate::InitOpenGL()
{
	CDC	dc;

	if(!SetUpPixelFormat())
	{
		return;
	}

	m_hRC = wglCreateContext(dc);
	wglMakeCurrent(dc,m_hRC);

	glShadeModel(GL_SMOOTH);
	glClearColor(0.0,0.0,0.0,0.0);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_NORMALIZE);
}

BOOL SeVisualRotate::SetUpPixelFormat()
{
	PIXELFORMATDESCRIPTOR pdf = 
	{   sizeof(PIXELFORMATDESCRIPTOR),
	1,                                         // 版本号
	PFD_DRAW_TO_WINDOW |                     // 格式支持窗口
	PFD_SUPPORT_OPENGL |                     // 格式必须支持OpenGL
	PFD_DOUBLEBUFFER|                          // 格式支持双缓冲   
	PFD_STEREO_DONTCARE,
	PFD_TYPE_RGBA,                             // 申请RGBA格式
	24,                                        // 色彩深度，此处为24位
	0,    0,    0,    0,    0,    0,                // 忽略的色彩位
	0,                                         // 无Alpha缓存
	0,                                         // 忽略Shift位
	0,                                         // 无累加缓存
	0,    0,    0,    0,                          // 忽略聚集位
	32,                                        // 32位 Z-缓存（深度缓存）
	0,                                         // 无蒙板缓存
	0,                                         // 无辅助缓存
	PFD_MAIN_PLANE,                            // 主绘图层
	0,                                         // Reserved
	0,    0,    0                      
	};
	int pixelformat;

	CDC dc;

	pixelformat = ChoosePixelFormat(dc,&pdf);
	if(!pixelformat)
	{
//		MessageBox("ChoosePixelFormat    failed");   
		return    FALSE;   
	}
	if(!SetPixelFormat(dc,pixelformat,&pdf))
	{
//		MessageBox("SetPixelFormat Failed");
		return   FALSE; 
	}
	return TRUE;
}
*/
void SeVisualRotate::UpdataTransX( bool bWorldorCurrent, bool bRotateCube, int nAxisNum, double dAngle )
{
	double dWorldX_x = 1.0;
	double dWorldX_y = 0.0;
	double dWorldX_z = 0.0;

	switch (nAxisNum)
	{
	case 1:
		{
			if (!bRotateCube)
			{
				if (bWorldorCurrent)
				{
					UpdateXYZAxis(m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z, dAngle, m_MprRotateInfo.dRotateWorldX, m_MprRotateInfo.dModelViewCurrentInvX, 
						dWorldX_x, dWorldX_y, dWorldX_z, m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z);
				}
				else
				{
					UpdateXYZAxis(1.0, 0.0, 0.0, dAngle, m_MprRotateInfo.dRotateWorldX, m_MprRotateInfo.dModelViewCurrentInvX, 
						dWorldX_x, dWorldX_y, dWorldX_z, m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z);
				}

			}
			else
			{
				if (bWorldorCurrent)
				{
					UpdateXYZAxis(m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z, dAngle, m_MprRotateInfo.dRotateWorldX, m_MprRotateInfo.dModelViewCurrentInvX, 
						dWorldX_x, dWorldX_y, dWorldX_z, m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z);
				}
				else
				{
					UpdateXYZAxis(1.0, 0.0, 0.0, dAngle, m_MprRotateInfo.dRotateWorldX, m_MprRotateInfo.dModelViewCurrentInvX, 
						dWorldX_x, dWorldX_y, dWorldX_z, m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z);
				}

			}
		}
		break;
	case 2:
		{
			if (!bRotateCube)
			{
				if (bWorldorCurrent)
				{
					UpdateXYZAxis(m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z, dAngle, m_MprRotateInfo.dRotateWorldX, m_MprRotateInfo.dModelViewCurrentInvX, 
						dWorldX_x, dWorldX_y, dWorldX_z, m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z);
				}
				else
				{
					UpdateXYZAxis(0.0, 1.0, 0.0, dAngle, m_MprRotateInfo.dRotateWorldX, m_MprRotateInfo.dModelViewCurrentInvX, 
						dWorldX_x, dWorldX_y, dWorldX_z, m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z);
				}

			}
			else
			{
				if (bWorldorCurrent)
				{
					UpdateXYZAxis(m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z, dAngle, m_MprRotateInfo.dRotateWorldX, m_MprRotateInfo.dModelViewCurrentInvX, 
						dWorldX_x, dWorldX_y, dWorldX_z, m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z);
				}
				else
				{
					UpdateXYZAxis(0.0, 1.0, 0.0, dAngle, m_MprRotateInfo.dRotateWorldX, m_MprRotateInfo.dModelViewCurrentInvX, 
						dWorldX_x, dWorldX_y, dWorldX_z, m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z);
				}
			}
		}
		break;
	case 3:
		{
			if (!bRotateCube)
			{
				if (bWorldorCurrent)
				{
					UpdateXYZAxis(m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z, dAngle, m_MprRotateInfo.dRotateWorldX, m_MprRotateInfo.dModelViewCurrentInvX, 
						dWorldX_x, dWorldX_y, dWorldX_z, m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z);
				} 
				else
				{
					UpdateXYZAxis(0.0, 0.0, 1.0, dAngle, m_MprRotateInfo.dRotateWorldX, m_MprRotateInfo.dModelViewCurrentInvX, 
						dWorldX_x, dWorldX_y, dWorldX_z, m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z);
				}	
			}
			else
			{
				if (bWorldorCurrent)
				{
					UpdateXYZAxis(m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z, dAngle, m_MprRotateInfo.dRotateWorldX, m_MprRotateInfo.dModelViewCurrentInvX, 
						dWorldX_x, dWorldX_y, dWorldX_z, m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z);
				}
				else
				{
					UpdateXYZAxis(0.0, 0.0, 1.0, dAngle, m_MprRotateInfo.dRotateWorldX, m_MprRotateInfo.dModelViewCurrentInvX, 
						dWorldX_x, dWorldX_y, dWorldX_z, m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z);
				}
			}
		}
		break;
	default:
		break;
	}
}

void SeVisualRotate::UpdataTransY( bool bWorldorCurrent, bool bRotateCube, int nAxisNum, double dAngle )
{
	double dWorldY_x = 0.0;
	double dWorldY_y = 1.0;
	double dWorldY_z = 0.0;

	switch (nAxisNum)
	{
	case 1:
		{
			if (!bRotateCube)
			{
				if (bWorldorCurrent)
				{
					UpdateXYZAxis(m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z, dAngle, m_MprRotateInfo.dRotateWorldY, m_MprRotateInfo.dModelViewCurrentInvY, 
						dWorldY_x, dWorldY_y, dWorldY_z, m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z);
				} 
				else
				{
					UpdateXYZAxis(1.0, 0.0, 0.0, dAngle, m_MprRotateInfo.dRotateWorldY, m_MprRotateInfo.dModelViewCurrentInvY, 
						dWorldY_x, dWorldY_y, dWorldY_z, m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z);
				}

			}
			else
			{
				if (bWorldorCurrent)
				{
					UpdateXYZAxis(m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z, dAngle, m_MprRotateInfo.dRotateWorldY, m_MprRotateInfo.dModelViewCurrentInvY, 
						dWorldY_x, dWorldY_y, dWorldY_z, m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z);
				} 
				else
				{
					UpdateXYZAxis(1.0, 0.0, 0.0, dAngle, m_MprRotateInfo.dRotateWorldY, m_MprRotateInfo.dModelViewCurrentInvY, 
						dWorldY_x, dWorldY_y, dWorldY_z, m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z);
				}
			}
		}
		break;
	case 2:
		{
			if (!bRotateCube)
			{
				if (bWorldorCurrent)
				{
					UpdateXYZAxis(m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z, dAngle, m_MprRotateInfo.dRotateWorldY, m_MprRotateInfo.dModelViewCurrentInvY, 
						dWorldY_x, dWorldY_y, dWorldY_z, m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z);
				} 
				else
				{
					UpdateXYZAxis(0.0, 1.0, 0.0, dAngle, m_MprRotateInfo.dRotateWorldY, m_MprRotateInfo.dModelViewCurrentInvY, 
						dWorldY_x, dWorldY_y, dWorldY_z, m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z);
				}

			}
			else
			{
				if (bWorldorCurrent)
				{
					UpdateXYZAxis(m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z, dAngle, m_MprRotateInfo.dRotateWorldY, m_MprRotateInfo.dModelViewCurrentInvY, 
						dWorldY_x, dWorldY_y, dWorldY_z, m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z);
				} 
				else
				{
					UpdateXYZAxis(0.0, 1.0, 0.0, dAngle, m_MprRotateInfo.dRotateWorldY, m_MprRotateInfo.dModelViewCurrentInvY, 
						dWorldY_x, dWorldY_y, dWorldY_z, m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z);
				}
			}
		}
		break;
	case 3:
		{
			if (!bRotateCube)
			{
				if (bWorldorCurrent)
				{
					UpdateXYZAxis(m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z, dAngle, m_MprRotateInfo.dRotateWorldY, m_MprRotateInfo.dModelViewCurrentInvY, 
						dWorldY_x, dWorldY_y, dWorldY_z, m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z);
				} 
				else
				{
					UpdateXYZAxis(0.0, 0.0, 1.0, dAngle, m_MprRotateInfo.dRotateWorldY, m_MprRotateInfo.dModelViewCurrentInvY, 
						dWorldY_x, dWorldY_y, dWorldY_z, m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z);
				}

			}
			else
			{
				if (bWorldorCurrent)
				{
					UpdateXYZAxis(m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z, dAngle, m_MprRotateInfo.dRotateWorldY, m_MprRotateInfo.dModelViewCurrentInvY, 
						dWorldY_x, dWorldY_y, dWorldY_z, m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z);
				} 
				else
				{
					UpdateXYZAxis(0.0, 0.0, 1.0, dAngle, m_MprRotateInfo.dRotateWorldY, m_MprRotateInfo.dModelViewCurrentInvY, 
						dWorldY_x, dWorldY_y, dWorldY_z, m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z);
				}

			}
		}
		break;
	default:
		break;
	}
}

void SeVisualRotate::UpdataTransZ( bool bWorldorCurrent, bool bRotateCube, int nAxisNum, double dAngle )
{
	double dWorldZ_x = 0.0;
	double dWorldZ_y = 0.0;
	double dWorldZ_z = 1.0;

	switch (nAxisNum)
	{
	case 1:
		{
			if (!bRotateCube)
			{
				if (bWorldorCurrent)
				{
					UpdateXYZAxis(m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z, dAngle, m_MprRotateInfo.dRotateWorldZ, m_MprRotateInfo.dModelViewCurrentInvZ, 
						dWorldZ_x, dWorldZ_y, dWorldZ_z, m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z);
				} 
				else
				{
					UpdateXYZAxis(1.0, 0.0, 0.0, dAngle, m_MprRotateInfo.dRotateWorldZ, m_MprRotateInfo.dModelViewCurrentInvZ, 
						dWorldZ_x, dWorldZ_y, dWorldZ_z, m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z);
				}

			}
			else
			{
				if (bWorldorCurrent)
				{
					UpdateXYZAxis(m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z, dAngle, m_MprRotateInfo.dRotateWorldZ, m_MprRotateInfo.dModelViewCurrentInvZ, 
						dWorldZ_x, dWorldZ_y, dWorldZ_z, m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z);
				} 
				else
				{
					UpdateXYZAxis(1.0, 0.0, 0.0, dAngle, m_MprRotateInfo.dRotateWorldZ, m_MprRotateInfo.dModelViewCurrentInvZ, 
						dWorldZ_x, dWorldZ_y, dWorldZ_z, m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z);
				}
			}
		}
		break;
	case 2:
		{
			if (!bRotateCube)
			{
				if (bWorldorCurrent)
				{
					UpdateXYZAxis(m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z, dAngle, m_MprRotateInfo.dRotateWorldZ, m_MprRotateInfo.dModelViewCurrentInvZ, 
						dWorldZ_x, dWorldZ_y, dWorldZ_z, m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z);
				} 
				else
				{
					UpdateXYZAxis(0.0, 1.0, 0.0, dAngle, m_MprRotateInfo.dRotateWorldZ, m_MprRotateInfo.dModelViewCurrentInvZ, 
						dWorldZ_x, dWorldZ_y, dWorldZ_z, m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z);
				}

			}
			else
			{
				if (bWorldorCurrent)
				{
					UpdateXYZAxis(m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z, dAngle, m_MprRotateInfo.dRotateWorldZ, m_MprRotateInfo.dModelViewCurrentInvZ, 
						dWorldZ_x, dWorldZ_y, dWorldZ_z, m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z);
				} 
				else
				{
					UpdateXYZAxis(0.0, 1.0, 0.0, dAngle, m_MprRotateInfo.dRotateWorldZ, m_MprRotateInfo.dModelViewCurrentInvZ, 
						dWorldZ_x, dWorldZ_y, dWorldZ_z, m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z);
				}
			}
		}
		break;
	case 3:
		{
			if (!bRotateCube)
			{
				if (bWorldorCurrent)
				{
					UpdateXYZAxis(m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z, dAngle, m_MprRotateInfo.dRotateWorldZ, m_MprRotateInfo.dModelViewCurrentInvZ, 
						dWorldZ_x, dWorldZ_y, dWorldZ_z, m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z);
				} 
				else
				{
					UpdateXYZAxis(0.0, 0.0, 1.0, dAngle, m_MprRotateInfo.dRotateWorldZ, m_MprRotateInfo.dModelViewCurrentInvZ, 
						dWorldZ_x, dWorldZ_y, dWorldZ_z, m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z);
				}

			}
			else
			{
				if (bWorldorCurrent)
				{
					UpdateXYZAxis(m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z, dAngle, m_MprRotateInfo.dRotateWorldZ, m_MprRotateInfo.dModelViewCurrentInvZ, 
						dWorldZ_x, dWorldZ_y, dWorldZ_z, m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z);
				} 
				else
				{
					UpdateXYZAxis(0.0, 0.0, 1.0, dAngle, m_MprRotateInfo.dRotateWorldZ, m_MprRotateInfo.dModelViewCurrentInvZ, 
						dWorldZ_x, dWorldZ_y, dWorldZ_z, m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z);
				}
			}
		}
		break;
	default:
		break;
	}
}

void SeVisualRotate::UpdateXYZAxis( double dCurRotateAxisX, double dCurRotateAxisY, double dCurRotateAxisZ, double dAngle, double* dRotateAccumu, double* dCurRotateInvMatrix, double dOriX, double dOriY, double dOriZ, double& dNewX, double& dNewY, double& dNewZ )
{
	double dMatrixTemp[16];

	//沿当前旋转轴反转
	glPushMatrix();
	glLoadIdentity();
	glRotated(-dAngle, dCurRotateAxisX, dCurRotateAxisY, dCurRotateAxisZ);
	glGetDoublev(GL_MODELVIEW_MATRIX, dCurRotateInvMatrix);
	glPopMatrix();

	//与之前累积反转结果结合，计算出当前X轴旋转至世界坐标下当前X轴的旋转矩阵
	dMatrixTemp[0] = dRotateAccumu[0]*dCurRotateInvMatrix[0] + dRotateAccumu[1]*dCurRotateInvMatrix[4] + dRotateAccumu[2]*dCurRotateInvMatrix[8] + dRotateAccumu[3]*dCurRotateInvMatrix[12];
	dMatrixTemp[1] = dRotateAccumu[0]*dCurRotateInvMatrix[1] + dRotateAccumu[1]*dCurRotateInvMatrix[5] + dRotateAccumu[2]*dCurRotateInvMatrix[9] + dRotateAccumu[3]*dCurRotateInvMatrix[13];
	dMatrixTemp[2] = dRotateAccumu[0]*dCurRotateInvMatrix[2] + dRotateAccumu[1]*dCurRotateInvMatrix[6] + dRotateAccumu[2]*dCurRotateInvMatrix[10] + dRotateAccumu[3]*dCurRotateInvMatrix[14];		
	dMatrixTemp[3] = dRotateAccumu[0]*dCurRotateInvMatrix[3] + dRotateAccumu[1]*dCurRotateInvMatrix[7] + dRotateAccumu[2]*dCurRotateInvMatrix[11] + dRotateAccumu[3]*dCurRotateInvMatrix[15];
	dMatrixTemp[4] = dRotateAccumu[4]*dCurRotateInvMatrix[0] + dRotateAccumu[5]*dCurRotateInvMatrix[4] + dRotateAccumu[6]*dCurRotateInvMatrix[8] + dRotateAccumu[7]*dCurRotateInvMatrix[12];
	dMatrixTemp[5] = dRotateAccumu[4]*dCurRotateInvMatrix[1] + dRotateAccumu[5]*dCurRotateInvMatrix[5] + dRotateAccumu[6]*dCurRotateInvMatrix[9] + dRotateAccumu[7]*dCurRotateInvMatrix[13];
	dMatrixTemp[6] = dRotateAccumu[4]*dCurRotateInvMatrix[2] + dRotateAccumu[5]*dCurRotateInvMatrix[6] + dRotateAccumu[6]*dCurRotateInvMatrix[10] + dRotateAccumu[7]*dCurRotateInvMatrix[14];
	dMatrixTemp[7] = dRotateAccumu[4]*dCurRotateInvMatrix[3] + dRotateAccumu[5]*dCurRotateInvMatrix[7] + dRotateAccumu[6]*dCurRotateInvMatrix[11] + dRotateAccumu[7]*dCurRotateInvMatrix[15];
	dMatrixTemp[8] = dRotateAccumu[8]*dCurRotateInvMatrix[0] + dRotateAccumu[9]*dCurRotateInvMatrix[4] + dRotateAccumu[10]*dCurRotateInvMatrix[8] + dRotateAccumu[11]*dCurRotateInvMatrix[12];
	dMatrixTemp[9] = dRotateAccumu[8]*dCurRotateInvMatrix[1] + dRotateAccumu[9]*dCurRotateInvMatrix[5] + dRotateAccumu[10]*dCurRotateInvMatrix[9] + dRotateAccumu[11]*dCurRotateInvMatrix[13];
	dMatrixTemp[10] = dRotateAccumu[8]*dCurRotateInvMatrix[2] + dRotateAccumu[9]*dCurRotateInvMatrix[6] + dRotateAccumu[10]*dCurRotateInvMatrix[10] + dRotateAccumu[11]*dCurRotateInvMatrix[14];
	dMatrixTemp[11] = dRotateAccumu[8]*dCurRotateInvMatrix[3] + dRotateAccumu[9]*dCurRotateInvMatrix[7] + dRotateAccumu[10]*dCurRotateInvMatrix[11] + dRotateAccumu[11]*dCurRotateInvMatrix[15];
	dMatrixTemp[12] = dRotateAccumu[12]*dCurRotateInvMatrix[0] + dRotateAccumu[13]*dCurRotateInvMatrix[4] + dRotateAccumu[14]*dCurRotateInvMatrix[8] + dRotateAccumu[15]*dCurRotateInvMatrix[12];
	dMatrixTemp[13] = dRotateAccumu[12]*dCurRotateInvMatrix[1] + dRotateAccumu[13]*dCurRotateInvMatrix[5] + dRotateAccumu[14]*dCurRotateInvMatrix[9] + dRotateAccumu[15]*dCurRotateInvMatrix[13];
	dMatrixTemp[14] = dRotateAccumu[12]*dCurRotateInvMatrix[2] + dRotateAccumu[13]*dCurRotateInvMatrix[6] + dRotateAccumu[14]*dCurRotateInvMatrix[10] + dRotateAccumu[15]*dCurRotateInvMatrix[14];
	dMatrixTemp[15] = dRotateAccumu[12]*dCurRotateInvMatrix[3] + dRotateAccumu[13]*dCurRotateInvMatrix[7] + dRotateAccumu[14]*dCurRotateInvMatrix[11] + dRotateAccumu[15]*dCurRotateInvMatrix[15];

	for (int i = 0; i < 16; i++)
	{
		dRotateAccumu[i] = dMatrixTemp[i];
	}

	//更新X轴
	dNewX = dOriX * dMatrixTemp[0] + dOriY * dMatrixTemp[4]
	+ dOriZ * dMatrixTemp[8];
	dNewY = dOriX * dMatrixTemp[1] + dOriY * dMatrixTemp[5]
	+ dOriZ * dMatrixTemp[9];
	dNewZ = dOriX * dMatrixTemp[2] + dOriY * dMatrixTemp[6]
	+ dOriZ * dMatrixTemp[10];

	//注意：世界坐标下当前XYZ轴在每次运算后要归一化，减小累积误差
	double dModule = sqrt(dNewX*dNewX + dNewY*dNewY + dNewZ*dNewZ);
	dNewX = dNewX/dModule;
	dNewY = dNewY/dModule;
	dNewZ = dNewZ/dModule;
}

void SeVisualRotate::UpdataV3DWorldTransY( double dAngleX )
{
	double dVertiX = 0.0;
	double dVertiY = 1.0;
	double dVertiZ = 0.0;

	double dAngleInvX_R = dAngleX*PI/180.0;
	double dMatrixTemp[16];

	glPushMatrix();

	glLoadIdentity();
	glRotated(-dAngleX, m_V3dRotateInfo.dCurrentHoriX, m_V3dRotateInfo.dCurrentHoriY, m_V3dRotateInfo.dCurrentHoriZ);
	glGetDoublev(GL_MODELVIEW_MATRIX, m_V3dRotateInfo.dModelViewCurrentInvX);
	glPopMatrix();
	//其实就是dModelViewCurrentInvX*dRotateMatrixX,两个矩阵相乘
	dMatrixTemp[0] = m_V3dRotateInfo.dRotateMatrixX[0]*m_V3dRotateInfo.dModelViewCurrentInvX[0] + m_V3dRotateInfo.dRotateMatrixX[1]*m_V3dRotateInfo.dModelViewCurrentInvX[4] + m_V3dRotateInfo.dRotateMatrixX[2]*m_V3dRotateInfo.dModelViewCurrentInvX[8] + m_V3dRotateInfo.dRotateMatrixX[3]*m_V3dRotateInfo.dModelViewCurrentInvX[12];
	dMatrixTemp[1] = m_V3dRotateInfo.dRotateMatrixX[0]*m_V3dRotateInfo.dModelViewCurrentInvX[1] + m_V3dRotateInfo.dRotateMatrixX[1]*m_V3dRotateInfo.dModelViewCurrentInvX[5] + m_V3dRotateInfo.dRotateMatrixX[2]*m_V3dRotateInfo.dModelViewCurrentInvX[9] + m_V3dRotateInfo.dRotateMatrixX[3]*m_V3dRotateInfo.dModelViewCurrentInvX[13];
	dMatrixTemp[2] = m_V3dRotateInfo.dRotateMatrixX[0]*m_V3dRotateInfo.dModelViewCurrentInvX[2] + m_V3dRotateInfo.dRotateMatrixX[1]*m_V3dRotateInfo.dModelViewCurrentInvX[6] + m_V3dRotateInfo.dRotateMatrixX[2]*m_V3dRotateInfo.dModelViewCurrentInvX[10] + m_V3dRotateInfo.dRotateMatrixX[3]*m_V3dRotateInfo.dModelViewCurrentInvX[14];
	dMatrixTemp[3] = m_V3dRotateInfo.dRotateMatrixX[0]*m_V3dRotateInfo.dModelViewCurrentInvX[3] + m_V3dRotateInfo.dRotateMatrixX[1]*m_V3dRotateInfo.dModelViewCurrentInvX[7] + m_V3dRotateInfo.dRotateMatrixX[2]*m_V3dRotateInfo.dModelViewCurrentInvX[11] + m_V3dRotateInfo.dRotateMatrixX[3]*m_V3dRotateInfo.dModelViewCurrentInvX[15];
	dMatrixTemp[4] = m_V3dRotateInfo.dRotateMatrixX[4]*m_V3dRotateInfo.dModelViewCurrentInvX[0] + m_V3dRotateInfo.dRotateMatrixX[5]*m_V3dRotateInfo.dModelViewCurrentInvX[4] + m_V3dRotateInfo.dRotateMatrixX[6]*m_V3dRotateInfo.dModelViewCurrentInvX[8] + m_V3dRotateInfo.dRotateMatrixX[7]*m_V3dRotateInfo.dModelViewCurrentInvX[12];
	dMatrixTemp[5] = m_V3dRotateInfo.dRotateMatrixX[4]*m_V3dRotateInfo.dModelViewCurrentInvX[1] + m_V3dRotateInfo.dRotateMatrixX[5]*m_V3dRotateInfo.dModelViewCurrentInvX[5] + m_V3dRotateInfo.dRotateMatrixX[6]*m_V3dRotateInfo.dModelViewCurrentInvX[9] + m_V3dRotateInfo.dRotateMatrixX[7]*m_V3dRotateInfo.dModelViewCurrentInvX[13];
	dMatrixTemp[6] = m_V3dRotateInfo.dRotateMatrixX[4]*m_V3dRotateInfo.dModelViewCurrentInvX[2] + m_V3dRotateInfo.dRotateMatrixX[5]*m_V3dRotateInfo.dModelViewCurrentInvX[6] + m_V3dRotateInfo.dRotateMatrixX[6]*m_V3dRotateInfo.dModelViewCurrentInvX[10] + m_V3dRotateInfo.dRotateMatrixX[7]*m_V3dRotateInfo.dModelViewCurrentInvX[14];
	dMatrixTemp[7] = m_V3dRotateInfo.dRotateMatrixX[4]*m_V3dRotateInfo.dModelViewCurrentInvX[3] + m_V3dRotateInfo.dRotateMatrixX[5]*m_V3dRotateInfo.dModelViewCurrentInvX[7] + m_V3dRotateInfo.dRotateMatrixX[6]*m_V3dRotateInfo.dModelViewCurrentInvX[11] + m_V3dRotateInfo.dRotateMatrixX[7]*m_V3dRotateInfo.dModelViewCurrentInvX[15];
	dMatrixTemp[8] = m_V3dRotateInfo.dRotateMatrixX[8]*m_V3dRotateInfo.dModelViewCurrentInvX[0] + m_V3dRotateInfo.dRotateMatrixX[9]*m_V3dRotateInfo.dModelViewCurrentInvX[4] + m_V3dRotateInfo.dRotateMatrixX[10]*m_V3dRotateInfo.dModelViewCurrentInvX[8] + m_V3dRotateInfo.dRotateMatrixX[11]*m_V3dRotateInfo.dModelViewCurrentInvX[12];
	dMatrixTemp[9] = m_V3dRotateInfo.dRotateMatrixX[8]*m_V3dRotateInfo.dModelViewCurrentInvX[1] + m_V3dRotateInfo.dRotateMatrixX[9]*m_V3dRotateInfo.dModelViewCurrentInvX[5] + m_V3dRotateInfo.dRotateMatrixX[10]*m_V3dRotateInfo.dModelViewCurrentInvX[9] + m_V3dRotateInfo.dRotateMatrixX[11]*m_V3dRotateInfo.dModelViewCurrentInvX[13];
	dMatrixTemp[10] = m_V3dRotateInfo.dRotateMatrixX[8]*m_V3dRotateInfo.dModelViewCurrentInvX[2] + m_V3dRotateInfo.dRotateMatrixX[9]*m_V3dRotateInfo.dModelViewCurrentInvX[6] + m_V3dRotateInfo.dRotateMatrixX[10]*m_V3dRotateInfo.dModelViewCurrentInvX[10] + m_V3dRotateInfo.dRotateMatrixX[11]*m_V3dRotateInfo.dModelViewCurrentInvX[14];
	dMatrixTemp[11] = m_V3dRotateInfo.dRotateMatrixX[8]*m_V3dRotateInfo.dModelViewCurrentInvX[3] + m_V3dRotateInfo.dRotateMatrixX[9]*m_V3dRotateInfo.dModelViewCurrentInvX[7] + m_V3dRotateInfo.dRotateMatrixX[10]*m_V3dRotateInfo.dModelViewCurrentInvX[11] + m_V3dRotateInfo.dRotateMatrixX[11]*m_V3dRotateInfo.dModelViewCurrentInvX[15];
	dMatrixTemp[12] = m_V3dRotateInfo.dRotateMatrixX[12]*m_V3dRotateInfo.dModelViewCurrentInvX[0] + m_V3dRotateInfo.dRotateMatrixX[13]*m_V3dRotateInfo.dModelViewCurrentInvX[4] + m_V3dRotateInfo.dRotateMatrixX[14]*m_V3dRotateInfo.dModelViewCurrentInvX[8] + m_V3dRotateInfo.dRotateMatrixX[15]*m_V3dRotateInfo.dModelViewCurrentInvX[12];
	dMatrixTemp[13] = m_V3dRotateInfo.dRotateMatrixX[12]*m_V3dRotateInfo.dModelViewCurrentInvX[1] + m_V3dRotateInfo.dRotateMatrixX[13]*m_V3dRotateInfo.dModelViewCurrentInvX[5] + m_V3dRotateInfo.dRotateMatrixX[14]*m_V3dRotateInfo.dModelViewCurrentInvX[9] + m_V3dRotateInfo.dRotateMatrixX[15]*m_V3dRotateInfo.dModelViewCurrentInvX[13];
	dMatrixTemp[14] = m_V3dRotateInfo.dRotateMatrixX[12]*m_V3dRotateInfo.dModelViewCurrentInvX[2] + m_V3dRotateInfo.dRotateMatrixX[13]*m_V3dRotateInfo.dModelViewCurrentInvX[6] + m_V3dRotateInfo.dRotateMatrixX[14]*m_V3dRotateInfo.dModelViewCurrentInvX[10] + m_V3dRotateInfo.dRotateMatrixX[15]*m_V3dRotateInfo.dModelViewCurrentInvX[14];
	dMatrixTemp[15] = m_V3dRotateInfo.dRotateMatrixX[12]*m_V3dRotateInfo.dModelViewCurrentInvX[3] + m_V3dRotateInfo.dRotateMatrixX[13]*m_V3dRotateInfo.dModelViewCurrentInvX[7] + m_V3dRotateInfo.dRotateMatrixX[14]*m_V3dRotateInfo.dModelViewCurrentInvX[11] + m_V3dRotateInfo.dRotateMatrixX[15]*m_V3dRotateInfo.dModelViewCurrentInvX[15];

	for (int i = 0; i < 16; i++)
	{
		m_V3dRotateInfo.dRotateMatrixX[i] = dMatrixTemp[i];
	}

	m_V3dRotateInfo.dCurrentVertiX = dVertiX * dMatrixTemp[0] + dVertiY * dMatrixTemp[4]
	+ dVertiZ * dMatrixTemp[8];
	m_V3dRotateInfo.dCurrentVertiY = dVertiX * dMatrixTemp[1] + dVertiY * dMatrixTemp[5]
	+ dVertiZ * dMatrixTemp[9];
	m_V3dRotateInfo.dCurrentVertiZ = dVertiX * dMatrixTemp[2] + dVertiY * dMatrixTemp[6]
	+ dVertiZ * dMatrixTemp[10];

	//注意：世界坐标下当前横轴和纵轴在每次运算后要归一化，减小累积误差

	double dModule = sqrt(m_V3dRotateInfo.dCurrentVertiX*m_V3dRotateInfo.dCurrentVertiX + m_V3dRotateInfo.dCurrentVertiY*m_V3dRotateInfo.dCurrentVertiY + m_V3dRotateInfo.dCurrentVertiZ*m_V3dRotateInfo.dCurrentVertiZ);
	m_V3dRotateInfo.dCurrentVertiX = m_V3dRotateInfo.dCurrentVertiX/dModule;
	m_V3dRotateInfo.dCurrentVertiY = m_V3dRotateInfo.dCurrentVertiY/dModule;
	m_V3dRotateInfo.dCurrentVertiZ = m_V3dRotateInfo.dCurrentVertiZ/dModule;	
}

void SeVisualRotate::UpdataV3DWorldTransX( double dAngleY )
{
	double dHoriX = 1.0;
	double dHoriY = 0.0;
	double dHoriZ = 0.0;

	double dAngleInvY_R = dAngleY*PI/180.0;
	double dMatrixTemp[16];

	//沿纵轴反转
	glPushMatrix();
	glLoadIdentity();
	glRotated(-dAngleY, m_V3dRotateInfo.dCurrentVertiX, m_V3dRotateInfo.dCurrentVertiY, m_V3dRotateInfo.dCurrentVertiZ);
	glGetDoublev(GL_MODELVIEW_MATRIX, m_V3dRotateInfo.dModelViewCurrentInvY);
	glPopMatrix();

	//与之前累积反转结果结合，计算出当前x轴旋转至世界坐标下当前横轴的旋转矩阵
	dMatrixTemp[0] = m_V3dRotateInfo.dRotateMatrixY[0]*m_V3dRotateInfo.dModelViewCurrentInvY[0] + m_V3dRotateInfo.dRotateMatrixY[1]*m_V3dRotateInfo.dModelViewCurrentInvY[4] + m_V3dRotateInfo.dRotateMatrixY[2]*m_V3dRotateInfo.dModelViewCurrentInvY[8] + m_V3dRotateInfo.dRotateMatrixY[3]*m_V3dRotateInfo.dModelViewCurrentInvY[12];
	dMatrixTemp[1] = m_V3dRotateInfo.dRotateMatrixY[0]*m_V3dRotateInfo.dModelViewCurrentInvY[1] + m_V3dRotateInfo.dRotateMatrixY[1]*m_V3dRotateInfo.dModelViewCurrentInvY[5] + m_V3dRotateInfo.dRotateMatrixY[2]*m_V3dRotateInfo.dModelViewCurrentInvY[9] + m_V3dRotateInfo.dRotateMatrixY[3]*m_V3dRotateInfo.dModelViewCurrentInvY[13];
	dMatrixTemp[2] = m_V3dRotateInfo.dRotateMatrixY[0]*m_V3dRotateInfo.dModelViewCurrentInvY[2] + m_V3dRotateInfo.dRotateMatrixY[1]*m_V3dRotateInfo.dModelViewCurrentInvY[6] + m_V3dRotateInfo.dRotateMatrixY[2]*m_V3dRotateInfo.dModelViewCurrentInvY[10] + m_V3dRotateInfo.dRotateMatrixY[3]*m_V3dRotateInfo.dModelViewCurrentInvY[14];
	dMatrixTemp[3] = m_V3dRotateInfo.dRotateMatrixY[0]*m_V3dRotateInfo.dModelViewCurrentInvY[3] + m_V3dRotateInfo.dRotateMatrixY[1]*m_V3dRotateInfo.dModelViewCurrentInvY[7] + m_V3dRotateInfo.dRotateMatrixY[2]*m_V3dRotateInfo.dModelViewCurrentInvY[11] + m_V3dRotateInfo.dRotateMatrixY[3]*m_V3dRotateInfo.dModelViewCurrentInvY[15];
	dMatrixTemp[4] = m_V3dRotateInfo.dRotateMatrixY[4]*m_V3dRotateInfo.dModelViewCurrentInvY[0] + m_V3dRotateInfo.dRotateMatrixY[5]*m_V3dRotateInfo.dModelViewCurrentInvY[4] + m_V3dRotateInfo.dRotateMatrixY[6]*m_V3dRotateInfo.dModelViewCurrentInvY[8] + m_V3dRotateInfo.dRotateMatrixY[7]*m_V3dRotateInfo.dModelViewCurrentInvY[12];
	dMatrixTemp[5] = m_V3dRotateInfo.dRotateMatrixY[4]*m_V3dRotateInfo.dModelViewCurrentInvY[1] + m_V3dRotateInfo.dRotateMatrixY[5]*m_V3dRotateInfo.dModelViewCurrentInvY[5] + m_V3dRotateInfo.dRotateMatrixY[6]*m_V3dRotateInfo.dModelViewCurrentInvY[9] + m_V3dRotateInfo.dRotateMatrixY[7]*m_V3dRotateInfo.dModelViewCurrentInvY[13];
	dMatrixTemp[6] = m_V3dRotateInfo.dRotateMatrixY[4]*m_V3dRotateInfo.dModelViewCurrentInvY[2] + m_V3dRotateInfo.dRotateMatrixY[5]*m_V3dRotateInfo.dModelViewCurrentInvY[6] + m_V3dRotateInfo.dRotateMatrixY[6]*m_V3dRotateInfo.dModelViewCurrentInvY[10] + m_V3dRotateInfo.dRotateMatrixY[7]*m_V3dRotateInfo.dModelViewCurrentInvY[14];
	dMatrixTemp[7] = m_V3dRotateInfo.dRotateMatrixY[4]*m_V3dRotateInfo.dModelViewCurrentInvY[3] + m_V3dRotateInfo.dRotateMatrixY[5]*m_V3dRotateInfo.dModelViewCurrentInvY[7] + m_V3dRotateInfo.dRotateMatrixY[6]*m_V3dRotateInfo.dModelViewCurrentInvY[11] + m_V3dRotateInfo.dRotateMatrixY[7]*m_V3dRotateInfo.dModelViewCurrentInvY[15];
	dMatrixTemp[8] = m_V3dRotateInfo.dRotateMatrixY[8]*m_V3dRotateInfo.dModelViewCurrentInvY[0] + m_V3dRotateInfo.dRotateMatrixY[9]*m_V3dRotateInfo.dModelViewCurrentInvY[4] + m_V3dRotateInfo.dRotateMatrixY[10]*m_V3dRotateInfo.dModelViewCurrentInvY[8] + m_V3dRotateInfo.dRotateMatrixY[11]*m_V3dRotateInfo.dModelViewCurrentInvY[12];
	dMatrixTemp[9] = m_V3dRotateInfo.dRotateMatrixY[8]*m_V3dRotateInfo.dModelViewCurrentInvY[1] + m_V3dRotateInfo.dRotateMatrixY[9]*m_V3dRotateInfo.dModelViewCurrentInvY[5] + m_V3dRotateInfo.dRotateMatrixY[10]*m_V3dRotateInfo.dModelViewCurrentInvY[9] + m_V3dRotateInfo.dRotateMatrixY[11]*m_V3dRotateInfo.dModelViewCurrentInvY[13];
	dMatrixTemp[10] = m_V3dRotateInfo.dRotateMatrixY[8]*m_V3dRotateInfo.dModelViewCurrentInvY[2] + m_V3dRotateInfo.dRotateMatrixY[9]*m_V3dRotateInfo.dModelViewCurrentInvY[6] + m_V3dRotateInfo.dRotateMatrixY[10]*m_V3dRotateInfo.dModelViewCurrentInvY[10] + m_V3dRotateInfo.dRotateMatrixY[11]*m_V3dRotateInfo.dModelViewCurrentInvY[14];
	dMatrixTemp[11] = m_V3dRotateInfo.dRotateMatrixY[8]*m_V3dRotateInfo.dModelViewCurrentInvY[3] + m_V3dRotateInfo.dRotateMatrixY[9]*m_V3dRotateInfo.dModelViewCurrentInvY[7] + m_V3dRotateInfo.dRotateMatrixY[10]*m_V3dRotateInfo.dModelViewCurrentInvY[11] + m_V3dRotateInfo.dRotateMatrixY[11]*m_V3dRotateInfo.dModelViewCurrentInvY[15];
	dMatrixTemp[12] = m_V3dRotateInfo.dRotateMatrixY[12]*m_V3dRotateInfo.dModelViewCurrentInvY[0] + m_V3dRotateInfo.dRotateMatrixY[13]*m_V3dRotateInfo.dModelViewCurrentInvY[4] + m_V3dRotateInfo.dRotateMatrixY[14]*m_V3dRotateInfo.dModelViewCurrentInvY[8] + m_V3dRotateInfo.dRotateMatrixY[15]*m_V3dRotateInfo.dModelViewCurrentInvY[12];
	dMatrixTemp[13] = m_V3dRotateInfo.dRotateMatrixY[12]*m_V3dRotateInfo.dModelViewCurrentInvY[1] + m_V3dRotateInfo.dRotateMatrixY[13]*m_V3dRotateInfo.dModelViewCurrentInvY[5] + m_V3dRotateInfo.dRotateMatrixY[14]*m_V3dRotateInfo.dModelViewCurrentInvY[9] + m_V3dRotateInfo.dRotateMatrixY[15]*m_V3dRotateInfo.dModelViewCurrentInvY[13];
	dMatrixTemp[14] = m_V3dRotateInfo.dRotateMatrixY[12]*m_V3dRotateInfo.dModelViewCurrentInvY[2] + m_V3dRotateInfo.dRotateMatrixY[13]*m_V3dRotateInfo.dModelViewCurrentInvY[6] + m_V3dRotateInfo.dRotateMatrixY[14]*m_V3dRotateInfo.dModelViewCurrentInvY[10] + m_V3dRotateInfo.dRotateMatrixY[15]*m_V3dRotateInfo.dModelViewCurrentInvY[14];
	dMatrixTemp[15] = m_V3dRotateInfo.dRotateMatrixY[12]*m_V3dRotateInfo.dModelViewCurrentInvY[3] + m_V3dRotateInfo.dRotateMatrixY[13]*m_V3dRotateInfo.dModelViewCurrentInvY[7] + m_V3dRotateInfo.dRotateMatrixY[14]*m_V3dRotateInfo.dModelViewCurrentInvY[11] + m_V3dRotateInfo.dRotateMatrixY[15]*m_V3dRotateInfo.dModelViewCurrentInvY[15];

	for (int i = 0; i < 16; i++)
	{
		m_V3dRotateInfo.dRotateMatrixY[i] = dMatrixTemp[i];
	}

	//注意glMultMatrixd做矩阵相乘好像没有立即计算结果并赋值给当前矩阵，导致glGetDoublev返回的仍然是矩阵乘法前的结果


	//更新水平轴

	m_V3dRotateInfo.dCurrentHoriX = dHoriX * dMatrixTemp[0] + dHoriY * dMatrixTemp[4]
	+ dHoriZ * dMatrixTemp[8];
	m_V3dRotateInfo.dCurrentHoriY = dHoriX * dMatrixTemp[1] + dHoriY * dMatrixTemp[5]
	+ dHoriZ * dMatrixTemp[9];
	m_V3dRotateInfo.dCurrentHoriZ = dHoriX * dMatrixTemp[2] + dHoriY * dMatrixTemp[6]
	+ dHoriZ * dMatrixTemp[10];

	//注意：世界坐标下当前横轴和纵轴在每次运算后要归一化，减小累积误差

	double dModule = sqrt(m_V3dRotateInfo.dCurrentHoriX*m_V3dRotateInfo.dCurrentHoriX + m_V3dRotateInfo.dCurrentHoriY*m_V3dRotateInfo.dCurrentHoriY + m_V3dRotateInfo.dCurrentHoriZ*m_V3dRotateInfo.dCurrentHoriZ);
	m_V3dRotateInfo.dCurrentHoriX = m_V3dRotateInfo.dCurrentHoriX/dModule;
	m_V3dRotateInfo.dCurrentHoriY = m_V3dRotateInfo.dCurrentHoriY/dModule;
	m_V3dRotateInfo.dCurrentHoriZ = m_V3dRotateInfo.dCurrentHoriZ/dModule;
}

MprRotateInfo SeVisualRotate::ChangeRotateAngle_Current( bool bCube, double dAngleX, double dAngleY, MprRotateInfo CurrentInfo )
{
	m_MprRotateInfo = CurrentInfo;
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(m_MprRotateInfo.dModelView);
	if (!bCube)
	{
		glRotated(dAngleX, 1.0, 0.0, 0.0);
		UpdataTransX(false, false, 1, dAngleX);
		UpdataTransY(false, false, 1, dAngleX);
		UpdataTransZ(false, false, 1, dAngleX);

		glRotated(dAngleY, 0.0, 1.0, 0.0);
		UpdataTransX(false, false, 2, dAngleY);
		UpdataTransY(false, false, 2, dAngleY);
		UpdataTransZ(false, false, 2, dAngleY);
	}
	else
	{
		glRotated(dAngleX, m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z);
		UpdataTransY(true, true, 1, dAngleX);
		UpdataTransZ(true, true, 1, dAngleX);

		glRotated(-dAngleY, m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z);
		UpdataTransX(true, true, 2, -dAngleY);
		UpdataTransZ(true, true, 2, -dAngleY);
	}
	glGetDoublev(GL_MODELVIEW_MATRIX, m_MprRotateInfo.dModelView);
	return m_MprRotateInfo;
}

MprRotateInfo SeVisualRotate::ChangeRotateAngle_CurrentZ( bool bCube, double dAngleZ, MprRotateInfo CurrentInfo )
{
	m_MprRotateInfo = CurrentInfo;
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(m_MprRotateInfo.dModelView);
	if (!bCube)
	{
		glRotated(dAngleZ, 0.0, 0.0, 1.0);
		UpdataTransX(false, false, 3, dAngleZ);
		UpdataTransY(false, false, 3, dAngleZ);
		UpdataTransZ(false, false, 3, dAngleZ);
	}
	else
	{
		glRotated(dAngleZ, m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z);
		UpdataTransX(true, true, 3, dAngleZ);
		UpdataTransY(true, true, 3, dAngleZ);
	}

	glGetDoublev(GL_MODELVIEW_MATRIX, m_MprRotateInfo.dModelView);
	return m_MprRotateInfo;
}

MprRotateInfo SeVisualRotate::ChangeRotateAngle_World( bool bCube, int nAxisNum, double dAngle,MprRotateInfo CurrentInfo )
{
	m_MprRotateInfo = CurrentInfo;
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(m_MprRotateInfo.dModelView);
	if (!bCube)
	{
		switch (nAxisNum)
		{
		case 1:
			glRotated(dAngle, m_MprRotateInfo.dWorldX_x, m_MprRotateInfo.dWorldX_y, m_MprRotateInfo.dWorldX_z);
			UpdataTransY(true, false, 1, dAngle);
			UpdataTransZ(true, false, 1, dAngle);
			break;
		case 2:
			glRotated(-dAngle, m_MprRotateInfo.dWorldY_x, m_MprRotateInfo.dWorldY_y, m_MprRotateInfo.dWorldY_z);
			UpdataTransX(true, false, 2, -dAngle);
			UpdataTransZ(true, false, 2, -dAngle);
			break;
		case 3:
			glRotated(dAngle, m_MprRotateInfo.dWorldZ_x, m_MprRotateInfo.dWorldZ_y, m_MprRotateInfo.dWorldZ_z);
			UpdataTransX(true, false, 3, dAngle);
			UpdataTransY(true, false, 3, dAngle);
			break;
		default:
			break;
		}
	}
	else
	{
		switch (nAxisNum)
		{
		case 1:
			glRotated(dAngle, 1.0, 0.0, 0.0);
			UpdataTransX(false, true, 1, dAngle);
			UpdataTransY(false, true, 1, dAngle);
			UpdataTransZ(false, true, 1, dAngle);
			break;
		case 2:
			glRotated(dAngle, 0.0, 1.0, 0.0);
			UpdataTransX(false, true, 2, dAngle);
			UpdataTransY(false, true, 2, dAngle);
			UpdataTransZ(false, true, 2, dAngle);
			break;
		case 3:
			glRotated(dAngle, 0.0, 0.0, 1.0);
			UpdataTransX(false, true, 3, dAngle);
			UpdataTransY(false, true, 3, dAngle);
			UpdataTransZ(false, true, 3, dAngle);
			break;
		default:
			break;
		}

	}
	glGetDoublev(GL_MODELVIEW_MATRIX, m_MprRotateInfo.dModelView);
	return m_MprRotateInfo;

}

V3dRotateInfo SeVisualRotate::ChangeRotateAngle_V3D( double dAngleX, double dAngleY,V3dRotateInfo CurrentInfo )
{
	m_V3dRotateInfo = CurrentInfo;
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(m_V3dRotateInfo.dModelView);
	glRotated(dAngleX, m_V3dRotateInfo.dCurrentHoriX, m_V3dRotateInfo.dCurrentHoriY, m_V3dRotateInfo.dCurrentHoriZ);
	UpdataV3DWorldTransY(dAngleX);

	glRotated(dAngleY, m_V3dRotateInfo.dCurrentVertiX, m_V3dRotateInfo.dCurrentVertiY, m_V3dRotateInfo.dCurrentVertiZ);
	UpdataV3DWorldTransX(dAngleY);
	glGetDoublev(GL_MODELVIEW_MATRIX, m_V3dRotateInfo.dModelView);
	return m_V3dRotateInfo;
}

//void			GetCurrent(double &dMatrixTemp[6])