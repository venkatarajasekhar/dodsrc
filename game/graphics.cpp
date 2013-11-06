#include "graphics.h"

#include <stdio.h>
#include "geom.h"
#include <string.h>
#include <string>
#include <cmath>
#include "FontRender.h"
#include "BadMesh.h"
#include "Shader.h"

#include "util.h"

bool gWireframeMode = false;
Vec3 gCamPos( 0, 1, 2 );
Vec3 gCamAim(0,0,0);
extern int win_width, win_height;

#define CATCH_GL_ERROR( WHAT ) do { if( int error = glGetError() ) printf( "%s %i %s (%i)\n", __FILE__, __LINE__, WHAT, error ); } while(0)

#include <math.h>

GLShader* GLShader::m_pCurrent = NULL;

extern F32 c_fBaseScale;

float o = 1.0f;
float O = 1.01f;

#include "assetloader.h"
extern int win_width;
extern int win_height;

GLuint textureLocation;
typedef std::string Key;
struct TextureAsset {
	Image *im;
	GLuint glTextureID;
};
#include <map>
typedef std::map<Key, TextureAsset*> TextureDic;
extern TextureDic gTextures;

#define _M(_r,_c) (((_c)-1)*4+(_r)-1)

float g_fLastPerspectiveValues[2];
float GLPerspectiveFactorX() { return g_fLastPerspectiveValues[0]; }
float GLPerspectiveFactorY() { return g_fLastPerspectiveValues[1]; }

void AddAsset( const std::string &name, Image *source );
void MakeGLTexture( TextureAsset *a );
bool TextureExists(const char* name);
GLuint GLReadTexture(const char* pFilename) {
	if (pFilename[0] == 0) return 0xFFFFFFFF;
	TextureAsset* pAss;
	if (TextureExists(pFilename)) {
		pAss = gTextures[pFilename];
	} else {
		char pPath[1024];
		sprintf(pPath, "meshes/textures/%s", pFilename);
		AddAsset(pFilename, LoadImageG(pPath));
		pAss = gTextures[pFilename];
		if (pAss->im != NULL) {
			MakeGLTexture(pAss);
		} else {
			printf("**** Tex %s read failed\n", pFilename);
		}
	}
	return pAss->glTextureID;
}

void SetTextureForLocation(GLuint textureID, int nSlot, int nLocation) {
	extern GLuint g_nSmallWhiteTexture;
	if( (unsigned int)nLocation != 0xFFFFFFFF) {
		if (textureID == 0xFFFFFFFF ) {
			textureID = g_nSmallWhiteTexture;
		} else {
			glActiveTexture(GL_TEXTURE0 + nSlot);
			glBindTexture(GL_TEXTURE_2D, textureID);
			glUniform1i(nLocation, nSlot);
		}
	}
}

void SetTexture(GLuint textureID, int slotID) {
	extern GLuint g_nSmallWhiteTexture;
	if( UniformExists(textureLocation) ) {
		if (textureID == 0xFFFFFFFF ) {
			textureID = g_nSmallWhiteTexture;
		}
		glActiveTexture(GL_TEXTURE0 + slotID);
		glBindTexture(GL_TEXTURE_2D, textureID);
		SetUniform1i(textureLocation, slotID);
	}
}

void SetTexture(const char* pName, int slotID) {
	GLuint tid = gTextures[pName]->glTextureID;
	if( tid == 0 || tid == 0xFFFFFFFF ) {
		printf( "failed to find texture %s\n", pName );
		return;
	}
	SetTexture(gTextures[pName]->glTextureID, slotID);
}

static char s_szTemp[1024];
char* Mtx2Str(float* p) {
	sprintf(s_szTemp, "%+0.3f,%+0.3f,%+0.3f,%+0.3f\n%+0.3f,%+0.3f,%+0.3f,%+0.3f\n%+0.3f,%+0.3f,%+0.3f,%+0.3f\n%+0.3f,%+0.3f,%+0.3f,%+0.3f\n",
			p[0],p[1],p[2],p[3], p[4],p[5],p[6],p[7], p[8],p[9],p[10],p[11], p[12],p[13],p[14],p[15]);
	return s_szTemp;
}
Mat44 gProjectionMat, gCameraMat;//, gCamProj;
Mat44 gIdentityMat;

