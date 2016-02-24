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

/* vertex array data for a colored 2D triangle, consisting of RGB color values
   and XY coordinates */
const GLfloat varray[] = {
  1.0f,  0.0f, 0.0f, /* red */
  5.0f,  5.0f,       /* lower left */

  0.0f,  1.0f, 0.0f, /* green */
  25.0f, 5.0f,       /* lower right */

  0.0f,  0.0f, 1.0f, /* blue */
  5.0f,  25.0f       /* upper left */
};

/* ISO C somehow enforces this silly use of 'enum' for compile-time constants */
enum
{
  numColorComponents = 3,
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
  "in vec4 fg_Color;\n",
  "in vec4 fg_Vertex;\n",
  "smooth out vec4 fg_SmoothColor;\n",
  "void main()\n",
  "{\n",
  "   fg_SmoothColor = fg_Color;\n",
  "   gl_Position = fg_ProjectionMatrix * fg_Vertex;\n",
  "}\n"
};

const char* fragmentShaderSource[] = { "#version 140\n",
                                       "smooth in vec4 fg_SmoothColor;\n",
                                       "out vec4 fg_FragColor;\n",
                                       "void main(void)\n",
                                       "{\n",
                                       "   fg_FragColor = fg_SmoothColor;\n",
                                       "}\n" };

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
GLuint fgColorIndex;
GLuint fgVertexIndex;

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

  fgColorIndex = glGetAttribLocation(program, "fg_Color");
  glEnableVertexAttribArray(fgColorIndex);

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
  glUniformMatrix4fv(fgProjectionMatrixIndex, 1, GL_FALSE, projectionMatrix);

  glBindBuffer(GL_ARRAY_BUFFER, vertexBufferName);
  glVertexAttribPointer(fgColorIndex, numColorComponents, GL_FLOAT, GL_FALSE,
                        stride, bufferObjectPtr(0));
  glVertexAttribPointer(fgVertexIndex, numVertexComponents, GL_FLOAT, GL_FALSE,
                        stride,
                        bufferObjectPtr(sizeof(GLfloat) * numColorComponents));
  glDrawArrays(GL_TRIANGLES, 0, numElements);
}

static void
triangle(void)
{
  GLint tmpFramebuffer;
  GLint tmpRenderbuffer;
  GLint viewport[4];

  glGetIntegerv(GL_VIEWPORT, viewport);
  glGenFramebuffers(1, &tmpFramebuffer);
  glGenRenderbuffers(1, &tmpRenderbuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, tmpFramebuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, tmpRenderbuffer);
  glRenderbufferStorageMultisample(GL_RENDERBUFFER, 16, GL_RGBA, viewport[2],
                                   viewport[3]);
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
  glClear(GL_COLOR_BUFFER_BIT);
  if (draw_normal) {
    triangle_normal();
  } else {
    triangle();
  }
  glFlush();
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
