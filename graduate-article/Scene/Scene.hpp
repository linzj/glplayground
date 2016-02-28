#ifndef SCENE_HPP
#define SCENE_HPP
#include <memory>
#include <exception>
#include <string>
#include <exception>

#include <memory>
#include "log.hpp"

namespace lin {

class SceneObject;

class Scene
{

public:
  Scene();
  Scene(const Scene&) = delete;
  const Scene& operator=(const Scene&) = delete;

protected:
  virtual ~Scene();

public:
  virtual void init();
  virtual void registerSceneObject(SceneObject*);
  virtual void setViewport(int x, int y);
  virtual void setRotate(float wx, float wy);
  virtual void draw();

  virtual void getRotate(float* wx, float* wy) const;
  virtual void getViewport(int* x, int* y) const;

public:
  static const int MAX_MODEL = 16;

protected:
  virtual void loadModels(SceneObject* sceneObject);
  virtual void loadTextures(SceneObject* sceneObject);
  virtual void loadShaders(SceneObject* sceneObject);
  virtual void loadPrograms(SceneObject* sceneObject);
  virtual void loadFBOs(SceneObject* sceneObject);

private:
  struct SceneImpl;
  std::unique_ptr<SceneImpl> m_private;
};

class GLError;

class SceneInitException : public std::exception
{

public:
  SceneInitException(const char* msg = NULL, ...);
  SceneInitException(const std::shared_ptr<GLError>& error,
                     const char* msg = NULL, ...);
  SceneInitException(const SceneInitException&);
  SceneInitException& operator=(const SceneInitException&);
  virtual ~SceneInitException() throw();

public:
  const GLError* getGLError() { return m_error.get(); }

  const char* what() const throw() { return m_msg; }

private:
  std::shared_ptr<GLError> m_error;
  char* m_msg;
};

typedef std::shared_ptr<SceneInitException> SceneInitExceptionContainer;
}

#endif // SCENE_HPP