void InitDrawing() {
}

void game_on_draw( double gameTime ) {
	CATCH_GL_ERROR("OnEntry to game_on_draw()");
	double drawStart = glfwGetTime();
	drawStart = 0.0f;
	const float yAngle = 0.0f;
	const float xAngle = 0.2f;

	float sy = sin( yAngle );
	float cy = cos( yAngle );
	float sx = sin( xAngle );
	float cx = cos( xAngle );
	const float dist = 20.0f;
	gCamPos = Vec3( cx * sy * dist, sx * dist, cx * cy * dist );

	DefaultShaderProgram.Use(true);
	CATCH_GL_ERROR("On UseDefaultShader");

	GLfloat mvmat[16];

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_GEQUAL);
	glDisable(GL_BLEND);//glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Vec3 camUp = Vec3(0,1,0);

	const float fov = 1.0f;
	gProjectionMat = Mat44Perspective( fov, (float)win_width / (float)win_height, 0.1f, 100.0f);
	g_fLastPerspectiveValues[0] = gProjectionMat.x.x;
	g_fLastPerspectiveValues[1] = gProjectionMat.y.y;
	gCameraMat = Mat44LookAt( gCamPos, gCamAim, camUp);
	//gCamProj = gCameraMat * gProjectionMat;// * gCameraMat;
	memset(mvmat, 0, sizeof(mvmat)); mvmat[0*4+0] = mvmat[1*4+1] = mvmat[2*4+2] = mvmat[3*4+3] = 1;
	CATCH_GL_ERROR("miscPreDraw");

	GLShader *current = GLShader::Current();
	if( current ) {
		glUniformMatrix4fv(GLShader::Current()->projLocation, 1, false, gProjectionMat);
		glUniformMatrix4fv(GLShader::Current()->viewLocation, 1, false, gCameraMat);
		CATCH_GL_ERROR("UploadProj");
	} else {
		printf( "No current shader!\n" );
	}

	glClearColor( 0.4f,0.4f,0.5f, 1.0f );
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	Vec4 point;
	point.x = 0.6f;
	point.y = 2.0f;
	point.z = 0.26;
	point.w = 1.0f;

	// the elements of space
	current = GLShader::Current();
	if( current ) {
		mvmat[3*4+0] = 0.0f; mvmat[3*4+2] = 0.0f;
		glUniformMatrix4fv(GLShader::Current()->modelLocation, 1, false, mvmat );
		CATCH_GL_ERROR("ModelMatrix");
	} else {
		printf( "No current shader\n" );
	}

	if( gWireframeMode ) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	CATCH_GL_ERROR("Draw");

	glEnable(GL_BLEND);

	DefaultShaderProgram.Use();
	glDepthMask(GL_TRUE);

	CATCH_GL_ERROR("Pre-Default Shader");
	DefaultShaderProgram.Use();
	CATCH_GL_ERROR("Setting Default Shader");

	glActiveTexture(GL_TEXTURE0);
	SetTexture( "cursor", 0 );

	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
	glMatrixMode(GL_MODELVIEW);

	//glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float w = float(win_width / 2);
	float h = float(win_height / 2);
	gProjectionMat = Mat44Orthographic( 0, win_width, win_height, 0, -1, 1 );
	glUniformMatrix4fv(GLShader::Current()->projLocation, 1, false, gProjectionMat );
	gIdentityMat.SetIdentity();
	glUniformMatrix4fv(GLShader::Current()->viewLocation, 1, false, gIdentityMat );
	CATCH_GL_ERROR("Proj");

	if( 1 ) {
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0.0f, 0.0f, 0.1f);
		glScalef( 8.0f, 8.0f, 1.0f );

		char buffer[128];
		sprintf( buffer, "%ims", (int)(gameTime * 1000) );
		FontPrint( gIdentityMat, buffer );
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glColor4f(1.0f,1.0f,1.0f,0.9f);
	glScalef(1.0f,1.0f,0.1f);
	glRotatef(drawStart*45.0f, 0.0f,1.0f,0.0f );
	glGetFloatv(GL_MODELVIEW_MATRIX,mvmat);
	//glUniformMatrix4fv(mvLocation, 1, false, mvmat );
	glUniformMatrix4fv(GLShader::Current()->modelLocation, 1, false, mvmat );
	CATCH_GL_ERROR("ModelMatrix");
	SetTexture( "cursor", 0 );


	glLoadIdentity();
	glTranslatef(8+16-w,8+16-h,0);
	glScalef(16.0f,16.0f,0.1f);
	glGetFloatv(GL_MODELVIEW_MATRIX,mvmat);
	//glUniformMatrix4fv(mvLocation, 1, false, mvmat );
	CATCH_GL_ERROR("ModelView");

	CATCH_GL_ERROR("EndOfDisplay");
}

