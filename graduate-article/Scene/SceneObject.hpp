#ifndef SCENEOBJECT_HPP
#define SCENEOBJECT_HPP

namespace lin {

struct PassDesc;

struct InitDesc;

struct DrawDesc;

struct ModelDesc;

struct ShaderDesc;

struct TextureDesc;

struct ProgramDesc;

struct FBODesc;

class SceneObject
{

public:
  virtual ~SceneObject() {}

public:
  virtual int getPassDescs(PassDesc**) = 0;
  virtual int getModelDesc(ModelDesc**) = 0;
  virtual int getShaderDesc(ShaderDesc**) = 0;
  virtual int getTextureDesc(TextureDesc**) = 0;
  virtual int getProgramDesc(ProgramDesc**) = 0;
  virtual int getFBODesc(FBODesc**) = 0;
  virtual void draw(const DrawDesc&) = 0;
  virtual void init(const InitDesc&) = 0;
  virtual const char* getName() const = 0;
};
}

#endif // SCENEOBJECT_HPP
