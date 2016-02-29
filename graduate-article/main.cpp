#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "MultiSampleSceneObject.hpp"
#include <Scene.hpp>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GLError.hpp>
#ifndef _WIN32
#include <sys/time.h>
#include <unistd.h>
#else
#include <Windows.h>
#endif
void render();
static void idle();
static void resize(int x, int y);
static void mouseKey(int button, int state, int x, int y);
static void keyFunc(unsigned char c, int x, int y);
static bool isDown;
static void mouseMove(int x, int y);
float wx, wy;
lin::Scene* g_scene;
static double sum;

static void draw();

static inline double
currentSeconds()
{
#ifndef _WIN32
  struct timeval now_time;
  gettimeofday(&now_time, 0);
  return now_time.tv_sec + now_time.tv_usec * 1.e-6;
#else
  SYSTEMTIME system;
  GetSystemTime(&system);
  return double(system.wMilliseconds) / 1000 + system.wSecond +
         system.wMinute * 60;
#endif
}

int
main(int argc, char* argv[])
{
  try {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    // glutInitContextVersion(3, 2);
    glutInitWindowSize(800, 600);
    // glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
    glutCreateWindow("Order Independent Translucency");
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err) {
      fprintf(stderr, "glew init fails");
      exit(1);
    }
    g_scene = new lin::Scene();
    g_scene->registerSceneObject(new MultiSampleSceneObject());

    g_scene->init();
    sum = currentSeconds();
    glutDisplayFunc(draw);
    glutIdleFunc(idle);
    glutReshapeFunc(resize);
    glutMouseFunc(mouseKey);
    glutMotionFunc(mouseMove);
    glutKeyboardFunc(keyFunc);

    glutMainLoop();
  } catch (lin::SceneInitExceptionContainer& err) {
    fprintf(stderr, "An fatal error occurs!\n");
    fprintf(stderr, "%s\n", err->what());
    const lin::GLError* pError = err->getGLError();
    if (pError) {
      fprintf(stderr, "%s\n", pError->what());
    }
    return 1;
  }
}

static int oldx, oldy;
void
mouseKey(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {
      isDown = true;
      oldx = x;
      oldy = y;
    } else if (state == GLUT_UP) {
      isDown = false;
    }
  }
}

static void
mouseMove(int x, int y)
{
  if (isDown) {
    // fprintf(stderr,"Changing wx and wy\n");
    wx += 0.003 * (oldy - y);
    wy += 0.003 * (oldx - x);
    oldx = x;
    oldy = y;
    g_scene->setRotate(wx, wy);
  }
}

void
idle()
{
  glutPostRedisplay();
}

static int frame;
void
draw()
{

  try {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    g_scene->draw();
    glutSwapBuffers();
  } catch (lin::GLErrorContainer& err) {
    fprintf(stderr, "%s; with code %d", err->what(), err->getError());
    exit(1);
  }
  double c2 = currentSeconds();
  ++frame;
  double diff = c2 - sum;
  if (diff > 5.0) {
    fprintf(stderr, "we got %d frames in %f second,"
                    "%f frames per sec\n",
            frame, diff, frame / diff);
    frame = 0;
    sum = c2;
  }
}

void
resize(int wx, int wy)
{
  try {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, wx, wy);
    g_scene->setViewport(wx, wy);
  } catch (lin::GLErrorContainer& err) {
    fprintf(stderr, "%s; with code %d", err->what(), err->getError());
    exit(1);
  }
}

int internal_func = 1;

static void
keyFunc(unsigned char c, int x, int y)
{
  switch (c) {
    case '1':
      internal_func = 1;
      fprintf(stderr, "Using my method!\n");
      break;
    case '2':
      internal_func = 2;
      fprintf(stderr, "Using nvidia's method!\n");
      break;
  }
}
