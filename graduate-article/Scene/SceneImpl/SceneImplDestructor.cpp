#include "SceneImpl.hpp"
#include "../SceneObject.hpp"

namespace lin {
Scene::SceneImpl::~SceneImpl()
{
  glUseProgram(0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  for (auto& res : Textures) {
    glDeleteTextures(1, &res.second.name);
  }

  for (auto& res : Programs) {
    glDeleteProgram(res.second);
  }

  for (auto& res : Shaders) {
    glDeleteShader(res.second);
  }

  for (auto& pair : SceneFBOsDynamic) {
    SceneFBODesc& fbodesc = pair.second;
    glDeleteFramebuffers(1, &fbodesc.fbo);
    // glDeleteTextures(fbodesc.desc.nColors, fbodesc.colors);

    // if (fbodesc.desc.depth_stencil)
    //     glDeleteTextures(1, &fbodesc.depth_stencil);
  }

  for (auto& pair : SceneFBOsFixed) {
    SceneFBODesc& fbodesc = pair.second;
    glDeleteFramebuffers(1, &fbodesc.fbo);
    // glDeleteTextures(fbodesc.desc.nColors, fbodesc.colors);

    // if (fbodesc.desc.depth_stencil)
    //     glDeleteTextures(1, &fbodesc.depth_stencil);
  }

  for (auto& pair : Models) {
    delete pair.second;
  }

  for (auto& pair : SceneObjects) {
    delete pair.second;
  }
}
}
