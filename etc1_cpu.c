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
#if !defined(_WIN32)
#include <unistd.h>
#endif
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

#define ETC1_PKM_FORMAT_OFFSET 6
#define ETC1_PKM_ENCODED_WIDTH_OFFSET 8
#define ETC1_PKM_ENCODED_HEIGHT_OFFSET 10
#define ETC1_PKM_WIDTH_OFFSET 12
#define ETC1_PKM_HEIGHT_OFFSET 14
#define ETC1_HEADER_SIZE 16
#define ETC1_RGB_NO_MIPMAPS 0
#define ETC1_DECODED_BLOCK_SIZE 48
#define ETC1_ENCODED_BLOCK_SIZE 8

#define false 0
#define true 1
typedef unsigned char etc1_byte;
typedef unsigned int etc1_uint32;
typedef int bool;
static unsigned encoded_width;
static unsigned encoded_height;
static void* textureData;

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

static const int kModifierTable[] = {
  /* 0 */ 2,  8,   -2,  -8,
  /* 1 */ 5,  17,  -5,  -17,
  /* 2 */ 9,  29,  -9,  -29,
  /* 3 */ 13, 42,  -13, -42,
  /* 4 */ 18, 60,  -18, -60,
  /* 5 */ 24, 80,  -24, -80,
  /* 6 */ 33, 106, -33, -106,
  /* 7 */ 47, 183, -47, -183
};

static const int kLookup[8] = { 0, 1, 2, 3, -4, -3, -2, -1 };

static inline etc1_byte
clamp(int x)
{
  return (etc1_byte)(x >= 0 ? (x < 255 ? x : 255) : 0);
}

static inline int
convert4To8(int b)
{
  int c = b & 0xf;
  return (c << 4) | c;
}

static inline int
convert5To8(int b)
{
  int c = b & 0x1f;
  return (c << 3) | (c >> 2);
}

static inline int
convert6To8(int b)
{
  int c = b & 0x3f;
  return (c << 2) | (c >> 4);
}

static inline int
divideBy255(int d)
{
  return (d + 128 + (d >> 8)) >> 8;
}

static inline int
convert8To4(int b)
{
  int c = b & 0xff;
  return divideBy255(c * 15);
}

static inline int
convert8To5(int b)
{
  int c = b & 0xff;
  return divideBy255(c * 31);
}

static inline int
convertDiff(int base, int diff)
{
  return convert5To8((0x1f & base) + kLookup[0x7 & diff]);
}

static void
decode_subblock(etc1_byte* pOut, int r, int g, int b, const int* table,
                etc1_uint32 low, bool second, bool flipped)
{
  int baseX = 0;
  int baseY = 0;
  if (second) {
    if (flipped) {
      baseY = 2;
    } else {
      baseX = 2;
    }
  }
  for (int i = 0; i < 8; i++) {
    int x, y;
    if (flipped) {
      x = baseX + (i >> 1);
      y = baseY + (i & 1);
    } else {
      x = baseX + (i >> 2);
      y = baseY + (i & 3);
    }
    int k = y + (x * 4);
    int offset = ((low >> k) & 1) | ((low >> (k + 15)) & 2);
    int delta = table[offset];
    etc1_byte* q = pOut + 3 * (x + 4 * y);
    *q++ = clamp(r + delta);
    *q++ = clamp(g + delta);
    *q++ = clamp(b + delta);
  }
}

// Input is an ETC1 compressed version of the data.
// Output is a 4 x 4 square of 3-byte pixels in form R, G, B

void
etc1_decode_block(const etc1_byte* pIn, etc1_byte* pOut)
{
  etc1_uint32 high = (pIn[0] << 24) | (pIn[1] << 16) | (pIn[2] << 8) | pIn[3];
  etc1_uint32 low = (pIn[4] << 24) | (pIn[5] << 16) | (pIn[6] << 8) | pIn[7];
  int r1, r2, g1, g2, b1, b2;
  if (high & 2) {
    // differential
    int rBase = high >> 27;
    int gBase = high >> 19;
    int bBase = high >> 11;
    r1 = convert5To8(rBase);
    r2 = convertDiff(rBase, high >> 24);
    g1 = convert5To8(gBase);
    g2 = convertDiff(gBase, high >> 16);
    b1 = convert5To8(bBase);
    b2 = convertDiff(bBase, high >> 8);
  } else {
    // not differential
    r1 = convert4To8(high >> 28);
    r2 = convert4To8(high >> 24);
    g1 = convert4To8(high >> 20);
    g2 = convert4To8(high >> 16);
    b1 = convert4To8(high >> 12);
    b2 = convert4To8(high >> 8);
  }
  int tableIndexA = 7 & (high >> 5);
  int tableIndexB = 7 & (high >> 2);
  const int* tableA = kModifierTable + tableIndexA * 4;
  const int* tableB = kModifierTable + tableIndexB * 4;
  bool flipped = (high & 1) != 0;
  decode_subblock(pOut, r1, g1, b1, tableA, low, false, flipped);
  decode_subblock(pOut, r2, g2, b2, tableB, low, true, flipped);
}

