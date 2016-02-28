#include "../Scene.hpp"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

namespace lin {
static char*
make_message(const char* fmt, va_list ap1, va_list ap2)
{
  /* Guess we need no more than 100 bytes. */
  int n, size;
  char* p;
  n = vsnprintf(NULL, 0, fmt, ap1);

  if (n > -1)
    size = n + 1;
  else
    return NULL;

  p = (char*)malloc(size);

  vsnprintf(p, size, fmt, ap2);

  return p;
}

SceneInitException::SceneInitException(const char* msg, ...)
{
  va_list ap1, ap2;
  va_start(ap1, msg);
  va_start(ap2, msg);
  m_msg = make_message(msg, ap1, ap2);
  va_end(ap1);
  va_end(ap2);
}

SceneInitException::SceneInitException(const std::shared_ptr<GLError>& err,
                                       const char* msg, ...)
  : m_error(err)
{
  va_list ap1, ap2;
  va_start(ap1, msg);
  va_start(ap2, msg);
  m_msg = make_message(msg, ap1, ap2);
  va_end(ap1);
  va_end(ap2);
}

SceneInitException::~SceneInitException() throw()
{
  if (m_msg)
    free(m_msg);
}
}
