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
#include <glsw.h>
#include <vectormath.h>

static unsigned draw_normal = 0;
static void CreateIcosahedron();
static void LoadEffect();
#define PEZ_VIEWPORT_HEIGHT 500
#define PEZ_VIEWPORT_WIDTH 500

static void _PezFatalError(const char* pStr, va_list a)
{
    char msg[1024] = {0};
    vsnprintf(msg, sizeof(msg), pStr, a);
    fputs(msg, stderr);
    exit(1);
}

static void PezCheckCondition(int condition, ...)
{
    va_list a;
    const char* pStr;

    if (condition)
        return;

    va_start(a, condition);
    pStr = va_arg(a, const char*);
    _PezFatalError(pStr, a);
}

typedef struct {
    GLuint Projection;
    GLuint Modelview;
    GLuint NormalMatrix;
    GLuint LightPosition;
    GLuint AmbientMaterial;
    GLuint DiffuseMaterial;
    GLuint TessLevelInner;
    GLuint TessLevelOuter;
} ShaderUniforms;

static GLsizei IndexCount;
static const GLuint PositionSlot = 0;
static GLuint ProgramHandle;
static Matrix4 ProjectionMatrix;
static Matrix4 ModelviewMatrix;
static Matrix3 NormalMatrix;
static ShaderUniforms Uniforms;
static float TessLevelInner;
static float TessLevelOuter;

static void PezRender(void)
{
    glUniform1f(Uniforms.TessLevelInner, TessLevelInner);
    glUniform1f(Uniforms.TessLevelOuter, TessLevelOuter);
    
    Vector4 lightPosition = V4MakeFromElems(0.25, 0.25, 1, 0);
    glUniform3fv(Uniforms.LightPosition, 1, &lightPosition.x);
    
    glUniformMatrix4fv(Uniforms.Projection, 1, 0, &ProjectionMatrix.col0.x);
    glUniformMatrix4fv(Uniforms.Modelview, 1, 0, &ModelviewMatrix.col0.x);

    Matrix3 nm = M3Transpose(NormalMatrix);
    float packed[9] = { nm.col0.x, nm.col1.x, nm.col2.x,
                        nm.col0.y, nm.col1.y, nm.col2.y,
                        nm.col0.z, nm.col1.z, nm.col2.z };
    glUniformMatrix3fv(Uniforms.NormalMatrix, 1, 0, &packed[0]);

    // Render the scene:
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPatchParameteri(GL_PATCH_VERTICES, 3);
    glUniform3f(Uniforms.AmbientMaterial, 0.04f, 0.04f, 0.04f);
    glUniform3f(Uniforms.DiffuseMaterial, 0, 0.75, 0.75);
    glDrawElements(GL_PATCHES, IndexCount, GL_UNSIGNED_INT, 0);
}

static const char* PezInitialize(void)
{
    TessLevelInner = 3;
    TessLevelOuter = 2;

    CreateIcosahedron();
    LoadEffect();

    Uniforms.Projection = glGetUniformLocation(ProgramHandle, "Projection");
    Uniforms.Modelview = glGetUniformLocation(ProgramHandle, "Modelview");
    Uniforms.NormalMatrix = glGetUniformLocation(ProgramHandle, "NormalMatrix");
    Uniforms.LightPosition = glGetUniformLocation(ProgramHandle, "LightPosition");
    Uniforms.AmbientMaterial = glGetUniformLocation(ProgramHandle, "AmbientMaterial");
    Uniforms.DiffuseMaterial = glGetUniformLocation(ProgramHandle, "DiffuseMaterial");
    Uniforms.TessLevelInner = glGetUniformLocation(ProgramHandle, "TessLevelInner");
    Uniforms.TessLevelOuter = glGetUniformLocation(ProgramHandle, "TessLevelOuter");

    // Set up the projection matrix:
    const float HalfWidth = 0.6f;
    const float HalfHeight = HalfWidth * PEZ_VIEWPORT_HEIGHT / PEZ_VIEWPORT_WIDTH;
    ProjectionMatrix = M4MakeFrustum(-HalfWidth, +HalfWidth, -HalfHeight, +HalfHeight, 5, 150);

    // Initialize various state:
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.7f, 0.6f, 0.5f, 1.0f);

    return "Tessellation Demo";
}

