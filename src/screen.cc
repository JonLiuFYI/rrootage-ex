/*
 * $Id: screen.c,v 1.6 2003/08/10 03:21:28 kenta Exp $
 *
 * Copyright 2003 Kenta Cho. All rights reserved.
 */

/**
 * OpenGL screen handler.
 *
 * @version $Revision: 1.6 $
 */
#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"

#include <math.h>
#include <string.h>

#include "genmcr.h"
#include "screen.h"
#include "rr.h"
#include "degutil.h"
#include "attractmanager.h"
#include "letterrender.h"
#include "boss_mtd.h"

#ifdef PLATFORM_NX
#include "swich.h"
#else
#include "pcplatform.h"
#endif

extern void imgui_input(SDL_Event* event);
extern void imgui_newframe(SDL_Window* window);
extern void init_imgui(SDL_Window* window);
#define FAR_PLANE 720

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define LOWRES_SCREEN_WIDTH 320
#define LOWRES_SCREEN_HEIGHT 240

static int screenWidth, screenHeight;

GLint buildMips(GLenum target, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* data);
// Replaces gluPerspective. Sets the frustum to perspective mode.
// fovY     - Field of vision in degrees in the y direction
// aspect   - Aspect ratio of the viewport
// zNear    - The near clipping distance
// zFar     - The far clipping distance

void f_gluPerspective(GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
	const GLdouble pi = 3.1415926535897932384626433832795;
	GLdouble fW, fH;

	fH = tan((fovY / 2) / 180 * pi) * zNear;
	fH = tan(fovY / 360 * pi) * zNear;
	fW = fH * aspect;
    glFrustum(-fW, fW, -fH, fH, zNear, zFar);
}

// Reset viewport when the screen is resized.
static void screenResized() {
  int viewportWidth = screenWidth;
  int viewportHeight = screenHeight;
  // Maintain aspect ratio between SCREEN_WIDTH and SCREEN_HEIGHT.
  if (screenHeight * SCREEN_WIDTH  > screenWidth * SCREEN_HEIGHT) {
    viewportHeight = SCREEN_HEIGHT * screenWidth / SCREEN_WIDTH;
  } else {
    viewportWidth = SCREEN_WIDTH * screenHeight / SCREEN_HEIGHT;
  }
  int offsetX = 0;
  int offsetY = 0;
  glViewport(offsetX, offsetY, viewportWidth, viewportHeight);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  f_gluPerspective(45.0f, (GLfloat)viewportWidth/(GLfloat)viewportHeight, 0.1f, FAR_PLANE);
  glMatrixMode(GL_MODELVIEW);
}

void resized(int width, int height) {
  screenWidth = width; screenHeight = height;
  screenResized();
}

// Init OpenGL.
static void initGL() {
  glViewport(0, 0, screenWidth, screenHeight);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  glLineWidth(1);
  glEnable(GL_LINE_SMOOTH);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);

  glDisable(GL_LIGHTING);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_COLOR_MATERIAL);

  resized(screenWidth, screenHeight);
}

// Load bitmaps and convert to textures.
void loadGLTexture(const char *fileName, GLuint *texture) {
  SDL_Surface *surface;

  char name[64];
#ifdef PLATFORM_NX
  strcpy(name, "Assets:/images/");
#else
  strcpy(name, "resources/images/");
#endif
  strcat(name, fileName);
  surface = SDL_LoadBMP(name);
  if ( !surface ) {
    fprintf(stderr, "Unable to load texture: %s\n", SDL_GetError());
    SDL_Quit();
    exit(1);
  }

  glGenTextures(1, texture);
  glBindTexture(GL_TEXTURE_2D, *texture);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);

  buildMips(GL_TEXTURE_2D, 3, surface->w, surface->h, GL_RGB, GL_UNSIGNED_BYTE, surface->pixels);
}

void generateTexture(GLuint *texture) {
   glGenTextures(1, texture);
}

void deleteTexture(GLuint *texture) {
  glDeleteTextures(1, texture);
}
static GLuint starTexture;
#define STAR_BMP "star.bmp"
static GLuint smokeTexture;
#define SMOKE_BMP "smoke.bmp"
static GLuint titleTexture;
#define TITLE_BMP "title.bmp"

int lowres = 0;
int windowMode = 0;
int brightness = DEFAULT_BRIGHTNESS;
Uint8 *keys;
SDL_Joystick *stick = NULL;
SDL_Window* Window = NULL;
TouchInputState touch;

#include "SDL_test_common.h"
void initSDL(int argc, char* argv[]) {
  Uint32 videoFlags;

  if ( lowres ) {
    screenWidth  = LOWRES_SCREEN_WIDTH;
    screenHeight = LOWRES_SCREEN_HEIGHT;
  } else {
    screenWidth  = SCREEN_WIDTH;
    screenHeight = SCREEN_HEIGHT;
  }

  /* Initialize SDL */
  if ( SDL_Init(/*SDL_INIT_VIDEO |*/ SDL_INIT_JOYSTICK) < 0 ) {
    fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
    exit(1);
  }

  /* Create an OpenGL screen */

  windowMode = 0;
  if ( windowMode ) {
      videoFlags = SDL_WINDOW_OPENGL;
  } else {
    if ( !lowres ) {
      // Use native desktop resolution if -lowres is not specified.
	  //screenWidth = 0;
	  //screenHeight = 0;

		
    }
    videoFlags = SDL_WINDOW_OPENGL ;// | SDL_WINDOW_FULLSCREEN; 
  } 

  initialize_platform();
  
  SDL_Init(SDL_INIT_VIDEO);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);
  Window = SDL_CreateWindow("OpenGL Test", 0, 0, screenWidth, screenHeight, videoFlags);

 if (Window == NULL) {
     fprintf(stderr, "Unable to create OpenGL screen: %s\n", SDL_GetError());
     SDL_Quit();
     exit(2);
 }

 SDL_GLContext Context = SDL_GL_CreateContext(Window);

 #ifndef __EMSCRIPTEN__
 // glad: load all OpenGL function pointers
  // ---------------------------------------
 if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
 {
	 printf("Failed to initialize GLAD");
	 return;
 }
 #endif

 
 for (int i = 0; i < 3; i++)
 {
	 SDL_Joystick* tocl = SDL_JoystickOpen(i);
	 if (tocl)
	 {
		 stick = tocl;
	 }
 }
  

  /* Set the title bar in environments that support it */
//  SDL_WM_SetCaption(CAPTION, NULL);

  initGL();
  loadGLTexture(STAR_BMP, &starTexture);
  loadGLTexture(SMOKE_BMP, &smokeTexture);
  loadGLTexture(TITLE_BMP, &titleTexture);

  SDL_ShowCursor(SDL_DISABLE);

  init_imgui(Window);
}

void closeSDL() {
  SDL_ShowCursor(SDL_ENABLE);
}

float zoom = 15;
static int screenShakeCnt = 0;
static int screenShakeType = 0;


static void __gluMakeIdentityf(GLfloat m[16])
{
	m[0 + 4 * 0] = 1; m[0 + 4 * 1] = 0; m[0 + 4 * 2] = 0; m[0 + 4 * 3] = 0;
	m[1 + 4 * 0] = 0; m[1 + 4 * 1] = 1; m[1 + 4 * 2] = 0; m[1 + 4 * 3] = 0;
	m[2 + 4 * 0] = 0; m[2 + 4 * 1] = 0; m[2 + 4 * 2] = 1; m[2 + 4 * 3] = 0;
	m[3 + 4 * 0] = 0; m[3 + 4 * 1] = 0; m[3 + 4 * 2] = 0; m[3 + 4 * 3] = 1;
}


