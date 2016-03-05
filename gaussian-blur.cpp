/*
 * smooth_opengl3.c, based on smooth.c, which is (c) by SGI, see below.
 * This program demonstrates smooth shading in a way which is fully
 * OpenGL-3.1-compliant.
 * A smooth shaded polygon is drawn in a 2-D projection.
 */

/*
 * Original copyright notice from smooth.c:
 *
 * License Applicability. Except to the extent portions of this file are
 * made subject to an alternative license as permitted in the SGI Free
 * Software License B, Version 1.1 (the "License"), the contents of this
 * file are subject only to the provisions of the License. You may not use
 * this file except in compliance with the License. You may obtain a copy
 * of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
 * Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
 *
 * http://oss.sgi.com/projects/FreeB
 *
 * Note that, as provided in the License, the Software is distributed on an
 * "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
 * DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
 * CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
 * PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 *
 * Original Code. The Original Code is: OpenGL Sample Implementation,
 * Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
 * Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
 * Copyright in any portions created by third parties is as indicated
 * elsewhere herein. All Rights Reserved.
 *
 * Additional Notice Provisions: The application programming interfaces
 * established by SGI in conjunction with the Original Code are The
 * OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
 * April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
 * 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
 * Window System(R) (Version 1.3), released October 19, 1998. This software
 * was created using the OpenGL(R) version 1.2.1 Sample Implementation
 * published by SGI, but has not been independently verified as being
 * compliant with the OpenGL(R) version 1.2.1 Specification.
 *
 */

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <nvImage.h>
#include <memory>
#include <cmath>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#endif

#define ENABLE_THRESHOLD_TEST 0

static std::unique_ptr<nv::Image> g_image;
static std::vector<GLfloat> g_kernel;

static GLint vPositionIndexRow;
static GLint uTextureRow;
static GLint uScreenGeometryRow;
static GLint uKernelRow;
static GLint g_programRow;

static GLint vPositionIndexColumn;
static GLint uTextureColumn;
static GLint uScreenGeometryColumn;
static GLint uKernelColumn;
static GLint g_programColumn;

static char** gaussianFragRowSource;
static int gaussianFragRowSourceLineCount;
static char** gaussianFragColumnSource;
static int gaussianFragColumnSourceLineCount;

static GLint vPositionIndexRender;
static GLint uTextureRender;
static GLint uScreenGeometryRender;
static GLint g_programRender;

#ifdef ENABLE_THRESHOLD_TEST
static char** thresholdSource;
static int thresholdSourceLine;
static GLint vPositionIndexThreshold;
static GLint uTextureOrigThreshold;
static GLint uTextureBlurThreshold;
static GLint uScreenGeometryThreshold;
static GLint g_programThreshold;
#endif // ENABLE_THRESHOLD_TEST
static const GLint g_block_size = 91;

static std::vector<GLfloat> getGaussianKernel(int n);

static nv::Image*
gaussianLoadImageFromFile(const char* file)
{
  std::unique_ptr<nv::Image> image(new nv::Image());

  if (!image->loadImageFromFile(file)) {
    return NULL;
  }

  return image.release();
}
/* report GL errors, if any, to stderr */
void
checkError(const char* functionName)
{
  GLenum error;
  while ((error = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "GL error 0x%X detected in %s\n", error, functionName);
  }
}

/* the name of the vertex buffer object */
GLuint vertexBufferName;
GLuint vertexArrayName;

const char* vertexShaderSource[] = { "#version 120\n",
                                     "attribute vec4 v_position;\n",
                                     "void main()\n",
                                     "{\n",
                                     "   gl_Position = v_position;\n",
                                     "}\n" };

const char* fragmentShaderSource[] = {
  "#version 120\n", "uniform sampler2D u_texture;\n",
  "uniform ivec2 u_screenGeometry;\n"
  "void main(void)\n",
  "{\n", "   vec2 texcoord = gl_FragCoord.xy / vec2(u_screenGeometry);\n",
  "   gl_FragColor = texture2D(u_texture, texcoord);\n", "}\n"
};

void
compileAndCheck(GLuint shader)
{
  GLint status;
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE) {
    GLint infoLogLength;
    char* infoLog;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
    infoLog = (char*)malloc(infoLogLength);
    glGetShaderInfoLog(shader, infoLogLength, NULL, infoLog);
    fprintf(stderr, "compile log: %s\n", infoLog);
    free(infoLog);
  }
}

