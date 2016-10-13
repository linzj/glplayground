#include "sdkmesh.h"
#include "../DrawableModel.hpp"
#include <GL/glew.h>
namespace lin {
void* mapWholeFile(const char* filename, size_t* length);
void unmapFile(void*);

class SdkModel : public lin::DrawableModel
{
public:
  SdkModel();
  ~SdkModel();

public:
  bool loadSdkMeshFromFile(const char* filename);
  virtual void draw();

private:
  GLenum index_type;
  GLuint vertex_array;
  GLuint buffer[3];
  GLsizei* index_counts;
  const GLvoid** index_starts;
  GLint* vertex_starts;
  GLuint numsubsets;

private:
  bool _allocateSubsets(int count);
  void initInDirectBufferIfOkay(SDKMESH_MESH*, SDKMESH_SUBSET*, UINT*);
};

SdkModel::SdkModel()
  : index_type(GL_UNSIGNED_SHORT)
  , vertex_array(0)
  , index_counts(0)
  , index_starts(0)
  , vertex_starts(0)
  , numsubsets(0)
{
  buffer[0] = buffer[1] = 0;
}
static void
adjustFloat3(char* content, UINT64 stride, UINT looptime)
{
  looptime /= 3;
  UINT stride3 = stride * 3;
  UINT stride2 = stride * 2;
  looptime += 1;
  for (UINT i = 0; i != looptime; ++i, content += stride3) {
    float* cur1 = (float*)content;
    float* cur2 = (float*)(content + stride);
    float* cur3 = (float*)(content + stride2);

    cur1[2] = -cur1[2];
    cur2[2] = -cur2[2];
    cur3[2] = -cur3[2];

    /*float tmp1, tmp2, tmp3;
    tmp1 = cur1[0];
    tmp2 = cur1[1];
    tmp3 = cur1[2];

    cur1[0] = cur3[0];
    cur1[1] = cur3[1];
    cur1[2] = cur3[2];

    cur3[0] = tmp1;
    cur3[1] = tmp2;
    cur3[2] = tmp3;*/
  }
}

static void
adjustTexcoord(char* content, UINT64 stride, UINT looptime)
{
  for (UINT i = 0; i != looptime; ++i, content += stride) {
    float* cur = (float*)content;
    cur[1] = 1.0 - cur[1];
  }
}

bool
SdkModel::loadSdkMeshFromFile(const char* filename)
{
  char* file_content;
  size_t file_size;
  SDKMESH_HEADER* sdkmesh_header;
  SDKMESH_MESH* mesh;
  SDKMESH_SUBSET* subset;
  UINT* subset_indices;
  SDKMESH_VERTEX_BUFFER_HEADER* vertex_buffer_header;
  SDKMESH_INDEX_BUFFER_HEADER* index_buffer_header;
  UINT loop_time;

  file_content = (char*)lin::mapWholeFile(filename, &file_size);
  if (file_content == NULL) {
    return false;
  }

  sdkmesh_header = (SDKMESH_HEADER*)file_content;
  if (sdkmesh_header->Version != SDKMESH_FILE_VERSION) {
    goto failed;
  }

  if (sdkmesh_header->HeaderSize + sdkmesh_header->NonBufferDataSize +
        sdkmesh_header->BufferDataSize >
      file_size) {
    goto failed;
  }
  mesh = (SDKMESH_MESH*)(file_content + sdkmesh_header->MeshDataOffset);

  if (mesh->NumVertexBuffers > 1)
    goto failed; // sorry.... but this is opengl
  subset = (SDKMESH_SUBSET*)(file_content + sdkmesh_header->SubsetDataOffset);
  subset_indices = (UINT*)(file_content + mesh->SubsetOffset);
  if (!_allocateSubsets(mesh->NumSubsets)) {
    goto failed; // no memory
  }
  numsubsets = mesh->NumSubsets;
  using namespace lin;
  for (UINT i = 0; i != mesh->NumSubsets; ++i) {
    UINT* cur_index = subset_indices + i;
    SDKMESH_SUBSET* cur = subset + *cur_index;
    if (cur->PrimitiveType != PT_TRIANGLE_LIST)
      goto failed;
    index_starts[i] = (const GLvoid*)cur->IndexStart;
    index_counts[i] = cur->IndexCount;
    vertex_starts[i] = cur->VertexStart;
  }

  glGetError(); // clear the error flags
  glGenVertexArrays(1, &vertex_array);
  if (glGetError() != GL_NO_ERROR) {
    goto failed;
  }
  glGenBuffers(3, buffer);
  if (glGetError() != GL_NO_ERROR)
    goto failed;

  glBindVertexArray(vertex_array);
  glBindBuffer(GL_ARRAY_BUFFER, buffer[0]);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer[1]);

  vertex_buffer_header =
    ((SDKMESH_VERTEX_BUFFER_HEADER*)(file_content +
                                     sdkmesh_header
                                       ->VertexStreamHeadersOffset)) +
    mesh->VertexBuffers[0];

