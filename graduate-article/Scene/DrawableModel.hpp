#ifndef DRAWABLEMODEL_HPP
#define DRAWABLEMODEL_HPP

namespace lin {

class DrawableModel
{

public:
  virtual ~DrawableModel() {}

  virtual void draw() = 0;
};
}

#endif // DRAWABLEMODEL_HPP
