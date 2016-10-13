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
#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#endif

/* report GL errors, if any, to stderr */
void
checkError(const char* functionName)
{
  GLenum error;
  while ((error = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "GL error 0x%X detected in %s\n", error, functionName);
  }
}

static int draw_normal = 0;
static unsigned encoded_width;
static unsigned encoded_height;
static void* textureData;

/* vertex array data for a colored 2D triangle, consisting of RGB color values
   and XY coordinates */
const GLfloat varray[] = { -1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0 };

/* ISO C somehow enforces this silly use of 'enum' for compile-time constants */
enum
{
  numVertexComponents = 2,
  stride = sizeof(GLfloat) * (numVertexComponents),
  numElements = sizeof(varray) / stride
};

/* the name of the vertex buffer object */
GLuint vertexBufferName;
GLuint vertexArrayName;

void
initBuffer(void)
{
  /* Need to setup a vertex array as otherwise invalid operation errors can
   * occur when accessing vertex buffer (OpenGL 3.3 has no default zero named
   * vertex array)
   */
  glGenVertexArrays(1, &vertexArrayName);
  glBindVertexArray(vertexArrayName);

  glGenBuffers(1, &vertexBufferName);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBufferName);
  glBufferData(GL_ARRAY_BUFFER, sizeof(varray), varray, GL_STATIC_DRAW);
  checkError("initBuffer");
}

const char* vertexShaderSource[] = {
  "#version 140\n",
  "in vec4 a_Position;\n",
  "out vec2 v_Position;\n",
  "void main() {\n",
  "  gl_Position = a_Position;\n",
  "  v_Position = vec2(a_Position.xy);\r",
  "}\n",
};

static char** fragmentShaderSourceETC1;
static int fragmentShaderSourceETC1LineCount;

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

GLuint
compileShaderSource(GLenum type, GLsizei count, const char** string)
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

static GLuint a_Position;
static GLuint u_Texture;
static GLuint u_width;
static GLuint u_height;

static void
loadWorker(const char* name, char*** ppfragmentShaderSource, int* lineCount)
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
  myfragmentShaderSource = malloc(maxlinecount * sizeof(char*));
  if (!myfragmentShaderSource) {
    perror("fails to malloc ppfragmentShaderSource:");
    exit(1);
  }
  currentlinecount = 0;
  while ((line = fgets(lineBuffer, 256, f))) {
    char* storeline = strdup(line);
    if (maxlinecount == currentlinecount) {
      myfragmentShaderSource =
        realloc(myfragmentShaderSource, maxlinecount * 2 * sizeof(char*));
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
loadETC1FragProg(void)
{
  loadWorker("etc1frag.glsl", &fragmentShaderSourceETC1,
             &fragmentShaderSourceETC1LineCount);
}

void
initShader(void)
{
  loadETC1FragProg();
  const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(char*);
  GLuint vertexShader = compileShaderSource(GL_VERTEX_SHADER, vertexShaderLines,
                                            vertexShaderSource);

  GLuint fragmentShader =
    compileShaderSource(GL_FRAGMENT_SHADER, fragmentShaderSourceETC1LineCount,
                        (const char**)fragmentShaderSourceETC1);

  GLuint program = createProgram(vertexShader, fragmentShader);

  glUseProgram(program);

  u_Texture = glGetUniformLocation(program, "u_Texture");
  u_width = glGetUniformLocation(program, "u_width");
  u_height = glGetUniformLocation(program, "u_height");

  a_Position = glGetAttribLocation(program, "a_Position");
  glEnableVertexAttribArray(a_Position);

  checkError("initShader");
}

void
initRendering(void)
{
  glClearColor(0.0, 0.0, 0.0, 0.0);
  checkError("initRendering");
}

static void initTextureDatas(void);

void
init(void)
{
  GLenum err;
  glewExperimental = GL_TRUE;
  err = glewInit();
  if (GLEW_OK != err) {
    fprintf(stderr, "glew init fails");
    exit(1);
  }
  if (!GLEW_VERSION_3_3) {
    fprintf(stderr, "OpenGL 3.3 fails");
    exit(1);
  }
  initTextureDatas();
  initBuffer();
  initShader();
  initRendering();
  checkError("init");
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

const GLvoid*
bufferObjectPtr(GLsizei index)
{
  return (const GLvoid*)(((char*)NULL) + index);
}

GLfloat projectionMatrix[16];

static void
triangle_normal(void)
{
  GLuint textureObject;

  glBindBuffer(GL_ARRAY_BUFFER, vertexBufferName);
  glVertexAttribPointer(a_Position, numVertexComponents, GL_FLOAT, GL_FALSE,
                        stride, 0);
  // load texture
  glGenTextures(1, &textureObject);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textureObject);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, encoded_width / 4 * 8,
               encoded_height / 4, 0, GL_RED, GL_UNSIGNED_BYTE, textureData);
  checkError("triangle_normal glTexImage2D");
  glUniform1i(u_Texture, 0);
  glUniform1f(u_width, (float)encoded_width);
  glUniform1f(u_height, (float)encoded_height);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, numElements);
  glDeleteTextures(1, &textureObject);
  checkError("triangle_normal");
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
  if (draw_normal) {
    triangle_normal();
  } else {
    triangle_normal();
  }
  glFinish();
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
  checkError("display");
}