static void CreateIcosahedron()
{
    const int Faces[] = {
        2, 1, 0,
        3, 2, 0,
        4, 3, 0,
        5, 4, 0,
        1, 5, 0,

        11, 6,  7,
        11, 7,  8,
        11, 8,  9,
        11, 9,  10,
        11, 10, 6,

        1, 2, 6,
        2, 3, 7,
        3, 4, 8,
        4, 5, 9,
        5, 1, 10,

        2,  7, 6,
        3,  8, 7,
        4,  9, 8,
        5, 10, 9,
        1, 6, 10 };

    const float Verts[] = {
         0.000f,  0.000f,  1.000f,
         0.894f,  0.000f,  0.447f,
         0.276f,  0.851f,  0.447f,
        -0.724f,  0.526f,  0.447f,
        -0.724f, -0.526f,  0.447f,
         0.276f, -0.851f,  0.447f,
         0.724f,  0.526f, -0.447f,
        -0.276f,  0.851f, -0.447f,
        -0.894f,  0.000f, -0.447f,
        -0.276f, -0.851f, -0.447f,
         0.724f, -0.526f, -0.447f,
         0.000f,  0.000f, -1.000f };

    IndexCount = sizeof(Faces) / sizeof(Faces[0]);

    // Create the VAO:
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create the VBO for positions:
    GLuint positions;
    GLsizei stride = 3 * sizeof(float);
    glGenBuffers(1, &positions);
    glBindBuffer(GL_ARRAY_BUFFER, positions);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Verts), Verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(PositionSlot);
    glVertexAttribPointer(PositionSlot, 3, GL_FLOAT, GL_FALSE, stride, 0);

    // Create the VBO for indices:
    GLuint indices;
    glGenBuffers(1, &indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Faces), Faces, GL_STATIC_DRAW);
}

static void LoadEffect()
{
    GLint compileSuccess, linkSuccess;
    GLchar compilerSpew[256];

    glswInit();
    glswSetPath("./", ".glsl");
    glswAddDirectiveToken("*", "#version 400");

    const char* vsSource = glswGetShader("Geodesic.Vertex");
    const char* tcsSource = glswGetShader("Geodesic.TessControl");
    const char* tesSource = glswGetShader("Geodesic.TessEval");
    const char* gsSource = glswGetShader("Geodesic.Geometry");
    const char* fsSource = glswGetShader("Geodesic.Fragment");
    const char* msg = "Can't find %s shader.  Does '../BicubicPath.glsl' exist?\n";
    PezCheckCondition(vsSource != 0, msg, "vertex");
    PezCheckCondition(tcsSource != 0, msg, "tess control");
    PezCheckCondition(tesSource != 0, msg, "tess eval");
    PezCheckCondition(gsSource != 0, msg, "geometry");
    PezCheckCondition(fsSource != 0, msg, "fragment");

    GLuint vsHandle = glCreateShader(GL_VERTEX_SHADER);
    GLuint tcsHandle = glCreateShader(GL_TESS_CONTROL_SHADER);
    GLuint tesHandle = glCreateShader(GL_TESS_EVALUATION_SHADER);
    GLuint gsHandle = glCreateShader(GL_GEOMETRY_SHADER);
    GLuint fsHandle = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vsHandle, 1, &vsSource, 0);
    glCompileShader(vsHandle);
    glGetShaderiv(vsHandle, GL_COMPILE_STATUS, &compileSuccess);
    glGetShaderInfoLog(vsHandle, sizeof(compilerSpew), 0, compilerSpew);
    PezCheckCondition(compileSuccess, "Vertex Shader Errors:\n%s", compilerSpew);

    glShaderSource(tcsHandle, 1, &tcsSource, 0);
    glCompileShader(tcsHandle);
    glGetShaderiv(tcsHandle, GL_COMPILE_STATUS, &compileSuccess);
    glGetShaderInfoLog(tcsHandle, sizeof(compilerSpew), 0, compilerSpew);
    PezCheckCondition(compileSuccess, "Tess Control Shader Errors:\n%s", compilerSpew);

    glShaderSource(tesHandle, 1, &tesSource, 0);
    glCompileShader(tesHandle);
    glGetShaderiv(tesHandle, GL_COMPILE_STATUS, &compileSuccess);
    glGetShaderInfoLog(tesHandle, sizeof(compilerSpew), 0, compilerSpew);
    PezCheckCondition(compileSuccess, "Tess Eval Shader Errors:\n%s", compilerSpew);

    glShaderSource(gsHandle, 1, &gsSource, 0);
    glCompileShader(gsHandle);
    glGetShaderiv(gsHandle, GL_COMPILE_STATUS, &compileSuccess);
    glGetShaderInfoLog(gsHandle, sizeof(compilerSpew), 0, compilerSpew);
    PezCheckCondition(compileSuccess, "Geometry Shader Errors:\n%s", compilerSpew);

    glShaderSource(fsHandle, 1, &fsSource, 0);
    glCompileShader(fsHandle);
    glGetShaderiv(fsHandle, GL_COMPILE_STATUS, &compileSuccess);
    glGetShaderInfoLog(fsHandle, sizeof(compilerSpew), 0, compilerSpew);
    PezCheckCondition(compileSuccess, "Fragment Shader Errors:\n%s", compilerSpew);

    ProgramHandle = glCreateProgram();
    glAttachShader(ProgramHandle, vsHandle);
    glAttachShader(ProgramHandle, tcsHandle);
    glAttachShader(ProgramHandle, tesHandle);
    glAttachShader(ProgramHandle, gsHandle);
    glAttachShader(ProgramHandle, fsHandle);
    glBindAttribLocation(ProgramHandle, PositionSlot, "Position");
    glLinkProgram(ProgramHandle);
    glGetProgramiv(ProgramHandle, GL_LINK_STATUS, &linkSuccess);
    glGetProgramInfoLog(ProgramHandle, sizeof(compilerSpew), 0, compilerSpew);
    PezCheckCondition(linkSuccess, "Shader Link Errors:\n%s", compilerSpew);

    glUseProgram(ProgramHandle);
}

