#include "SceneImpl.hpp"
#include "../SceneObject.hpp"
#include "../GLError.hpp"
#include <stdio.h>
#include <string.h>
#include <memory>
#ifdef _WIN32
#define snprintf _snprintf
#endif
namespace lin {
static bool
isColorFormat(GLenum format)
{
  switch (format) {
    case GL_RGBA32F:
    case GL_RGBA32I:
    case GL_RGBA32UI:
    case GL_RGBA16:
    case GL_RGBA16F:
    case GL_RGBA16I:
    case GL_RGBA16UI:
    case GL_RGBA8:
    case GL_RGBA_SNORM:
    case GL_RGBA8_SNORM:
    case GL_RGBA8I:
    case GL_RGBA8UI:
    case GL_RGB10_A2:
    case GL_RGBA12:
    case GL_RGBA16_SNORM:
    case GL_SRGB8:
    case GL_SRGB8_ALPHA8:
    case GL_RED:
    case GL_R16F:
    case GL_RG:
    case GL_RG16F:
    case GL_RGB:
    case GL_RGBA:
      return true;
    default:
      return false;
  }
}

static bool
isDepthFormat(GLenum format)
{
  switch (format) {
    case GL_DEPTH_COMPONENT:
    case GL_DEPTH_STENCIL:
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32:
    case GL_DEPTH_COMPONENT32F:
    case GL_DEPTH32F_STENCIL8:
    case GL_DEPTH24_STENCIL8:
      return true;
    default:
      return false;
  }
}

static bool
isDepthStencilFormat(GLenum format)
{
  switch (format) {
    case GL_DEPTH_STENCIL:
    case GL_DEPTH32F_STENCIL8:
    case GL_DEPTH24_STENCIL8:
      return true;
    default:
      return false;
  }
}

#define clearResource()                                                        \
  {                                                                            \
    if (cur->nColors > 0)                                                      \
      glDeleteTextures(cur->nColors, scene_fbo_desc.colors);                   \
    if (cur->depth_stencil)                                                    \
      glDeleteTextures(1, &scene_fbo_desc.depth_stencil);                      \
    glBindFramebuffer(GL_FRAMEBUFFER, 0);                                      \
    glDeleteFramebuffers(1, &scene_fbo_desc.fbo);                              \
    glGetError();                                                              \
  }
void
Scene::SceneImpl::loadFBOs(SceneObject* psceneObject)
{
  FBODesc* desc;
  int nFBOs = psceneObject->getFBODesc(&desc);

  if (nFBOs <= 0)
    return;

  // Unused names in textures are silently ignored, as is the value zero
  // So deleting a value-zero texture is legal

  for (int i = 0; i != nFBOs; ++i) {
    FBODesc* cur = desc + i;
    SceneFBODesc scene_fbo_desc;
    memset(&scene_fbo_desc, 0, sizeof(SceneFBODesc));
    scene_fbo_desc.desc = *cur;
    glGenFramebuffers(1, &scene_fbo_desc.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, scene_fbo_desc.fbo);
    std::unique_ptr<SceneTextureDesc[]> color_texture_descs;
    SceneTextureDesc depth_stencil_desc = { 0 };

    if (cur->nColors > 0 && cur->nColors <= FBODesc::MAX_COLOR_ATTACHMENT) {
      glGenTextures(cur->nColors, scene_fbo_desc.colors);
      color_texture_descs.reset(new SceneTextureDesc[cur->nColors]);
      memset(color_texture_descs.get(), 0,
             sizeof(SceneTextureDesc) * cur->nColors);
      for (int i = 0; i != cur->nColors; ++i) {
        AttachmentDesc* cur_desc = &cur->colors[i];
        GLenum target;

        switch (cur_desc->dimension) {
          case 1:
            if (cur_desc->array_size > 1)
              target = GL_TEXTURE_1D_ARRAY;
            else
              target = GL_TEXTURE_1D;
            break;
          case 2:
            if (cur_desc->array_size > 1) {
              if (cur_desc->sample > 1)
                target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
              else
                target = GL_TEXTURE_2D_ARRAY;
            } else if (cur_desc->sample > 1)
              target = GL_TEXTURE_2D_MULTISAMPLE;
            else
              target = GL_TEXTURE_RECTANGLE;
            break;
          default:
            clearResource();
            throw SceneInitExceptionContainer(new SceneInitException(
              "Invalid attachment dimension;FBO name:%s;Attachment index %d",
              cur->name, i));
        }

        if (!isColorFormat(cur_desc->format)) {
          clearResource();
          throw SceneInitExceptionContainer(new SceneInitException(
            "Invalid color attachment format in FBO name %s", cur->name));
        }

        glBindTexture(target, scene_fbo_desc.colors[i]);

        scene_fbo_desc.targets[i] = target;
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                             scene_fbo_desc.colors[i], 0);
        glBindTexture(target, 0);
        SceneTextureDesc* texture_desc = color_texture_descs.get() + i;
        texture_desc->target = target;
        texture_desc->name = scene_fbo_desc.colors[i];
      }

      GLenum err;

      err = glGetError();

      if (err != GL_NO_ERROR) {
        clearResource();
        GLErrorContainer glerr(new GLError(err));
        throw glerr;
      }
    }
    // gen depth_stencil
    if (cur->depth_stencil) {
      if (!isDepthFormat(cur->depth_stencil->format)) {
        clearResource();
        throw SceneInitExceptionContainer(new SceneInitException(
          "Invalid depth attachment format in FBO name %s", cur->name));
      }

      glGenTextures(1, &scene_fbo_desc.depth_stencil);

      GLenum target;

      switch (cur->depth_stencil->dimension) {
        case 1:

          if (cur->depth_stencil->array_size > 1)
            target = GL_TEXTURE_1D_ARRAY;
          else
            target = GL_TEXTURE_1D;

          break;
        case 2:
          if (cur->depth_stencil->array_size > 1) {
            if (cur->depth_stencil->sample > 1)
              target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
            else
              target = GL_TEXTURE_2D_ARRAY;
          } else if (cur->depth_stencil->sample > 1)
            target = GL_TEXTURE_2D_MULTISAMPLE;
          else
            target = GL_TEXTURE_RECTANGLE;

          break;

        default:
          clearResource();
          throw SceneInitExceptionContainer(new SceneInitException(
            "Invalid attachment dimension;FBO name:%s;Attachment index %d",
            cur->name, i));
      }

      scene_fbo_desc.depth_stencil_target = target;
      depth_stencil_desc.target = target;
      depth_stencil_desc.name = scene_fbo_desc.depth_stencil;

      glBindTexture(target, scene_fbo_desc.depth_stencil);

      if (isDepthStencilFormat(cur->depth_stencil->format))
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                             scene_fbo_desc.depth_stencil, 0);
      else
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                             scene_fbo_desc.depth_stencil, 0);

