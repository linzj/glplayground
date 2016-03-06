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

struct ImageDesc
{
  GLint width, height;
  GLenum format;
  void* data;
};

static std::unique_ptr<nv::Image> g_image;
static const int g_maxVal = 211;

class ImageProcessor
{
public:
    ImageProcessor();
    ~ImageProcessor();
    bool init(int maxValue);
    void process(const ImageDesc& desc);
    std::vector<GLfloat> m_kernel;

    GLint m_vPositionIndexRow;
    GLint m_uTextureRow;
    GLint m_uScreenGeometryRow;
    GLint m_uKernelRow;
    GLint m_programRow;

    GLint m_vPositionIndexColumn;
    GLint m_uTextureColumn;
    GLint m_uScreenGeometryColumn;
    GLint m_uKernelColumn;
    GLint m_programColumn;

    GLint m_vPositionIndexRender;
    GLint m_uTextureRender;
    GLint m_uScreenGeometryRender;
    GLint m_programRender;
    GLint m_maxValue;

#if defined(ENABLE_THRESHOLD_TEST) && (ENABLE_THRESHOLD_TEST == 1)
    GLint m_vPositionIndexThreshold;
    GLint m_uTextureOrigThreshold;
    GLint m_uTextureBlurThreshold;
    GLint m_uScreenGeometryThresholdg;
    GLint m_uMaxValueThreshold;
    GLint m_programThreshold;
#endif // ENABLE_THRESHOLD_TEST
private:
    static const GLint s_block_size = 91;
    void initGaussianBlurKernel();
    bool initProgram();
    void allocateTexture(GLuint texture, GLint width, GLint height, GLenum format, void* data = nullptr);
    static std::vector<GLfloat> getGaussianKernel(int n);
};

ImageProcessor::ImageProcessor()
    : m_vPositionIndexRow(0)
    , m_uTextureRow(0)
    , m_uScreenGeometryRow(0)
    , m_uKernelRow(0)
    , m_programRow(0)
    , m_vPositionIndexColumn(0)
    , m_uTextureColumn(0)
    , m_uScreenGeometryColumn(0)
    , m_uKernelColumn(0)
    , m_programColumn(0)
    , m_vPositionIndexRender(0)
    , m_uTextureRender(0)
    , m_uScreenGeometryRender(0)
    , m_programRender(0)
     , m_maxValue(0)
#if defined(ENABLE_THRESHOLD_TEST) && (ENABLE_THRESHOLD_TEST == 1)
    , m_vPositionIndexThreshold(0)
    , m_uTextureOrigThreshold(0)
    , m_uTextureBlurThreshold(0)
    , m_uScreenGeometryThresholdg(0)
    , m_uMaxValueThreshold(0)
    , m_programThreshold(0)
#endif // ENABLE_THRESHOLD_TEST
{}

ImageProcessor::~ImageProcessor()
{
    if (m_programRow) {
        glDeleteProgram(m_programRow);
    }
    if (m_programColumn) {
        glDeleteProgram(m_programColumn);
    }
    if (m_programRender) {
        glDeleteProgram(m_programRender);
    }
#if defined(ENABLE_THRESHOLD_TEST) && (ENABLE_THRESHOLD_TEST == 1)
    if (m_programThreshold) {
        glDeleteProgram(m_programThreshold);
    }
#endif // ENABLE_THRESHOLD_TEST
}

bool ImageProcessor::init(int maxValue)
{
    m_maxValue = maxValue;
    initGaussianBlurKernel();
    return initProgram();
}

std::vector<GLfloat>
ImageProcessor::getGaussianKernel(int n)
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


void ImageProcessor::initGaussianBlurKernel()
{
    m_kernel = std::move(getGaussianKernel(s_block_size));
}

static const char* vertexShaderSource = "#version 120\n"
                                     "attribute vec4 v_position;\n"
                                     "void main()\n"
                                     "{\n"
                                     "   gl_Position = v_position;\n"
                                     "}\n";

static const char* renderTexture = 
  "#version 120\n" "uniform sampler2D u_texture;\n"
  "uniform ivec2 u_screenGeometry;\n"
  "void main(void)\n"
  "{\n"
  "   vec2 texcoord = gl_FragCoord.xy / vec2(u_screenGeometry);\n"
  "   gl_FragColor = texture2D(u_texture, texcoord);\n"
  "}\n";