static void cross(float v1[3], float v2[3], float result[3])
{
	result[0] = v1[1] * v2[2] - v1[2] * v2[1];
	result[1] = v1[2] * v2[0] - v1[0] * v2[2];
	result[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

static void normalize(float v[3])
{
	float r;

	r = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	if (r == 0.0) return;

	v[0] /= r;
	v[1] /= r;
	v[2] /= r;
}

void f_gluLookAt(GLdouble eyex, GLdouble eyey, GLdouble eyez, GLdouble centerx,

	GLdouble centery, GLdouble centerz, GLdouble upx, GLdouble upy,

	GLdouble upz)

{

	int i;

	float forward[3], side[3], up[3];

	GLfloat m[4][4];

	forward[0] = centerx - eyex;

	forward[1] = centery - eyey;

	forward[2] = centerz - eyez;



	up[0] = upx;

	up[1] = upy;

	up[2] = upz;

	normalize(forward);
	/* Side = forward x up */

	cross(forward, up, side);

	normalize(side);

	/* Recompute up as: up = side x forward */

	cross(side, forward, up);

    __gluMakeIdentityf(&m[0][0]);

	m[0][0] = side[0];

	m[1][0] = side[1];

	m[2][0] = side[2];


    m[0][1] = up[0];
    m[1][1] = up[1];
	m[2][1] = up[2];



	m[0][2] = -forward[0];
	m[1][2] = -forward[1];
	m[2][2] = -forward[2];

	glMultMatrixf(&m[0][0]);

	glTranslated(-eyex, -eyey, -eyez);
}

static void setEyepos() {
  float x, y;
  glPushMatrix();
  if ( screenShakeCnt > 0 ) {
    switch ( screenShakeType ) {
    case 0:
      x = (float)randNS2(256)/5000.0f;
      y = (float)randNS2(256)/5000.0f;
      break;
    default:
      x = (float)randNS2(256)*screenShakeCnt/21000.0f;
      y = (float)randNS2(256)*screenShakeCnt/21000.0f;
      break;
    }
    f_gluLookAt(0, 0, zoom, x, y, 0, 0.0f, 1.0f, 0.0f);
  } else {
    f_gluLookAt(0, 0, zoom, 0, 0, 0, 0.0f, 1.0f, 0.0f);
  }
}

void setScreenShake(int type, int cnt) {
  screenShakeType = type; screenShakeCnt = cnt;
}

void moveScreenShake() {
  if ( screenShakeCnt > 0 ) {
    screenShakeCnt--;
  }
}

void drawGLSceneStart() {
  glClear(GL_COLOR_BUFFER_BIT);
  setEyepos();
}

void drawGLSceneEnd() {
  glPopMatrix();
}

void swapGLScene() {
  SDL_GL_SwapWindow(Window);

}

void drawBox(GLfloat x, GLfloat y, GLfloat width, GLfloat height, 
	     int r, int g, int b) {
  glPushMatrix();
  glTranslatef(x, y, 0);
  glColor4ub(r, g, b, 128);
  glBegin(GL_TRIANGLE_FAN);
  glVertex3f(-width, -height,  0);
  glVertex3f( width, -height,  0);
  glVertex3f( width,  height,  0);
  glVertex3f(-width,  height,  0);
  glEnd();
  glColor4ub(r, g, b, 255);
  glBegin(GL_LINE_LOOP);
  glVertex3f(-width, -height,  0);
  glVertex3f( width, -height,  0);
  glVertex3f( width,  height,  0);
  glVertex3f(-width,  height,  0);
  glEnd();
  glPopMatrix();
}

void drawLine(GLfloat x1, GLfloat y1, GLfloat z1,
	      GLfloat x2, GLfloat y2, GLfloat z2, int r, int g, int b, int a) {
  glColor4ub(r, g, b, a);
  glBegin(GL_LINES);
  glVertex3f(x1, y1, z1);
  glVertex3f(x2, y2, z2);
  glEnd();
}

void drawLinePart(GLfloat x1, GLfloat y1, GLfloat z1,
		  GLfloat x2, GLfloat y2, GLfloat z2, int r, int g, int b, int a, int len) {
  glColor4ub(r, g, b, a);
  glBegin(GL_LINES);
  glVertex3f(x1, y1, z1);
  glVertex3f(x1+(x2-x1)*len/256, y1+(y2-y1)*len/256, z1+(z2-z1)*len/256);
  glEnd();
}

void drawRollLineAbs(GLfloat x1, GLfloat y1, GLfloat z1,
		     GLfloat x2, GLfloat y2, GLfloat z2, int r, int g, int b, int a, int d1) {
  glPushMatrix();
  glRotatef((float)d1*360/1024, 0, 0, 1);
  glColor4ub(r, g, b, a);
  glBegin(GL_LINES);
  glVertex3f(x1, y1, z1);
  glVertex3f(x2, y2, z2);
  glEnd();
  glPopMatrix();
}

void drawRollLine(GLfloat x, GLfloat y, GLfloat z, GLfloat width,
		  int r, int g, int b, int a, int d1, int d2) {
  glPushMatrix();
  glTranslatef(x, y, z);
  glRotatef((float)d1*360/1024, 0, 0, 1);
  glRotatef((float)d2*360/1024, 1, 0, 0);
  glColor4ub(r, g, b, a);
  glBegin(GL_LINES);
  glVertex3f(0, -width, 0);
  glVertex3f(0,  width, 0);
  glEnd();
  glPopMatrix();
}

void drawSquare(GLfloat x1, GLfloat y1, GLfloat z1, 
		GLfloat x2, GLfloat y2, GLfloat z2, 
		GLfloat x3, GLfloat y3, GLfloat z3, 
		GLfloat x4, GLfloat y4, GLfloat z4, 
		int r, int g, int b) {
  glColor4ub(r, g, b, 64);
  glBegin(GL_TRIANGLE_FAN);
  glVertex3f(x1, y1, z1);
  glVertex3f(x2, y2, z2);
  glVertex3f(x3, y3, z3);
  glVertex3f(x4, y4, z4);
  glEnd();
}

void drawStar(int f, GLfloat x, GLfloat y, GLfloat z, int r, int g, int b, float size) {
  glEnable(GL_TEXTURE_2D);
  if ( f ) {
    glBindTexture(GL_TEXTURE_2D, starTexture);
  } else {
    glBindTexture(GL_TEXTURE_2D, smokeTexture);
  }
  glColor4ub(r, g, b, 255);
  glPushMatrix();
  glTranslatef(x, y, z);
  glRotatef(rand()%360, 0.0f, 0.0f, 1.0f);
  glBegin(GL_TRIANGLE_FAN);
  glTexCoord2f(0.0f, 1.0f); 
  glVertex3f(-size, -size,  0);
  glTexCoord2f(1.0f, 1.0f);
  glVertex3f( size, -size,  0);
  glTexCoord2f(1.0f, 0.0f);
  glVertex3f( size,  size,  0);
  glTexCoord2f(0.0f, 0.0f);
  glVertex3f(-size,  size,  0);
  glEnd();
  glPopMatrix();
  glDisable(GL_TEXTURE_2D);
}

#define LASER_ALPHA 100
#define LASER_LINE_ALPHA 50
#define LASER_LINE_ROLL_SPEED 17
#define LASER_LINE_UP_SPEED 16

void drawLaser(GLfloat x, GLfloat y, GLfloat width, GLfloat height,
	       int cc1, int cc2, int cc3, int cc4, int cnt, int type) {
  int i, d;
  float gx, gy;
  glBegin(GL_TRIANGLE_FAN);
  if ( type != 0 ) {
    glColor4ub(cc1, cc1, cc1, LASER_ALPHA);
    glVertex3f(x-width, y, 0);
  }
  glColor4ub(cc2, 255, cc2, LASER_ALPHA);
  glVertex3f(x, y, 0);
  glColor4ub(cc4, 255, cc4, LASER_ALPHA);
  glVertex3f(x, y+height, 0);
  glColor4ub(cc3, cc3, cc3, LASER_ALPHA);
  glVertex3f(x-width, y+height, 0);
  glEnd();
  glBegin(GL_TRIANGLE_FAN);
  if ( type != 0 ) {
    glColor4ub(cc1, cc1, cc1, LASER_ALPHA);
    glVertex3f(x+width, y, 0);
  }
  glColor4ub(cc2, 255, cc2, LASER_ALPHA);
  glVertex3f(x, y, 0);
  glColor4ub(cc4, 255, cc4, LASER_ALPHA);
  glVertex3f(x, y+height, 0);
  glColor4ub(cc3, cc3, cc3, LASER_ALPHA);
  glVertex3f(x+width, y+height, 0);
  glEnd();
  if ( type == 2 ) return;
  glColor4ub(80, 240, 80, LASER_LINE_ALPHA);
  glBegin(GL_LINES);
  d = (cnt*LASER_LINE_ROLL_SPEED)&(512/4-1);
  for ( i=0 ; i<4 ; i++, d+=(512/4) ) {
    d &= 1023;
    gx = x + width*sctbl[d+256]/256.0f;
    if ( type == 1 ) {
      glVertex3f(gx, y, 0);
    } else {
      glVertex3f(x, y, 0);
    }
    glVertex3f(gx, y+height, 0);
  }
  if ( type == 0 ) {
    glEnd();
    return;
  }
  gy = y + (height/4/LASER_LINE_UP_SPEED) * (cnt&(LASER_LINE_UP_SPEED-1));
  for ( i=0 ; i<4 ; i++, gy+=height/4 ) {
    glVertex3f(x-width, gy, 0);
    glVertex3f(x+width, gy, 0);
  }
  glEnd();
}

#define SHAPE_POINT_SIZE 0.05f
#define SHAPE_BASE_COLOR_R 250
#define SHAPE_BASE_COLOR_G 240
#define SHAPE_BASE_COLOR_B 180

#define CORE_HEIGHT 0.2f
#define CORE_RING_SIZE 0.6f

#define SHAPE_POINT_SIZE_L 0.07f

static void drawRing(GLfloat x, GLfloat y, int d1, int d2, int r, int g, int b) {
  int i, d;
  float x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4;
  glPushMatrix();
  glTranslatef(x, y, 0);
  glRotatef((float)d1*360/1024, 0, 0, 1);
  glRotatef((float)d2*360/1024, 1, 0, 0);
  glColor4ub(r, g, b, 255);
  x1 = x2 = 0;
  y1 = y4 =  CORE_HEIGHT/2;
  y2 = y3 = -CORE_HEIGHT/2;
  z1 = z2 = CORE_RING_SIZE;
  for ( i=0,d=0 ; i<8 ; i++ ) {
    d+=(1024/8); d &= 1023;
    x3 = x4 = sctbl[d+256]*CORE_RING_SIZE/256;
    z3 = z4 = sctbl[d]    *CORE_RING_SIZE/256;
    drawSquare(x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, r, g, b);
    x1 = x3; y1 = y3; z1 = z3;
    x2 = x4; y2 = y4; z2 = z4;
  }
  glPopMatrix();
}

void drawCore(GLfloat x, GLfloat y, int cnt, int r, int g, int b) {
  int i;
  float cy;
  glPushMatrix();
  glTranslatef(x, y, 0);
  glColor4ub(r, g, b, 255);
  glBegin(GL_TRIANGLE_FAN);
  glVertex3f(-SHAPE_POINT_SIZE_L, -SHAPE_POINT_SIZE_L,  0);
  glVertex3f( SHAPE_POINT_SIZE_L, -SHAPE_POINT_SIZE_L,  0);
  glVertex3f( SHAPE_POINT_SIZE_L,  SHAPE_POINT_SIZE_L,  0);
  glVertex3f(-SHAPE_POINT_SIZE_L,  SHAPE_POINT_SIZE_L,  0);
  glEnd();
  glPopMatrix();
  cy = y - CORE_HEIGHT*2.5f;
  for ( i=0 ; i<4 ; i++, cy+=CORE_HEIGHT ) {
    drawRing(x, cy, (cnt*(4+i))&1023, (sctbl[(cnt*(5+i))&1023]/4)&1023, r, g, b);
  }
}

#define SHIP_DRUM_R 0.4f
#define SHIP_DRUM_WIDTH 0.05f
#define SHIP_DRUM_HEIGHT 0.35f

void drawShipShape(GLfloat x, GLfloat y, float d, int inv) {
  int i;
  glPushMatrix();
  glTranslatef(x, y, 0);
  glColor4ub(255, 100, 100, 255);
  glBegin(GL_TRIANGLE_FAN);
  glVertex3f(-SHAPE_POINT_SIZE_L, -SHAPE_POINT_SIZE_L,  0);
  glVertex3f( SHAPE_POINT_SIZE_L, -SHAPE_POINT_SIZE_L,  0);
  glVertex3f( SHAPE_POINT_SIZE_L,  SHAPE_POINT_SIZE_L,  0);
  glVertex3f(-SHAPE_POINT_SIZE_L,  SHAPE_POINT_SIZE_L,  0);
  glEnd();
  if ( inv ) {
    glPopMatrix();
    return;
  }
  glRotatef(d, 0, 1, 0);
    glColor4ub(120, 220, 100, 150);
    /*if ( mode == IKA_MODE ) {
    glColor4ub(180, 200, 160, 150);
  } else {
    glColor4ub(120, 220, 100, 150);
    }*/
  for ( i=0 ; i<8 ; i++ ) {
    glRotatef(45, 0, 1, 0);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-SHIP_DRUM_WIDTH, -SHIP_DRUM_HEIGHT, SHIP_DRUM_R);
    glVertex3f( SHIP_DRUM_WIDTH, -SHIP_DRUM_HEIGHT, SHIP_DRUM_R);
    glVertex3f( SHIP_DRUM_WIDTH,  SHIP_DRUM_HEIGHT, SHIP_DRUM_R);
    glVertex3f(-SHIP_DRUM_WIDTH,  SHIP_DRUM_HEIGHT, SHIP_DRUM_R);
    glEnd();
  }
  glPopMatrix();
}

void drawBomb(GLfloat x, GLfloat y, GLfloat width, int cnt) {
  int i, d, od, c;
  GLfloat x1, y1, x2, y2;
  d = cnt*48; d &= 1023;
  c = 4+(cnt>>3); if ( c > 16 ) c = 16;
  od = 1024/c;
  x1 = (sctbl[d]    *width)/256 + x;
  y1 = (sctbl[d+256]*width)/256 + y;
  for ( i=0 ; i<c ; i++ ) {
    d += od; d &= 1023;
    x2 = (sctbl[d]    *width)/256 + x;
    y2 = (sctbl[d+256]*width)/256 + y;
    drawLine(x1, y1, 0, x2, y2, 0, 255, 255, 255, 255);
    x1 = x2; y1 = y2;
  }
}

void drawCircle(GLfloat x, GLfloat y, GLfloat width, int cnt, 
		int r1, int g1, int b1, int r2, int b2, int g2) {
  int i, d;
  GLfloat x1, y1, x2, y2;
  if ( (cnt&1) == 0 ) { 
    glColor4ub(r1, g1, b1, 64);
  } else {
    glColor4ub(255, 255, 255, 64);
  }
  glBegin(GL_TRIANGLE_FAN);
  glVertex3f(x, y, 0);
  d = cnt*48; d &= 1023;
  x1 = (sctbl[d]    *width)/256 + x;
  y1 = (sctbl[d+256]*width)/256 + y;
  glColor4ub(r2, g2, b2, 150);
  for ( i=0 ; i<16 ; i++ ) {
    d += 64; d &= 1023;
    x2 = (sctbl[d]    *width)/256 + x;
    y2 = (sctbl[d+256]*width)/256 + y;
    glVertex3f(x1, y1, 0);
    glVertex3f(x2, y2, 0);
    x1 = x2; y1 = y2;
  }
  glEnd();
}

void drawShape(GLfloat x, GLfloat y, GLfloat size, int d, int cnt, int type,
	       int r, int g, int b) {
  GLfloat sz, sz2;
  glPushMatrix();
  glTranslatef(x, y, 0);
  glColor4ub(r, g, b, 255);
  glBegin(GL_TRIANGLE_FAN);
  glVertex3f(-SHAPE_POINT_SIZE, -SHAPE_POINT_SIZE,  0);
  glVertex3f( SHAPE_POINT_SIZE, -SHAPE_POINT_SIZE,  0);
  glVertex3f( SHAPE_POINT_SIZE,  SHAPE_POINT_SIZE,  0);
  glVertex3f(-SHAPE_POINT_SIZE,  SHAPE_POINT_SIZE,  0);
  glEnd();
  switch ( type ) {
  case 0:
    sz = size/2;
    glRotatef((float)d*360/1024, 0, 0, 1);
    glDisable(GL_BLEND);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-sz, -sz,  0);
    glVertex3f( sz, -sz,  0);
    glVertex3f( 0, size,  0);
    glEnd();
    glEnable(GL_BLEND);
    glColor4ub(r, g, b, 150);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(-sz, -sz,  0);
    glVertex3f( sz, -sz,  0);
    glColor4ub(SHAPE_BASE_COLOR_R, SHAPE_BASE_COLOR_G, SHAPE_BASE_COLOR_B, 150);
    glVertex3f( 0, size,  0);
    glEnd();
    break;
  case 1:
    sz = size/2;
    glRotatef((float)((cnt*23)&1023)*360/1024, 0, 0, 1);
    glDisable(GL_BLEND);
    glBegin(GL_LINE_LOOP);
    glVertex3f(  0, -size,  0);
    glVertex3f( sz,     0,  0);
    glVertex3f(  0,  size,  0);
    glVertex3f(-sz,     0,  0);
    glEnd();
    glEnable(GL_BLEND);
    glColor4ub(r, g, b, 180);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(  0, -size,  0);
    glVertex3f( sz,     0,  0);
    glColor4ub(SHAPE_BASE_COLOR_R, SHAPE_BASE_COLOR_G, SHAPE_BASE_COLOR_B, 150);
    glVertex3f(  0,  size,  0);
    glVertex3f(-sz,     0,  0);
    glEnd();
    break;
  case 2:
    sz = size/4; sz2 = size/3*2;
    glRotatef((float)d*360/1024, 0, 0, 1);
    glDisable(GL_BLEND);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-sz, -sz2,  0);
    glVertex3f( sz, -sz2,  0);
    glVertex3f( sz,  sz2,  0);
    glVertex3f(-sz,  sz2,  0);
    glEnd();
    glEnable(GL_BLEND);
    glColor4ub(r, g, b, 120);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(-sz, -sz2,  0);
    glVertex3f( sz, -sz2,  0);
    glColor4ub(SHAPE_BASE_COLOR_R, SHAPE_BASE_COLOR_G, SHAPE_BASE_COLOR_B, 150);
    glVertex3f( sz, sz2,  0);
    glVertex3f(-sz, sz2,  0);
    glEnd();
    break;
  case 3:
    sz = size/2;
    glRotatef((float)((cnt*37)&1023)*360/1024, 0, 0, 1);
    glDisable(GL_BLEND);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-sz, -sz,  0);
    glVertex3f( sz, -sz,  0);
    glVertex3f( sz,  sz,  0);
    glVertex3f(-sz,  sz,  0);
    glEnd();
    glEnable(GL_BLEND);
    glColor4ub(r, g, b, 180);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(-sz, -sz,  0);
    glVertex3f( sz, -sz,  0);
    glColor4ub(SHAPE_BASE_COLOR_R, SHAPE_BASE_COLOR_G, SHAPE_BASE_COLOR_B, 150);
    glVertex3f( sz,  sz,  0);
    glVertex3f(-sz,  sz,  0);
    glEnd();
    break;
  case 4:
    sz = size/2;
    glRotatef((float)((cnt*53)&1023)*360/1024, 0, 0, 1);
    glDisable(GL_BLEND);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-sz/2, -sz,  0);
    glVertex3f( sz/2, -sz,  0);
    glVertex3f( sz,  -sz/2,  0);
    glVertex3f( sz,   sz/2,  0);
    glVertex3f( sz/2,  sz,  0);
    glVertex3f(-sz/2,  sz,  0);
    glVertex3f(-sz,   sz/2,  0);
    glVertex3f(-sz,  -sz/2,  0);
    glEnd();
    glEnable(GL_BLEND);
    glColor4ub(r, g, b, 220);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(-sz/2, -sz,  0);
    glVertex3f( sz/2, -sz,  0);
    glVertex3f( sz,  -sz/2,  0);
    glVertex3f( sz,   sz/2,  0);
    glColor4ub(SHAPE_BASE_COLOR_R, SHAPE_BASE_COLOR_G, SHAPE_BASE_COLOR_B, 150);
    glVertex3f( sz/2,  sz,  0);
    glVertex3f(-sz/2,  sz,  0);
    glVertex3f(-sz,   sz/2,  0);
    glVertex3f(-sz,  -sz/2,  0);
    glEnd();
    break;
  case 5:
    sz = size*2/3; sz2 = size/5;
    glRotatef((float)d*360/1024, 0, 0, 1);
    glDisable(GL_BLEND);
    glBegin(GL_LINE_STRIP);
    glVertex3f(-sz, -sz+sz2,  0);
    glVertex3f( 0, sz+sz2,  0);
    glVertex3f( sz, -sz+sz2,  0);
    glEnd();
    glEnable(GL_BLEND);
    glColor4ub(r, g, b, 150);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(-sz, -sz+sz2,  0);
    glVertex3f( sz, -sz+sz2,  0);
    glColor4ub(SHAPE_BASE_COLOR_R, SHAPE_BASE_COLOR_G, SHAPE_BASE_COLOR_B, 150);
    glVertex3f( 0, sz+sz2,  0);
    glEnd();
    break;
  case 6:
    sz = size/2;
    glRotatef((float)((cnt*13)&1023)*360/1024, 0, 0, 1);
    glDisable(GL_BLEND);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-sz, -sz,  0);
    glVertex3f(  0, -sz,  0);
    glVertex3f( sz,   0,  0);
    glVertex3f( sz,  sz,  0);
    glVertex3f(  0,  sz,  0);
    glVertex3f(-sz,   0,  0);
    glEnd();
    glEnable(GL_BLEND);
    glColor4ub(r, g, b, 210);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(-sz, -sz,  0);
    glVertex3f(  0, -sz,  0);
    glVertex3f( sz,   0,  0);
    glColor4ub(SHAPE_BASE_COLOR_R, SHAPE_BASE_COLOR_G, SHAPE_BASE_COLOR_B, 150);
    glVertex3f( sz,  sz,  0);
    glVertex3f(  0,  sz,  0);
    glVertex3f(-sz,   0,  0);
    glEnd();
    break;
  }
  glPopMatrix();
}