      GLenum err;

      err = glGetError();

      if (err != GL_NO_ERROR) {
        clearResource();
        GLErrorContainer glerr(new GLError(err));
        throw glerr;
      }
      glBindTexture(target, 0);
    }
    // when get here all the color and depth stencil component is finished
    // initializing

    if (cur->fix_size) {
      updateFBO(scene_fbo_desc, cur->fix_size->width, cur->fix_size->height);
      SceneFBOsFixed.insert(
        SceneFBOList::value_type(cur->name, scene_fbo_desc));
    } else {
      SceneFBOsDynamic.insert(
        SceneFBOList::value_type(cur->name, scene_fbo_desc));
    }

    // insert all framebuffer attachment to textures
    if (cur->nColors > 0 && cur->nColors <= FBODesc::MAX_COLOR_ATTACHMENT) {
      for (int i = 0; i != cur->nColors; ++i) {
        SceneTextureDesc* cur_texture_desc = color_texture_descs.get() + i;
        int size = snprintf(NULL, 0, "%s:COLOR_ATTACHMENT%d", cur->name, i) + 1;
        std::unique_ptr<char[]> buf(new char[size]);
        snprintf(buf.get(), size, "%s:COLOR_ATTACHMENT%d", cur->name, i);
        Textures.insert(
          SceneTextureList::value_type(buf.get(), *cur_texture_desc));
      }
    }
    // depth_stencil
    if (cur->depth_stencil) {
      int size = snprintf(NULL, 0, "%s:DEPTH_STENCIL", cur->name) + 1;
      std::unique_ptr<char[]> buf(new char[size]);
      snprintf(buf.get(), size, "%s:DEPTH_STENCIL", cur->name);
      Textures.insert(
        SceneTextureList::value_type(buf.get(), depth_stencil_desc));
    }
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
#undef clearResource
}

