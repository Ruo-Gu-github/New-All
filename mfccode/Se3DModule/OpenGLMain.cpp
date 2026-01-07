#include "StdAfx.h"
#include "OpenGLMain.h"
#include "OpenGLShader.h"
#include "OpenGLCamera.h"
#include "OpenGLDataMgr.h"

GLfloat dOrthoSize = 1.0f;


COpenGLMain::COpenGLMain(void)
{
	m_bRayCasting = TRUE;
	m_bShowBorder = FALSE;
	m_fXS = 0.0f;
	m_fXE = 0.0f;
	m_fYS = 0.0f;
	m_fYE = 0.0f;
	m_bLine = TRUE;
}


COpenGLMain::~COpenGLMain(void)
{
	wglDeleteContext(m_hRC); 
}

GLboolean COpenGLMain::Initialize(HDC hContext, COpenGLDataMgr* pGLDataMgr, COpenGLCamera* pGLCamera, COpenGLShader* pGLShader)
{
	//Setting up the dialog to support the OpenGL.
	PIXELFORMATDESCRIPTOR stPixelFormatDescriptor;

	memset( &stPixelFormatDescriptor, 0, sizeof( PIXELFORMATDESCRIPTOR ));
	stPixelFormatDescriptor.nSize = sizeof( PIXELFORMATDESCRIPTOR );
	stPixelFormatDescriptor.nVersion = 1;
	stPixelFormatDescriptor.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_STEREO_DONTCARE; 
	stPixelFormatDescriptor.iPixelType = PFD_TYPE_RGBA;
	stPixelFormatDescriptor.cColorBits = 24;
	stPixelFormatDescriptor.cDepthBits = 32;
	stPixelFormatDescriptor.cStencilBits = 8;
	stPixelFormatDescriptor.iLayerType = PFD_MAIN_PLANE ;
	int nPixelFormat = ChoosePixelFormat( hContext, &stPixelFormatDescriptor ); //Collect the pixel format.

	if( nPixelFormat == 0 )
	{
		AfxMessageBox( _T( "Error while Choosing Pixel format" ));
		return false;
	}
	//Set the pixel format to the current dialog.
	if( !SetPixelFormat( hContext, nPixelFormat, &stPixelFormatDescriptor ))
	{
		AfxMessageBox( _T( "Error while setting pixel format" ));
		return false;
	}
	//Create a device context.
	m_hRC = wglCreateContext( hContext );
	if( !hContext )
	{
		AfxMessageBox( _T( "Rendering Context Creation Failed" ));
		return false;
	}
	//Make the created device context as the current device context.
	BOOL bResult = wglMakeCurrent( hContext, m_hRC );
	if( !bResult )
	{
		AfxMessageBox( _T( "wglMakeCurrent Failed" ));
		return false;
	}

	glewInit();
	if(GL_TRUE != glewGetExtension("GL_EXT_texture3D"))
	{
		AfxMessageBox( _T( "3D texture is not supported !" ));
		return false;
	}
	glEnable(GL_MULTISAMPLE);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);

	m_pGLDataMgr = pGLDataMgr;
	m_pGLCamera = pGLCamera;
	m_pGLShader = pGLShader;

	m_pGLShader->InitShader();
	m_pGLDataMgr->InitVAO();
	m_pGLDataMgr->InitFreeCutVAO();
	m_pGLDataMgr->InitTransFunc();
	m_pGLDataMgr->InitFont();
	m_pGLDataMgr->InitRulerTex();

	return true;
}

void COpenGLMain::Resize(GLuint nWidth, GLuint nHeight)
{
 
	glViewport( 0, 0, nWidth, nHeight);

	if(nWidth !=0 && nHeight != 0)
	{
		m_pGLDataMgr->InitFace2DTex(nWidth, nHeight);
		//m_pGLDataMgr->InitFace2DTexForFreeCut(nWidth, nHeight);
		m_pGLDataMgr->InitFrameBuffer(nWidth, nHeight);
		//m_pGLDataMgr->InitFrameBufferForFreeCut(nWidth, nHeight);
	}
}

void COpenGLMain::Render(GLuint nWinWidth, GLuint nWinHeight)
{

	try
	{
		if (m_bRayCasting)
		{
			RenderRayCasting(nWinWidth, nWinHeight);
		}
		else
		{
			RenderVolumeRender(nWinWidth, nWinHeight);
		}
		if (m_bShowBorder)
			RenderBorder(nWinWidth, nWinHeight);
		RenderRuler(nWinWidth, nWinHeight);

	}
	catch (CException* e)
	{
		glFlush();
	}
}

void COpenGLMain::SetRulerRenderInfo(std::string text, GLfloat fScale)
{
	m_text = text;
	m_fScale = fScale;
}

void COpenGLMain::SetLineRenderInfo(std::string text, GLfloat fXS, GLfloat fXE, GLfloat fYS, GLfloat fYE, GLboolean bMoving)
{
	if (bMoving) {
		m_linetext = text;
		m_fXS = fXS;
		m_fXE = fXE;
		m_fYS = fYS;
		m_fYE = fYE;
	} 
	else
	{
		m_vecTexts.push_back(text);
		m_vecPoints.push_back(fXS);
		m_vecPoints.push_back(fXE);
		m_vecPoints.push_back(fYS);
		m_vecPoints.push_back(fYE);
	}


}