static int ikaClr[2][3][3] = {
  {{230, 230, 255}, {100, 100, 200}, {50, 50, 150}},
  {{0, 0, 0}, {200, 0, 0}, {100, 0, 0}},
};

void drawShapeIka(GLfloat x, GLfloat y, GLfloat size, int d, int cnt, int type, int c) {
  GLfloat sz, sz2, sz3;
  glPushMatrix();
  glTranslatef(x, y, 0);
  glColor4ub(ikaClr[c][0][0], ikaClr[c][0][1], ikaClr[c][0][2], 255);
  glDisable(GL_BLEND);
  glBegin(GL_TRIANGLE_FAN);
  glVertex3f(-SHAPE_POINT_SIZE, -SHAPE_POINT_SIZE,  0);
  glVertex3f( SHAPE_POINT_SIZE, -SHAPE_POINT_SIZE,  0);
  glVertex3f( SHAPE_POINT_SIZE,  SHAPE_POINT_SIZE,  0);
  glVertex3f(-SHAPE_POINT_SIZE,  SHAPE_POINT_SIZE,  0);
  glEnd();
  glColor4ub(ikaClr[c][0][0], ikaClr[c][0][1], ikaClr[c][0][2], 255);
  switch ( type ) {
  case 0:
    sz = size/2; sz2 = sz/3; sz3 = size*2/3;
    glRotatef((float)d*360/1024, 0, 0, 1);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-sz, -sz3,  0);
    glVertex3f( sz, -sz3,  0);
    glVertex3f( sz2, sz3,  0);
    glVertex3f(-sz2, sz3,  0);
    glEnd();
    glEnable(GL_BLEND);
    glColor4ub(ikaClr[c][1][0], ikaClr[c][1][1], ikaClr[c][1][2], 250);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(-sz, -sz3,  0);
    glVertex3f( sz, -sz3,  0);
    glColor4ub(ikaClr[c][2][0], ikaClr[c][2][1], ikaClr[c][2][2], 250);
    glVertex3f( sz2, sz3,  0);
    glVertex3f(-sz2, sz3,  0);
    glEnd();
    break;
  case 1:
    sz = size/2;
    glRotatef((float)((cnt*53)&1023)*360/1024, 0, 0, 1);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-sz/2, -sz,  0);
    glVertex3f( sz/2, -sz,  0);
    glVertex3f( sz,  -sz/2,  0);
    glVertex3f( sz,   sz/2,  0);
    glVertex3f( sz/2,  sz,  0);
    glVertex3f(-sz/2,  sz,  0);
    glVertex3f(-sz,   sz/2,  0);
    glVertex3f(-sz,  -sz/2,  0);
    glEnd();
    glEnable(GL_BLEND);
    glColor4ub(ikaClr[c][1][0], ikaClr[c][1][1], ikaClr[c][1][2], 250);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(-sz/2, -sz,  0);
    glVertex3f( sz/2, -sz,  0);
    glVertex3f( sz,  -sz/2,  0);
    glVertex3f( sz,   sz/2,  0);
    glColor4ub(ikaClr[c][2][0], ikaClr[c][2][1], ikaClr[c][2][2], 250);
    glVertex3f( sz/2,  sz,  0);
    glVertex3f(-sz/2,  sz,  0);
    glVertex3f(-sz,   sz/2,  0);
    glVertex3f(-sz,  -sz/2,  0);
    glEnd();
    break;
  }
  glPopMatrix();
}


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>



