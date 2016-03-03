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

const char* vertexShaderSource[] = { "#version 140\n",
                                     "in vec4 a_Position;\n",
                                     "smooth out vec2 v_position;\n",
                                     "void main()\n",
                                     "{\n",
                                     "   gl_Position = a_Position;\n",
                                     "   v_position = a_Position.xy;\n",
                                     "}\n" };

const char* fragmentShaderSource[] = {
  "#version 140\n", "smooth in vec2 v_position;\n", "out vec4 outputColor;\n",
  "uniform sampler2D myt;\n", "void main(void)\n", "{\n",
  "   outputColor = texture(myt, vec2((v_position.x + 1.0) / 2.0, "
  "(v_position.y + 1.0) / 2.0));\n",
  "}\n"
};

static char** computeShaderSourceETC1;
static int computeShaderSourceETC1LineCount;
static char** computeShaderSourceS3TC;
static int computeShaderSourceS3TCLineCount;
static GLuint renderProgram;
static GLuint computeProgramETC1;
static GLuint computeProgramS3TC;

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

static GLint
checkProgram(GLuint program, const char* type)
{
  GLint status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE) {
    GLint infoLogLength;
    char* infoLog;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
    infoLog = (char*)malloc(infoLogLength);
    glGetProgramInfoLog(program, infoLogLength, NULL, infoLog);
    fprintf(stderr, "%s link log: %s\n", type, infoLog);
    free(infoLog);
  }
  return status;
}