static void
reshape(int w, int h)
{
  glViewport(0, 0, encoded_width, encoded_height);
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
      draw_normal ^= 1;
      printf("switching the draw mode to %d.\n", draw_normal);
      glutPostRedisplay();
      break;
  }
}

void
samplemenu(int menuID)
{
}

#define ETC1_PKM_FORMAT_OFFSET 6
#define ETC1_PKM_ENCODED_WIDTH_OFFSET 8
#define ETC1_PKM_ENCODED_HEIGHT_OFFSET 10
#define ETC1_PKM_WIDTH_OFFSET 12
#define ETC1_PKM_HEIGHT_OFFSET 14
#define ETC1_HEADER_SIZE 16
#define ETC1_RGB_NO_MIPMAPS 0

static const char kMagic[] = { 'P', 'K', 'M', ' ', '1', '0' };

static unsigned
readBEUint16(const unsigned char* pIn)
{
  return (pIn[0] << 8) | pIn[1];
}

static int
etc1_pkm_is_valid(const unsigned char* pHeader, unsigned* pencodedWidth,
                  unsigned* pencodedHeight)
{
  if (memcmp(pHeader, kMagic, sizeof(kMagic))) {
    return 0;
  }
  unsigned format = readBEUint16(pHeader + ETC1_PKM_FORMAT_OFFSET);
  unsigned encodedWidth = readBEUint16(pHeader + ETC1_PKM_ENCODED_WIDTH_OFFSET);
  unsigned encodedHeight =
    readBEUint16(pHeader + ETC1_PKM_ENCODED_HEIGHT_OFFSET);
  unsigned width = readBEUint16(pHeader + ETC1_PKM_WIDTH_OFFSET);
  unsigned height = readBEUint16(pHeader + ETC1_PKM_HEIGHT_OFFSET);
  *pencodedWidth = encodedWidth;
  *pencodedHeight = encodedHeight;
  return format == ETC1_RGB_NO_MIPMAPS && encodedWidth >= width &&
         encodedWidth - width < 4 && encodedHeight >= height &&
         encodedHeight - height < 4;
}

static void
initTextureDatas(void)
{
  FILE* f;
  struct stat inputstat;
  unsigned char* header;

  f = fopen("input.pkm", "rb");
  if (0 != stat("input.pkm", &inputstat)) {
    perror("fails to get stat");
    exit(1);
  }
  printf("input.pkm: size: %d\n", (int)inputstat.st_size);
  header = malloc(inputstat.st_size);
  if (!header) {
    perror("fails to malloc textureData");
    exit(1);
  }
  fread(header, 1, inputstat.st_size, f);
  fclose(f);
  if (!etc1_pkm_is_valid(header, &encoded_width, &encoded_height)) {
    fprintf(stderr, "fail to test etc1 texture.\n");
    exit(1);
  }
  printf("encoded_width: %d, encoded_height: %d.\n", encoded_width,
         encoded_height);
  textureData = header + ETC1_HEADER_SIZE;
}

int
main(int argc, char** argv)
{
  int menuA;
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  /* add command line argument "classic" for a pre-3.x context */
  if ((argc != 2) || (strcmp(argv[1], "classic") != 0)) {
    glutInitContextVersion(3, 3);
    glutInitContextFlags(GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);
  }
  glutInitWindowSize(500, 500);
  glutInitWindowPosition(100, 100);
  glutCreateWindow(argv[0]);
  dumpInfo();
  init();
  glutReshapeWindow(encoded_width, encoded_height);
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
