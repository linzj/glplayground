#ifndef LOG_H
#define LOG_H
#include <stdio.h>

namespace lin {
static inline void
printInfo(const char* str, ...)
{
#ifdef _DEBUG
  va_list ap;
  va_start(ap, str);
  vfprintf(stderr, str, ap);
  va_end(ap);
#endif
}
}

#endif /* LOG_H */