template<int N>
struct VertDraw {
	struct V2 {
		GLfloat x, y;
	};

	V2 batch[N];

	int count = 0;

	glm::mat4 matrix;
	void reset() {
		count = 0;
		matrix = glm::mat4(1.f);
	}
	void addvert(GLfloat x, GLfloat y, GLfloat z) {
		batch[count].x = x;
		batch[count].y = y;

		count++;
	}

	void translate(GLfloat x, GLfloat y, GLfloat z) {
		matrix = glm::translate(matrix, glm::vec3(x, y, z));
	}

	void rotate(GLfloat n) {
		matrix = glm::rotate(matrix, glm::radians(n), glm::vec3(0, 0, 1));
	}

	void glsubmit() {

		glm::vec2 verts[N];

		for (int i = 0; i < count; i++)
		{
			glm::vec4 v = { batch[i].x, batch[i].y,0.0,1.f };

			v = matrix * v;
			
			verts[i].x = v.x;
			verts[i].y = v.y;
		}

		for (int i = 0; i < (count - 1); i++)
		{
			glVertex3f(verts[i].x, verts[i].y, 0);
			glVertex3f(verts[i + 1].x, verts[i + 1].y, 0);
		}
	}
};


template<int N>
struct ColorVertDraw {
	struct V2 {
		GLfloat x, y;
	};

	V2 batch[N];
	glm::u8vec4 colors[N];

	glm::u8vec4 current_color;

	int count = 0;

	glm::mat4 matrix;
	void reset() {
		count = 0;
		current_color = glm::u8vec4{ 255,255,255,255 };
		matrix = glm::mat4(1.f);
	}
	void addvert(GLfloat x, GLfloat y, GLfloat z) {
		batch[count].x = x;
		batch[count].y = y;
		
		colors[count] = current_color;
		count++;
	}

	void translate(GLfloat x, GLfloat y, GLfloat z) {
		matrix = glm::translate(matrix, glm::vec3(x, y, z));
	}

	void rotate(GLfloat n) {
		matrix = glm::rotate(matrix, glm::radians(n), glm::vec3(0, 0, 1));
	}

	void set_color(int r, int b, int g, int a)
	{
		current_color = glm::u8vec4{ r,b,g,a };
	}
};


struct DrawArray {
	std::vector<glm::vec2> positions;
	std::vector<glm::u8vec4> colors;

	void reserve(int count) {
		positions.reserve(count);
		colors.reserve(count);
	}

	template<int N>
	void adddraw_line(VertDraw<N>& draw, glm::u8vec4 color)
	{
		glm::vec4 verts[N];

		for (int i = 0; i < draw.count; i++)
		{
			glm::vec4 v = { draw.batch[i].x, draw.batch[i].y,0.0,1.f };

			v = draw.matrix * v;

			verts[i] = v;
		}

		for (int i = 0; i < (draw.count - 1); i++)
		{
			positions.push_back({ verts[i].x, verts[i].y });
			positions.push_back({ verts[i + 1].x, verts[i + 1].y });

			colors.push_back(color);
			colors.push_back(color);
		}
	}

