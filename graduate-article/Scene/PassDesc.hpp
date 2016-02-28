#ifndef PASSDESC_HPP
#define PASSDESC_HPP
#include "InitDesc.hpp"
#include "DrawDesc.hpp"
#include "DrawableModel.hpp"

namespace lin {

struct ModelDesc
{
  // int name;
  const char* file_name;
};

struct ShaderDesc;

struct TextureDesc;

struct TextureOptionalDesc;

struct FBODesc;

struct ProgramDesc;
static const int MAX_TEXTURE = 32;
/*	enum AttributeType {
            TYPE_GENERIC  = 0,
            TYPE_VERTEX   = 1,
            TYPE_TEXCOORD = 2,
            TYPE_NORMAL   = 3,
            TYPE_TANGENT  = 4,
            TYPE_BINORMAL = 5,
    };
*/

struct PassDesc
{
  const char* pass_name;
  const char* const* const Models;
  int nModels;
  const char* Program;
  const char* const* const Textures;
  int nTextures;
  const char* FBO;
};

struct TextureDesc
{
  const char* name;
  const char* FileName[MAX_TEXTURE];
  GLenum type;
  int internal_format;
  int nFiles;
  TextureOptionalDesc* option;
};

struct TextureOptionalDesc
{
  int wrap_s;
  int wrap_t;
  int wrap_r;

  int min;
  int mag;
  float anisotropy;
};

struct ShaderDesc
{
  //		const char * name;
  const char* file_name; // filename also as name
  int type;
};

struct AttachmentDesc
{
  int format;
  int sample;
  int dimension;
  int array_size;
};

struct Size
{
  int width;
  int height;
};

struct FBODesc
{
  const char* name;
  static const int MAX_COLOR_ATTACHMENT = 32;
  AttachmentDesc* colors;
  int nColors;
  AttachmentDesc* depth_stencil; // enable depth stencil if set

  Size* fix_size; // if set the FBO will be fix-size
};

struct ProgramDesc
{
  const char* name;
  const char* const* const shaders;
  int nShader;
  const char* const* const attribute_names;
  int nAttributes;
  const char* const* const color_names;
  int nColors;
  const char* const* const uniform_names;
  int nUniforms;
};
}

#endif // PASSDESC_HPP