static const char* gaussianFragRowSource =
"#version 120\n"
"uniform sampler2D u_texture;\n"
"uniform ivec2 u_screenGeometry;\n"
"uniform float u_kernel[91];\n"
"const int c_blockSize = 91;\n"
"\n"
"void main(void)\n"
"{\n"
"    int i;\n"
"    vec2 texcoord = (gl_FragCoord.xy - ivec2(c_blockSize / 2, 0)) / vec2(u_screenGeometry);\n"
"    float toffset = 1.0 / u_screenGeometry.x;\n"
"    vec3 color = texture2D(u_texture, texcoord).rgb * vec3(u_kernel[0]);\n"
"    for (i = 1; i < c_blockSize; ++i) {\n"
"        color += texture2D(u_texture, texcoord + vec2(i * toffset, 0.0)).rgb * vec3(u_kernel[i]);\n"
"    }\n"
"    gl_FragColor = vec4(color, 1.0);\n"
"}\n";

static const char* gaussianFragColumnSource =
"#version 120\n"
"uniform sampler2D u_texture;\n"
"uniform ivec2 u_screenGeometry;\n"
"uniform float u_kernel[91];\n"
"const int c_blockSize = 91;\n"
"\n"
"void main(void)\n"
"{\n"
"    int i;\n"
"    vec2 texcoord = (gl_FragCoord.xy + ivec2(0, c_blockSize / 2)) / vec2(u_screenGeometry);\n"
"    float toffset = 1.0 / u_screenGeometry.y;\n"
"    vec3 color = texture2D(u_texture, texcoord).rgb * vec3(u_kernel[0]);\n"
"    for (i = 1; i < c_blockSize; ++i) {\n"
"        color += texture2D(u_texture, texcoord - vec2(0.0, i * toffset)).rgb * vec3(u_kernel[i]);\n"
"    }\n"
"    gl_FragColor = vec4(color, 1.0);\n"
"}\n";

#if defined(ENABLE_THRESHOLD_TEST) && (ENABLE_THRESHOLD_TEST == 1)
static const char* thresholdFragSource =
"#version 120\n"
"\n"
"uniform float u_maxValue;\n"
"uniform ivec2 u_screenGeometry;\n"
"uniform sampler2D u_textureOrig;\n"
"uniform sampler2D u_textureBlur;\n"
"\n"
"void main(void)\n"
"{\n"
"    vec2 texcoord = gl_FragCoord.xy / vec2(u_screenGeometry);\n"
"    vec3 colorOrig = texture2D(u_textureOrig, texcoord).rgb;\n"
"    vec3 colorBlur = texture2D(u_textureBlur, texcoord).rgb;\n"
"    vec3 result;\n"
"    result.r = colorOrig.r > colorBlur.r ? u_maxValue : 0.0;\n"
"    result.g = colorOrig.g > colorBlur.g ? u_maxValue : 0.0;\n"
"    result.b = colorOrig.b > colorBlur.b ? u_maxValue : 0.0;\n"
"    gl_FragColor = vec4(result, 1.0);\n"
"}\n";
#endif // ENABLE_THRESHOLD_TEST

/* report GL errors, if any, to stderr */
static bool 
checkError(const char* functionName)
{
  GLenum error;
  bool entered = false;
  while ((error = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "GL error 0x%X detected in %s\n", error, functionName);
    entered = true;
  }
  return !entered;
}

static void
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

static void
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

static GLuint
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