	template<int N>
	void adddraw_lineloop(VertDraw<N>& draw, glm::u8vec4 color)
	{
		glm::vec4 verts[N];

		for (int i = 0; i < draw.count; i++)
		{
			glm::vec4 v = { draw.batch[i].x, draw.batch[i].y,0.0,1.f };

			v = draw.matrix * v;

			verts[i] = v;
		}

		for (int i = 0; i < (draw.count); i++)
		{
			positions.push_back({ verts[i].x, verts[i].y });

			if (i + 1 == draw.count)
			{
				positions.push_back({ verts[0].x, verts[0].y });
			}
			else {
				positions.push_back({ verts[i + 1].x, verts[i + 1].y });
			}
			

			colors.push_back(color);
			colors.push_back(color);
		}
	}



	template<int N>
	void adddraw_triangle(VertDraw<N>& draw, glm::u8vec4 color)
	{
		//glm::vec4 verts[N];

		for (int i = 0; i < draw.count; i++)
		{
			glm::vec4 v = { draw.batch[i].x, draw.batch[i].y,0.0,1.f };

			v = draw.matrix * v;


			positions.push_back({ v.x, v.y });
			colors.push_back(color);
		}
	}

	template<int N>
	void adddraw_triangle(ColorVertDraw<N>& draw)
	{
		for (int i = 0; i < draw.count; i++)
		{
			glm::vec4 v = { draw.batch[i].x, draw.batch[i].y,0.0,1.f };

			v = draw.matrix * v;

			positions.push_back({ v.x, v.y });
			colors.push_back(draw.colors[i].color);
		}
	}

	template<int N>
	void adddraw_trianglefan(ColorVertDraw<N>& draw)
	{
		glm::vec2 verts[N];

		for (int i = 0; i < draw.count; i++)
		{
			glm::vec4 v = { draw.batch[i].x, draw.batch[i].y,0.0,1.f };

			v = draw.matrix * v;

			verts[i].x = v.x;
			verts[i].y = v.y;
		}

		glm::vec2 center = verts[0];

		for (int i = 1; i < draw.count-1; i++)
		{
			positions.push_back({ verts[0].x, verts[0].y });
			positions.push_back({ verts[i].x, verts[i].y });
			positions.push_back({ verts[i + 1].x, verts[i + 1].y });

			colors.push_back(draw.colors[0]);
			colors.push_back(draw.colors[i]);
			colors.push_back(draw.colors[i + 1]);
		}
	}

	void draw() {
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, positions.data());

		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors.data());

		glDrawArrays(GL_LINES, 0, positions.size());

		// deactivate vertex arrays after drawing
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
	}

	void draw_triangles() {
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, positions.data());

		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors.data());

		glDrawArrays(GL_TRIANGLES, 0, positions.size());

		// deactivate vertex arrays after drawing
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
	}
};

VertDraw<8> ikaoutlines[1024];

void draw_ika_centers(int count, FoeDrawIka* draw)
{
	DrawArray boxes;

	boxes.reserve(count * 6);

	for (int i = 0; i < count; i++)
	{
		VertDraw<6> glbatch;
		glbatch.reset();

		GLfloat x = draw[i].x;
		GLfloat y = draw[i].y;
		int c = draw[i].c;
		//glColor4ub(ikaClr[c][0][0], ikaClr[c][0][1], ikaClr[c][0][2], 255);

		glbatch.addvert(-SHAPE_POINT_SIZE + x, -SHAPE_POINT_SIZE + y, 0);
		glbatch.addvert(SHAPE_POINT_SIZE + x, -SHAPE_POINT_SIZE + y, 0);
		glbatch.addvert(SHAPE_POINT_SIZE + x, SHAPE_POINT_SIZE + y, 0);

		glbatch.addvert(-SHAPE_POINT_SIZE + x, -SHAPE_POINT_SIZE + y, 0);
		glbatch.addvert(SHAPE_POINT_SIZE + x, SHAPE_POINT_SIZE + y, 0);
		glbatch.addvert(-SHAPE_POINT_SIZE + x, SHAPE_POINT_SIZE + y, 0);

		boxes.adddraw_triangle(glbatch, glm::u8vec4{ ikaClr[c][0][0], ikaClr[c][0][1], ikaClr[c][0][2], 255 });
	}
	boxes.draw_triangles();
}

void draw_ika_outlines(int count, FoeDrawIka* draw)
{
	GLfloat sz, sz2, sz3;
	DrawArray outlines;

	outlines.reserve(count * 8);


	for (int i = 0; i < count; i++)
	{
		VertDraw<8> glbatch;
		glbatch.reset();

		glbatch.translate(draw[i].x, draw[i].y, 0.f);

		GLfloat x = draw[i].x;
		GLfloat y = draw[i].y;
		GLfloat size = draw[i].size;
		int d = draw[i].d;
		int cnt = draw[i].cnt;
		int type = draw[i].type;

		int c = draw[i].c;

		switch (type) {
		case 0:
			sz = size / 2; sz2 = sz / 3; sz3 = size * 2 / 3;

			glbatch.rotate((float)d * 360 / 1024.0);

			glbatch.addvert(-sz, -sz3, 0);
			glbatch.addvert(sz, -sz3, 0);
			glbatch.addvert(sz2, sz3, 0);
			glbatch.addvert(-sz2, sz3, 0);


			outlines.adddraw_lineloop(glbatch, glm::u8vec4{ ikaClr[c][0][0], ikaClr[c][0][1], ikaClr[c][0][2], 255 });
			break;
		case 1:
			sz = size / 2;
			glbatch.rotate((float)((cnt * 53) & 1023) * 360 / 1024.0);

			glbatch.addvert(-sz / 2, -sz, 0);
			glbatch.addvert(sz / 2, -sz, 0);
			glbatch.addvert(sz, -sz / 2, 0);
			glbatch.addvert(sz, sz / 2, 0);
			glbatch.addvert(sz / 2, sz, 0);
			glbatch.addvert(-sz / 2, sz, 0);
			glbatch.addvert(-sz, sz / 2, 0);
			glbatch.addvert(-sz, -sz / 2, 0);

			outlines.adddraw_lineloop(glbatch, glm::u8vec4{ ikaClr[c][0][0], ikaClr[c][0][1], ikaClr[c][0][2], 255 });

			break;
		}

	}

	outlines.draw();
}

void draw_ika_shapes(int count, FoeDrawIka* draw)
{
	DrawArray shapes;

	shapes.reserve(count * 8);

	GLfloat sz, sz2, sz3;

	for (int i = 0; i < count; i++)
	{
		ColorVertDraw<8> batch;

		batch.reset();
		batch.translate(draw[i].x, draw[i].y, 0);


		GLfloat x = draw[i].x;
		GLfloat y = draw[i].y;
		GLfloat size = draw[i].size;
		int d = draw[i].d;
		int cnt = draw[i].cnt;
		int type = draw[i].type;

		int c = draw[i].c;

		batch.set_color(ikaClr[c][0][0], ikaClr[c][0][1], ikaClr[c][0][2], 255);
		switch (type) {
		case 0:
			sz = size / 2; sz2 = sz / 3; sz3 = size * 2 / 3;

			batch.rotate((float)d * 360 / 1024);

			batch.set_color(ikaClr[c][1][0], ikaClr[c][1][1], ikaClr[c][1][2], 250);

			batch.addvert(-sz, -sz3, 0);
			batch.addvert(sz, -sz3, 0);
			batch.set_color(ikaClr[c][2][0], ikaClr[c][2][1], ikaClr[c][2][2], 250);
			batch.addvert(sz2, sz3, 0);
			batch.addvert(-sz2, sz3, 0);

			shapes.adddraw_trianglefan(batch);

			break;
		case 1:
			sz = size / 2;
			batch.rotate((float)((cnt * 53) & 1023) * 360 / 1024);

			batch.set_color(ikaClr[c][1][0], ikaClr[c][1][1], ikaClr[c][1][2], 250);

			batch.addvert(-sz / 2, -sz, 0);
			batch.addvert(sz / 2, -sz, 0);
			batch.addvert(sz, -sz / 2, 0);
			batch.addvert(sz, sz / 2, 0);
			batch.set_color(ikaClr[c][2][0], ikaClr[c][2][1], ikaClr[c][2][2], 250);
			batch.addvert(sz / 2, sz, 0);
			batch.addvert(-sz / 2, sz, 0);
			batch.addvert(-sz, sz / 2, 0);
			batch.addvert(-sz, -sz / 2, 0);

			shapes.adddraw_trianglefan(batch);

			break;
		}
	}

	glEnable(GL_BLEND);

	shapes.draw_triangles();

	glDisable(GL_BLEND);
}

void batchDrawShapeIka(FoeDrawIka* draw, int count) {
    glDisable(GL_BLEND);
    
	draw_ika_centers(count, draw);
	
	draw_ika_outlines(count, draw);

	draw_ika_shapes(count, draw);
}