static void PezUpdate(unsigned int elapsedMicroseconds)
{
    const float RadiansPerMicrosecond = 0.0000005f;
    static float Theta = 0;
    Theta += elapsedMicroseconds * RadiansPerMicrosecond;
    Matrix4 rotation = M4MakeRotationX(Theta);
    Point3 eyePosition = P3MakeFromElems(0, 0, -10);
    Point3 targetPosition = P3MakeFromElems(0, 0, 0);
    Vector3 upVector = V3MakeFromElems(0, 1, 0);
    Matrix4 lookAt = M4MakeLookAt(eyePosition, targetPosition, upVector);
    ModelviewMatrix = M4Mul(lookAt, rotation);
    NormalMatrix = M4GetUpper3x3(ModelviewMatrix);
}

/* report GL errors, if any, to stderr */
static void checkError(const char *functionName)
{
   GLenum error;
   while (( error = glGetError() ) != GL_NO_ERROR) {
      fprintf (stderr, "GL error 0x%X detected in %s\n", error, functionName);
   }
}

void init(void) 
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
   PezInitialize();
}

void dumpInfo(void)
{
   printf ("Vendor: %s\n", glGetString (GL_VENDOR));
   printf ("Renderer: %s\n", glGetString (GL_RENDERER));
   printf ("Version: %s\n", glGetString (GL_VERSION));
   printf ("GLSL: %s\n", glGetString (GL_SHADING_LANGUAGE_VERSION));
   checkError ("dumpInfo");
}

