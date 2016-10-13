#ifndef SCENEIMPL_HPP
#define SCENEIMPL_HPP
#include "../Scene.hpp"
#include <memory>
#include <unordered_map>
#include <GL/glew.h>
#include "../PassDesc.hpp"
namespace lin {
void* mapWholeFile(const char* filename, size_t* length);
void unmapFile(void*);

struct SceneFBODesc
{
  FBODesc desc;
  GLuint fbo;
  GLuint colors[FBODesc::MAX_COLOR_ATTACHMENT];
  GLenum targets[FBODesc::MAX_COLOR_ATTACHMENT];
  GLuint depth_stencil;
  GLenum depth_stencil_target;
};

struct SceneTextureDesc
{
  GLenum target;
  GLuint name;
};

typedef std::unordered_map<std::string, GLuint> GLResourceList;
typedef std::unordered_map<std::string, DrawableModel*> ModelList;
typedef std::unordered_map<std::string, SceneObject*> SceneObjectList;
typedef std::unordered_map<std::string, SceneFBODesc> SceneFBOList;
typedef std::unordered_map<std::string, SceneTextureDesc> SceneTextureList;

struct Scene::SceneImpl
{
  SceneTextureList Textures;
  GLResourceList Shaders;
  GLResourceList Programs;
  SceneFBOList SceneFBOsDynamic; // for the fbo change with window size
  SceneFBOList SceneFBOsFixed;   // for the fbo size
  ModelList Models;
  SceneObjectList SceneObjects;

  int wx, wy;
  float rx, ry;
  // helper functions
  void loadModels(SceneObject* sceneObject);
  void loadTextures(SceneObject* sceneObject);
  void loadShaders(SceneObject* sceneObject);
  void loadPrograms(SceneObject* sceneObject);

  void loadFBOs(SceneObject* sceneObject);

  ~SceneImpl();

private:
  void addObjModel(const char* filename);
  void addSdkMeshModel(const char* filename);
};

void updateFBO(const SceneFBODesc&, int, int);
}
#endif // SCENEIMPL_HPP