//#include "MemTexture.h"

void GLMemoryTextureReload(GLuint handle, S32 nW, S32 nH, U32* pnImage)
{
	glBindTexture(GL_TEXTURE_2D, handle);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, nW, nH, GL_RGBA, GL_UNSIGNED_BYTE, pnImage);
}

GLuint MemoryTexture(S32 nW, S32 nH, U32* pnImage)
{
	S32 nFormat = 0;
	GLuint nTex;
	S32 bPalettised = (nW >> 16) & 1;
	(void)bPalettised;
	nW &= 0xFFFF; nH &= 0xFFFF;

	// check whether we have alpha or grey
	S32 i;
	S32 bNoAlpha = 1, bGrey = 1;
	S32 bWhite = 1;
	U8* pbImage = (U8*)pnImage;

	// analyse contents
	for (i = 0; i < nW*nH; i ++)
	{
		if (pbImage[i*4+3] != 255)
			bNoAlpha = 0;
		if (pbImage[i*4+0] != pbImage[i*4+1] || pbImage[i*4+0] != pbImage[i*4+2] || pbImage[i*4+1] != pbImage[i*4+2])
			bGrey = 0;
		if (pbImage[i*4+0] != 255)
			bWhite = 0;
		if (!bNoAlpha && !bGrey) break;
	}

	// depending on contents, convert
	if (bNoAlpha && bGrey)
	{
		// convert to GL_LUMINANCE
		nFormat = 4;
		for (i = 0; i < nW*nH; i ++)
			pbImage[i] = pbImage[i*4+0];
	}
	else if (!bNoAlpha && bGrey && bWhite)
	{
		// convert to GL_ALPHA
		nFormat = 2;
		for (i = 0; i < nW*nH; i ++)
			pbImage[i] = pbImage[i*4+3];
	}
	else if (!bNoAlpha && bGrey)
	{
		// convert to GL_LUMINANCE_ALPHA
		nFormat = 1;
		for (i = 0; i < nW*nH; i ++)
			*(U16*)&pbImage[i*2] = (pbImage[i*4+3]<<8) | pbImage[i*4+0];
	}
	else if (bNoAlpha)
	{
		// convert to GL_RGB
		for (i = 0; i < nW*nH; i ++)
		{
			nFormat = 3;
			pbImage[i*3+0] = pbImage[i*4+0];
			pbImage[i*3+1] = pbImage[i*4+1];
			pbImage[i*3+2] = pbImage[i*4+2];
		}
	}

	GLint nGLFmt;
	switch (nFormat)
	{
		case 0: nGLFmt = GL_RGBA; break;
		case 1: nGLFmt = GL_LUMINANCE_ALPHA; break;
		case 2: nGLFmt = GL_ALPHA; break;
		case 3: nGLFmt = GL_RGB; break;
		case 4: nGLFmt = GL_LUMINANCE; break;
	}
	glGenTextures(1, &nTex);

	glBindTexture(GL_TEXTURE_2D, nTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE );
	glTexImage2D(GL_TEXTURE_2D, 0, nGLFmt, nW, nH, 0, nGLFmt, GL_UNSIGNED_BYTE, pnImage);

	return nTex;
}

GLuint g_nSmallWhiteTexture;
void GLMakeSmallWhite()
{
	U32 nWhite[8*8];
	memset(nWhite, 0xFF, sizeof(nWhite));
	g_nSmallWhiteTexture = MemoryTexture(8, 8, nWhite );
}

