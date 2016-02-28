#include <GL/glew.h>
#include "../Scene.hpp"
#include "../SceneObject.hpp"
#include "../PassDesc.hpp"
#include "../InitDesc.hpp"
#include "../DrawDesc.hpp"
#include "../DrawableModel.hpp"
#include "../GLError.hpp"
#include <string.h>
#include <stdio.h>

#include <vector>
#include <iostream>
#include <string>

#include "../SceneImpl/SceneImpl.hpp"

namespace lin {

Scene::Scene()
  : m_private(new SceneImpl)
{
}

Scene::~Scene()
{
}

void
Scene::loadModels(SceneObject* p)
{
  m_private->loadModels(p);
}

void
Scene::loadTextures(SceneObject* p)
{
  m_private->loadTextures(p);
}

void
Scene::loadShaders(SceneObject* p)
{
  m_private->loadShaders(p);
}

void
Scene::loadPrograms(SceneObject* p)
{
  m_private->loadPrograms(p);
}

void
Scene::loadFBOs(SceneObject* p)
{
  m_private->loadFBOs(p);
}

void
Scene::init()
{
  InitDesc initDesc;
  initDesc.m_scene = this;
  GLfloat largest_supported_anisotropy;
  {
    GLenum errcode;

    if (GL_NO_ERROR != (errcode = glGetError())) {
      printInfo("Error occur with code %d\n", errcode);
#ifdef _DEBUG
      exit(1);
#endif
    }
  }

  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &largest_supported_anisotropy);

  if (GL_NO_ERROR != glGetError()) {
    initDesc.max_anisotropy = 0;
  } else {
    initDesc.max_anisotropy = largest_supported_anisotropy;
  }

  for (auto& v : m_private->SceneObjects) {

    SceneObject* psceneObject = v.second;

    try {
      loadModels(psceneObject);
      loadTextures(psceneObject);
      loadShaders(psceneObject);
      loadPrograms(psceneObject);
      loadFBOs(psceneObject);
    } catch (const GLErrorContainer& err) {
      throw SceneInitExceptionContainer(
        new SceneInitException(err, "Failed to init for opengl error"));
    } catch (std::bad_alloc& err) {
      throw SceneInitExceptionContainer(new SceneInitException(err.what()));
    }

    psceneObject->init(initDesc);

    {
      GLenum errcode = glGetError();

      if (errcode != GL_NO_ERROR) {
        printInfo("An gl error occur when init;code:%d\n", errcode);
#ifdef _DEBUG
        exit(1);
#else
        throw SceneInitExceptionContainer(
          new SceneInitException(GLErrorContainer(new GLError(errcode)),
                                 "A scene object init failed!"));
#endif
      }
    }
  }
}

void
Scene::registerSceneObject(SceneObject* pObject)
{
  if (pObject) {
    const char* name = pObject->getName();

    if (m_private->SceneObjects.find(name) != m_private->SceneObjects.end()) {
      printInfo("Ignoring same scene object name %s\n", name);
    } else {
      m_private->SceneObjects.insert(
        SceneObjectList::value_type(name, pObject));
    }
  }
}

void
Scene::setViewport(int wx, int wy)
{
  m_private->wx = wx;
  m_private->wy = wy;

  for (auto& pair : m_private->SceneFBOsDynamic) {
    updateFBO(pair.second, wx, wy);
  }
}

void
Scene::setRotate(float wx, float wy)
{
  m_private->rx = wx;
  m_private->ry = wy;
}

void
Scene::getRotate(float* wx, float* wy) const
{
  *wx = m_private->rx;
  *wy = m_private->ry;
}

void
Scene::getViewport(int* x, int* y) const
{
  *x = m_private->wx;
  *y = m_private->wy;
}

void
Scene::draw()
{
  SceneObjectList SceneObjectList;
  for (auto& pair : m_private->SceneObjects) {
    PassDesc* passdesc;
    GLuint nPassDesc;
    SceneObject* psceneObject = pair.second;
    nPassDesc = psceneObject->getPassDescs(&passdesc);

    if (nPassDesc == 0)
      continue;

    GLuint ndraw = 0;

    for (; ndraw < nPassDesc; ++ndraw) {
      PassDesc* cur = passdesc + ndraw;

      DrawableModel* models[MAX_MODEL] = { 0 };

      ModelList ModelList;
      ModelList::iterator model_end = m_private->Models.end();

      if (cur->nModels > 0) {
        for (int j = 0; j != cur->nModels; ++j) {
          const char* model_name = cur->Models[j];

          ModelList::iterator f = m_private->Models.find(model_name);

          if (f == model_end) {
            printInfo("Failed to find model name %s\n", model_name);
            continue;
          }

          models[j] = (f->second);
        }
      }

      if (cur->nTextures > 0) {
        SceneTextureList::const_iterator texture_end =
          m_private->Textures.end();

        for (int j = 0; j != cur->nTextures; ++j) {
          const char* texturename = cur->Textures[j];
          SceneTextureList::const_iterator f =
            m_private->Textures.find(texturename);

          if (f == texture_end) {
            printInfo("Failed to find texture name %s\n", texturename);
            continue;
          }

          glActiveTexture(GL_TEXTURE0 + j);
          glBindTexture(f->second.target, f->second.name);
        }
      }

      GLResourceList::const_iterator program_end = m_private->Programs.end();

      const char* programname = cur->Program;
      GLResourceList::const_iterator f = m_private->Programs.find(programname);
      GLuint program = 0;

      if (f == program_end) {
        printInfo("Failed to find program name %s\n", programname);
      } else {
        program = f->second;
      }

      GLuint fbo = 0;
      if (cur->FBO) {
        SceneFBOList::iterator fbo_dynamic_end =
          m_private->SceneFBOsDynamic.end();
        SceneFBOList::iterator f = m_private->SceneFBOsDynamic.find(cur->FBO);
        if (f != fbo_dynamic_end) {
          fbo = f->second.fbo;
        } else {
          SceneFBOList::iterator fbo_fix_end = m_private->SceneFBOsFixed.end();
          f = m_private->SceneFBOsFixed.find(cur->FBO);
          if (f != fbo_fix_end)
            fbo = f->second.fbo;
          else
            printInfo("Failed to find fbo name %s\n", cur->FBO);
        }
      }

      // Fill the DrawDesc
      DrawDesc drawDesc;
      drawDesc.models = models;
      drawDesc.program = program;
      drawDesc.fbo = fbo;
      drawDesc.idraw = ndraw;
      drawDesc.pass_name = cur->pass_name;
      glUseProgram(program);
      if (fbo != 0) {

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
#ifdef _DEBUG
        if (GL_FRAMEBUFFER_COMPLETE !=
            glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
          printInfo("FBO name %s is not complete!\n", cur->FBO);
          glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
#endif
      } else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
      }
      psceneObject->draw(drawDesc);
    }
  }
}
}