void draw_foe_centers(int count, FoeDraw* draw)
{
	DrawArray boxes;

	boxes.reserve(count * 6);

	for (int i = 0; i < count; i++)
	{
		VertDraw<6> glbatch;
		glbatch.reset();

		GLfloat x = draw[i].x;
		GLfloat y = draw[i].y;

		glbatch.addvert(-SHAPE_POINT_SIZE + x, -SHAPE_POINT_SIZE + y, 0);
		glbatch.addvert(SHAPE_POINT_SIZE + x, -SHAPE_POINT_SIZE + y, 0);
		glbatch.addvert(SHAPE_POINT_SIZE + x, SHAPE_POINT_SIZE + y, 0);

		glbatch.addvert(-SHAPE_POINT_SIZE + x, -SHAPE_POINT_SIZE + y, 0);
		glbatch.addvert(SHAPE_POINT_SIZE + x, SHAPE_POINT_SIZE + y, 0);
		glbatch.addvert(-SHAPE_POINT_SIZE + x, SHAPE_POINT_SIZE + y, 0);

		boxes.adddraw_triangle(glbatch, glm::u8vec4{ draw[i].r, draw[i].g, draw[i].b, 255 });
	}
	boxes.draw_triangles();
}

void draw_foe_outlines(int count, FoeDraw* draw)
{
	DrawArray outlines;

	outlines.reserve(count * 8);


	GLfloat sz, sz2;
	for (int i = 0; i < count; i++)
	{
		VertDraw<8> glbatch;
		glbatch.reset();

		glbatch.translate(draw[i].x, draw[i].y, 0.f);

		//glPushMatrix();
		//glTranslatef(draw[i].x, draw[i].y, 0);
		//glColor4ub(draw[i].r, draw[i].g, draw[i].b, 255);

		glm::u8vec4 color(draw[i].r, draw[i].g, draw[i].b, 255);

		GLfloat x = draw[i].x;
		GLfloat y = draw[i].y;
		GLfloat size = draw[i].size;
		int d = draw[i].d;
		int cnt = draw[i].cnt;
		int type = draw[i].type;

		int r = draw[i].r;
		int g = draw[i].g;
		int b = draw[i].b;

		switch (type) {
		case 0:
			sz = size / 2;
			glbatch.rotate((float)d * 360.0 / 1024.0);

			glbatch.addvert(-sz, -sz, 0);
			glbatch.addvert(sz, -sz, 0);
			glbatch.addvert(0, size, 0);

			outlines.adddraw_lineloop(glbatch, color);

			break;
		case 1:
			sz = size / 2;
			glbatch.rotate((float)((cnt * 23) & 1023) * 360.0 / 1024.0);

			glbatch.addvert(0, -size, 0);
			glbatch.addvert(sz, 0, 0);
			glbatch.addvert(0, size, 0);
			glbatch.addvert(-sz, 0, 0);

			outlines.adddraw_lineloop(glbatch, color);

			break;
		case 2:
			sz = size / 4; sz2 = size / 3 * 2;
			glbatch.rotate((float)d * 360.0 / 1024.0);
			
			glbatch.addvert(-sz, -sz2, 0);
			glbatch.addvert(sz, -sz2, 0);
			glbatch.addvert(sz, sz2, 0);
			glbatch.addvert(-sz, sz2, 0);

			outlines.adddraw_lineloop(glbatch, color);
			break;
		case 3:
			sz = size / 2;
			glbatch.rotate((float)((cnt * 37) & 1023) * 360.0 / 1024.0);


			glbatch.addvert(-sz, -sz, 0);
			glbatch.addvert(sz, -sz, 0);
			glbatch.addvert(sz, sz, 0);
			glbatch.addvert(-sz, sz, 0);

			outlines.adddraw_lineloop(glbatch, color);
			break;
		case 4:
			sz = size / 2;
			glbatch.rotate((float)((cnt * 53) & 1023) * 360.0 / 1024.0);
			
			glbatch.addvert(-sz / 2, -sz, 0);
			glbatch.addvert(sz / 2, -sz, 0);
			glbatch.addvert(sz, -sz / 2, 0);
			glbatch.addvert(sz, sz / 2, 0);
			glbatch.addvert(sz / 2, sz, 0);
			glbatch.addvert(-sz / 2, sz, 0);
			glbatch.addvert(-sz, sz / 2, 0);
			glbatch.addvert(-sz, -sz / 2, 0);

			outlines.adddraw_lineloop(glbatch, color);
			break;
		case 5:
			sz = size * 2 / 3; sz2 = size / 5;

			glbatch.rotate((float)d * 360.0 / 1024.0);
			
			glbatch.addvert(-sz, -sz + sz2, 0);
			glbatch.addvert(0, sz + sz2, 0);
			glbatch.addvert(sz, -sz + sz2, 0);

			outlines.adddraw_line(glbatch, color);

			break;
		case 6:
			sz = size / 2;

			glbatch.rotate((float)((cnt * 13) & 1023) * 360.0 / 1024.0);
			
			glbatch.addvert(-sz, -sz, 0);
			glbatch.addvert(0, -sz, 0);
			glbatch.addvert(sz, 0, 0);
			glbatch.addvert(sz, sz, 0);
			glbatch.addvert(0, sz, 0);
			glbatch.addvert(-sz, 0, 0);

			outlines.adddraw_lineloop(glbatch, color);
			break;
		}

	}

	outlines.draw();
}

void draw_foe_shapes(int count, FoeDraw* draw)
{
	GLfloat sz, sz2;

	DrawArray shapes;

	shapes.reserve(count * 8);

	for (int i = 0; i < count; i++)
	{


		ColorVertDraw<8> batch;
		batch.reset();
		batch.translate(draw[i].x, draw[i].y, 0);
		batch.set_color(draw[i].r, draw[i].g, draw[i].b, 255);

		GLfloat x = draw[i].x;
		GLfloat y = draw[i].y;
		GLfloat size = draw[i].size;
		int d = draw[i].d;
		int cnt = draw[i].cnt;
		int type = draw[i].type;

		int r = draw[i].r;
		int g = draw[i].g;
		int b = draw[i].b;

		switch (type) {
		case 0:
			sz = size / 2;

			batch.rotate((float)d * 360 / 1024);
			batch.set_color(r, g, b, 150);
			batch.addvert(-sz, -sz, 0);
			batch.addvert(sz, -sz, 0);
			batch.set_color(SHAPE_BASE_COLOR_R, SHAPE_BASE_COLOR_G, SHAPE_BASE_COLOR_B, 150);
			batch.addvert(0, size, 0);

			shapes.adddraw_trianglefan(batch);


			break;
		case 1:
			sz = size / 2;
			
			batch.rotate(((cnt * 23) & 1023) * 360 / 1024);
			batch.set_color(r, g, b, 180);
			batch.addvert(0, -size, 0);
			batch.addvert(sz, 0, 0);
			batch.set_color(SHAPE_BASE_COLOR_R, SHAPE_BASE_COLOR_G, SHAPE_BASE_COLOR_B, 150);
			batch.addvert(0, size, 0);
			batch.addvert(-sz, 0, 0);

			shapes.adddraw_trianglefan(batch);

			break;
		case 2:
			sz = size / 4; sz2 = size / 3 * 2;

			batch.rotate((float)d * 360 / 1024);
			batch.set_color(r, g, b, 120);
			batch.addvert(-sz, -sz2, 0);
			batch.addvert(sz, -sz2, 0);
			batch.set_color(SHAPE_BASE_COLOR_R, SHAPE_BASE_COLOR_G, SHAPE_BASE_COLOR_B, 150);
			batch.addvert(sz, sz2, 0);
			batch.addvert(-sz, sz2, 0);

			shapes.adddraw_trianglefan(batch);

			break;
		case 3:
			sz = size / 2;

			batch.rotate((float)((cnt * 37) & 1023) * 360 / 1024);

			batch.set_color(r, g, b, 180);
			batch.addvert(-sz, -sz, 0);
			batch.addvert(sz, -sz, 0);
			batch.set_color(SHAPE_BASE_COLOR_R, SHAPE_BASE_COLOR_G, SHAPE_BASE_COLOR_B, 150);
			batch.addvert(sz, sz, 0);
			batch.addvert(-sz, sz, 0);


			shapes.adddraw_trianglefan(batch);

			break;
		case 4:
			sz = size / 2;

			batch.rotate((float)((cnt * 53) & 1023) * 360 / 1024);

			batch.set_color(r, g, b, 220);

			batch.addvert(-sz / 2, -sz, 0);
			batch.addvert(sz / 2, -sz, 0);
			batch.addvert(sz, -sz / 2, 0);
			batch.addvert(sz, sz / 2, 0);
			batch.set_color(SHAPE_BASE_COLOR_R, SHAPE_BASE_COLOR_G, SHAPE_BASE_COLOR_B, 150);
			batch.addvert(sz / 2, sz, 0);
			batch.addvert(-sz / 2, sz, 0);
			batch.addvert(-sz, sz / 2, 0);
			batch.addvert(-sz, -sz / 2, 0);

			shapes.adddraw_trianglefan(batch);

			break;
		case 5:
			sz = size * 2 / 3; sz2 = size / 5;

			batch.rotate((float)d * 360 / 1024);

			batch.set_color(r, g, b, 150);
			batch.addvert(-sz, -sz + sz2, 0);
			batch.addvert(sz, -sz + sz2, 0);
			batch.set_color(SHAPE_BASE_COLOR_R, SHAPE_BASE_COLOR_G, SHAPE_BASE_COLOR_B, 150);
			batch.addvert(0, sz + sz2, 0);

			shapes.adddraw_trianglefan(batch);
			break;
		case 6:
			sz = size / 2;

			batch.rotate((float)((cnt * 13) & 1023) * 360 / 1024);

			batch.set_color(r, g, b, 210);
			batch.addvert(-sz, -sz, 0);
			batch.addvert(0, -sz, 0);
			batch.addvert(sz, 0, 0);
			batch.set_color(SHAPE_BASE_COLOR_R, SHAPE_BASE_COLOR_G, SHAPE_BASE_COLOR_B, 150);
			batch.addvert(sz, sz, 0);
			batch.addvert(0, sz, 0);
			batch.addvert(-sz, 0, 0);

			shapes.adddraw_trianglefan(batch);
			break;
		}
	}

	shapes.draw_triangles();
}

