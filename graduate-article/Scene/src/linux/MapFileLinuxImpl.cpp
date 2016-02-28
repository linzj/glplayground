#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <unordered_map>

namespace lin {
void* mapWholeFile(const char* filename, int* length);
void unmapFile(void*);
typedef std::unordered_map<void*, int> FileMappingMap;
static FileMappingMap g_map;

class Disposer
{
public:
  ~Disposer()
  {
    for (auto& pair : g_map) {
      struct stat file_stat;

      if (-1 == fstat(pair.second, &file_stat))
        munmap(pair.first, 4096);
      else
        munmap(pair.first, file_stat.st_size);
    }
  }
};
static Disposer g_disposer;

void*
mapWholeFile(const char* filename, int* length)
{
  void* ret;
  int fd;

  fd = open(filename, O_RDONLY);

  if (fd == -1)
    return NULL;

  struct stat file_stat;

  if (-1 == fstat(fd, &file_stat)) {
    close(fd);
    return NULL;
  }

  *length = file_stat.st_size;

  ret = mmap(0, file_stat.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

  if (ret == MAP_FAILED) {
    close(fd);
    return NULL;
  }

  try {
    g_map.insert(FileMappingMap::value_type(ret, fd));
  } catch (...) {
    close(fd);
    return NULL;
  }

  return ret;
}

void
unmapFile(void* file)
{
  FileMappingMap::iterator end = g_map.end();
  FileMappingMap::iterator result = g_map.find(file);

  if (result == end)
    return;

  struct stat file_stat;

  if (-1 == fstat(result->second, &file_stat))
    munmap(result->first, 4096);
  else
    munmap(result->first, file_stat.st_size);

  close(result->second);

  g_map.erase(result);
}
}