bool ImageProcessor::initProgram()
{
  GLuint vertexShader = compileShaderSource(GL_VERTEX_SHADER, 1,
                                            &vertexShaderSource);

  GLuint fragmentRowShader = compileShaderSource(
    GL_FRAGMENT_SHADER, 1, &gaussianFragRowSource);

  GLuint fragmentColumnShader = compileShaderSource(
    GL_FRAGMENT_SHADER, 1, &gaussianFragColumnSource);
  GLuint fragmentRender = compileShaderSource(
    GL_FRAGMENT_SHADER, 1, &renderTexture);

  m_programRow = createProgram(vertexShader, fragmentRowShader);
  m_programColumn = createProgram(vertexShader, fragmentColumnShader);
  m_programRender = createProgram(vertexShader, fragmentRender);
  GLint program = m_programRow;
  m_vPositionIndexRow = glGetAttribLocation(program, "v_position");
  printf("m_vPositionIndexRow: %d.\n", m_vPositionIndexRow);
  m_uTextureRow = glGetUniformLocation(program, "u_texture");
  m_uScreenGeometryRow = glGetUniformLocation(program, "u_screenGeometry");
  m_uKernelRow = glGetUniformLocation(program, "u_kernel");
  printf("m_uTextureRow: %d, m_uScreenGeometryRow: %d, m_uKernelRow: %d.\n",
         m_uTextureRow, m_uScreenGeometryRow, m_uKernelRow);

  program = m_programColumn;

  m_vPositionIndexColumn = glGetAttribLocation(program, "v_position");
  printf("m_vPositionIndexColumn: %d.\n", m_vPositionIndexColumn);
  m_uTextureColumn = glGetUniformLocation(program, "u_texture");
  m_uScreenGeometryColumn = glGetUniformLocation(program, "u_screenGeometry");
  m_uKernelColumn = glGetUniformLocation(program, "u_kernel");
  printf("m_uTextureColumn: %d, m_uScreenGeometryColumn: %d, m_uKernelColumn: %d.\n",
         m_uTextureColumn, m_uScreenGeometryColumn, m_uKernelColumn);

  program = m_programRender;

  m_vPositionIndexRender = glGetAttribLocation(program, "v_position");
  printf("m_vPositionIndexRender: %d.\n", m_vPositionIndexRender);
  m_uTextureRender = glGetUniformLocation(program, "u_texture");
  m_uScreenGeometryRender = glGetUniformLocation(program, "u_screenGeometry");
  printf("m_uTextureRender: %d, m_uScreenGeometryRender: %d.\n", m_uTextureRender,
         m_uScreenGeometryRender);
#if defined(ENABLE_THRESHOLD_TEST) && (ENABLE_THRESHOLD_TEST == 1)
  GLuint fragmentThreshold = compileShaderSource(
    GL_FRAGMENT_SHADER, 1, &thresholdFragSource);
  m_programThreshold = createProgram(vertexShader, fragmentThreshold);
  glDeleteShader(fragmentThreshold);
  program = m_programThreshold;

  m_vPositionIndexThreshold = glGetAttribLocation(program, "v_position");
  printf("m_vPositionIndexThreshold: %d.\n", m_vPositionIndexThreshold);
  m_uTextureOrigThreshold = glGetUniformLocation(program, "u_textureOrig");
  m_uTextureBlurThreshold = glGetUniformLocation(program, "u_textureBlur");
  m_uScreenGeometryThresholdg = glGetUniformLocation(program, "u_screenGeometry");
  m_uMaxValueThreshold = glGetUniformLocation(program, "u_maxValue");
  printf("m_uTextureOrigThreshold: %d, m_uTextureBlurThreshold: %d, "
         "m_uScreenGeometryRender: %d, m_uMaxValueThreshold: %d.\n",
         m_uTextureOrigThreshold, m_uTextureBlurThreshold, m_uScreenGeometryThresholdg,
         m_uMaxValueThreshold);
#endif // ENABLE_THRESHOLD_TEST
  // clean up
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentRowShader);
  glDeleteShader(fragmentColumnShader);
  glDeleteShader(fragmentRender);
  return checkError("initProgram");
}

