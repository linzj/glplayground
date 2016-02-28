#include <Scene.hpp>
#include "MultiSampleSceneObject.hpp"
#include <GL/glew.h>
#include <PassDesc.hpp>
#include <DrawDesc.hpp>
#include <InitDesc.hpp>
#include <DrawableModel.hpp>
#include <Vector.hpp>

struct MultiSampleSceneObject::Impl
{
  lin::Scene* m_scene;
  GLuint m_dummy_vertex_array;
  GLuint m_dummy_buffer;
};

MultiSampleSceneObject::MultiSampleSceneObject()
  : m_private(new MultiSampleSceneObject::Impl)
{
}

MultiSampleSceneObject::~MultiSampleSceneObject()
{
  using namespace lin;

  glDeleteVertexArrays(1, &m_private->m_dummy_vertex_array);
  glDeleteBuffers(1, &m_private->m_dummy_buffer);
  GLenum err = glGetError();
  lin::printInfo("glError occur;Code %04x", err);
}

using namespace lin;
extern int internal_func;
int
MultiSampleSceneObject::getPassDescs(lin::PassDesc** ppdesc)
{
  static const char* model_name = "dragon.sdkmesh";
  static const char* textures_name[2] = { "depth_fbo:COLOR_ATTACHMENT0",
                                          "depth_fbo:DEPTH_STENCIL" };
  static PassDesc passdescs_my[3] = {
    { "ClearStencil", 0, 0, "clearstencil_pass", 0, 0, "depth_fbo" },
    { "DrawDragon", &model_name, 1, "drawdragon_pass", 0, 0, "depth_fbo" },
    { "DrawQuads", 0, 0, "drawquads_pass", textures_name, 2, 0 }
  };
  static PassDesc passdescs_nvidia[3] = {
    { "ClearStencil", 0, 0, "clearstencil_pass", 0, 0, "depth_fbo" },
    { "DrawDragon", &model_name, 1, "drawdragon_pass", 0, 0, "depth_fbo" },
    { "DrawQuads", 0, 0, "drawquadsnvidia_pass", textures_name, 2, 0 }
  };
  if (internal_func == 1)
    *ppdesc = passdescs_my;
  else
    *ppdesc = passdescs_nvidia;
  return 3;
}

int
MultiSampleSceneObject::getModelDesc(lin::ModelDesc** ppdesc)
{
  static ModelDesc desc[1] = { { "dragon.sdkmesh" } };
  *ppdesc = desc;
  return 1;
}

int
MultiSampleSceneObject::getShaderDesc(lin::ShaderDesc** ppdesc)
{
  static ShaderDesc shaders[] = { { "drawquads_v.glsl", GL_VERTEX_SHADER },
                                  { "drawquads_f.glsl", GL_FRAGMENT_SHADER },
                                  { "drawdragon_f.glsl", GL_FRAGMENT_SHADER },
                                  { "drawdragon_v.glsl", GL_VERTEX_SHADER },
                                  { "sort_my.glsl", GL_FRAGMENT_SHADER },
                                  { "sort_nvidia.glsl", GL_FRAGMENT_SHADER } };
  *ppdesc = shaders;
  return 6;
}

int
MultiSampleSceneObject::getTextureDesc(lin::TextureDesc**)
{
  return 0;
}

int
MultiSampleSceneObject::getProgramDesc(lin::ProgramDesc** ppdesc)
{
  static const char* shaders[] = { "drawquads_v.glsl", "drawquads_f.glsl",
                                   "sort_my.glsl", "drawdragon_f.glsl",
                                   "drawdragon_v.glsl" };
  static const char* shaders_nvidia[] = { "drawquads_v.glsl",
                                          "drawquads_f.glsl",
                                          "sort_nvidia.glsl" };
  static const char* attrib_names[4] = { "position", 0, "normal" };
  static ProgramDesc desc[4] = {
    { "drawdragon_pass", shaders + 3, 2, attrib_names, 3, 0, 0, 0, 0 },
    { "drawquads_pass", shaders, 3, 0, 0, 0, 0, 0, 0 },
    { "clearstencil_pass", shaders, 1, 0, 0, 0, 0, 0, 0 },
    { "drawquadsnvidia_pass", shaders_nvidia, 3, 0, 0, 0, 0, 0, 0 }
  };
  *ppdesc = desc;
  return 4;
}

