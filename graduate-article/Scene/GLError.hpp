#ifndef GLERROR_HPP
#define GLERROR_HPP

#include <GL/glew.h>
#include <exception>
#include <memory>

namespace lin {

class GLError : public std::exception
{

public:
  explicit GLError(GLenum error = GL_NO_ERROR)
    : m_error(error)
  {
  }

  virtual ~GLError() throw() {}

public:
  GLenum getError() const { return m_error; }

  virtual const char* what() const throw()
  {
    return "An OpenGL Internal Error";
  }

private:
  GLenum m_error;
};

typedef std::shared_ptr<GLError> GLErrorContainer;
}

#endif // GLERROR_HPP