static void
linkAndCheck(GLuint program)
{
  GLint programBinaryLength;

  glLinkProgram(program);
  if (!checkProgram(program, "render")) {
    exit(1);
  }
  programBinaryLength = 0;
  glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &programBinaryLength);
  if (programBinaryLength > 0) {
    void* buf;
    GLenum format;
    GLsizei length;
    FILE* file;

    printf("programBinaryLength: %d.\n", programBinaryLength);
    buf = malloc(programBinaryLength);
    glGetProgramBinary(program, programBinaryLength, &length, &format, buf);
    printf("programBinaryFormat: %x.\n", format);
    file = fopen("p_binary.dat", "wb");
    if (file) {
      printf("write binary to p_binary.dat, length: %d.\n", length);
      fwrite(buf, 1, length, file);
      fclose(file);
    }
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
static GLuint u_TextureRender;
static GLuint u_TextureComputeETC1;
static GLuint u_OutComputeETC1;
static GLuint u_TextureComputeS3TC;

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
  while (line = fgets(lineBuffer, 256, f)) {
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
  loadWorker("etc1frag_cs.glsl", &computeShaderSourceETC1,
             &computeShaderSourceETC1LineCount);
  loadWorker("s3tc_encode.glsl", &computeShaderSourceS3TC,
             &computeShaderSourceS3TCLineCount);
}

void
initShader(void)
{
  int maxImageUnits;
  loadETC1FragProg();
  const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(char*);
  GLuint vertexShader = compileShaderSource(GL_VERTEX_SHADER, vertexShaderLines,
                                            vertexShaderSource);

  const GLsizei fragmentShaderLines =
    sizeof(fragmentShaderSource) / sizeof(char*);
  GLuint fragmentShader = compileShaderSource(
    GL_FRAGMENT_SHADER, fragmentShaderLines, fragmentShaderSource);

  GLuint program = createProgram(vertexShader, fragmentShader);

  checkError("createProgram");

  u_TextureRender = glGetUniformLocation(program, "myt");
  checkError("GetRenderUniform");

  a_Position = glGetAttribLocation(program, "a_Position");
  glEnableVertexAttribArray(a_Position);
  checkError("initRender");

  computeProgramETC1 =
    glCreateShaderProgramv(GL_COMPUTE_SHADER, computeShaderSourceETC1LineCount,
                           (const char**)computeShaderSourceETC1);
  if (!checkProgram(computeProgramETC1, "computeProgramETC1")) {
    exit(1);
  }
  renderProgram = program;
  u_TextureComputeETC1 = glGetUniformLocation(computeProgramETC1, "u_Texture");
  u_OutComputeETC1 = glGetUniformLocation(computeProgramETC1, "u_Output");
  printf(
    "u_TextureRender: %u, u_TextureComputeETC1: %u, u_OutComputeETC1: %u.\n",
    u_TextureRender, u_TextureComputeETC1, u_OutComputeETC1);

  computeProgramS3TC =
    glCreateShaderProgramv(GL_COMPUTE_SHADER, computeShaderSourceS3TCLineCount,
                           (const char**)computeShaderSourceS3TC);
  if (!checkProgram(computeProgramS3TC, "computeProgramS3TC")) {
    exit(1);
  }
  u_TextureComputeS3TC = glGetUniformLocation(computeProgramS3TC, "u_Texture");
  printf("u_TextureComputeS3TC: %u.\n", u_TextureComputeS3TC);

  glGetIntegerv(GL_MAX_IMAGE_UNITS, &maxImageUnits);
  printf("maxImageUnits: %d.\n", maxImageUnits);
  checkError("initCompute");
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
  if (!GLEW_VERSION_4_3) {
    fprintf(stderr, "OpenGL 4.3 fails");
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
  FILE* file;
  GLubyte* data;
  GLuint textureObject[3];
  GLuint pbo[1];

  // load texture
  glGenTextures(3, textureObject);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textureObject[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, encoded_width / 4 * 8,
               encoded_height / 4, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE,
               textureData);
  glBindTexture(GL_TEXTURE_2D, textureObject[1]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, encoded_width, encoded_height, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  // compute etc1
  glUseProgram(computeProgramETC1);
  // assume output image unit is 0
  glUniform1i(u_OutComputeETC1, 0);
  // assume input image unit is 1
  glUniform1i(u_TextureComputeETC1, 1);
  glBindImageTexture(0, textureObject[1], 0, GL_FALSE, 0, GL_WRITE_ONLY,
                     GL_RGBA8);
  glBindImageTexture(1, textureObject[0], 0, GL_FALSE, 0, GL_READ_ONLY,
                     GL_R8UI);
  glDispatchCompute(encoded_width / 4, encoded_height / 4, 1);
  // image memory barrier
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  // compute s3tc
  glUseProgram(computeProgramS3TC);
  glGenBuffers(1, pbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, pbo[0]);
  glBufferData(GL_SHADER_STORAGE_BUFFER, encoded_width * encoded_height / 2,
               NULL, GL_DYNAMIC_COPY);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, pbo[0]);
  glBindImageTexture(0, textureObject[1], 0, GL_FALSE, 0, GL_READ_ONLY,
                     GL_RGBA8);
  // assume input image unit is 1
  glUniform1i(u_TextureComputeS3TC, 0);
  glDispatchCompute(encoded_width / 4, encoded_height / 4, 1);
  // pixel buffer barrier
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_PIXEL_BUFFER_BARRIER_BIT);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  // init the render texture
  glBindTexture(GL_TEXTURE_2D, textureObject[2]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[0]);
  glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
                         encoded_width, encoded_height, 0,
                         encoded_width * encoded_height / 2, NULL);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  // render
  glUseProgram(renderProgram);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBufferName);
  glVertexAttribPointer(a_Position, numVertexComponents, GL_FLOAT, GL_FALSE,
                        stride, 0);
  glUniform1i(u_TextureRender, 0);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, numElements);

  glDeleteTextures(3, textureObject);
  glDeleteBuffers(1, pbo);
  checkError("triangle_normal");
}

static void
triangle(void)
{
  GLint tmpFramebuffer;
  GLint tmpRenderbuffer;
  GLint viewport[4];
  GLenum err;

  checkError("triangle enter");
  glGetIntegerv(GL_VIEWPORT, viewport);
  glGenFramebuffers(1, (GLuint*)&tmpFramebuffer);
  glGenRenderbuffers(1, (GLuint*)&tmpRenderbuffer);
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
  glDeleteFramebuffers(1, (GLuint*)&tmpFramebuffer);
  glDeleteRenderbuffers(1, (GLuint*)&tmpRenderbuffer);
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
    glutInitContextVersion(4, 3);
    glutInitContextFlags(GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);
  }
  glutInitWindowSize(500, 500);
  glutInitWindowPosition(100, 100);
  glutCreateWindow(argv[0]);
  dumpInfo();
  init();
  // glutReshapeWindow(encoded_width, encoded_height);
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