void COpenGLMain::RenderObject(GLenum cullFace)
{
	glClearColor(m_pGLCamera->GetBackgroundColor().r, m_pGLCamera->GetBackgroundColor().g, m_pGLCamera->GetBackgroundColor().b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	DrawBox(cullFace);
}

void COpenGLMain::RenderRuler(GLuint nWinWidth, GLuint nWinHeight)
{
	//  if you just debug this only sense , uncomment the follow two lines
// 	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
// 	glClear(GL_COLOR_BUFFER_BIT);
// 	glDisable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport(0, 0, nWinWidth, nWinHeight);
	glUseProgram(m_pGLShader->GetTextProgramHandle());
	glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(nWinWidth), 0.0f, static_cast<GLfloat>(nWinHeight));
	glUniformMatrix4fv(glGetUniformLocation(m_pGLShader->GetTextProgramHandle(), "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, m_pGLDataMgr->GetFtTex());

	RenderScale(static_cast<GLfloat>(nWinWidth - 14), 87.0f, m_fScale, glm::vec3(1.0f, 1.0f, 1.0f));
	RenderText(m_text, static_cast<GLfloat>(nWinWidth - 14), 67.0f, 0.28f, glm::vec3(1.0f, 1.0f, 1.0f));
	if (m_bLine) {
		RenderText(m_linetext, m_fXS - 30.0f, (nWinHeight - m_fYS) - 30.0f, 0.28f, glm::vec3(1.0f, 1.0f, 1.0f));
		for(int i=0; i<m_vecTexts.size(); i++) {
			RenderText(m_vecTexts[i], m_vecPoints[i*4] - 30.0f, (nWinHeight - m_vecPoints[i*4 + 2]) - 30.0f, 0.28f, glm::vec3(1.0f, 1.0f, 1.0f));

		}
	}
	glUseProgram(0);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);



	if (m_bLine)
	{
		GLboolean bRotate = m_pGLCamera->IsRotate();
		if (bRotate) {
			m_vecPoints.clear();
			m_vecTexts.clear();
			m_pGLCamera->ResetRotate();
		}

		for(int i=0; i<m_vecTexts.size(); i++) {
			RenderText(m_vecTexts[i], m_fXS - 30.0f, (nWinHeight - m_fYS) - 30.0f, 0.28f, glm::vec3(1.0f, 1.0f, 1.0f));
			glBegin(GL_LINES);
			GLfloat xs = (m_vecPoints[i*4] - nWinWidth/2.0f)/(nWinWidth/2.0f);
			GLfloat xe = (m_vecPoints[i*4 + 1] - nWinWidth/2.0f)/(nWinWidth/2.0f);
			GLfloat ys = (nWinHeight/2.0f - m_vecPoints[i*4 + 2])/(nWinHeight/2.0f);
			GLfloat ye = (nWinHeight/2.0f - m_vecPoints[i*4 + 3])/(nWinHeight/2.0f);
			glVertex2f(xs, ys);
			glVertex2f(xe, ye);
			glEnd();
		}
		
		glBegin(GL_LINES);
		GLfloat xs = (m_fXS - nWinWidth/2.0f)/(nWinWidth/2.0f);
		GLfloat xe = (m_fXE - nWinWidth/2.0f)/(nWinWidth/2.0f);
		GLfloat ys = (nWinHeight/2.0f - m_fYS)/(nWinHeight/2.0f);
		GLfloat ye = (nWinHeight/2.0f - m_fYE)/(nWinHeight/2.0f);
		glVertex2f(xs, ys);
		glVertex2f(xe, ye);
		glEnd();

	}
	
	
}

void COpenGLMain::RenderScale(GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
	// Activate corresponding render state	
	glUniform3f(glGetUniformLocation(m_pGLShader->GetTextProgramHandle(), "textColor"), color.x, color.y, color.z);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(m_pGLDataMgr->GetFtVAO());
	

	GLfloat xpos = x;
	GLfloat ypos = y;

	GLfloat w = 80.0f * scale;
	GLfloat h = 5;
	// Update VBO for each character
	GLfloat vertices[6][4] = {
		{ xpos - w,     ypos + h,   0.0, 0.0 },            
		{ xpos - w,     ypos,       0.0, 1.0 },
		{ xpos,			ypos,       1.0, 1.0 },

		{ xpos - w,     ypos + h,   0.0, 0.0 },
		{ xpos,			ypos,       1.0, 1.0 },
		{ xpos,			ypos + h,   1.0, 0.0 }           
	};
	// Render glyph texture over quad
	glBindTexture(GL_TEXTURE_2D, m_pGLDataMgr->GetFtTex());
	// Update content of VBO memory
	glBindBuffer(GL_ARRAY_BUFFER, m_pGLDataMgr->GetFtVBO());
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	// Render quad
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void COpenGLMain::SetUniforms(GLuint ProgramHandle, GLuint nWinWidth, GLuint nWinHeight, GLboolean bFront)
{
	GLfloat fWidth = (GLfloat)m_pGLDataMgr->GetWidth();
	GLfloat fHeight = (GLfloat)m_pGLDataMgr->GetHeight();
	GLfloat fLength = (GLfloat)m_pGLDataMgr->GetLength();
	GLfloat fCaclLength = m_pGLDataMgr->GetPixelScale() * fLength;

	glm::vec3 myScale;
	//if(fWidth >= fHeight && fWidth >= fCaclLength) 
	//{
	//	myScale = glm::vec3(1.0f, fHeight/fWidth, fCaclLength/fWidth);
	//}
	//else if (fHeight >= fWidth && fHeight >= fCaclLength)
	//{
	//	//myScale = glm::vec3(1.0f, fHeight/fWidth, fCaclLength/fWidth);
	//	myScale = glm::vec3(fWidth/fHeight, 1.0f, fCaclLength/fHeight);
	//}
	//else
	//{
	//	myScale = glm::vec3(fWidth/fCaclLength, fHeight/fCaclLength, 1.0f);
	//}
	if(fWidth > fHeight) {
		myScale = glm::vec3(1.0f, fHeight/fWidth, fCaclLength/fWidth);
	} else {
		myScale = glm::vec3(fWidth/fHeight, 1.0f, fCaclLength/fHeight);
	}

	if (bFront)
	{
		GLint nLoc;
		if (GetUniformLocation(ProgramHandle, "ScreenSize", nLoc))
			glUniform2f(nLoc, (GLfloat)nWinWidth, (GLfloat)nWinHeight);
		if (GetUniformLocation(ProgramHandle, "VolumeSize", nLoc))
			glUniform3f(nLoc, fWidth, fHeight, fLength);
		//if (GetUinformLocation(ProgramHandle, "ObjectColor", nLoc))
			//glUniform1f(nLoc, m_pGLDataMgr);
		if (GetUniformLocation(ProgramHandle, "LightColor", nLoc))
			glUniform3f(nLoc, m_pGLCamera->GetLightColor().r, m_pGLCamera->GetLightColor().g, m_pGLCamera->GetLightColor().b);
		if (GetUniformLocation(ProgramHandle, "BackgroundColor", nLoc))
			glUniform3f(nLoc, m_pGLCamera->GetBackgroundColor().r, m_pGLCamera->GetBackgroundColor().g, m_pGLCamera->GetBackgroundColor().b);
 		if (GetUniformLocation(ProgramHandle, "LightPos", nLoc))
			glUniform3f(nLoc, m_pGLCamera->GetLightPos().x, m_pGLCamera->GetLightPos().y, m_pGLCamera->GetLightPos().z);
		if (GetUniformLocation(ProgramHandle, "ViewPos", nLoc))
			glUniform3f(nLoc, m_pGLCamera->GetCameraPos().x, m_pGLCamera->GetCameraPos().y, m_pGLCamera->GetCameraPos().z);

		if (GetUniformLocation(ProgramHandle, "ExitPoints", nLoc))
		{
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, m_pGLDataMgr->GetBfTex());
			glUniform1i(nLoc, 1);
		}
// 		if (GetUniformLocation(ProgramHandle, "VolumeTex", nLoc))
// 		{
// 			glActiveTexture(GL_TEXTURE2);
// 			glBindTexture(GL_TEXTURE_3D, m_pGLDataMgr->GetVolTex());
// 			glUniform1i(nLoc, 2);
// 		}

		vector <VolTexObjInfo> infoList = m_pGLDataMgr->GetVolTexList();
		if (GetUniformLocation(ProgramHandle, "VolumeTex0", nLoc))
		{
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_3D, infoList.size() > 0 && infoList[0].show == TRUE ? infoList[0].id : 0);
			glUniform1i(nLoc, 2);
		}
		if (GetUniformLocation(ProgramHandle, "VolumeTex1", nLoc))
		{
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_3D, infoList.size() > 1 && infoList[1].show == TRUE ? infoList[1].id : 0);
			glUniform1i(nLoc, 3);
		}
		if (GetUniformLocation(ProgramHandle, "VolumeTex2", nLoc))
		{
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_3D, infoList.size() > 2 && infoList[2].show == TRUE ? infoList[2].id : 0);
			glUniform1i(nLoc, 4);
		}
		if (GetUniformLocation(ProgramHandle, "VolumeTex3", nLoc))
		{
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_3D, infoList.size() > 3 && infoList[3].show == TRUE ? infoList[3].id : 0);
			glUniform1i(nLoc, 5);
		}
		if (GetUniformLocation(ProgramHandle, "VolumeTex4", nLoc))
		{
			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_3D, infoList.size() > 4 && infoList[4].show == TRUE ? infoList[4].id : 0);
			glUniform1i(nLoc, 6);
		}
		if (GetUniformLocation(ProgramHandle, "VolumeTex5", nLoc))
		{
			glActiveTexture(GL_TEXTURE7);
			glBindTexture(GL_TEXTURE_3D, infoList.size() > 5 && infoList[5].show == TRUE ? infoList[5].id : 0);
			glUniform1i(nLoc, 7);
		}
		if (GetUniformLocation(ProgramHandle, "VolumeTex6", nLoc))
		{
			glActiveTexture(GL_TEXTURE8);
			glBindTexture(GL_TEXTURE_3D, infoList.size() > 6 && infoList[6].show == TRUE ? infoList[6].id : 0);
			glUniform1i(nLoc, 8);
		}
		if (GetUniformLocation(ProgramHandle, "VolumeTex7", nLoc))
		{
			glActiveTexture(GL_TEXTURE9);
			glBindTexture(GL_TEXTURE_3D, infoList.size() > 7 && infoList[7].show == TRUE ? infoList[7].id : 0);
			glUniform1i(nLoc, 9);
		}

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (GetUniformLocation(ProgramHandle, "ObjectColor0", nLoc))
			glUniform3f(nLoc, (infoList.size() > 0 && infoList[0].show == TRUE) ? infoList[0].color.x : 0.0f, 
								(infoList.size() > 0 && infoList[0].show == TRUE) ? infoList[0].color.y : 0.0f, 
								(infoList.size() > 0 && infoList[0].show == TRUE) ? infoList[0].color.z : 0.0f);
		if (GetUniformLocation(ProgramHandle, "ObjectColor1", nLoc))
			glUniform3f(nLoc, (infoList.size() > 1 && infoList[1].show == TRUE) ? infoList[1].color.x : 0.0f, 
								(infoList.size() > 1 && infoList[1].show == TRUE) ? infoList[1].color.y : 0.0f, 
								(infoList.size() > 1 && infoList[1].show == TRUE) ? infoList[1].color.z : 0.0f);
		if (GetUniformLocation(ProgramHandle, "ObjectColor2", nLoc))
			glUniform3f(nLoc, (infoList.size() > 2 && infoList[2].show == TRUE) ? infoList[2].color.x : 0.0f, 
								(infoList.size() > 2 && infoList[2].show == TRUE) ? infoList[2].color.y : 0.0f, 
								(infoList.size() > 2 && infoList[2].show == TRUE) ? infoList[2].color.z : 0.0f);
		if (GetUniformLocation(ProgramHandle, "ObjectColor3", nLoc))
			glUniform3f(nLoc, (infoList.size() > 3 && infoList[3].show == TRUE) ? infoList[3].color.x : 0.0f, 
								(infoList.size() > 3 && infoList[3].show == TRUE) ? infoList[3].color.y : 0.0f, 
								(infoList.size() > 3 && infoList[3].show == TRUE) ? infoList[3].color.z : 0.0f);
		if (GetUniformLocation(ProgramHandle, "ObjectColor4", nLoc))
			glUniform3f(nLoc, (infoList.size() > 4 && infoList[4].show == TRUE) ? infoList[4].color.x : 0.0f, 
								(infoList.size() > 4 && infoList[4].show == TRUE) ? infoList[4].color.y : 0.0f, 
								(infoList.size() > 4 && infoList[4].show == TRUE) ? infoList[4].color.z : 0.0f);
		if (GetUniformLocation(ProgramHandle, "ObjectColor5", nLoc))
			glUniform3f(nLoc, (infoList.size() > 5 && infoList[5].show == TRUE) ? infoList[5].color.x : 0.0f, 
								(infoList.size() > 5 && infoList[5].show == TRUE) ? infoList[5].color.y : 0.0f, 
								(infoList.size() > 5 && infoList[5].show == TRUE) ? infoList[5].color.z : 0.0f);
		if (GetUniformLocation(ProgramHandle, "ObjectColor6", nLoc))
			glUniform3f(nLoc, (infoList.size() > 6 && infoList[6].show == TRUE) ? infoList[6].color.x : 0.0f, 
								(infoList.size() > 6 && infoList[6].show == TRUE) ? infoList[6].color.y : 0.0f, 
								(infoList.size() > 6 && infoList[6].show == TRUE) ? infoList[6].color.z : 0.0f);
		if (GetUniformLocation(ProgramHandle, "ObjectColor7", nLoc))
			glUniform3f(nLoc, (infoList.size() > 7 && infoList[7].show == TRUE) ? infoList[7].color.x : 0.0f, 
								(infoList.size() > 7 && infoList[7].show == TRUE) ? infoList[7].color.y : 0.0f, 
								(infoList.size() > 7 && infoList[7].show == TRUE) ? infoList[7].color.z : 0.0f);

		if (GetUniformLocation(ProgramHandle, "Translate0", nLoc))
			glUniform1f(nLoc, infoList.size() > 0 ? infoList[0].translate : 0.0f);
		if (GetUniformLocation(ProgramHandle, "Translate1", nLoc))
			glUniform1f(nLoc, infoList.size() > 1 ? infoList[1].translate : 0.0f);
		if (GetUniformLocation(ProgramHandle, "Translate2", nLoc))
			glUniform1f(nLoc, infoList.size() > 2 ? infoList[2].translate : 0.0f);
		if (GetUniformLocation(ProgramHandle, "Translate3", nLoc))
			glUniform1f(nLoc, infoList.size() > 3 ? infoList[3].translate : 0.0f);
		if (GetUniformLocation(ProgramHandle, "Translate4", nLoc))
			glUniform1f(nLoc, infoList.size() > 4 ? infoList[4].translate : 0.0f);
		if (GetUniformLocation(ProgramHandle, "Translate5", nLoc))
			glUniform1f(nLoc, infoList.size() > 5 ? infoList[5].translate : 0.0f);
		if (GetUniformLocation(ProgramHandle, "Translate6", nLoc))
			glUniform1f(nLoc, infoList.size() > 6 ? infoList[6].translate : 0.0f);
		if (GetUniformLocation(ProgramHandle, "Translate7", nLoc))
			glUniform1f(nLoc, infoList.size() > 7 ? infoList[7].translate : 0.0f);
	}

	GLint nLoc;

	glm::mat4 model, projection, view;
	model = glm::mat4(1.0f);//m_pGLMatrix->GetMatrix();
	glm::vec4 move = m_pGLCamera->GetMove();
	model = glm::translate(model, glm::vec3(-0.5f + move.x, -0.5f + move.y, -0.5f + move.z));

	model = glm::scale(model, myScale);
	//model = glm::scale(model, glm::vec3(1.0f, fHeight/fWidth, fLength * m_pGLDataMgr->GetPixelScale() / fWidth));
	// model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
	view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	//view *= m_pGLCamera->GetMoveMatrix();
	view *= m_pGLCamera->GetMatrix();
	//projection = glm::perspective(glm::radians(m_pGLCamera->GetViewAngle()), (GLfloat)nWinWidth/(GLfloat)nWinHeight, 0.1f, 100.0f);
	//projection = glm::ortho(0,600,0,800, 0.1f, 100.0f);
	GLfloat fScale = m_pGLCamera->GetScale();
	GLfloat fRulerScale = fWidth/nWinHeight * 2;

	projection = glm::orthoLH(	-1.0f * fScale, 
								1.0f * fScale, 
								-1.0f * nWinHeight/nWinWidth * fScale, 
								1.0f * nWinHeight/nWinWidth * fScale,
								-100.0f, -0.01f);
	if (GetUniformLocation(ProgramHandle, "model", nLoc))
		glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(model));
	if (GetUniformLocation(ProgramHandle, "view", nLoc))
		glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(view));
	if (GetUniformLocation(ProgramHandle, "projection", nLoc))
		glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(projection));

}