static GLuint
compileShaderSource(GLenum type, GLsizei count, char const** string)
{
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, count, string, NULL);
  compileAndCheck(shader);
  return shader;
}

void
linkAndCheck(GLuint program)
{
  GLint status;
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE) {
    GLint infoLogLength;
    char* infoLog;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
    infoLog = (char*)malloc(infoLogLength);
    glGetProgramInfoLog(program, infoLogLength, NULL, infoLog);
    fprintf(stderr, "link log: %s\n", infoLog);
    free(infoLog);
  }
}

GLuint
createProgram(GLuint vertexShader, GLuint fragmentShader)
{
  GLuint program = glCreateProgram();
  if (vertexShader != 0) {
    glAttachShader(program, vertexShader);
  }
  if (fragmentShader != 0) {
    glAttachShader(program, fragmentShader);
  }
  linkAndCheck(program);
  return program;
}

static void
gaussianLoadWorker(const char* name, char*** ppfragmentShaderSource,
                   int* lineCount)
{
  FILE* f;
  struct stat mystat;
  char** myfragmentShaderSource;
  char lineBuffer[256];
  char* line;
  int maxlinecount, currentlinecount;

  f = fopen(name, "r");
  if (!f) {
    fprintf(stderr, "fails to open etc1frag.glsl.\n");
    exit(1);
  }
  if (0 != stat(name, &mystat)) {
    perror("fails to get stat");
    exit(1);
  }
  maxlinecount = 16;
  myfragmentShaderSource =
    static_cast<char**>(malloc(maxlinecount * sizeof(char*)));
  if (!myfragmentShaderSource) {
    perror("fails to malloc ppfragmentShaderSource:");
    exit(1);
  }
  currentlinecount = 0;
  while (line = fgets(lineBuffer, 256, f)) {
    char* storeline = strdup(line);
    if (maxlinecount == currentlinecount) {
      myfragmentShaderSource = static_cast<char**>(
        realloc(myfragmentShaderSource, maxlinecount * 2 * sizeof(char*)));
      if (!myfragmentShaderSource) {
        fprintf(stderr, "fails to realloc ppfragmentShaderSource to: %d",
                maxlinecount * 2);
        exit(1);
      }
      maxlinecount *= 2;
    }
    myfragmentShaderSource[currentlinecount++] = storeline;
  }
  *lineCount = currentlinecount;
  fclose(f);
  *ppfragmentShaderSource = myfragmentShaderSource;
}

static void
loadGaussianFragProg(void)
{
  gaussianLoadWorker("gaussian-row.glsl", &gaussianFragRowSource,
                     &gaussianFragRowSourceLineCount);
  gaussianLoadWorker("gaussian-column.glsl", &gaussianFragColumnSource,
                     &gaussianFragColumnSourceLineCount);
}

static void
initShader(void)
{
  loadGaussianFragProg();
  const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(char*);
  GLuint vertexShader = compileShaderSource(GL_VERTEX_SHADER, vertexShaderLines,
                                            vertexShaderSource);

  const GLsizei fragmentShaderLines =
    sizeof(fragmentShaderSource) / sizeof(char*);
  GLuint fragmentRowShader =
    compileShaderSource(GL_FRAGMENT_SHADER, gaussianFragRowSourceLineCount,
                        const_cast<const char**>(gaussianFragRowSource));

  GLuint fragmentColumnShader =
    compileShaderSource(GL_FRAGMENT_SHADER, gaussianFragColumnSourceLineCount,
                        const_cast<const char**>(gaussianFragColumnSource));
  GLuint fragmentRender = compileShaderSource(
    GL_FRAGMENT_SHADER, fragmentShaderLines, fragmentShaderSource);

  g_programRow = createProgram(vertexShader, fragmentRowShader);
  g_programColumn = createProgram(vertexShader, fragmentColumnShader);
  g_programRender = createProgram(vertexShader, fragmentRender);
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentRowShader);
  glDeleteShader(fragmentColumnShader);
  glDeleteShader(fragmentRender);
  GLint program = g_programRow;
  vPositionIndexRow = glGetAttribLocation(program, "v_position");
  printf("vPositionIndexRow: %d.\n", vPositionIndexRow);
  uTextureRow = glGetUniformLocation(program, "u_texture");
  uScreenGeometryRow = glGetUniformLocation(program, "u_screenGeometry");
  uKernelRow = glGetUniformLocation(program, "u_kernel");
  printf("uTextureRow: %d, uScreenGeometryRow: %d, uKernelRow: %d.\n",
         uTextureRow, uScreenGeometryRow, uKernelRow);

  program = g_programColumn;

  vPositionIndexColumn = glGetAttribLocation(program, "v_position");
  printf("vPositionIndexColumn: %d.\n", vPositionIndexColumn);
  uTextureColumn = glGetUniformLocation(program, "u_texture");
  uScreenGeometryColumn = glGetUniformLocation(program, "u_screenGeometry");
  uKernelColumn = glGetUniformLocation(program, "u_kernel");
  printf("uTextureColumn: %d, uScreenGeometryColumn: %d, uKernelColumn: %d.\n",
         uTextureColumn, uScreenGeometryColumn, uKernelColumn);

  program = g_programRender;

  vPositionIndexRender = glGetAttribLocation(program, "v_position");
  printf("vPositionIndexRender: %d.\n", vPositionIndexRender);
  uTextureRender = glGetUniformLocation(program, "u_texture");
  uScreenGeometryRender = glGetUniformLocation(program, "u_screenGeometry");
  printf("uTextureRender: %d, uScreenGeometryRender: %d.\n", uTextureRender,
         uScreenGeometryRender);
  checkError("initShader");
}