void ClearScreen( float r, float g, float b ) {
	glClearColor( r, g, b, 1.0f );
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

void Ortho( const char * shader ) {
	ActivateShader( shader );
	gProjectionMat = Mat44Orthographic( 0, win_width, win_height, 0, -1, 1 );
	if( GLShader *current = GLShader::Current() ) {
		glUniformMatrix4fv(current->projLocation, 1, false, gProjectionMat);
		CATCH_GL_ERROR("UploadProj");
	}
}
void DefaultOrtho() {
	DefaultShaderProgram.Use(true);
	gProjectionMat = Mat44Orthographic( 0, win_width, win_height, 0, -1, 1 );
	if( GLShader *current = GLShader::Current() ) {
		glUniformMatrix4fv(current->projLocation, 1, false, gProjectionMat);
		CATCH_GL_ERROR("UploadProj");
	}
}

void DefaultProjection() {
	DefaultShaderProgram.Use(true);
	const float fov = 1.0f;
	//Log( 3, "Proj %i,%i\n", win_width, win_height );
	gProjectionMat = Mat44Perspective( fov, (float)win_width / (float)win_height, 0.1f, 100.0f);
	if( GLShader *current = GLShader::Current() ) {
		glUniformMatrix4fv(current->projLocation, 1, false, gProjectionMat);
		CATCH_GL_ERROR("UploadProj");
	}
}
void SetCamera(const Mat44 &camTransform) {
	gCamPos = camTransform.w;
	gCamAim = camTransform.z + gCamPos;
	gCameraMat = camTransform;
	if( GLShader *current = GLShader::Current() ) {
		glUniformMatrix4fv(current->viewLocation, 1, false, gCameraMat);
		CATCH_GL_ERROR("UploadView");
	}
}
void SetCamera(const Vec3 &pos, const Vec3 &target) {
	gCamPos = pos;
	gCamAim = target;
	Vec3 camUp = Vec3(0,1,0);
	gCameraMat = Mat44LookAt( gCamPos, gCamAim, camUp);
	if( GLShader *current = GLShader::Current() ) {
		glUniformMatrix4fv(current->viewLocation, 1, false, gCameraMat);
		CATCH_GL_ERROR("UploadView");
	}
}
void DrawSquare( float x, float y, float width, float height, uint32_t colour ) {
#if 0
	uint32_t abgr = 0xFFFFFFFF;
	glBegin(GL_QUADS);
	glColor4ubv( (GLubyte*)&colour );
	glNormal3f(0,0,-1);
	glTexCoord2f( 0, 0); glVertex3f( x, y, 0.0f );
	glTexCoord2f( 1, 0); glVertex3f( x+width, y, 0.0f );
	glTexCoord2f( 1, 1); glVertex3f( x+width, y+height, 0.0f );
	glTexCoord2f( 0, 1); glVertex3f( x, y+height, 0.0f );
	glEnd();
#else
	struct RenderV3 {
		float x,y,z;
	};
	struct RenderV2 {
		float x,y;
	};
	RenderV3 verts[] = {
		{ x,y,0 },
		{ x+width,y,0 },
		{ x+width,y+height,0 },
		{ x,y+height,0 }
		};
	RenderV3 normals[] = {
		{ 0,0,-1 },
		{ 0,0,-1 },
		{ 0,0,-1 },
		{ 0,0,-1 },
		};
	RenderV2 uvs[] = {
		{ 0,0 },
		{ 1,0 },
		{ 1,1 },
		{ 0,1 },
		};
	uint32_t colours[] = {
		colour,
		colour,
		colour,
		colour,
	};

	glEnableVertexAttribArray(ATTR_VERTEX);
	glEnableVertexAttribArray(ATTR_NORMAL);
	glEnableVertexAttribArray(ATTR_COLOR);
	glEnableVertexAttribArray(ATTR_UV);
	glVertexAttribPointer(ATTR_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, &verts[0].x );
	glVertexAttribPointer(ATTR_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, &normals[0].x );
	glVertexAttribPointer(ATTR_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &colours );
	glVertexAttribPointer(ATTR_UV, 2, GL_FLOAT, GL_FALSE, 0, &uvs[0].x );
	glDrawArrays(GL_QUADS, 0, 4 );
#endif
}

void ClearTexture() {
	SetTexture(-1,0);
}