void COpenGLMain::RenderLine(GLuint nWinWidth, GLuint nWinHeight, GLfloat xs, GLfloat xe, GLfloat ys, GLfloat ye, glm::vec3 color)
{
	//glUniform3f(glGetUniformLocation(m_pGLShader->GetTextProgramHandle(), "textColor"), color.x, color.y, color.z);
	//glActiveTexture(GL_TEXTURE0);
	//glBindVertexArray(m_pGLDataMgr->GetFtVAO());

	//// Update VBO for each character
	//GLfloat vertices[2][3] = {
	//	{ xs/nWinWidth, ys/nWinHeight, 0.0},            
	//	{ xe/nWinWidth, ye/nWinHeight, 0.0}      
	//};
	//// Render glyph texture over quad
	//// Update content of VBO memory
	//glBindBuffer(GL_ARRAY_BUFFER, m_pGLDataMgr->GetFtVBO());
	//glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

	//glBindBuffer(GL_ARRAY_BUFFER, 0);
	//// Render quad
	//glDrawArrays(GL_LINES, 0, 2);
	//glBindVertexArray(0);
	glBegin(GL_LINES);
	glVertex2f(xs/nWinWidth, ys/nWinHeight);
	glVertex2f(xe/nWinWidth, ye/nWinHeight);
	glEnd();
}

