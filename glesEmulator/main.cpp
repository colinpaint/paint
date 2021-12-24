// main.cpp - minimal arm OPENGLES emulator example
//{{{  includes
#define WIN32_LEAN_AND_MEAN 1
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "GLES2/gl2.h"
#include "EGL/egl.h"
//}}}

//{{{
struct vec3 {
  float x, y, z;
  };
//}}}
//{{{
#define GL_CHECK(x) \
        x; \
        { \
          GLenum glError = glGetError(); \
          if(glError != GL_NO_ERROR) { \
            fprintf(stderr, "glGetError() = %i (0x%.8x) at line %i\n", glError, glError, __LINE__); \
            exit(1); \
          } \
        }
//}}}
//{{{
#define EGL_CHECK(x) \
    x; \
    { \
        EGLint eglError = eglGetError(); \
        if(eglError != EGL_SUCCESS) { \
            fprintf(stderr, "eglGetError() = %i (0x%.8x) at line %i\n", eglError, eglError, __LINE__); \
            exit(1); \
        } \
    }
//}}}
#define M_PI 3.14159265358979323846

HWND hWindow;
HDC  hDisplay;
//{{{
/* 3D data. Vertex range -0.5..0.5 in all axes.
* Z -0.5 is near, 0.5 is far. */
const float aVertices[] =
{
    /* Front face. */
    /* Bottom left */
    -0.5,  0.5, -0.5,
    0.5, -0.5, -0.5,
    -0.5, -0.5, -0.5,
    /* Top right */
    -0.5,  0.5, -0.5,
    0.5,  0.5, -0.5,
    0.5, -0.5, -0.5,
    /* Left face */
    /* Bottom left */
    -0.5,  0.5,  0.5,
    -0.5, -0.5, -0.5,
    -0.5, -0.5,  0.5,
    /* Top right */
    -0.5,  0.5,  0.5,
    -0.5,  0.5, -0.5,
    -0.5, -0.5, -0.5,
    /* Top face */
    /* Bottom left */
    -0.5,  0.5,  0.5,
    0.5,  0.5, -0.5,
    -0.5,  0.5, -0.5,
    /* Top right */
    -0.5,  0.5,  0.5,
    0.5,  0.5,  0.5,
    0.5,  0.5, -0.5,
    /* Right face */
    /* Bottom left */
    0.5,  0.5, -0.5,
    0.5, -0.5,  0.5,
    0.5, -0.5, -0.5,
    /* Top right */
    0.5,  0.5, -0.5,
    0.5,  0.5,  0.5,
    0.5, -0.5,  0.5,
    /* Back face */
    /* Bottom left */
    0.5,  0.5,  0.5,
    -0.5, -0.5,  0.5,
    0.5, -0.5,  0.5,
    /* Top right */
    0.5,  0.5,  0.5,
    -0.5,  0.5,  0.5,
    -0.5, -0.5,  0.5,
    /* Bottom face */
    /* Bottom left */
    -0.5, -0.5, -0.5,
    0.5, -0.5,  0.5,
    -0.5, -0.5,  0.5,
    /* Top right */
    -0.5, -0.5, -0.5,
    0.5, -0.5, -0.5,
    0.5, -0.5,  0.5,
};
//}}}
//{{{
const float aColours[] =
{
    /* Front face */
    /* Bottom left */
    1.0, 0.0, 0.0, /* red */
    0.0, 0.0, 1.0, /* blue */
    0.0, 1.0, 0.0, /* green */
    /* Top right */
    1.0, 0.0, 0.0, /* red */
    1.0, 1.0, 0.0, /* yellow */
    0.0, 0.0, 1.0, /* blue */
    /* Left face */
    /* Bottom left */
    1.0, 1.0, 1.0, /* white */
    0.0, 1.0, 0.0, /* green */
    0.0, 1.0, 1.0, /* cyan */
    /* Top right */
    1.0, 1.0, 1.0, /* white */
    1.0, 0.0, 0.0, /* red */
    0.0, 1.0, 0.0, /* green */
    /* Top face */
    /* Bottom left */
    1.0, 1.0, 1.0, /* white */
    1.0, 1.0, 0.0, /* yellow */
    1.0, 0.0, 0.0, /* red */
    /* Top right */
    1.0, 1.0, 1.0, /* white */
    0.0, 0.0, 0.0, /* black */
    1.0, 1.0, 0.0, /* yellow */
    /* Right face */
    /* Bottom left */
    1.0, 1.0, 0.0, /* yellow */
    1.0, 0.0, 1.0, /* magenta */
    0.0, 0.0, 1.0, /* blue */
    /* Top right */
    1.0, 1.0, 0.0, /* yellow */
    0.0, 0.0, 0.0, /* black */
    1.0, 0.0, 1.0, /* magenta */
    /* Back face */
    /* Bottom left */
    0.0, 0.0, 0.0, /* black */
    0.0, 1.0, 1.0, /* cyan */
    1.0, 0.0, 1.0, /* magenta */
    /* Top right */
    0.0, 0.0, 0.0, /* black */
    1.0, 1.0, 1.0, /* white */
    0.0, 1.0, 1.0, /* cyan */
    /* Bottom face */
    /* Bottom left */
    0.0, 1.0, 0.0, /* green */
    1.0, 0.0, 1.0, /* magenta */
    0.0, 1.0, 1.0, /* cyan */
    /* Top right */
    0.0, 1.0, 0.0, /* green */
    0.0, 0.0, 1.0, /* blue */
    1.0, 0.0, 1.0, /* magenta */
};
//}}}

