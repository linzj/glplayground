#include "SceneImpl.hpp"
#include "../SceneObject.hpp"
#include "../GLError.hpp"
#include "../log.hpp"
#include <stdio.h>
namespace lin {
void
Scene::SceneImpl::loadShaders(SceneObject* pScene)
{
  ShaderDesc* desc = NULL;
  const int nShaders = pScene->getShaderDesc(&desc);

  for (int i = 0; i != nShaders; ++i) {
    ShaderDesc* cur = desc + i;

    if (Shaders.find(cur->file_name) != Shaders.end()) {
      printInfo("Ignoring same shader name %s\n", cur->file_name);
      continue;
    }

    size_t length;

    void* filecontent = mapWholeFile(cur->file_name, &length);

    if (!filecontent) {
      throw SceneInitExceptionContainer(new SceneInitException(
        "Unable to open shader file %s", cur->file_name));
    }

    GLuint shader = glCreateShader(cur->type);

    if (!shader) {
      throw SceneInitExceptionContainer(
        new SceneInitException("Unable to create a shader!"));
    }

    glShaderSource(shader, 1, (const GLchar**)&filecontent, (const GLint*)&length);

    unmapFile(filecontent);
    glCompileShader(shader);
    GLint compile_status;

    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);

    if (compile_status == GL_FALSE) { // faile to compile!
      GLint log_length;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

      if (log_length == 0)
        throw SceneInitExceptionContainer(new SceneInitException(
          "Failed to compile the shader %s;No log!", cur->file_name));

      std::unique_ptr<char[]> buf(new char[log_length]);

      glGetShaderInfoLog(shader, log_length, NULL, buf.get());

      throw SceneInitExceptionContainer(
        new SceneInitException("Failed to compile the shader %s;\nLog is:\n%s",
                               cur->file_name, buf.get()));
    }

    // Successfully compile the shader!
    Shaders.insert(GLResourceList::value_type(cur->file_name, shader));
  }
}

void
Scene::SceneImpl::loadPrograms(SceneObject* pSceneObject)
{
  ProgramDesc* desc = NULL;
  const int nPrograms = pSceneObject->getProgramDesc(&desc);

  if (nPrograms <= 0)
    return;

  for (int i = 0; i != nPrograms; ++i) {
    ProgramDesc* cur = desc + i;

    if (Programs.find(cur->name) != Programs.end()) {
      printInfo("Ignoring same program name %s\n", cur->name);
      continue;
    }

    if (cur->nShader <= 0) {
      continue;
    }

    GLuint pro = glCreateProgram();

    if (pro == 0) {
      throw SceneInitExceptionContainer(
        new SceneInitException(GLErrorContainer(new GLError(glGetError())),
                               "Faile to create a program!"));
    }

    for (int j = 0; j != cur->nShader; ++j) {
      GLResourceList::iterator found = Shaders.find(cur->shaders[j]);
      if (found == Shaders.end()) {
        throw SceneInitExceptionContainer(
          new SceneInitException("Runtime error at line %d", __LINE__));
      }
      glAttachShader(pro, found->second);
    }

    if (cur->nAttributes > 0) {
      for (int i = 0; i != cur->nAttributes; ++i) {
        if (cur->attribute_names[i]) {
          glBindAttribLocation(pro, i, cur->attribute_names[i]);
        }
      }
    }

    if (cur->nColors > 0) {
      for (int i = 0; i != cur->nColors; ++i) {
        if (cur->color_names[i]) {
          glBindFragDataLocation(pro, i, cur->color_names[i]);
        }
      }
    }

    glLinkProgram(pro);

    GLint link_status;
    glGetProgramiv(pro, GL_LINK_STATUS, &link_status);
    GLint log_length;
    glGetProgramiv(pro, GL_INFO_LOG_LENGTH, &log_length);

    std::unique_ptr<char[]> buf;
#define Helper()                                                               \
  if (log_length > 0) {                                                        \
    buf.reset(new char[log_length]);                                           \
    glGetProgramInfoLog(pro, log_length, NULL, buf.get());                     \
    printInfo("Log is %s\n", buf.get());                                       \
  }
#ifdef _DEBUG
    Helper()
#endif

      if (link_status == GL_FALSE)
    {
#ifndef _DEBUG
      Helper()
#endif
        throw SceneInitExceptionContainer(
          new SceneInitException("Link failed!Log is%s\n", buf.get()));
    }

#undef Helper
    // No problem here
    Programs.insert(GLResourceList::value_type(cur->name, pro));
  }
}
}