void COpenGLMain::SetUniformForFreecut(GLuint ProgramHandle, GLuint nWinWidth, GLuint nWinHeight)
{
	GLfloat fWidth = (GLfloat)m_pGLDataMgr->GetWidth();
	GLfloat fHeight = (GLfloat)m_pGLDataMgr->GetHeight();
	GLfloat fLength = (GLfloat)m_pGLDataMgr->GetLength();
	GLfloat fCaclLength = m_pGLDataMgr->GetPixelScale() * fLength;

	glm::vec3 myScale;
	//if(fWidth >= fHeight && fWidth >= fCaclLength) 
	//{
	//	myScale = glm::vec3(1.0f, fHeight/fWidth, fCaclLength/fWidth);
	//}
	//else if (fHeight >= fWidth && fHeight >= fCaclLength)
	//{
	//	//myScale = glm::vec3(1.0f, fHeight/fWidth, fCaclLength/fWidth);
	//	myScale = glm::vec3(fWidth/fHeight, 1.0f, fCaclLength/fHeight);
	//}
	//else
	//{
	//	myScale = glm::vec3(fWidth/fCaclLength, fHeight/fCaclLength, 1.0f);
	//}

	if(fWidth > fHeight) {
		myScale = glm::vec3(1.0f, fHeight/fWidth, fCaclLength/fWidth);
	} else {
		myScale = glm::vec3(fWidth/fHeight, 1.0f, fCaclLength/fHeight);
	}

	GLint nLoc;

	glm::mat4 model, projection, view;
	model = glm::mat4(1.0f);//m_pGLMatrix->GetMatrix();
	glm::vec4 move = m_pGLCamera->GetFreeCutMove();
	model = glm::translate(model, glm::vec3(-0.5f + move.x, -0.5f + move.y, -0.5f + move.z));

	model = glm::scale(model, myScale);
	// model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
	view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	
	view *= m_pGLCamera->GetFreeCutMatrix();
	GLfloat fScale = m_pGLCamera->GetScale();
	GLfloat fRulerScale = fWidth/nWinHeight * 2;


	glm::vec4 objpos = m_pGLCamera->GetMove();
	glm::mat4 objrot = m_pGLCamera->GetMatrix();
	glm::vec4 position = m_pGLCamera->GetFreeCutMove();
	glm::mat4 rotate = m_pGLCamera->GetFreeCutMatrix();

	projection = glm::orthoLH(	-1.0f * fScale, 
		1.0f * fScale, 
		-1.0f * nWinHeight/nWinWidth * fScale, 
		1.0f * nWinHeight/nWinWidth * fScale,
		-100.0f, -0.01f);
	if (GetUniformLocation(ProgramHandle, "model", nLoc))
		glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(model));
	if (GetUniformLocation(ProgramHandle, "view", nLoc))
		glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(view));
	if (GetUniformLocation(ProgramHandle, "projection", nLoc))
		glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(projection));

}


