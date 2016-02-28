#ifndef INITDESC_HPP
#define INITDESC_HPP

namespace lin {

struct Scene;

struct InitDesc
{
  Scene* m_scene;
  float max_anisotropy;
};
}

#endif // INITDESC_HPP
