#include "SceneImpl.hpp"
#include <nvModel.h>
#include "../SceneObject.hpp"
#include "../GLError.hpp"
#include <string.h>
#include "../log.hpp"
#ifdef _WIN32
#include <Windows.h>
#define strncasecmp(a, b, c) lstrcmpiA(a, b)
#endif
namespace lin {
struct model_buffer : public DrawableModel
{
  GLuint vertex_array_object;
  GLuint array_buffer;
  GLuint index_buffer;

  GLuint indices_size;
  virtual void draw()
  {
    glBindVertexArray(this->vertex_array_object);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->index_buffer);
    glDrawElements(GL_TRIANGLES, this->indices_size, GL_UNSIGNED_INT, 0);
  }

  ~model_buffer()
  {
    glDeleteVertexArrays(1, &vertex_array_object);
    glDeleteBuffers(1, &array_buffer);
    glDeleteBuffers(1, &index_buffer);
  }
};

void
Scene::SceneImpl::addObjModel(const char* file_name)
{
  nv::Model model;

  if (model.loadModelFromFile(file_name)) {
    model.compileModel(nv::Model::eptTriangles);

    GLuint vertex_array, buffers[2], indices_size; // 0 for vertices 1 for index
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    glGenBuffers(2, buffers);
    // set vertices
    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(float) * model.getCompiledVertexCount() *
                   model.getCompiledVertexSize(),
                 model.getCompiledVertices(), GL_STATIC_DRAW);
    const int stride = model.getCompiledVertexSize() * sizeof(float);
    glVertexAttribPointer(
      0, model.getPositionSize(), GL_FLOAT, GL_FALSE, stride,
      (void*)(model.getCompiledPositionOffset() * sizeof(GLfloat)));
    glEnableVertexAttribArray(0);

    if (model.hasTexCoords()) {
      glVertexAttribPointer(
        1, model.getTexCoordSize(), GL_FLOAT, GL_FALSE, stride,
        (void*)(model.getCompiledTexCoordOffset() * sizeof(GLfloat)));
      glEnableVertexAttribArray(1);
    }

    if (model.hasNormals()) {
      glVertexAttribPointer(
        2, model.getNormalSize(), GL_FLOAT, GL_FALSE, stride,
        (void*)(model.getCompiledNormalOffset() * sizeof(GLfloat)));
      glEnableVertexAttribArray(2);
    }

    if (model.hasTangents()) {
      glVertexAttribPointer(
        3, model.getTangentSize(), GL_FLOAT, GL_FALSE, stride,
        (void*)(model.getCompiledTangentOffset() * sizeof(GLfloat)));
      glEnableVertexAttribArray(3);
    }

    // set index;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[1]);

    indices_size = model.getCompiledIndexCount(nv::Model::eptTriangles);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices_size,
                 model.getCompiledIndices(nv::Model::eptTriangles),
                 GL_STATIC_DRAW);

    GLenum error = glGetError();

    if (error != GL_NO_ERROR) {
      glBindVertexArray(0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

      if (vertex_array) {
        glDeleteVertexArrays(1, &vertex_array);
      }

      if (buffers[0]) {
        glDeleteBuffers(1, &buffers[0]);
      }

      if (buffers[1]) {
        glDeleteBuffers(1, &buffers[1]);
      }

      printInfo("Failed to load model!%s\n", file_name);

      throw GLErrorContainer(new GLError(error));
    }

    // No Error here yeah push the data into models
    model_buffer* model_buffer = new lin::model_buffer;
    model_buffer->vertex_array_object = vertex_array;
    model_buffer->array_buffer = buffers[0];
    model_buffer->index_buffer = buffers[1];
    model_buffer->indices_size = indices_size;
    this->Models.insert(ModelList::value_type(file_name, model_buffer));
  } else {
    // failed to load throw an exception
    throw SceneInitExceptionContainer(
      new SceneInitException("Faile to load model %s", file_name));
  }
}

lin::DrawableModel* createDrawableModel(const char* file_name);

void
Scene::SceneImpl::addSdkMeshModel(const char* file_name)
{

  lin::DrawableModel* model = createDrawableModel(file_name);
  if (model == NULL)
    throw SceneInitExceptionContainer(
      new SceneInitException("Faile to load model %s", file_name));
  Models.insert(ModelList::value_type(file_name, model));
}

void
lin::Scene::SceneImpl::loadModels(SceneObject* sceneObject)
{
  int nModel;
  ModelDesc* models;

  nModel = sceneObject->getModelDesc(&models);

  if (nModel > 0) {
    for (int i = 0; i != nModel && i < MAX_MODEL; ++i) {

      if (Models.find(models[i].file_name) != Models.end()) {
        printInfo("Ignoring same model filename %s\n", models[i].file_name);
        continue;
      }
      const char* file_ext = strrchr(models[i].file_name, '.');
      if (!file_ext)
        throw SceneInitExceptionContainer(new SceneInitException(
          "Faile to load model %s;File extension is need!",
          models[i].file_name));
      file_ext += 1;
      if (strncasecmp(file_ext, "obj", 3) == 0)
        addObjModel(models[i].file_name);
      else if (strncasecmp(file_ext, "sdkmesh", 7) == 0)
        addSdkMeshModel(models[i].file_name);
      else
        throw SceneInitExceptionContainer(new SceneInitException(
          "Faile to load model %s;Unsupported type!", models[i].file_name));
    }

    // Finshed set the vertex array object to normal
    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }
}
}
