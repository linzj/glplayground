#ifndef MULTISAMPLESCENEOBJECT_HPP
#define MULTISAMPLESCENEOBJECT_HPP
#include <SceneObject.hpp>
#include <memory>
namespace lin {
struct DrawDesc;
}
class MultiSampleSceneObject : public lin::SceneObject
{
public:
  virtual int getPassDescs(lin::PassDesc**);
  virtual int getModelDesc(lin::ModelDesc**);
  virtual int getShaderDesc(lin::ShaderDesc**);
  virtual int getTextureDesc(lin::TextureDesc**);
  virtual int getProgramDesc(lin::ProgramDesc**);
  virtual int getFBODesc(lin::FBODesc**);
  virtual void draw(const lin::DrawDesc&);
  virtual void init(const lin::InitDesc&);
  virtual const char* getName() const { return "Mutlisample Scene Object"; }
public:
  MultiSampleSceneObject();
  ~MultiSampleSceneObject();

private:
  struct Impl;
  std::unique_ptr<Impl> m_private;

private:
  void pass_one(const lin::DrawDesc&);
  void pass_zero(const lin::DrawDesc&);
  void pass_two(const lin::DrawDesc&);
};

#endif // MULTISAMPLESCENEOBJECT_HPP