//{{{
/*
 * process_window(): This function handles Windows callbacks.
 */
LRESULT CALLBACK process_window (HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam) {

  switch (uiMsg) {
    case WM_CLOSE:
        PostQuitMessage (0);
        return 0;

    case WM_ACTIVATE:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SIZE:
        return 0;
  }

  return DefWindowProc (hWnd, uiMsg, wParam, lParam);
}
//}}}
//{{{
/*
 * create_window(): Set up Windows specific bits.
 *
 * uiWidth:  Width of window to create.
 * uiHeight:  Height of window to create.
 *
 * Returns:  Device specific window handle.
 */
HWND create_window (int uiWidth, int uiHeight) {

  WNDCLASS wc;
  RECT wRect;
  HWND sWindow;
  HINSTANCE hInstance;

  wRect.left = 0L;
  wRect.right = (long)uiWidth;
  wRect.top = 0L;
  wRect.bottom = (long)uiHeight;

  hInstance = GetModuleHandle(NULL);

  wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wc.lpfnWndProc = (WNDPROC)process_window;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NULL;
  wc.lpszClassName = "OGLES";

  RegisterClass (&wc);

  AdjustWindowRectEx (&wRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);

  sWindow = CreateWindowEx (WS_EX_APPWINDOW | WS_EX_WINDOWEDGE, "OGLES", "main", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, uiWidth, uiHeight, NULL, NULL, hInstance, NULL);

  ShowWindow (sWindow, SW_SHOW);
  SetForegroundWindow (sWindow);
  SetFocus (sWindow);

  return sWindow;
}
//}}}

//{{{
char* loadShader(const char* fileName)
{
  char *pResult = NULL;
  FILE *pFile = NULL;
  long iLen = 0;

  pFile = fopen(fileName, "r");

  if (pFile == NULL) {
    fprintf(stderr, "Error: Cannot read file '%s'\n", fileName);
    return NULL;
    }

  fseek(pFile, 0, SEEK_END); /* Seek end of file */
  iLen = ftell(pFile);
  fseek(pFile, 0, SEEK_SET); /* Seek start of file again */
  pResult = (char*)calloc(iLen + 1, sizeof(char));
  fread(pResult, sizeof(char), iLen, pFile);
  pResult[iLen] = '\0';
  fclose(pFile);

  return pResult;
}
//}}}
//{{{

void processShader(GLuint * pShader, const char * fileName, GLint shaderType)
{
  GLint iStatus;
  const char *aStrings[1] = { NULL };
  *pShader = glCreateShader(shaderType);
  aStrings[0] = loadShader(fileName);
  glShaderSource(*pShader, 1, aStrings, NULL);
  free((void *)aStrings[0]);
  aStrings[0] = NULL;
  glCompileShader(*pShader);
  glGetShaderiv(*pShader, GL_COMPILE_STATUS, &iStatus);
}
//}}}
//{{{
/*
 * Loads the shader source into memory.
 *
 * sFilename: String holding filename to load
 */
char* load_shader(char *sFilename) {

  char *pResult = NULL;
  FILE *pFile = NULL;
  long iLen = 0;

  pFile = fopen(sFilename, "r");

  if(pFile == NULL) {
      fprintf(stderr, "Error: Cannot read file '%s'\n", sFilename);
    exit(-1);
  }

  fseek(pFile, 0, SEEK_END); /* Seek end of file */
  iLen = ftell(pFile);
  fseek(pFile, 0, SEEK_SET); /* Seek start of file again */
  pResult = (char*)calloc(iLen+1, sizeof(char));
  fread(pResult, sizeof(char), iLen, pFile);
  pResult[iLen] = '\0';
  fclose(pFile);

  return pResult;
  }
//}}}
//{{{
/*
 * Create shader, load in source, compile, dump debug as necessary.
 *
 * pShader: Pointer to return created shader ID.
 * sFilename: Passed-in filename from which to load shader source.
 * iShaderType: Passed to GL, e.g. GL_VERTEX_SHADER.
 */