static inline void
updateAttachment(const AttachmentDesc* pattachment, GLuint name, GLenum target,
                 int wx, int wy)
{
  static GLint max_sample = 0;

  if (!max_sample)
    glGetIntegerv(GL_MAX_SAMPLES, &max_sample);

  GLenum format = pattachment->format;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  GLenum extern_format = isDepthFormat(format) ? GL_DEPTH_COMPONENT : GL_RGB;
  switch (target) {

    case GL_TEXTURE_2D_MULTISAMPLE: {
      GLint samples =
        pattachment->sample > max_sample ? max_sample : pattachment->sample;
      glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, name);
      {
        GLenum err = glGetError();

        if (err != GL_NO_ERROR)
          throw GLErrorContainer(new GLError(err));
      }
      glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, wx,
                              wy, GL_TRUE);
      {
        GLenum err = glGetError();

        if (err != GL_NO_ERROR)
          throw GLErrorContainer(new GLError(err));
      }
      glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    }

    break;

    case GL_TEXTURE_2D:
      glBindTexture(GL_TEXTURE_2D, name);
      glTexImage2D(GL_TEXTURE_2D, 0, format, wx, wy, 0, extern_format,
                   GL_UNSIGNED_BYTE, 0);
      glBindTexture(GL_TEXTURE_2D, 0);
      break;

    case GL_TEXTURE_2D_ARRAY:
      glBindTexture(GL_TEXTURE_2D_ARRAY, name);
      glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, format, wx, wy,
                   pattachment->array_size, 0, extern_format, GL_UNSIGNED_BYTE,
                   0);
      glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
      break;

    case GL_TEXTURE_2D_MULTISAMPLE_ARRAY: {
      GLint samples =
        pattachment->sample > max_sample ? max_sample : pattachment->sample;
      glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, name);
      glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, samples, format,
                              wx, wy, pattachment->array_size, GL_TRUE);
      glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 0);
    }

    break;

    case GL_TEXTURE_1D:
      glBindTexture(GL_TEXTURE_1D, name);
      glTexImage1D(GL_TEXTURE_1D, 0, format, wx, 0, extern_format,
                   GL_UNSIGNED_BYTE, 0);
      glBindTexture(GL_TEXTURE_1D, 0);
      break;

    case GL_TEXTURE_1D_ARRAY:
      glBindTexture(GL_TEXTURE_1D_ARRAY, name);
      glTexImage2D(GL_TEXTURE_1D_ARRAY, 0, format, wx, pattachment->array_size,
                   0, extern_format, GL_UNSIGNED_BYTE, 0);
      glBindTexture(GL_TEXTURE_1D_ARRAY, 0);
      break;
    case GL_TEXTURE_RECTANGLE:
      glBindTexture(GL_TEXTURE_RECTANGLE, name);

      glTexImage2D(GL_TEXTURE_RECTANGLE, 0, format, wx, wy, 0, extern_format,
                   GL_UNSIGNED_BYTE, 0);
      glBindTexture(GL_TEXTURE_RECTANGLE, 0);
      break;
  }

  GLenum err = glGetError();

  if (err != GL_NO_ERROR)
    throw GLErrorContainer(new GLError(err));
}

void
updateFBO(const SceneFBODesc& desc, int wx, int wy)
{
  // update colors
  if (desc.desc.nColors > 0) {
    for (int i = 0; i != desc.desc.nColors; ++i) {
      updateAttachment(&desc.desc.colors[i], desc.colors[i], desc.targets[i],
                       wx, wy);
    }
  }

  // update depth
  if (desc.desc.depth_stencil) {
    updateAttachment(desc.desc.depth_stencil, desc.depth_stencil,
                     desc.depth_stencil_target, wx, wy);
  }
}
}
