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
#include <nvImage.h>
#include <memory>

static nv::Image*
gaussianLoadImageFromFile(const char* file)
{
  std::unique_ptr<nv::Image> image(new nv::Image());

  if (!image->loadImageFromFile(file)) {
    return NULL;
  }

  return image.release();
}

static std::unique_ptr<nv::Image> g_image;
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

static GLint vPositionIndex;
static GLint uTexture;
static GLint uScreenGeometry;

static void
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
  vPositionIndex = glGetAttribLocation(program, "v_position");
  printf("vPositionIndex: %d.\n", vPositionIndex);
  uTexture = glGetUniformLocation(program, "u_texture");
  uScreenGeometry = glGetUniformLocation(program, "u_screenGeometry");
  printf("uTexture: %d, uScreenGeometry: %d.\n", uTexture, uScreenGeometry);

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
  float positions[][4] = {
    { -1.0, 1.0, 0.0, 1.0 },
    { -1.0, -1.0, 0.0, 1.0 },
    { 1.0, 1.0, 0.0, 1.0 },
    { 1.0, -1.0, 0.0, 1.0 },
  };
  GLint viewport[4];
  glVertexAttribPointer(vPositionIndex, 4, GL_FLOAT, GL_FALSE, 0, positions);
  glEnableVertexAttribArray(vPositionIndex);
  // setup uniforms
  glGetIntegerv(GL_VIEWPORT, viewport);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, g_texture);
  glUniform1i(uTexture, 0);
  glUniform2iv(uScreenGeometry, 1, &viewport[2]);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableVertexAttribArray(vPositionIndex);
}

static void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  triangle_normal();
  glFlush();
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
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  if (argc != 2) {
    fprintf(stderr, "need a file name.\n");
    exit(1);
  }
  g_image.reset(gaussianLoadImageFromFile(argv[1]));
  if (g_image.get() == nullptr) {
    fprintf(stderr, "fails to load image.\n");
    exit(1);
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