void batchdrawShape(FoeDraw* draw, int count) {
	

	draw_foe_centers(count, draw);

	glDisable(GL_BLEND);

	draw_foe_outlines(count, draw);

	glEnable(GL_BLEND);
	
	draw_foe_shapes(count, draw);

}

#define SHOT_WIDTH 0.1
#define SHOT_HEIGHT 0.2

static int shtClr[3][3][3] = {
  {{200, 200, 225}, {50, 50, 200}, {200, 200, 225}},
  {{100, 0, 0}, {100, 0, 0}, {200, 0, 0}},
  {{100, 200, 100}, {50, 100, 50}, {100, 200, 100}},
};

void drawShot(GLfloat x, GLfloat y, GLfloat d, int c, float width, float height) {
  glPushMatrix();
  glTranslatef(x, y, 0);
  glRotatef(d, 0, 0, 1);
  glColor4ub(shtClr[c][0][0], shtClr[c][0][1], shtClr[c][0][2], 240);
  glDisable(GL_BLEND);
  glBegin(GL_LINES);
  glVertex3f(-width, -height, 0);
  glVertex3f(-width,  height, 0);
  glVertex3f( width, -height, 0);
  glVertex3f( width,  height, 0);
  glEnd();
  glEnable(GL_BLEND);

  glColor4ub(shtClr[c][1][0], shtClr[c][1][1], shtClr[c][1][2], 240);
  glBegin(GL_TRIANGLE_FAN);
  glVertex3f(-width, -height, 0);
  glVertex3f( width, -height, 0);
  glColor4ub(shtClr[c][2][0], shtClr[c][2][1], shtClr[c][2][2], 240);
  glVertex3f( width,  height, 0);
  glVertex3f(-width,  height, 0);
  glEnd();
  glPopMatrix();
}

void startDrawBoards() {
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, 640, 480, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
}

void endDrawBoards() {
  glPopMatrix();
  screenResized();
}

static void drawBoard(int x, int y, int width, int height) {
  glColor4ub(0, 0, 0, 255);
  glBegin(GL_QUADS);
  glVertex2f(x,y);
  glVertex2f(x+width,y);
  glVertex2f(x+width,y+height);
  glVertex2f(x,y+height);
  glEnd();
}

void drawSideBoards() {
  glDisable(GL_BLEND);
  drawBoard(0, 0, 160, 480);
  drawBoard(480, 0, 160, 480);
  glEnable(GL_BLEND);
  drawScore();
  drawRPanel();
}

void drawTitleBoard() {
   
	float offsetx = 0;// -330;
	float offsety = 0;// 70;

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, titleTexture);
  glColor4ub(255, 255, 255, 255);
  glBegin(GL_TRIANGLE_FAN);
  glTexCoord2f(0.0f, 0.0f); 
  glVertex3f(350 + offsetx, 78 + offsety,  0);
  glTexCoord2f(1.0f, 0.0f);
  glVertex3f(470 + offsetx, 78 + offsety,  0);
  glTexCoord2f(1.0f, 1.0f);
  glVertex3f(470 + offsetx, 114 + offsety,  0);
  glTexCoord2f(0.0f, 1.0f);
  glVertex3f(350 + offsetx, 114 + offsety,  0);
  glEnd();
  glDisable(GL_TEXTURE_2D); 
  glColor4ub(200, 200, 200, 255);
  glBegin(GL_TRIANGLE_FAN);
  glVertex3f(350+ offsetx, 30+ offsety, 0);
  glVertex3f(400+ offsetx, 30+ offsety, 0);
  glVertex3f(380+ offsetx, 56+ offsety, 0);
  glVertex3f(380+ offsetx, 80+ offsety, 0);
  glVertex3f(350+ offsetx, 80+ offsety, 0);
  glEnd();
  glBegin(GL_TRIANGLE_FAN);
  glVertex3f(404 + offsetx, 80 + offsety, 0);
  glVertex3f(404 + offsetx,  8 + offsety, 0);
  glVertex3f(440 + offsetx, 8  + offsety, 0);
  glVertex3f(440 + offsetx, 44 + offsety, 0);
  glVertex3f(465 + offsetx, 80 + offsety, 0);
  glEnd();
  glColor4ub(255, 255, 255, 255);
  glBegin(GL_LINE_LOOP);
  glVertex3f(350 + offsetx, 30+ offsety, 0);
  glVertex3f(400 + offsetx, 30+ offsety, 0);
  glVertex3f(380 + offsetx, 56+ offsety, 0);
  glVertex3f(380 + offsetx, 80+ offsety, 0);
  glVertex3f(350 + offsetx, 80+ offsety, 0);
  glEnd();
  glBegin(GL_LINE_LOOP);
  glVertex3f(404 + offsetx, 80+ offsety, 0);
  glVertex3f(404 + offsetx, 8 + offsety, 0);
  glVertex3f(440 + offsetx, 8 + offsety, 0);
  glVertex3f(440 + offsetx, 44+ offsety, 0);
  glVertex3f(465 + offsetx, 80+ offsety, 0);
  glEnd();
}

// Draw the numbers.
int drawNum(int n, int x ,int y, int s, int r, int g, int b) {
  for ( ; ; ) {
    drawLetter(n%10, x, y, s, 3, r, g, b);
    y += s*1.7f;
    n /= 10;
    if ( n <= 0 ) break;
  }
  return y;
}

int drawNumRight(int n, int x ,int y, int s, int r, int g, int b) {
  int d, nd, drawn = 0;
  for ( d = 100000000 ; d > 0 ; d /= 10 ) {
    nd = (int)(n/d);
    if ( nd > 0 || drawn ) {
      n -= d*nd;
      drawLetter(nd%10, x, y, s, 1, r, g, b);
      y += s*1.7f;
      drawn = 1;
    }
  }
  if ( !drawn ) {
    drawLetter(0, x, y, s, 1, r, g, b);
    y += s*1.7f;
  }
  return y;
}

int drawNumCenter(int n, int x ,int y, int s, int r, int g, int b) {
  for ( ; ; ) {
    drawLetter(n%10, x, y, s, 0, r, g, b);
    x -= s*1.7f;
    n /= 10;
    if ( n <= 0 ) break;
  }
  return y;
}

int drawTimeCenter(int n, int x ,int y, int s, int r, int g, int b) {
  int i;
  for ( i=0 ; i<7 ; i++ ) {
    if ( i != 4 ) {
      drawLetter(n%10, x, y, s, 0, r, g, b);
      n /= 10;
    } else {
      drawLetter(n%6, x, y, s, 0, r, g, b);
      n /= 6;
    }
    if ( (i&1) == 1 || i == 0 ) {
      switch ( i ) {
      case 3:
	drawLetter(41, x+s*1.16f, y, s, 0, r, g, b);
	break;
      case 5:
	drawLetter(40, x+s*1.16f, y, s, 0, r, g, b);
	break;
      }
      x -= s*1.7f;
    } else {
      x -= s*2.2f;
    }
    if ( n <= 0 ) break;
  }
  return y;
}

//#define JOYSTICK_AXIS 16384
#define JOYSTICK_AXIS 5000
int framecount = 0;

SDL_GameController* mainController = NULL;
SDL_GameController* get_main_controller()
{
    if (mainController == NULL)
    {
	int id = 0;
		if (framecount == 0)
		{
			id = refresh_input_device();
			framecount = 200;
		}
		
		for (int i = 0; i < 3; i++)
		{
			SDL_GameController* cont = SDL_GameControllerOpen(i);
			if (cont)
			{
				mainController = cont;
			}
		}
       // mainController = SDL_GameControllerOpen(SDL_JoystickGetDeviceInstanceID(id));
    }
	return mainController;
}