void
initRendering(void)
{
  glClearColor(0.0, 0.0, 0.0, 0.0);
  checkError("initRendering");
}

static GLuint g_texture;
void
initTexture(void)
{
  glGenTextures(1, &g_texture);
  glBindTexture(GL_TEXTURE_2D, g_texture);
  nv::Image* image = g_image.get();
  glTexImage2D(GL_TEXTURE_2D, 0, image->getInternalFormat(), image->getWidth(),
               image->getHeight(), 0, image->getFormat(), image->getType(),
               image->getLevel(0));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void
init(void)
{
  GLenum err;
  err = glewInit();
  if (GLEW_OK != err) {
    fprintf(stderr, "glew init fails");
    exit(1);
  }
  if (!GLEW_VERSION_2_0) {
    fprintf(stderr, "OpenGL 3.3 fails");
    exit(1);
  }
  initTexture();
  initShader();
  initRendering();
  g_kernel = std::move(getGaussianKernel(g_block_size));
}

void
dumpInfo(void)
{
  printf("Vendor: %s\n", glGetString(GL_VENDOR));
  printf("Renderer: %s\n", glGetString(GL_RENDERER));
  printf("Version: %s\n", glGetString(GL_VERSION));
  printf("GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
  checkError("dumpInfo");
}

static std::vector<GLfloat>
getGaussianKernel(int n)
{
  std::vector<GLfloat> kernel(n);
  float* cf = const_cast<float*>(kernel.data());

  double sigmaX = ((n - 1) * 0.5 - 1) * 0.3 + 0.8;
  double scale2X = -0.5 / (sigmaX * sigmaX);
  double sum = 0;

  int i;
  for (i = 0; i < n; i++) {
    double x = i - (n - 1) * 0.5;
    double t = std::exp(scale2X * x * x);
    {
      cf[i] = (float)t;
      sum += cf[i];
    }
  }

  sum = 1. / sum;
  for (i = 0; i < n; i++) {
    cf[i] = (float)(cf[i] * sum);
  }

  return kernel;
}

static void
allocateTexture(GLuint texture, GLint width, GLint height, GLint iformat,
                GLint format)
{
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, iformat, width, height, 0, format,
               GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

static void
triangle_normal(void)
{
  float positions[][4] = {
    { -1.0, 1.0, 0.0, 1.0 },
    { -1.0, -1.0, 0.0, 1.0 },
    { 1.0, 1.0, 0.0, 1.0 },
    { 1.0, -1.0, 0.0, 1.0 },
  };
  GLuint tmpFramebuffer;
  GLuint tmpTexture[2];
  nv::Image* image = g_image.get();
  // allocate fbo and a texture
  glGenFramebuffers(1, &tmpFramebuffer);
  glGenTextures(2, tmpTexture);
  allocateTexture(tmpTexture[0], image->getWidth(), image->getHeight(), GL_RGB,
                  GL_RGB);
  allocateTexture(tmpTexture[1], image->getWidth(), image->getHeight(), GL_RGB,
                  GL_RGB);
  // bind fbo and complete it.
  glBindFramebuffer(GL_FRAMEBUFFER, tmpFramebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         tmpTexture[0], 0);
  if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
    fprintf(stderr, "fbo is not completed.\n");
    exit(1);
  }
  GLint imageGeometry[2] = { g_image->getWidth(), g_image->getHeight() };
  glVertexAttribPointer(vPositionIndexRow, 4, GL_FLOAT, GL_FALSE, 0, positions);
  glEnableVertexAttribArray(vPositionIndexRow);
  // setup uniforms
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, g_texture);
  glUseProgram(g_programRow);
  glUniform1i(uTextureRow, 0);

  glUniform2iv(uScreenGeometryRow, 1, imageGeometry);
  // setup kernel and block size

  glUniform1fv(uKernelRow, g_block_size, g_kernel.data());
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  // bind fbo and complete it.
  glBindFramebuffer(GL_FRAMEBUFFER, tmpFramebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         tmpTexture[1], 0);
  if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
    fprintf(stderr, "fbo is not completed.\n");
    exit(1);
  }
  glDisableVertexAttribArray(vPositionIndexRow);

  glVertexAttribPointer(vPositionIndexColumn, 4, GL_FLOAT, GL_FALSE, 0,
                        positions);
  glEnableVertexAttribArray(vPositionIndexColumn);
  // setup uniforms
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tmpTexture[0]);
  glUseProgram(g_programColumn);
  glUniform1i(uTextureColumn, 0);

  glUniform2iv(uScreenGeometryColumn, 1, imageGeometry);
  // setup kernel and block size

  glUniform1fv(uKernelColumn, g_block_size, g_kernel.data());
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableVertexAttribArray(vPositionIndexColumn);
  // render to screen
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glEnableVertexAttribArray(vPositionIndexRender);
  // setup uniforms
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tmpTexture[1]);
  glUseProgram(g_programRender);
  glUniform1i(uTextureRender, 0);

  glUniform2iv(uScreenGeometryRender, 1, imageGeometry);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableVertexAttribArray(vPositionIndexRender);
  glDeleteTextures(2, tmpTexture);
  glDeleteFramebuffers(1, &tmpFramebuffer);
}

