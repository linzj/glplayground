#ifndef DRAWDESC_HPP
#define DRAWDESC_HPP

namespace lin {

struct DrawableModel;

struct DrawDesc
{
  DrawableModel** models;
  GLuint program;
  GLuint fbo;
  GLuint idraw;
  const char* pass_name;
};
}

#endif // DRAWDESC_HPP