static int
etc1_decode_image(const etc1_byte* pIn, etc1_byte* pOut, etc1_uint32 width,
                  etc1_uint32 height, etc1_uint32 pixelSize, etc1_uint32 stride)
{
  etc1_byte block[ETC1_DECODED_BLOCK_SIZE];

  etc1_uint32 encodedWidth = (width + 3) & ~3;
  etc1_uint32 encodedHeight = (height + 3) & ~3;

  for (etc1_uint32 y = 0; y < encodedHeight; y += 4) {
    etc1_uint32 yEnd = height - y;
    if (yEnd > 4) {
      yEnd = 4;
    }
    for (etc1_uint32 x = 0; x < encodedWidth; x += 4) {
      etc1_uint32 xEnd = width - x;
      if (xEnd > 4) {
        xEnd = 4;
      }
      etc1_decode_block(pIn, block);
      pIn += ETC1_ENCODED_BLOCK_SIZE;
      for (etc1_uint32 cy = 0; cy < yEnd; cy++) {
        const etc1_byte* q = block + (cy * 4) * 3;
        etc1_byte* p = pOut + pixelSize * x + stride * (y + cy);
        if (pixelSize == 3) {
          memcpy(p, q, xEnd * 3);
        } else {
          for (etc1_uint32 cx = 0; cx < xEnd; cx++) {
            etc1_byte r = *q++;
            etc1_byte g = *q++;
            etc1_byte b = *q++;
            etc1_uint32 pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
            *p++ = (etc1_byte)pixel;
            *p++ = (etc1_byte)(pixel >> 8);
          }
        }
      }
    }
  }
  return 0;
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

/* vertex array data for a colored 2D triangle, consisting of RGB color values
   and XY coordinates */
const GLfloat varray[] = {
  0.0f, 1.0f, 5.0f,  5.0f,

  0.0f, 0.0f, 5.0f,  25.0f,

  1.0f, 0.0f, 25.0f, 25.0f,

  1.0f, 1.0f, 25.f,  5.0f,
};

/* ISO C somehow enforces this silly use of 'enum' for compile-time constants */
enum
{
  numColorComponents = 2,
  numVertexComponents = 2,
  stride = sizeof(GLfloat) * (numColorComponents + numVertexComponents),
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
  "uniform mat4 fg_ProjectionMatrix;\n",
  "in vec2 tex_coord_in;\n",
  "in vec4 fg_Vertex;\n",
  "smooth out vec4 fg_SmoothColor;\n",
  "smooth out vec2 tex_coord;\n",
  "void main()\n",
  "{\n",
  "   tex_coord = tex_coord_in;\n",
  "   gl_Position = fg_ProjectionMatrix * fg_Vertex;\n",
  "}\n"
};

const char* fragmentShaderSource[] = {
  "#version 140\n", "smooth in vec4 fg_SmoothColor;\n",
  "smooth in vec2 tex_coord;\n", "uniform sampler2D myt;\n"
                                 "out vec4 fg_FragColor;\n",
  "void main(void)\n", "{\n", "   fg_FragColor = texture(myt, tex_coord);\n",
  "}\n"
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

GLuint fgProjectionMatrixIndex;
GLuint texCoordinateIn;
GLuint fgVertexIndex;
GLuint mytIndex;

void
initShader(void)
{
  const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(char*);
  GLuint vertexShader = compileShaderSource(GL_VERTEX_SHADER, vertexShaderLines,
                                            vertexShaderSource);

  const GLsizei fragmentShaderLines =
    sizeof(fragmentShaderSource) / sizeof(char*);
  GLuint fragmentShader = compileShaderSource(
    GL_FRAGMENT_SHADER, fragmentShaderLines, fragmentShaderSource);

  GLuint program = createProgram(vertexShader, fragmentShader);

  glUseProgram(program);

  fgProjectionMatrixIndex =
    glGetUniformLocation(program, "fg_ProjectionMatrix");
  mytIndex = glGetUniformLocation(program, "myt");

  texCoordinateIn = glGetAttribLocation(program, "tex_coord_in");
  glEnableVertexAttribArray(texCoordinateIn);

  fgVertexIndex = glGetAttribLocation(program, "fg_Vertex");
  glEnableVertexAttribArray(fgVertexIndex);

  checkError("initShader");
}

void
initRendering(void)
{
  glClearColor(0.0, 0.0, 0.0, 0.0);
  checkError("initRendering");
}

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
  FILE* file;
  unsigned char* data;
  GLuint textureObject;

  glUniformMatrix4fv(fgProjectionMatrixIndex, 1, GL_FALSE, projectionMatrix);

  glBindBuffer(GL_ARRAY_BUFFER, vertexBufferName);
  glVertexAttribPointer(texCoordinateIn, numColorComponents, GL_FLOAT, GL_FALSE,
                        stride, bufferObjectPtr(0));
  glVertexAttribPointer(fgVertexIndex, numVertexComponents, GL_FLOAT, GL_FALSE,
                        stride,
                        bufferObjectPtr(sizeof(GLfloat) * numColorComponents));
  // load texture
  glGenTextures(1, &textureObject);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textureObject);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  data = malloc(encoded_width * encoded_height * 3);
  etc1_decode_image(textureData, data, encoded_width, encoded_height, 3,
                    encoded_width * 3);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, encoded_width, encoded_height, 0,
               GL_RGB, GL_UNSIGNED_BYTE, data);
  if (data) {
    free(data);
  }
  glUniform1i(mytIndex, 0);
  glDrawArrays(GL_TRIANGLE_FAN, 0, numElements);
  glDeleteTextures(1, &textureObject);
}