GLboolean COpenGLMain::GetUniformLocation(GLuint ProgramHandle, GLchar* chUniformName, GLint& nLoc)
{
	nLoc = 0;
	nLoc = glGetUniformLocation(ProgramHandle, chUniformName);
	CString str;
	str.Format("%d:%s", ProgramHandle, chUniformName);
	//if the error already post, do not post again.
	if (nLoc < 0 && !InErrorList(str))
	{
		m_csaUniformError.Add(str);
		AfxMessageBox(_T("Program handle ") + str + _T(" is not bind to the uniform"));
		return false;
	}
	return true;
}

void COpenGLMain::DrawBox(GLenum glFaces)
{
	glEnable(GL_CULL_FACE);
	glCullFace(glFaces);
	glBindVertexArray(m_pGLDataMgr->GetVAO());
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (GLuint *)NULL);
	glDisable(GL_CULL_FACE);
}


void COpenGLMain::RenderVolumeRender(GLuint nWinWidth, GLuint nWinHeight)
{
	glEnable(GL_DEPTH_TEST);
	// test the gl_error
	// render to texture
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pGLDataMgr->GetFrameBuffer());
	glViewport(0, 0, nWinWidth, nWinHeight);
	glUseProgram(m_pGLShader->GetBackProgramHandle());

	// cull front face
	SetUniforms(m_pGLShader->GetBackProgramHandle(), nWinWidth, nWinHeight, false);
	RenderObject(GL_FRONT);
	glUseProgram(0);

// 	// render freecut
// 	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pGLDataMgr->GetFreeCutFrontFrameBuffer());
// 	glUseProgram(m_pGLShader->GetBackProgramHandle());
// 	SetUniformForFreecut(m_pGLShader->GetBackProgramHandle(), nWinWidth, nWinHeight);
// 	RenderObject(GL_FRONT);
// 	glUseProgram(0);
// 
// 	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pGLDataMgr->GetFreeCutBackFrameBuffer());
// 	glUseProgram(m_pGLShader->GetBackProgramHandle());
// 	SetUniformForFreecut(m_pGLShader->GetBackProgramHandle(), nWinWidth, nWinHeight);
// 	RenderObject(GL_BACK);
// 	glUseProgram(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, nWinWidth, nWinHeight);

	glUseProgram(m_pGLShader->GetVolumeRenderProgramHandle());
	SetUniformForVolumeRender(m_pGLShader->GetVolumeRenderProgramHandle(), nWinWidth, nWinHeight);
	RenderObject(GL_BACK);

	glUseProgram(0);
	glFlush();

}

void COpenGLMain::RenderRayCasting(GLuint nWinWidth, GLuint nWinHeight)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// test the gl_error
	if (m_pGLDataMgr->GetVolTexList().size() == 0)
	{
		nWinHeight = 0;
		nWinHeight = 0;
	}
 	// render to texture
 	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pGLDataMgr->GetFrameBuffer());
 	glViewport(0, 0, nWinWidth, nWinHeight);
 	glUseProgram(m_pGLShader->GetBackProgramHandle());
 
 	// cull front face
 	SetUniforms(m_pGLShader->GetBackProgramHandle(), nWinWidth, nWinHeight, false);
 	RenderObject(GL_FRONT);
 	glUseProgram(0);

 	glBindFramebuffer(GL_FRAMEBUFFER, 0);
 	glViewport(0, 0, nWinWidth, nWinHeight);
 
 	glUseProgram(m_pGLShader->GetFrontProgramHandle());
 	SetUniforms(m_pGLShader->GetFrontProgramHandle(), nWinWidth, nWinHeight, true);
	RenderObject(GL_BACK);

	glUseProgram(0);
	glFlush();
}