void process_shader(GLuint *pShader, char *sFilename, GLint iShaderType) {

  GLint iStatus;
  const char *aStrings[1] = { NULL };

  /* Create shader and load into GL. */
  *pShader = GL_CHECK(glCreateShader(iShaderType));

  aStrings[0] = load_shader(sFilename);

  GL_CHECK(glShaderSource(*pShader, 1, aStrings, NULL));

  /* Clean up shader source. */
  free((void *)aStrings[0]);
  aStrings[0] = NULL;

  /* Try compiling the shader. */
  GL_CHECK(glCompileShader(*pShader));
  GL_CHECK(glGetShaderiv(*pShader, GL_COMPILE_STATUS, &iStatus));

  // Dump debug info (source and log) if compilation failed.
  if(iStatus != GL_TRUE) {
    exit(-1);
  }
}
//}}}

//{{{
/*
 * Simulates desktop's glRotatef. The matrix is returned in column-major
 * order.
 */
void rotate_matrix(double angle, double x, double y, double z, float *R) {

  double radians, c, s, c1, u[3], length;
  int i, j;

  radians = (angle * M_PI) / 180.0;

  c = cos(radians);
  s = sin(radians);

  c1 = 1.0 - cos(radians);

  length = sqrt(x * x + y * y + z * z);

  u[0] = x / length;
  u[1] = y / length;
  u[2] = z / length;

  for (i = 0; i < 16; i++) {
      R[i] = 0.0;
  }

  R[15] = 1.0;

  for (i = 0; i < 3; i++) {
      R[i * 4 + (i + 1) % 3] = u[(i + 2) % 3] * s;
      R[i * 4 + (i + 2) % 3] = -u[(i + 1) % 3] * s;
  }

  for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) {
          R[i * 4 + j] += c1 * u[i] * u[j] + (i == j ? c : 0.0);
      }
  }
}
//}}}
//{{{
/*
 * Simulates gluPerspectiveMatrix
 */
void perspective_matrix(double fovy, double aspect, double znear, double zfar, float *P) {

  int i;
  double f;

  f = 1.0/tan(fovy * 0.5);

  for (i = 0; i < 16; i++) {
      P[i] = 0.0;
  }

  P[0] = f / aspect;
  P[5] = f;
  P[10] = (znear + zfar) / (znear - zfar);
  P[11] = -1.0;
  P[14] = (2.0 * znear * zfar) / (znear - zfar);
  P[15] = 0.0;
}
//}}}
//{{{
/*
 * Multiplies A by B and writes out to C. All matrices are 4x4 and column
 * major. In-place multiplication is supported.
 */
void multiply_matrix(float *A, float *B, float *C) {

  int i, j, k;
  float aTmp[16];

  for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++) {
          aTmp[j * 4 + i] = 0.0;

          for (k = 0; k < 4; k++) {
              aTmp[j * 4 + i] += A[k * 4 + i] * B[j * 4 + k];
          }
      }
  }

  for (i = 0; i < 16; i++) {
      C[i] = aTmp[i];
  }
}
//}}}