static void
triangle(void)
{
  GLint tmpFramebuffer;
  GLint tmpRenderbuffer;
  GLint viewport[4];
  GLenum err;

  glGetIntegerv(GL_VIEWPORT, viewport);
  glGenFramebuffers(1, &tmpFramebuffer);
  glGenRenderbuffers(1, &tmpRenderbuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, tmpFramebuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, tmpRenderbuffer);
  glRenderbufferStorageMultisample(GL_RENDERBUFFER, 16, GL_RGBA, viewport[2],
                                   viewport[3]);
  err = glGetError();
  if (err != GL_NO_ERROR) {
    fprintf(stderr, "error occurs: %X.\n", err);
    exit(1);
  }
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_RENDERBUFFER, tmpRenderbuffer);
  if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
    fprintf(stderr, "fbo is not completed.\n");
    exit(1);
  }
  triangle_normal();
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  // blit here
  glBindFramebuffer(GL_READ_FRAMEBUFFER, tmpFramebuffer);
  glBlitFramebuffer(0, 0, viewport[2], viewport[3], 0, 0, viewport[2],
                    viewport[3], GL_COLOR_BUFFER_BIT, GL_LINEAR);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  glDeleteFramebuffers(1, &tmpFramebuffer);
  glDeleteRenderbuffers(1, &tmpRenderbuffer);
  checkError("triangle");
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

void
loadOrthof(GLfloat* m, GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n,
           GLfloat f)
{
  m[0] = 2.0f / (r - l);
  m[1] = 0.0f;
  m[2] = 0.0f;
  m[3] = 0.0f;

  m[4] = 0.0f;
  m[5] = 2.0f / (t - b);
  m[6] = 0.0f;
  m[7] = 0.0f;

  m[8] = 0.0f;
  m[9] = 0.0f;
  m[10] = -2.0f / (f - n);
  m[11] = 0.0f;

  m[12] = -(r + l) / (r - l);
  m[13] = -(t + b) / (t - b);
  m[14] = -(f + n) / (f - n);
  m[15] = 1.0f;
}

void
loadOrtho2Df(GLfloat* m, GLfloat l, GLfloat r, GLfloat b, GLfloat t)
{
  loadOrthof(m, l, r, b, t, -1.0f, 1.0f);
}

static void
reshape(int w, int h)
{
  glViewport(0, 0, (GLsizei)w, (GLsizei)h);
  if (w <= h) {
    loadOrtho2Df(projectionMatrix, 0.0f, 30.0f, 0.0f,
                 30.0f * (GLfloat)h / (GLfloat)w);
  } else {
    loadOrtho2Df(projectionMatrix, 0.0f, 30.0f * (GLfloat)w / (GLfloat)h, 0.0f,
                 30.0f);
  }
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