void COpenGLMain::SetUniformForVolumeRender(GLuint ProgramHandle, GLuint nWinWidth, GLuint nWinHeight)
{
	GLfloat fWidth = (GLfloat)m_pGLDataMgr->GetWidth();
	GLfloat fHeight = (GLfloat)m_pGLDataMgr->GetHeight();
	GLfloat fLength = (GLfloat)m_pGLDataMgr->GetLength();
	GLfloat fCaclLength = m_pGLDataMgr->GetPixelScale() * fLength;

	glm::vec3 myScale;
	//if(fWidth >= fHeight && fWidth >= fCaclLength) 
	//{
	//	myScale = glm::vec3(1.0f, fHeight/fWidth, fCaclLength/fWidth);
	//}
	//else if (fHeight >= fWidth && fHeight >= fCaclLength)
	//{
	//	//myScale = glm::vec3(1.0f, fHeight/fWidth, fCaclLength/fWidth);
	//	myScale = glm::vec3(fWidth/fHeight, 1.0f, fCaclLength/fHeight);
	//}
	//else
	//{
	//	myScale = glm::vec3(fWidth/fCaclLength, fHeight/fCaclLength, 1.0f);
	//}
	if(fWidth > fHeight) {
		myScale = glm::vec3(1.0f, fHeight/fWidth, fCaclLength/fWidth);
	} else {
		myScale = glm::vec3(fWidth/fHeight, 1.0f, fCaclLength/fHeight);
	}

	GLint nLoc;
	if (GetUniformLocation(ProgramHandle, "ScreenSize", nLoc))
		glUniform2f(nLoc, (GLfloat)nWinWidth, (GLfloat)nWinHeight);
	if (GetUniformLocation(ProgramHandle, "VolumeSize", nLoc))
		glUniform3f(nLoc, fWidth, fHeight, fLength);
	//if (GetUinformLocation(ProgramHandle, "ObjectColor", nLoc))
	//glUniform1f(nLoc, m_pGLDataMgr);
	if (GetUniformLocation(ProgramHandle, "BackgroundColor", nLoc))
		glUniform3f(nLoc, m_pGLCamera->GetBackgroundColor().r, m_pGLCamera->GetBackgroundColor().g, m_pGLCamera->GetBackgroundColor().b);
// 	if (GetUniformLocation(ProgramHandle, "ViewPos", nLoc))
// 		glUniform3f(nLoc, m_pGLCamera->GetCameraPos().x, m_pGLCamera->GetCameraPos().y, m_pGLCamera->GetCameraPos().z);

	if (GetUniformLocation(ProgramHandle, "LightColor", nLoc))
		//glUniform3f(nLoc, 0.824f, 0.824f, 0.824f);
		glUniform3f(nLoc, m_pGLCamera->GetLightColor().r, m_pGLCamera->GetLightColor().g, m_pGLCamera->GetLightColor().b);
	if (GetUniformLocation(ProgramHandle, "MaterialColor", nLoc))
		glUniform3f(nLoc, m_pGLCamera->GetMaterialColor().r, m_pGLCamera->GetMaterialColor().g, m_pGLCamera->GetMaterialColor().b);
	if (GetUniformLocation(ProgramHandle, "LightMat", nLoc))
		glUniform4f(nLoc, m_pGLCamera->GetLightMat().r, m_pGLCamera->GetLightMat().g, m_pGLCamera->GetLightMat().b, m_pGLCamera->GetLightMat().a);
	if (GetUniformLocation(ProgramHandle, "LightPos", nLoc))
		glUniform3f(nLoc, m_pGLCamera->GetLightPos().x, m_pGLCamera->GetLightPos().y, m_pGLCamera->GetLightPos().z);
	if (GetUniformLocation(ProgramHandle, "NeedLightAndShadow", nLoc))
		glUniform2i(nLoc, m_pGLCamera->NeedLight(), m_pGLCamera->NeedShadow());
	if (GetUniformLocation(ProgramHandle, "ShadowScale", nLoc))
		glUniform1f(nLoc, m_pGLCamera->GetShadow());

	if (GetUniformLocation(ProgramHandle, "ExitPoints", nLoc))
	{
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_pGLDataMgr->GetBfTex());
		glUniform1i(nLoc, 2);
	}
	if (GetUniformLocation(ProgramHandle, "VolumeTex", nLoc))
	{
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_3D, m_pGLDataMgr->GetVolumeTex());
		glUniform1i(nLoc, 3);
	}

	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (GetUniformLocation(ProgramHandle, "Transfunc", nLoc))
	{
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, m_pGLDataMgr->GetTransFunc());
		glUniform1i(nLoc, 4);
	}



	glm::mat4 model, projection, view;
	model = glm::mat4(1.0f);//m_pGLMatrix->GetMatrix();
	glm::vec4 move = m_pGLCamera->GetMove();
	model = glm::translate(model, glm::vec3(-0.5f + move.x, -0.5f + move.y, -0.5f + move.z));

	model = glm::scale(model, myScale);
	//model = glm::scale(model, glm::vec3(1.0f, fHeight/fWidth, fLength * m_pGLDataMgr->GetPixelScale() / fWidth));
	// model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
	view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	view *= m_pGLCamera->GetMatrix();

	glm::vec4 position = m_pGLCamera->GetMove();
	glm::mat4 rorate = m_pGLCamera->GetMatrix();

	GLfloat fScale = m_pGLCamera->GetScale();
	projection = glm::orthoLH(	-1.0f * fScale, 
		1.0f * fScale, 
		-1.0f * nWinHeight/nWinWidth * fScale, 
		1.0f * nWinHeight/nWinWidth * fScale,
		-100.0f, -0.01f);
	if (GetUniformLocation(ProgramHandle, "model", nLoc))
		glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(model));
	if (GetUniformLocation(ProgramHandle, "view", nLoc))
		glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(view));
	if (GetUniformLocation(ProgramHandle, "projection", nLoc))
		glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// 	uniform sampler2D   FreeCutFrontEntryPoints;
	// 	uniform sampler2D   FreeCutBackExitPoints;



 	glm::mat4 fcmodel, fcview;

	fcmodel = glm::mat4(1.0f);//m_pGLMatrix->GetMatrix();
	glm::vec4 fcmove = m_pGLCamera->GetFreeCutMove();
	fcmodel = glm::translate(fcmodel, glm::vec3(-0.5f + fcmove.x, -0.5f + fcmove.y, -0.5f + fcmove.z));

	fcmodel = glm::scale(fcmodel, glm::vec3(1.0f, fHeight/fWidth, fLength/fWidth));
	fcview = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	//view *= m_pGLCamera->GetMoveMatrix();
	fcview *= m_pGLCamera->GetFreeCutMatrix();


	if (GetUniformLocation(ProgramHandle, "fcmodel", nLoc))
		glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(fcmodel));
	if (GetUniformLocation(ProgramHandle, "fcview", nLoc))
		glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(fcview));
	
	GLfloat range[6];
	range[0] = m_pGLDataMgr->GetRange()[0] * myScale.x;
	range[1] = m_pGLDataMgr->GetRange()[1] * myScale.x;
	range[2] = m_pGLDataMgr->GetRange()[2] * myScale.x;
	range[3] = m_pGLDataMgr->GetRange()[3] * myScale.x;
	range[4] = m_pGLDataMgr->GetRange()[4] * myScale.x * m_pGLDataMgr->GetPixelScale();
	range[5] = m_pGLDataMgr->GetRange()[5] * myScale.x * m_pGLDataMgr->GetPixelScale();

	GLfloat fStepScale = m_pGLCamera->GetStepScale();
	GLfloat fOffsetScale = m_pGLCamera->GetOffsetScale();

	if (GetUniformLocation(ProgramHandle, "Range", nLoc))
		glUniform1fv(nLoc, 6, range);

	vector <VolTexObjInfo> infoList = m_pGLDataMgr->GetVolTexList2();

	if (GetUniformLocation(ProgramHandle, "VolumeTex0", nLoc))
	{
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_3D, infoList.size() > 0 && infoList[0].show == TRUE ? infoList[0].id : 0);
		glUniform1i(nLoc, 5);
		//glActiveTexture(GL_TEXTURE5);
		//glBindTexture(GL_TEXTURE_3D, infoList.size() > 0 && infoList[0].show == TRUE ? infoList[0].id : 0);
		//glUniform1i(nLoc, 7);
	}
	if (GetUniformLocation(ProgramHandle, "VolumeTex1", nLoc))
	{
		glActiveTexture(GL_TEXTURE12);
		glBindTexture(GL_TEXTURE_3D, infoList.size() > 1 && infoList[1].show == TRUE ? infoList[1].id : 0);
		glUniform1i(nLoc, 12);
	}
	if (GetUniformLocation(ProgramHandle, "VolumeTex2", nLoc))
	{
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_3D, infoList.size() > 2 && infoList[2].show == TRUE ? infoList[2].id : 0);
		glUniform1i(nLoc, 6);
	}
	if (GetUniformLocation(ProgramHandle, "VolumeTex3", nLoc))
	{
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_3D, infoList.size() > 3 && infoList[3].show == TRUE ? infoList[3].id : 0);
		glUniform1i(nLoc, 7);
	}
	if (GetUniformLocation(ProgramHandle, "VolumeTex4", nLoc))
	{
		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_3D, infoList.size() > 4 && infoList[4].show == TRUE ? infoList[4].id : 0);
		glUniform1i(nLoc, 8);
	}
	if (GetUniformLocation(ProgramHandle, "VolumeTex5", nLoc))
	{
		glActiveTexture(GL_TEXTURE9);
		glBindTexture(GL_TEXTURE_3D, infoList.size() > 5 && infoList[5].show == TRUE ? infoList[5].id : 0);
		glUniform1i(nLoc, 9);
	}
	if (GetUniformLocation(ProgramHandle, "VolumeTex6", nLoc))
	{
		glActiveTexture(GL_TEXTURE10);
		glBindTexture(GL_TEXTURE_3D, infoList.size() > 6 && infoList[6].show == TRUE ? infoList[6].id : 0);
		glUniform1i(nLoc, 10);
	}
	if (GetUniformLocation(ProgramHandle, "VolumeTex7", nLoc))
	{
		glActiveTexture(GL_TEXTURE11);
		glBindTexture(GL_TEXTURE_3D, infoList.size() > 7 && infoList[7].show == TRUE ? infoList[7].id : 0);
		glUniform1i(nLoc, 11);
	}
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (GetUniformLocation(ProgramHandle, "ObjectColor0", nLoc))
		glUniform3f(nLoc, (infoList.size() > 0 && infoList[0].show == TRUE) ? infoList[0].color.x : 0.0f, 
		(infoList.size() > 0 && infoList[0].show == TRUE) ? infoList[0].color.y : 0.0f, 
		(infoList.size() > 0 && infoList[0].show == TRUE) ? infoList[0].color.z : 0.0f);
	if (GetUniformLocation(ProgramHandle, "ObjectColor1", nLoc))
		glUniform3f(nLoc, (infoList.size() > 1 && infoList[1].show == TRUE) ? infoList[1].color.x : 0.0f, 
		(infoList.size() > 1 && infoList[1].show == TRUE) ? infoList[1].color.y : 0.0f, 
		(infoList.size() > 1 && infoList[1].show == TRUE) ? infoList[1].color.z : 0.0f);
	if (GetUniformLocation(ProgramHandle, "ObjectColor2", nLoc))
		glUniform3f(nLoc, (infoList.size() > 2 && infoList[2].show == TRUE) ? infoList[2].color.x : 0.0f, 
		(infoList.size() > 2 && infoList[2].show == TRUE) ? infoList[2].color.y : 0.0f, 
		(infoList.size() > 2 && infoList[2].show == TRUE) ? infoList[2].color.z : 0.0f);
	if (GetUniformLocation(ProgramHandle, "ObjectColor3", nLoc))
		glUniform3f(nLoc, (infoList.size() > 3 && infoList[3].show == TRUE) ? infoList[3].color.x : 0.0f, 
		(infoList.size() > 3 && infoList[3].show == TRUE) ? infoList[3].color.y : 0.0f, 
		(infoList.size() > 3 && infoList[3].show == TRUE) ? infoList[3].color.z : 0.0f);
	if (GetUniformLocation(ProgramHandle, "ObjectColor4", nLoc))
		glUniform3f(nLoc, (infoList.size() > 4 && infoList[4].show == TRUE) ? infoList[4].color.x : 0.0f, 
		(infoList.size() > 4 && infoList[4].show == TRUE) ? infoList[4].color.y : 0.0f, 
		(infoList.size() > 4 && infoList[4].show == TRUE) ? infoList[4].color.z : 0.0f);
	if (GetUniformLocation(ProgramHandle, "ObjectColor5", nLoc))
		glUniform3f(nLoc, (infoList.size() > 5 && infoList[5].show == TRUE) ? infoList[5].color.x : 0.0f, 
		(infoList.size() > 5 && infoList[5].show == TRUE) ? infoList[5].color.y : 0.0f, 
		(infoList.size() > 5 && infoList[5].show == TRUE) ? infoList[5].color.z : 0.0f);
	if (GetUniformLocation(ProgramHandle, "ObjectColor6", nLoc))
		glUniform3f(nLoc, (infoList.size() > 6 && infoList[6].show == TRUE) ? infoList[6].color.x : 0.0f, 
		(infoList.size() > 6 && infoList[6].show == TRUE) ? infoList[6].color.y : 0.0f, 
		(infoList.size() > 6 && infoList[6].show == TRUE) ? infoList[6].color.z : 0.0f);
	if (GetUniformLocation(ProgramHandle, "ObjectColor7", nLoc))
		glUniform3f(nLoc, (infoList.size() > 7 && infoList[7].show == TRUE) ? infoList[7].color.x : 0.0f, 
		(infoList.size() > 7 && infoList[7].show == TRUE) ? infoList[7].color.y : 0.0f, 
		(infoList.size() > 7 && infoList[7].show == TRUE) ? infoList[7].color.z : 0.0f);

	//if (GetUniformLocation(ProgramHandle, "Translate0", nLoc))
	//	glUniform1f(nLoc, infoList.size() > 0 ? infoList[0].translate : 0.0f);
	//if (GetUniformLocation(ProgramHandle, "Translate1", nLoc))
	//	glUniform1f(nLoc, infoList.size() > 1 ? infoList[1].translate : 0.0f);
	//if (GetUniformLocation(ProgramHandle, "Translate2", nLoc))
	//	glUniform1f(nLoc, infoList.size() > 2 ? infoList[2].translate : 0.0f);
	//if (GetUniformLocation(ProgramHandle, "Translate3", nLoc))
	//	glUniform1f(nLoc, infoList.size() > 3 ? infoList[3].translate : 0.0f);
	//if (GetUniformLocation(ProgramHandle, "Translate4", nLoc))
	//	glUniform1f(nLoc, infoList.size() > 4 ? infoList[4].translate : 0.0f);
	//if (GetUniformLocation(ProgramHandle, "Translate5", nLoc))
	//	glUniform1f(nLoc, infoList.size() > 5 ? infoList[5].translate : 0.0f);
	//if (GetUniformLocation(ProgramHandle, "Translate6", nLoc))
	//	glUniform1f(nLoc, infoList.size() > 6 ? infoList[6].translate : 0.0f);
	//if (GetUniformLocation(ProgramHandle, "Translate7", nLoc))
	//	glUniform1f(nLoc, infoList.size() > 7 ? infoList[7].translate : 0.0f);
	//}

