#include "SceneImpl.hpp"
#include <nvImage.h>
#include "../SceneObject.hpp"
#include "../GLError.hpp"
namespace lin {
static void
setTexParameterByOption(GLenum target, const TextureOptionalDesc& desc)
{
  glTexParameteri(target, GL_TEXTURE_WRAP_S, desc.wrap_s);
  glTexParameteri(target, GL_TEXTURE_WRAP_T, desc.wrap_t);
  glTexParameteri(target, GL_TEXTURE_WRAP_R, desc.wrap_r);

  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, desc.min);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, desc.mag);

  if (desc.anisotropy != 0.0)
    glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, desc.anisotropy);

  GLenum error;

  error = glGetError();

  if (error != GL_NO_ERROR) {
    throw GLErrorContainer(new GLError(error));
  }
}

static nv::Image* loadImageFromFile(const char*);
void
Scene::SceneImpl::loadTextures(SceneObject* sceneObject)
{
  int nTextures;
  TextureDesc* desc;

  nTextures = sceneObject->getTextureDesc(&desc);

  if (nTextures <= 0)
    return;

  for (int i = 0; i != nTextures; ++i) {
    TextureDesc* cur = desc + i;

    if (Textures.find(cur->name) != Textures.end()) {
      printInfo("Ignoring same texture name! %s\n", cur->name);
      continue;
    }

    GLuint texRes;

    std::unique_ptr<nv::Image> image[MAX_TEXTURE];
    // load texture according to its format

    switch (cur->type) {
      case GL_TEXTURE_1D:
      case GL_TEXTURE_2D:
      case GL_TEXTURE_3D:
      case GL_TEXTURE_CUBE_MAP:
      case GL_TEXTURE_2D_ARRAY:

        for (int i = 0; i != cur->nFiles && i != MAX_TEXTURE; ++i) {
          image[i].reset(loadImageFromFile(cur->FileName[i]));

          if (!image[i].get()) {
            printInfo("Fail to load image as texture ;File name is %s\n",
                      cur->FileName[i]);
            throw GLErrorContainer(new GLError(GL_OUT_OF_MEMORY));
            continue;
          }
        }
        break;
      default:
        throw GLErrorContainer(new GLError(GL_INVALID_VALUE));
        break;
    }

    glGenTextures(1, &texRes);

    glBindTexture(cur->type, texRes);

    if (cur->option)
      setTexParameterByOption(cur->type, *cur->option);
    else {
      TextureOptionalDesc desc;
      desc.wrap_s = GL_CLAMP_TO_EDGE;
      desc.wrap_t = GL_CLAMP_TO_EDGE;
      desc.wrap_r = GL_CLAMP_TO_EDGE;
      desc.min = GL_NEAREST;
      desc.mag = GL_LINEAR;

      setTexParameterByOption(cur->type, desc);
    }

    switch (cur->type) {

      case GL_TEXTURE_1D:

        if (image[0]->isCompressed()) {
          int mipLevel = image[0]->getMipLevels();
          GLenum internal_format = cur->internal_format
                                     ? cur->internal_format
                                     : image[0]->getInternalFormat();
          int width = image[0]->getWidth();

          for (int i = 0; i != mipLevel; ++i) {
            glCompressedTexImage1D(GL_TEXTURE_1D, i, internal_format, width, 0,
                                   image[0]->getImageSize(i),
                                   image[0]->getLevel(i));
            width >>= 1;
          }
        } else {
          int mipLevel = image[0]->getMipLevels();
          GLenum internal_format = cur->internal_format
                                     ? cur->internal_format
                                     : image[0]->getInternalFormat();
          int width = image[0]->getWidth();

          for (int i = 0; i != mipLevel; ++i) {
            glTexImage1D(GL_TEXTURE_1D, i, internal_format, width, 0,
                         image[0]->getFormat(), image[0]->getType(),
                         image[0]->getLevel(i));
            width >>= 1;
          }
        }

        break;

      case GL_TEXTURE_2D:

        if (image[0]->isCompressed()) {
          int mipLevel = image[0]->getMipLevels();
          GLenum internal_format = cur->internal_format
                                     ? cur->internal_format
                                     : image[0]->getInternalFormat();
          int width = image[0]->getWidth(), height = image[0]->getHeight();

          for (int i = 0; i != mipLevel; ++i) {
            glCompressedTexImage2D(GL_TEXTURE_2D, i, internal_format, width,
                                   height, 0, image[0]->getImageSize(i),
                                   image[0]->getLevel(i));
            width >>= 1;
            height >>= 1;
          }
        } else {
          int mipLevel = image[0]->getMipLevels();
          GLenum internal_format = cur->internal_format
                                     ? cur->internal_format
                                     : image[0]->getInternalFormat();
          int width = image[0]->getWidth(), height = image[0]->getHeight();

          for (int i = 0; i != mipLevel; ++i, width >>= 1, height >>= 1) {
            glTexImage2D(GL_TEXTURE_2D, i, internal_format, width, height, 0,
                         image[0]->getFormat(), image[0]->getType(),
                         image[0]->getLevel(i));
          }
        }

        break;

      case GL_TEXTURE_3D:

        if (image[0]->isCompressed()) {
          int mipLevel = image[0]->getMipLevels();
          GLenum internal_format = cur->internal_format
                                     ? cur->internal_format
                                     : image[0]->getInternalFormat();
          int width = image[0]->getWidth(), height = image[0]->getHeight(),
              depth = image[0]->getDepth();

          for (int i = 0; i != mipLevel; ++i) {
            glCompressedTexImage3D(GL_TEXTURE_3D, i, internal_format, width,
                                   height, depth, 0, image[0]->getImageSize(i),
                                   image[0]->getLevel(i));
            width >>= 1;
            height >>= 1;
            depth >>= 1;
          }
        } else {
          int mipLevel = image[0]->getMipLevels();
          GLenum internal_format = cur->internal_format
                                     ? cur->internal_format
                                     : image[0]->getInternalFormat();
          int width = image[0]->getWidth(), height = image[0]->getHeight(),
              depth = image[0]->getDepth();

          for (int i = 0; i != mipLevel; ++i) {
            glTexImage3D(GL_TEXTURE_3D, i, internal_format, width, height,
                         depth, 0, image[0]->getFormat(), image[0]->getType(),
                         image[0]->getLevel(i));
            width >>= 1;
            height >>= 1;
            depth >>= 1;
          }
        }

        break;

      case GL_TEXTURE_CUBE_MAP:

        if (image[0]->getFaces() == 6) {
          int mipLevel = image[0]->getMipLevels();
          GLenum internal_format = cur->internal_format
                                     ? cur->internal_format
                                     : image[0]->getInternalFormat();
          int width = image[0]->getWidth(), height = image[0]->getHeight();

          if (image[0]->isCompressed()) {
            for (int i = 0; i != mipLevel; ++i) {
              GLuint image_size = image[0]->getImageSize(i);
              glCompressedTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X, i, internal_format, width,
                height, 0, image_size,
                image[0]->getLevel(i, GL_TEXTURE_CUBE_MAP_POSITIVE_X));
              glCompressedTexImage2D(
                GL_TEXTURE_CUBE_MAP_NEGATIVE_X, i, internal_format, width,
                height, 0, image_size,
                image[0]->getLevel(i, GL_TEXTURE_CUBE_MAP_NEGATIVE_X));

              glCompressedTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_Y, i, internal_format, width,
                height, 0, image_size,
                image[0]->getLevel(i, GL_TEXTURE_CUBE_MAP_POSITIVE_Y));
              glCompressedTexImage2D(
                GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, i, internal_format, width,
                height, 0, image_size,
                image[0]->getLevel(i, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y));
              glCompressedTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_Z, i, internal_format, width,
                height, 0, image_size,
                image[0]->getLevel(i, GL_TEXTURE_CUBE_MAP_POSITIVE_Z));
              glCompressedTexImage2D(
                GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, i, internal_format, width,
                height, 0, image_size,
                image[0]->getLevel(i, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z));
              width >>= 1;
              height >>= 1;
            }
          } else {
            GLenum format = image[0]->getFormat();
            GLenum type = image[0]->getType();

            for (int i = 0; i != mipLevel; ++i, width >>= 1, height >>= 1) {

              glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X, i, internal_format, width,
                height, 0, format, type,
                image[0]->getLevel(i, GL_TEXTURE_CUBE_MAP_POSITIVE_X));
              glTexImage2D(
                GL_TEXTURE_CUBE_MAP_NEGATIVE_X, i, internal_format, width,
                height, 0, format, type,
                image[0]->getLevel(i, GL_TEXTURE_CUBE_MAP_NEGATIVE_X));

              glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_Y, i, internal_format, width,
                height, 0, format, type,
                image[0]->getLevel(i, GL_TEXTURE_CUBE_MAP_POSITIVE_Y));
              glTexImage2D(
                GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, i, internal_format, width,
                height, 0, format, type,
                image[0]->getLevel(i, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y));

              glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_Z, i, internal_format, width,
                height, 0, format, type,
                image[0]->getLevel(i, GL_TEXTURE_CUBE_MAP_POSITIVE_Z));
              glTexImage2D(
                GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, i, internal_format, width,
                height, 0, format, type,
                image[0]->getLevel(i, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z));
            }
          }
        } else if (cur->nFiles == 6) {
          GLenum internal_format = cur->internal_format
                                     ? cur->internal_format
                                     : image[0]->getInternalFormat();

          for (int i = 0; i != 6; ++i) {
            if (image[0]->isCompressed()) {
              glCompressedTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internal_format,
                image[i]->getWidth(), image[i]->getHeight(), 0,
                image[i]->getImageSize(0), image[i]->getLevel(0));
            } else {
              glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
                           internal_format, image[i]->getWidth(),
                           image[i]->getHeight(), 0, image[i]->getFormat(),
                           image[i]->getType(), image[0]->getLevel(0));
            }
          }
        }

        break;

      case GL_TEXTURE_2D_ARRAY:

        if (image[0]->isCompressed()) {
          int width = image[0]->getWidth(), height = image[0]->getHeight();

          int mipLevel = image[0]->getMipLevels();
          int width1 = width, height1 = height;
          GLenum internal_format = image[0]->getFormat();

          for (int i = 0; i != mipLevel; ++i, width1 >>= 1, height1 >>= 1) {
            glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, i, internal_format,
                                   width1, height1, cur->nFiles,
                                   0, // border
                                   image[0]->getImageSize(i), 0);
          }

          for (int j = 0; j != cur->nFiles && j != MAX_TEXTURE; ++j) {
            width1 = width, height1 = height;

            for (int i = 0; i != mipLevel; ++i, width1 >>= 1, height1 >>= 1) {
              glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, i, 0, 0, j, width1,
                                        height1, 1, internal_format,
                                        image[j]->getImageSize(i),
                                        image[j]->getLevel(i));
            }
          }
        } else {
          int mipLevel = image[0]->getMipLevels();
          int width = image[0]->getWidth(), height = image[0]->getHeight();
          GLenum format = image[0]->getFormat(), type = image[0]->getType();
          GLenum internal_format = cur->internal_format
                                     ? cur->internal_format
                                     : image[0]->getInternalFormat();
          int width1 = width, height1 = height;

          for (int i = 0; i != mipLevel; ++i, width1 >>= 1, height >>= 1) {
            glTexImage3D(GL_TEXTURE_2D_ARRAY, i, internal_format, width1,
                         height1, cur->nFiles,
                         0, // border
                         format, type, 0);
          }

          for (int j = 0; j != cur->nFiles && j != MAX_TEXTURE; ++j) {
            width1 = width, height1 = height;

            for (int i = 0; i != mipLevel; ++i, width1 >>= 1, height1 >>= 1) {
              glTexSubImage3D(GL_TEXTURE_2D_ARRAY, i, 0, 0, j, width1, height1,
                              1, format, type, image[j]->getLevel(i));
            }
          }
        }

        break;
    }

    GLenum error = glGetError();

    glBindTexture(cur->type, 0);

    if (error != GL_NO_ERROR) {
      glDeleteTextures(1, &texRes);
      throw GLErrorContainer(new GLError(error));
    }
    {
      SceneTextureDesc desc = { cur->type, texRes };
      Textures.insert(SceneTextureList::value_type(cur->name, desc));
    }
  }
}

nv::Image*
loadImageFromFile(const char* file)
{
  std::unique_ptr<nv::Image> image(new nv::Image());

  if (!image->loadImageFromFile(file)) {
    return NULL;
  }

  return image.release();
}
}