void RemapSticks(int inX, int inY, float& outX, float& outY)
{
	if (inX == 0 && inY == 0)
	{
		outX = 0;
		outY = 0;
		return;
	}
	const float g_AnalogStickDeadZoneFloat = 0.1f;
	const float g_AnalogStickDeadZoneFloatScale = 1.0f / (1.0f - g_AnalogStickDeadZoneFloat);

	const float TwoRootTwo = 2.0f * sqrt(2.0f);
	float FloatInX = ((float)inX / (float)32767);
	float FloatInY = ((float)inY / (float)32767);
	float SquareInX = FloatInX * FloatInX;
	float SquareInY = FloatInY * FloatInY;
	float a = 2.0 + TwoRootTwo * FloatInX + SquareInX - SquareInY;
	float aSign = a < 0.0f ? -1.0f : 1.0f;
	float b = 2.0 - TwoRootTwo * FloatInX + SquareInX - SquareInY;
	float bSign = b < 0.0f ? -1.0f : 1.0f;
	float c = 2.0 + TwoRootTwo * FloatInY - SquareInX + SquareInY;
	float cSign = c < 0.0f ? -1.0f : 1.0f;
	float d = 2.0 - TwoRootTwo * FloatInY - SquareInX + SquareInY;
	float dSign = d < 0.0f ? -1.0f : 1.0f;
	outX = 0.5f * aSign * sqrt(aSign * (a)) -
		0.5f * bSign * sqrt(bSign * (b));
	outY = 0.5f * cSign * sqrt(cSign * (c)) -
		0.5f * dSign * sqrt(dSign * (d));
	if (outX > g_AnalogStickDeadZoneFloat)
	{
		outX -= g_AnalogStickDeadZoneFloat;
		outX *= g_AnalogStickDeadZoneFloatScale;
	}
	else if (outX < -g_AnalogStickDeadZoneFloat)
	{
		outX += g_AnalogStickDeadZoneFloat;
		outX *= g_AnalogStickDeadZoneFloatScale;
	}
	else
	{
		outX = 0;
	}
	if (outY > g_AnalogStickDeadZoneFloat)
	{
		outY -= g_AnalogStickDeadZoneFloat;
		outY *= g_AnalogStickDeadZoneFloatScale;
	}
	else if (outY < -g_AnalogStickDeadZoneFloat)
	{
		outY += g_AnalogStickDeadZoneFloat;
		outY *= g_AnalogStickDeadZoneFloatScale;
	}
	else
	{
		outY = 0;
	}
}


PadState getPadState() {
    PadState state;
  int x = 0, y = 0;
  int hat = SDL_HAT_CENTERED;
  int pad = 0; 

  SDL_GameController* controller = get_main_controller();

  state.x = 0;
  state.y = 0;

  if (controller)
  {
	  x = SDL_GameControllerGetAxis(get_main_controller(), SDL_CONTROLLER_AXIS_LEFTX);
	  y = SDL_GameControllerGetAxis(get_main_controller(), SDL_CONTROLLER_AXIS_LEFTY);

	  if (SDL_GameControllerGetButton(controller,SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) {
		  pad |= PAD_RIGHT;
	  }
      if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT)) {
		  pad |= PAD_LEFT;
	  }
      if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN)) {
		  pad |= PAD_DOWN;
	  }
      if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP)) {
		  pad |= PAD_UP;
	  }

	  if (bSmoothJoystick)
	  {
		  RemapSticks(x, y, state.x, state.y);
	  }
	  else {

		  // 8 way joystick simulation.

		  int axis_deadzone = 5000;
		  
		  int ix = 0;
		  int iy = 0;

		  if (x > axis_deadzone)
		  {
			  ix = 1;
		  }
		  else if (x < -axis_deadzone)
		  {
			  ix = -1;
		  }

		  if (y > axis_deadzone)
		  {
			  iy = 1;
		  }
		  else if (y < -axis_deadzone)
		  {
			  iy = -1;
		  }
		
		  float mul = 1.f;
		  if (abs(ix) + abs(iy) == 2)
		  {
			  mul = 0.7f;
		  }

		  state.x = (float)ix * mul;
		  state.y = (float)iy * mul;
	  }

	  

	  //int axis_deadzone = 5000;
	  //double axis_divider = 32767.0 - axis_deadzone;
	  //
      //if (x > axis_deadzone)
      //{
      //    state.x = (x - axis_deadzone )/ axis_divider;
      //}
      //else if (x < -axis_deadzone)
	  //{
		//  state.x = (x + axis_deadzone) / axis_divider;
	  //}
	  //
	  //if (y > axis_deadzone)
	  //{
		//  state.y = (y - axis_deadzone) / axis_divider;
	  //}
	  //else if (y < -axis_deadzone)
	  //{
		//  state.y = (y + axis_deadzone) / axis_divider;
	  //}
  }

  //finger is touching, override x/y
  if (touch.TouchState)
  {
      state.x = touch.dx * 100.0 * touchsens;
      state.y = touch.dy * 100.0 * touchsens;
  }

      
  if ( keys[SDL_GetScancodeFromKey(SDLK_RIGHT)] == SDL_PRESSED  || x > JOYSTICK_AXIS || (hat & SDL_HAT_RIGHT)) {
    pad |= PAD_RIGHT;
  }
  if ( keys[SDL_GetScancodeFromKey(SDLK_LEFT)] == SDL_PRESSED || x < -JOYSTICK_AXIS || (hat & SDL_HAT_LEFT)) {
    pad |= PAD_LEFT;
  }
  if ( keys[SDL_GetScancodeFromKey(SDLK_DOWN)] == SDL_PRESSED || y > JOYSTICK_AXIS || (hat & SDL_HAT_DOWN)) {
    pad |= PAD_DOWN;
  }
  if ( keys[SDL_GetScancodeFromKey(SDLK_UP)] == SDL_PRESSED  || y < -JOYSTICK_AXIS || (hat & SDL_HAT_UP)) {
    pad |= PAD_UP;
  }
  state.pad = pad;
  return state;
}

int buttonReversed = 0;

int getButtonState() {
  int btn = 0;
  int btn1 = 0, btn2 = 0, btn3 = 0, btn4 = 0;
  int btn5 = 0, btn6 = 0, btn7 = 0, btn8 = 0, btn9 = 0;
 
  SDL_GameController* controller = get_main_controller();
  if (controller)
  {
	  btn1 = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B);
	  btn2 = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A);
	  btn3 = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_Y);
	  btn4 = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X);
	  btn5 = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
      btn6 = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START);

	  int lefttrigger = SDL_GameControllerGetAxis(get_main_controller(), SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 100;
	  int righttrigger = SDL_GameControllerGetAxis(get_main_controller(), SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 100;

      int trigcount = 0;
      if (lefttrigger) trigcount++;
      if (righttrigger) trigcount++;

      if (trigcount == 2)
      {
          btn2 = 1;
      }
      else if (trigcount == 1)
      {
          btn1 = 1;
      }
  }
  

  if (keys[SDL_GetScancodeFromKey(SDLK_z)] == SDL_PRESSED || btn1 || btn4) {
    if ( !buttonReversed ) {
      btn |= PAD_BUTTON1;
    } else {
      btn |= PAD_BUTTON2;
    }
  }
  if ( keys[SDL_GetScancodeFromKey(SDLK_x)] == SDL_PRESSED || btn2 || btn3 ) {
    if ( !buttonReversed ) {
      btn |= PAD_BUTTON2;
    } else {
      btn |= PAD_BUTTON1;
    }
  }
  if (keys [SDL_GetScancodeFromKey(SDLK_p)] == SDL_PRESSED || btn5 || btn6 || btn7 || btn8 || btn9) {
    btn |= PAD_BUTTONP;
  }
  return btn;
}

float touchsens = 1.f;

void refresh_touch_input()
{
	framecount--;

	int touchpress = 0;

    float aspect_ratio = (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH ;

    SDL_Event ev;

	touch.dx = 0;
	touch.dy = 0;

	touch.x = 0;
	touch.y = 0;

	if (mainController) 
	{
		if (!SDL_GameControllerGetAttached(mainController))
		{
		
			SDL_GameControllerClose(mainController);

			refresh_input_device();

			mainController = nullptr;
			goto end;
		}
	}

	while (SDL_PollEvent(&ev))
	{
        imgui_input(&ev);

		if ((ev.type == SDL_JOYDEVICEADDED || ev.type == SDL_JOYDEVICEREMOVED || ev.type == SDL_CONTROLLERDEVICEADDED || ev.type == SDL_CONTROLLERDEVICEREMOVED) && framecount == 0)
		{
			if (mainController)
			{
				SDL_GameControllerClose(mainController);
			}
			refresh_input_device();

			mainController = nullptr;

			goto end;		
		}

		if (ev.type == SDL_FINGERDOWN)
		{
            if (touch.TouchState == 0)
            {
                touch.TouchState = 1;
                printf("FINGER DOWN");

                touch.x = ev.tfinger.x;
                touch.y = ev.tfinger.y;
            }
		}
        else if (ev.type == SDL_FINGERMOTION)
        {
			touch.x = ev.tfinger.x;
			touch.y = ev.tfinger.y;

			touch.dx += ev.tfinger.dx;
            touch.dy += ev.tfinger.dy * aspect_ratio;
        }
        else if (ev.type == SDL_FINGERUP) {
			if (touch.TouchState == 1)
			{
				touch.TouchState = 0;
				printf("FINGER UP");
			}
        }
	}

	keys = const_cast<Uint8*>(SDL_GetKeyboardState(NULL));

end:
    imgui_newframe(Window);
}


GLint buildMips(GLenum target, GLint internalFormat,
	GLsizei width, GLsizei height,
	GLenum format, GLenum type,
	const void* data)
{
    glTexImage2D(target, 0,internalFormat, width, height,0, format, type, data);
    glGenerateMipmap(target);

    return 0;
}