// 	if (GetUniformLocation(ProgramHandle, "StepScale", nLoc))
// 		glUniform1fv(nLoc, 1, &fStepScale);
// 	if (GetUniformLocation(ProgramHandle, "OffsetScale", nLoc))
// 		glUniform1fv(nLoc, 1, &fOffsetScale);
}

GLboolean COpenGLMain::InErrorList(CString str)
{
 	if (m_csaUniformError.IsEmpty())
		return false;
	for (int i=0; i<m_csaUniformError.GetSize(); i++)
	{
		if (str == m_csaUniformError[i])
			return true;
	}
	return false;
}

void COpenGLMain::SetUniformForFreecut(GLuint ProgramHandle, GLuint nWinWidth, GLuint nWinHeight, GLboolean bFront)
{
	GLfloat fWidth = (GLfloat)m_pGLDataMgr->GetWidth();
	GLfloat fHeight = (GLfloat)m_pGLDataMgr->GetHeight();
	GLfloat fLength = (GLfloat)m_pGLDataMgr->GetLength();
	GLfloat fCaclLength = m_pGLDataMgr->GetPixelScale() * fLength;

	glm::vec3 myScale;
	//if(fWidth >= fHeight && fWidth >= fCaclLength) 
	//{
	//	myScale = glm::vec3(1.0f, fHeight/fWidth, fCaclLength/fWidth);
	//}
	//else if (fHeight >= fWidth && fHeight >= fCaclLength)
	//{
	//	//myScale = glm::vec3(1.0f, fHeight/fWidth, fCaclLength/fWidth);
	//	myScale = glm::vec3(fWidth/fHeight, 1.0f, fCaclLength/fHeight);
	//}
	//else
	//{
	//	myScale = glm::vec3(fWidth/fCaclLength, fHeight/fCaclLength, 1.0f);
	//}

	if(fWidth > fHeight) {
		myScale = glm::vec3(1.0f, fHeight/fWidth, fCaclLength/fWidth);
	} else {
		myScale = glm::vec3(fWidth/fHeight, 1.0f, fCaclLength/fHeight);
	}


	GLint nLoc;

	glm::mat4 model, projection, view;
	model = glm::mat4(1.0f);//m_pGLMatrix->GetMatrix();
	glm::vec4 move = m_pGLCamera->GetMove();
	model = glm::translate(model, glm::vec3(-0.5f + move.x, -0.5f + move.y, -0.5f + move.z));
	
	model = glm::scale(model, myScale);
	//model = glm::scale(model, glm::vec3(1.0f, fHeight/fWidth, fLength * m_pGLDataMgr->GetPixelScale() / fWidth));
	//model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
	view = glm::lookAt (glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	//view *= m_pGLCamera->GetMoveMatrix();
	view *= m_pGLCamera->GetMatrix();
	//projection = glm::perspective(glm::radians(m_pGLCamera->GetViewAngle()), (GLfloat)nWinWidth/(GLfloat)nWinHeight, 0.1f, 100.0f);
	//projection = glm::ortho(0,600,0,800, 0.1f, 100.0f);
	GLfloat fScale = m_pGLCamera->GetScale();
	GLfloat fRulerScale = fWidth/nWinHeight * 2;

	projection = glm::orthoLH(	-1.0f * fScale, 
		1.0f * fScale, 
		-1.0f * nWinHeight/nWinWidth * fScale, 
		1.0f * nWinHeight/nWinWidth * fScale,
		-100.0f, -0.01f);
	if (GetUniformLocation(ProgramHandle, "model", nLoc))
		glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(model));
	if (GetUniformLocation(ProgramHandle, "view", nLoc))
		glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(view));
	if (GetUniformLocation(ProgramHandle, "projection", nLoc))
		glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(projection));



}