static void
display(void)
{
#ifdef _WIN32
  LARGE_INTEGER t1, t2, freq;
  QueryPerformanceCounter(&t1);
#else
  struct timespec t1, t2;
  clock_gettime(CLOCK_MONOTONIC, &t1);
#endif
  glClear(GL_COLOR_BUFFER_BIT);
  triangle_normal();
  glutSwapBuffers();
  checkError("display");
#ifdef _WIN32
  QueryPerformanceCounter(&t2);
  QueryPerformanceFrequency(&freq);
  printf("one frame: %lf.\n",
         (double)(t2.QuadPart - t1.QuadPart) / freq.QuadPart);
#else
  clock_gettime(CLOCK_MONOTONIC, &t2);
  printf("one frame: %lf.\n", ((double)(t2.tv_sec - t1.tv_sec) +
                               ((double)(t2.tv_nsec - t1.tv_nsec) / 1e9)));
#endif
}

static void
reshape(int w, int h)
{
  glViewport(0, 0, (GLsizei)w, (GLsizei)h);
  checkError("reshape");
}

static void
keyboard(unsigned char key, int x, int y)
{
  switch (key) {
    case 27:
      exit(0);
      break;
    case ' ':
      glutPostRedisplay();
      break;
  }
}

void
samplemenu(int menuID)
{
}

int
main(int argc, char** argv)
{
  int menuA;
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  if (argc != 2) {
    fprintf(stderr, "need a file name.\n");
    exit(1);
  }
  g_image.reset(gaussianLoadImageFromFile(argv[1]));
  if (g_image.get() == nullptr) {
    fprintf(stderr, "fails to load image.\n");
    exit(1);
  }
  glutInitWindowSize(g_image->getWidth(), g_image->getHeight());
  glutInitWindowPosition(100, 100);
  glutCreateWindow(argv[0]);
  dumpInfo();
  init();
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);

  /* Add a menu. They have their own context and should thus work with forward
   * compatible main windows too. */
  menuA = glutCreateMenu(samplemenu);
  glutAddMenuEntry("Sub menu A1 (01)", 1);
  glutAddMenuEntry("Sub menu A2 (02)", 2);
  glutAddMenuEntry("Sub menu A3 (03)", 3);
  glutSetMenu(menuA);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  glutMainLoop();
  return 0;
}