int main(int argc, char **argv) {

  // EGL Configuration /
  EGLint aEGLAttributes[] = {
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_DEPTH_SIZE, 16,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
    };

  EGLint aEGLContextAttributes[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
    };

  EGLConfig aEGLConfigs[1];
  EGLint cEGLConfigs;


  GLint iLocPosition = 0;
  GLint iLocColour, iLocMVP;
  GLuint uiProgram, uiFragShader, uiVertShader;
  int bDone = 0;

  const unsigned int uiWidth  = 640;
  const unsigned int uiHeight = 480;

  int iXangle = 0, iYangle = 0, iZangle = 0;
  float aLightPos[] = { 0.0f, 0.0f, -1.0f }; // Light is nearest camera.
  float aRotate[16], aModelView[16], aPerspective[16], aMVP[16];

  unsigned char* myPixels = (unsigned char*)calloc (1, 128*128*4); // Holds texture data.
  unsigned char* myPixels2 = (unsigned char*)calloc (1, 128*128*4); // Holds texture data.

  hDisplay = EGL_DEFAULT_DISPLAY;
  EGLDisplay sEGLDisplay = EGL_CHECK (eglGetDisplay(hDisplay));
  EGL_CHECK (eglInitialize (sEGLDisplay, NULL, NULL));
  EGL_CHECK (eglChooseConfig (sEGLDisplay, aEGLAttributes, aEGLConfigs, 1, &cEGLConfigs));

  if (cEGLConfigs == 0) {
    printf("No EGL configurations were returned.\n");
    exit(-1);
    }

  hWindow = create_window (uiWidth, uiHeight);

  EGLSurface sEGLSurface = EGL_CHECK (eglCreateWindowSurface (sEGLDisplay, aEGLConfigs[0], (EGLNativeWindowType)hWindow, NULL));
  if (sEGLSurface == EGL_NO_SURFACE) {
    printf("Failed to create EGL surface.\n");
    exit(-1);
    }

  EGLContext sEGLContext = EGL_CHECK (eglCreateContext(sEGLDisplay, aEGLConfigs[0], EGL_NO_CONTEXT, aEGLContextAttributes));
  if (sEGLContext == EGL_NO_CONTEXT) {
    printf("Failed to create EGL context.\n");
    exit(-1);
    }

  EGL_CHECK (eglMakeCurrent (sEGLDisplay, sEGLSurface, sEGLSurface, sEGLContext));

  // Shader Initialisation */
  process_shader (&uiVertShader, "shader.vert", GL_VERTEX_SHADER);
  process_shader (&uiFragShader, "shader.frag", GL_FRAGMENT_SHADER);

  // Create uiProgram (ready to attach shaders) */
  uiProgram = GL_CHECK (glCreateProgram());

  // Attach shaders and link uiProgram */
  GL_CHECK (glAttachShader(uiProgram, uiVertShader));
  GL_CHECK (glAttachShader(uiProgram, uiFragShader));
  GL_CHECK (glLinkProgram(uiProgram));

  // Get attribute locations of non-fixed attributes like colour and texture coordinates. */
  iLocPosition = GL_CHECK (glGetAttribLocation(uiProgram, "av4position"));
  iLocColour = GL_CHECK (glGetAttribLocation(uiProgram, "av3colour"));
  printf("iLocPosition = %i\n", iLocPosition);
  printf("iLocColour   = %i\n", iLocColour);

  // Get uniform locations */
  iLocMVP = GL_CHECK (glGetUniformLocation(uiProgram, "mvp"));
  printf("iLocMVP      = %i\n", iLocMVP);

  GL_CHECK (glUseProgram (uiProgram));

  /* Enable attributes for position, colour and texture coordinates etc. */
  GL_CHECK (glEnableVertexAttribArray(iLocPosition));
  GL_CHECK (glEnableVertexAttribArray(iLocColour));

  /* Populate attributes for position, colour and texture coordinates etc. */
  GL_CHECK (glVertexAttribPointer (iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, aVertices));
  GL_CHECK (glVertexAttribPointer (iLocColour, 3, GL_FLOAT, GL_FALSE, 0, aColours));

  GL_CHECK (glEnable (GL_CULL_FACE));
  GL_CHECK (glEnable (GL_DEPTH_TEST));

  // Enter event loop
  MSG sMessage;
  while (!bDone) {
    if (PeekMessage (&sMessage, NULL, 0, 0, PM_REMOVE)) {
      if (sMessage.message == WM_QUIT)
        bDone = 1;
      else {
        TranslateMessage (&sMessage);
        DispatchMessage (&sMessage);
        }
      }

    rotate_matrix (iXangle, 1.0, 0.0, 0.0, aModelView);
    rotate_matrix (iYangle, 0.0, 1.0, 0.0, aRotate);
    multiply_matrix (aRotate, aModelView, aModelView);
    rotate_matrix (iZangle, 0.0, 1.0, 0.0, aRotate);
    multiply_matrix (aRotate, aModelView, aModelView);
    aModelView[14] -= 2.5;
    perspective_matrix (45.0, (double)uiWidth/(double)uiHeight, 0.01, 100.0, aPerspective);
    multiply_matrix (aPerspective, aModelView, aMVP);
    GL_CHECK (glUniformMatrix4fv (iLocMVP, 1, GL_FALSE, aMVP));

    iXangle += 3;
    iYangle += 2;
    iZangle += 1;

    if (iXangle >= 360)
      iXangle -= 360;
    if (iXangle < 0)
      iXangle += 360;
    if (iYangle >= 360)
      iYangle -= 360;
    if (iYangle < 0)
      iYangle += 360;
    if (iZangle >= 360)
      iZangle -= 360;
    if (iZangle < 0)
      iZangle += 360;

    GL_CHECK (glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
    GL_CHECK (glDrawArrays(GL_TRIANGLES, 0, 36));

    if (!eglSwapBuffers (sEGLDisplay, sEGLSurface))
      printf ("Failed to swap buffers.\n");

    Sleep (20);
    }

  // Cleanup shaders */
  GL_CHECK (glUseProgram(0));
  GL_CHECK (glDeleteShader(uiVertShader));
  GL_CHECK (glDeleteShader(uiFragShader));
  GL_CHECK (glDeleteProgram(uiProgram));

  // EGL clean up */
  EGL_CHECK (eglMakeCurrent(sEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
  EGL_CHECK (eglDestroySurface(sEGLDisplay, sEGLSurface));
  EGL_CHECK (eglDestroyContext(sEGLDisplay, sEGLContext));
  EGL_CHECK (eglTerminate(sEGLDisplay));

  return 0;
  }