int
MultiSampleSceneObject::getFBODesc(lin::FBODesc** ppdesc)
{
  static AttachmentDesc attachment_descs[] = { { GL_DEPTH24_STENCIL8, 8, 2, 1 },
                                               { GL_RGB, 8, 2, 1 } };
  static FBODesc desc[1] = { { "depth_fbo", attachment_descs + 1, 1,
                               attachment_descs, 0 } };
  *ppdesc = desc;
  return 1;
}
static mat4 camera = translate(0, -0.15, -1.05);
void
MultiSampleSceneObject::draw(const lin::DrawDesc& desc)
{
  switch (desc.idraw) {
    case 0:
      pass_zero(desc);
      break;
    case 1:
      pass_one(desc);
      break;
    case 2:
      pass_two(desc);
      break;
  }
}

void
MultiSampleSceneObject::init(const lin::InitDesc& desc)
{
  m_private->m_scene = desc.m_scene;
  glGenVertexArrays(1, &m_private->m_dummy_vertex_array);
  glGenBuffers(1, &m_private->m_dummy_buffer);

  glBindVertexArray(m_private->m_dummy_vertex_array);
  glBindBuffer(GL_ARRAY_BUFFER, m_private->m_dummy_buffer);
  glBufferData(GL_ARRAY_BUFFER, 1, 0, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void
MultiSampleSceneObject::pass_two(const DrawDesc& desc)
{
  static GLuint tex_uniform = glGetUniformLocation(desc.program, "tex");
  static GLuint depth_uniform = glGetUniformLocation(desc.program, "depth_tex");
  glUniform1i(tex_uniform, 0);
  glUniform1i(depth_uniform, 1);
  glBindVertexArray(m_private->m_dummy_vertex_array);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);
}

void
MultiSampleSceneObject::pass_one(const DrawDesc& desc)
{
  static GLuint mvp_uniform = glGetUniformLocation(desc.program, "mvp");
  static GLuint normal_matrix_uniform =
    glGetUniformLocation(desc.program, "normal_matrix");
  float rx, ry;
  m_private->m_scene->getRotate(&rx, &ry);
  mat4 modelview = camera * rotateXY(-rx, -ry);
  mat4 inv_modelview = modelview;
  mat4 projection = glperspectiveMatrix(1.2, 0.1, 100.0);
  mat3 normal_matrix =
    mat3(inv_modelview.rows[0].xyz(), inv_modelview.rows[1].xyz(),
         inv_modelview.rows[2].xyz());
  glUniformMatrix4fv(mvp_uniform, 1, GL_TRUE, projection * modelview);
  glUniformMatrix3fv(normal_matrix_uniform, 1, GL_FALSE, normal_matrix);

  desc.models[0]->draw();
  glDisable(GL_STENCIL_TEST);
  glDisable(GL_DEPTH_TEST);
}

void
MultiSampleSceneObject::pass_zero(const DrawDesc& desc)
{
  glEnable(GL_MULTISAMPLE);
  glEnable(GL_STENCIL_TEST);
  glEnable(GL_SAMPLE_MASK);
  glEnable(GL_DEPTH_TEST);
  glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

  glBindVertexArray(m_private->m_dummy_vertex_array);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glDepthMask(GL_FALSE);
  static const int max_sample_bit = 1 << 8;
  for (int i = 1, j = 1; i <= max_sample_bit; i <<= 1, ++j) {
    glSampleMaski(0, i);

    glStencilFunc(GL_ALWAYS, j, 0xff);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }
  glDisable(GL_MULTISAMPLE);
  glDisable(GL_SAMPLE_MASK);
  glStencilFunc(GL_EQUAL, 0x1, 0xff);
  glStencilOp(GL_DECR, GL_DECR, GL_DECR);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDepthMask(GL_TRUE);
  glBindVertexArray(0);
}