static void drawOnFbo(void)
{
   GLint tmpFramebuffer;
   GLint tmpRenderbuffer;
   GLint viewport[4];
   
   glGetIntegerv(GL_VIEWPORT, viewport);
   glGenFramebuffers(1, &tmpFramebuffer);
   glGenRenderbuffers(1, &tmpRenderbuffer);
   glBindFramebuffer(GL_FRAMEBUFFER, tmpFramebuffer);
   glBindRenderbuffer(GL_RENDERBUFFER, tmpRenderbuffer);
   glRenderbufferStorageMultisample(GL_RENDERBUFFER, 16, GL_RGBA, viewport[2], viewport[3]);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, tmpRenderbuffer);
   if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
       fprintf(stderr, "fbo is not completed.\n");
       exit(1);
   }
   PezRender();
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   // blit here
   glBindFramebuffer(GL_READ_FRAMEBUFFER, tmpFramebuffer);
   glBlitFramebuffer(0, 0, viewport[2], viewport[3], 0, 0, viewport[2], viewport[3], GL_COLOR_BUFFER_BIT, GL_LINEAR);
   glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
   glDeleteFramebuffers(1, &tmpFramebuffer);
   glDeleteRenderbuffers(1, &tmpRenderbuffer);
   checkError ("triangle");
}

static unsigned g_frames = 0;

static void display(void)
{
   glClear (GL_COLOR_BUFFER_BIT);
   if (draw_normal) {
       PezRender ();
   } else {
       drawOnFbo ();
   }
   glutSwapBuffers ();
   checkError ("display");
   g_frames++;
}

static void reshape (int w, int h)
{
   glViewport (0, 0, (GLsizei) w, (GLsizei) h);
   const float HalfWidth = 0.6f;
   const float HalfHeight = HalfWidth * h / w ;
   ProjectionMatrix = M4MakeFrustum(-HalfWidth, +HalfWidth, -HalfHeight, +HalfHeight, 5, 150);
   checkError ("reshape");
}

static void keyboard(unsigned char key, int x, int y)
{

    const int VK_LEFT = 0x25;
    const int VK_UP = 0x26;
    const int VK_RIGHT = 0x27;
    const int VK_DOWN = 0x28;

    switch (key) {
        case 27:
            exit(0);
            break;
        case 'a':
            TessLevelInner = TessLevelInner > 1 ? TessLevelInner - 1 : 1;
            break;
        case 's':
            TessLevelOuter = TessLevelOuter > 1 ? TessLevelOuter - 1 : 1;
            break;
        case 'd':
            TessLevelInner++;
            break;
        case 'w':
            TessLevelOuter++;
            break;
        case ' ':
            draw_normal ^= 1;
            printf("switching the draw mode to %d.\n", draw_normal);
            glutPostRedisplay();
            break;
    }
}

void samplemenu(int menuID)
{}

static void idle(void)
{
    unsigned int elapsed;
    static unsigned int lastUpdateElpased = 0U;
    elapsed = glutGet(GLUT_ELAPSED_TIME);
    PezUpdate(elapsed);
    glutPostRedisplay();
    if (elapsed - lastUpdateElpased > 1000) {
        unsigned diff = elapsed - lastUpdateElpased;
        unsigned int frames = g_frames;

        g_frames = 0;
        lastUpdateElpased = elapsed;
        fprintf(stderr, "fps: %f.\n", (float)frames / diff * 1000);
    }
}

int main(int argc, char** argv)
{
   int menuA;
   glutInit(&argc, argv);
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB);
   /* add command line argument "classic" for a pre-3.x context */
   if ((argc != 2) || (strcmp (argv[1], "classic") != 0)) {
      glutInitContextVersion (4, 3);
      glutInitContextFlags (GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);
   }
   glutInitWindowSize (PEZ_VIEWPORT_WIDTH, PEZ_VIEWPORT_HEIGHT); 
   glutInitWindowPosition (100, 100);
   glutCreateWindow (argv[0]);
   dumpInfo ();
   init ();
   glutDisplayFunc(display); 
   glutReshapeFunc(reshape);
   glutKeyboardFunc (keyboard);
   glutIdleFunc(idle);

   /* Add a menu. They have their own context and should thus work with forward compatible main windows too. */
   menuA = glutCreateMenu(samplemenu);
   glutAddMenuEntry("Sub menu A1 (01)",1);
   glutAddMenuEntry("Sub menu A2 (02)",2);
   glutAddMenuEntry("Sub menu A3 (03)",3);
   glutSetMenu(menuA);
   glutAttachMenu(GLUT_RIGHT_BUTTON);

   glutMainLoop();
   return 0;
}