  index_buffer_header =
    (SDKMESH_INDEX_BUFFER_HEADER*)(file_content +
                                   sdkmesh_header->IndexStreamHeadersOffset) +
    mesh->IndexBuffer;
  loop_time =
    vertex_buffer_header->SizeBytes / vertex_buffer_header->StrideBytes;
  for (int i = 0; i != MAX_VERTEX_ELEMENTS; ++i) {
    D3DVERTEXELEMENT9* cur = vertex_buffer_header->Decl + i;
    int index = -1;
    int size = cur->Type;
    if (size > 3)
      continue;
    switch (cur->Usage) {
      case D3DDECLUSAGE_POSITION:
        index = 0;
        if (size >= 2)
          adjustFloat3(file_content + vertex_buffer_header->DataOffset +
                         cur->Offset,
                       vertex_buffer_header->StrideBytes, loop_time);
        break;
      case D3DDECLUSAGE_TEXCOORD:
        index = 1;
        adjustTexcoord(file_content + vertex_buffer_header->DataOffset +
                         cur->Offset,
                       vertex_buffer_header->StrideBytes, loop_time);
        break;
      case D3DDECLUSAGE_NORMAL:
        index = 2;
        if (size >= 2)
          adjustFloat3(file_content + vertex_buffer_header->DataOffset +
                         cur->Offset,
                       vertex_buffer_header->StrideBytes, loop_time);
        break;
      case D3DDECLUSAGE_TANGENT:
        index = 3;
        if (size >= 2)
          adjustFloat3(file_content + vertex_buffer_header->DataOffset +
                         cur->Offset,
                       vertex_buffer_header->StrideBytes, loop_time);
        break;
    }
    if (index < 0)
      continue;

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 1 + size, GL_FLOAT, GL_FALSE,
                          vertex_buffer_header->StrideBytes,
                          (GLvoid*)(GLintptr)cur->Offset);
  }
  switch (index_buffer_header->IndexType) {
    case 0:
      index_type = GL_UNSIGNED_SHORT;
      break;
    case 1:
      index_type = GL_UNSIGNED_INT;
      break;
  }
  glBufferData(GL_ARRAY_BUFFER, vertex_buffer_header->SizeBytes,
               file_content + vertex_buffer_header->DataOffset, GL_STATIC_DRAW);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_buffer_header->SizeBytes,
               file_content + index_buffer_header->DataOffset, GL_STATIC_DRAW);
  initInDirectBufferIfOkay(mesh, subset, subset_indices);
  lin::unmapFile(file_content);

  if (GL_NO_ERROR != glGetError()) {
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vertex_array);
    glDeleteBuffers(2, buffer);
    return false;
  }
  glBindVertexArray(0);
  return true;
failed:
  lin::unmapFile(file_content);
  return false;
}

SdkModel::~SdkModel()
{
  using namespace lin;
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, buffer[0]);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer[1]);
  glDeleteVertexArrays(1, &vertex_array);
  glDeleteBuffers(3, buffer);
  if (index_counts) {
    delete[] index_counts;
  }
  if (index_starts) {
    delete[] index_starts;
  }
  if (vertex_starts) {
    delete[] vertex_starts;
  }
}

void
SdkModel::draw()
{
  using namespace lin;
  glBindVertexArray(vertex_array);
  glFrontFace(GL_CW);
  if (!GLEW_VERSION_4_3) {
    glMultiDrawElementsBaseVertex(GL_TRIANGLES, index_counts, index_type,
                                  index_starts, numsubsets, vertex_starts);
  } else {
    glMultiDrawElementsIndirect(GL_TRIANGLES, index_type, nullptr, numsubsets,
                                0);
  }
  glFrontFace(GL_CCW);
  glBindVertexArray(0);
}

bool
SdkModel::_allocateSubsets(int count)
{
  if (index_counts) {
    delete[] index_counts;
    index_counts = 0;
  }
  if (index_starts) {
    delete[] index_starts;
    index_starts = 0;
  }
  if (vertex_starts) {
    delete[] vertex_starts;
    vertex_starts = 0;
  }

  try {
    index_counts = new GLsizei[count];
    index_starts = new const GLvoid*[count];
    vertex_starts = new GLint[count];
  } catch (...) {
    if (index_counts) {
      delete[] index_counts;
      index_counts = 0;
    }
    if (index_starts) {
      delete[] index_starts;
      index_starts = 0;
    }
    if (vertex_starts) {
      delete[] vertex_starts;
      vertex_starts = 0;
    }
    return false;
  }
  return true;
}

lin::DrawableModel*
createDrawableModel(const char* file_name)
{
  SdkModel* model = new SdkModel();
  if (!model->loadSdkMeshFromFile(file_name))
    return NULL;
  return model;
}
void
SdkModel::initInDirectBufferIfOkay(SDKMESH_MESH* mesh, SDKMESH_SUBSET* subset,
                                   UINT* subset_indices)
{
  typedef struct
  {
    GLuint count;
    GLuint instanceCount;
    GLuint firstIndex;
    GLuint baseVertex;
    GLuint baseInstance;
  } DrawElementsIndirectCommand;
  DrawElementsIndirectCommand* cmds;
  if (!GLEW_VERSION_4_3) {
    return;
  }
  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer[2]);
  glBufferData(GL_DRAW_INDIRECT_BUFFER,
               sizeof(DrawElementsIndirectCommand) * mesh->NumSubsets, nullptr,
               GL_DYNAMIC_DRAW);
  cmds = static_cast<DrawElementsIndirectCommand*>(
    glMapBuffer(GL_DRAW_INDIRECT_BUFFER, GL_WRITE_ONLY));
  for (UINT i = 0; i != mesh->NumSubsets; ++i) {
    UINT* cur_index = subset_indices + i;
    SDKMESH_SUBSET* cur = subset + *cur_index;
    if (cur->PrimitiveType != PT_TRIANGLE_LIST)
      __builtin_unreachable();
    DrawElementsIndirectCommand* curcmd = cmds + i;
    curcmd->count = cur->IndexCount;
    curcmd->instanceCount = 1;
    curcmd->firstIndex = cur->IndexStart;
    curcmd->baseVertex = 0;
    curcmd->baseInstance = 0;
  }
  glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
}
}