void ImageProcessor::process(const ImageDesc& desc)
{
  static float positions[][4] = {
    { -1.0, 1.0, 0.0, 1.0 },
    { -1.0, -1.0, 0.0, 1.0 },
    { 1.0, 1.0, 0.0, 1.0 },
    { 1.0, -1.0, 0.0, 1.0 },
  };
  GLuint tmpFramebuffer;
  // zero for row blur, one for column blur, two for input image.
  GLuint tmpTexture[3];
  // allocate fbo and a texture
  glGenFramebuffers(1, &tmpFramebuffer);
  glGenTextures(3, tmpTexture);
  allocateTexture(tmpTexture[0], desc.width, desc.height, desc.format);
  allocateTexture(tmpTexture[1], desc.width, desc.height, desc.format);
  allocateTexture(tmpTexture[2], desc.width, desc.height, desc.format, desc.data);
  // bind fbo and complete it.
  glBindFramebuffer(GL_FRAMEBUFFER, tmpFramebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         tmpTexture[0], 0);
  if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
    fprintf(stderr, "fbo is not completed.\n");
    exit(1);
  }
  GLint imageGeometry[2] = { desc.width, desc.height };
  glVertexAttribPointer(m_vPositionIndexRow, 4, GL_FLOAT, GL_FALSE, 0, positions);
  glEnableVertexAttribArray(m_vPositionIndexRow);
  glUseProgram(m_programRow);
  // setup uniforms
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tmpTexture[2]);
  glUniform1i(m_uTextureRow, 0);

  glUniform2iv(m_uScreenGeometryRow, 1, imageGeometry);
  // setup kernel and block size

  glUniform1fv(m_uKernelRow, s_block_size, m_kernel.data());
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  // bind fbo and complete it.
  glBindFramebuffer(GL_FRAMEBUFFER, tmpFramebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         tmpTexture[1], 0);
  if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
    fprintf(stderr, "fbo is not completed.\n");
    exit(1);
  }
  glDisableVertexAttribArray(m_vPositionIndexRow);

  glVertexAttribPointer(m_vPositionIndexColumn, 4, GL_FLOAT, GL_FALSE, 0,
                        positions);
  glEnableVertexAttribArray(m_vPositionIndexColumn);
  glUseProgram(m_programColumn);
  // setup uniforms
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tmpTexture[0]);
  glUniform1i(m_uTextureColumn, 0);

  glUniform2iv(m_uScreenGeometryColumn, 1, imageGeometry);
  // setup kernel and block size

  glUniform1fv(m_uKernelColumn, s_block_size, m_kernel.data());
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableVertexAttribArray(m_vPositionIndexColumn);
  // render to screen
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glEnableVertexAttribArray(m_vPositionIndexRender);
  glUseProgram(m_programRender);
  // setup uniforms
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tmpTexture[1]);
  glUniform1i(m_uTextureRender, 0);

  glUniform2iv(m_uScreenGeometryRender, 1, imageGeometry);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableVertexAttribArray(m_vPositionIndexRender);
#if defined(ENABLE_THRESHOLD_TEST) && (ENABLE_THRESHOLD_TEST == 1)
  glEnableVertexAttribArray(m_vPositionIndexThreshold);
  glUseProgram(m_programThreshold);
  // setup uniforms
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tmpTexture[2]);
  glUniform1i(m_uTextureOrigThreshold, 0);
  glActiveTexture(GL_TEXTURE0 + 1);
  glBindTexture(GL_TEXTURE_2D, tmpTexture[1]);
  glUniform1i(m_uTextureBlurThreshold, 1);

  glUniform2iv(m_uScreenGeometryThresholdg, 1, imageGeometry);
  glUniform1f(m_uMaxValueThreshold, static_cast<float>(m_maxValue) / 255.0f);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableVertexAttribArray(m_vPositionIndexThreshold);

#endif // ENABLE_THRESHOLD_TEST
  glDeleteTextures(3, tmpTexture);
  glDeleteFramebuffers(1, &tmpFramebuffer);
}

void
ImageProcessor::allocateTexture(GLuint texture, GLint width, GLint height, GLenum format, void* data)
{
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
               GL_UNSIGNED_BYTE, data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

static nv::Image*
gaussianLoadImageFromFile(const char* file)
{
  std::unique_ptr<nv::Image> image(new nv::Image());

  if (!image->loadImageFromFile(file)) {
    return NULL;
  }

  return image.release();
}

void
initRendering(void)
{
  glClearColor(0.0, 0.0, 0.0, 0.0);
  checkError("initRendering");
}

static ImageProcessor g_ip;
void
init(void)
{
  GLenum err;
  err = glewInit();
  if (GLEW_OK != err) {
    fprintf(stderr, "glew init fails.\n");
    exit(1);
  }
  if (!GLEW_VERSION_2_0) {
    fprintf(stderr, "OpenGL 2.0 fails.\n");
    exit(1);
  }
  if (!g_ip.init(g_maxVal)) {
    fprintf(stderr, "image process initialize failed.\n");
    exit(1);
  }
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

static void
renderBlur(const ImageDesc& imgDesc)
{
    g_ip.process(imgDesc);
}

static void
triangle_normal(void)
{
#ifdef _WIN32
  LARGE_INTEGER t1, t2, freq;
  QueryPerformanceCounter(&t1);
#else
  struct timespec t1, t2;
  clock_gettime(CLOCK_MONOTONIC, &t1);
#endif
  ImageDesc desc = { g_image->getWidth(), g_image->getHeight(),
                     g_image->getFormat(), g_image->getLevel(0) };
  renderBlur(desc);
  std::unique_ptr<char[]> buffer(new char[desc.width * desc.height * 3]);
  glReadPixels(0, 0, desc.width, desc.height, GL_RGB, GL_UNSIGNED_BYTE, buffer.get());
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
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  triangle_normal();
  glutSwapBuffers();
  checkError("display");
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