void COpenGLMain::RenderBorder(GLuint nWinWidth, GLuint nWinHeight)
{
//  if you just debug this only sense , uncomment the follow two lines
// 	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
// 	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, nWinWidth, nWinHeight);
	glUseProgram(m_pGLShader->GetFreeCutProgramHandle());
	SetUniformForFreecut(m_pGLShader->GetFreeCutProgramHandle(), nWinWidth, nWinHeight);
	glBindVertexArray(m_pGLDataMgr->GetFreeCutVAO());
	glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, (GLuint *)NULL);
	glBindVertexArray(0);
	glUseProgram(0);
}


void COpenGLMain::RenderText(std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
	// Activate corresponding render state	
	glUniform3f(glGetUniformLocation(m_pGLShader->GetTextProgramHandle(), "textColor"), color.x, color.y, color.z);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(m_pGLDataMgr->GetFtVAO());
	map <GLchar, Character> Characters = m_pGLDataMgr->GetTextMap();
	// Iterate through all characters
	std::string::reverse_iterator  c;
	for (c = text.rbegin(); c != text.rend(); c++) 
	{
		
		Character ch = Characters[*c];
		x -= (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))

		GLfloat xpos = x + ch.Bearing.x * scale;
		GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

		GLfloat w = ch.Size.x * scale;
		GLfloat h = ch.Size.y * scale;
		// Update VBO for each character
		GLfloat vertices[6][4] = {
			{ xpos,     ypos + h,   0.0, 0.0 },            
			{ xpos,     ypos,       0.0, 1.0 },
			{ xpos + w,	ypos,       1.0, 1.0 },

			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos + w,	ypos,       1.0, 1.0 },
			{ xpos + w,	ypos + h,   1.0, 0.0 }           
		};
		// Render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, ch.TextureID);
		// Update content of VBO memory
		glBindBuffer(GL_ARRAY_BUFFER, m_pGLDataMgr->GetFtVBO());
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// Render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}



// 	glUniform3f(glGetUniformLocation(m_pGLShader->GetTextProgramHandle(), "textColor"), 1.0f, 0.5f, 0.7f);
// 
// 	glBindVertexArray(m_pGLDataMgr->GetFtVAO());
// 	std::string text = "0123456789";
// 	std::string::const_iterator c;
// 	map <GLchar, Character> Characters = m_pGLDataMgr->GetTextMap();
// 
// 	GLfloat x = 25.0f;
// 	GLfloat y = 25.0f;
// 	GLfloat scale = 1.0f;
// 	glActiveTexture(GL_TEXTURE0);
// 
// 	for (c = text.begin(); c != text.end(); c++) 
// 	{
// 		Character ch = Characters[*c];
// 		GLfloat xpos = x + ch.Bearing.x * scale;
// 		GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
// 
// 		GLfloat w = ch.Size.x * scale;
// 		GLfloat h = ch.Size.y * scale;
// 		
// 
// 		//Update VBO for each character
// 		GLfloat vertices[6][4] = {
// 			{ xpos,     ypos + h,   0.0, 0.0 },            
// 			{ xpos,     ypos,       0.0, 1.0 },
// 			{ xpos + w, ypos,       1.0, 1.0 },
// 
// 			{ xpos,     ypos + h,   0.0, 0.0 },
// 			{ xpos + w, ypos,       1.0, 1.0 },
// 			{ xpos + w, ypos + h,   1.0, 0.0 }           
// 		};
// 		glBindTexture(GL_TEXTURE_2D, ch.TextureID);
// 
// 		// Update content of VBO memory
// 		glBindBuffer(GL_ARRAY_BUFFER, m_pGLDataMgr->GetFtVBO());
// 		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData
// 
// 		glBindBuffer(GL_ARRAY_BUFFER, 0);
// 		// Render quad
// 		glDrawArrays(GL_TRIANGLES, 0, 6);
// 		// Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
// 		x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
// 	}
// 
// 	glBindVertexArray(0);
// 	glBindTexture(GL_TEXTURE_2D, 0);