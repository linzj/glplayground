#include <Windows.h>
namespace lin {
void*
mapWholeFile(const char* filename, int* length)
{
  HANDLE hFile = NULL;
  HANDLE hFileMapping = NULL;
  DWORD dwFileLength = 0;
  void* ret;

  hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                      0, OPEN_EXISTING, 0, 0);
  if (hFile == INVALID_HANDLE_VALUE)
    return NULL;
  dwFileLength = GetFileSize(hFile, 0);

  hFileMapping = CreateFileMapping(hFile, 0, PAGE_READWRITE, 0, 0, 0);
  if (hFileMapping == NULL) {
    DWORD err;
    err = GetLastError();
    goto failed;
  }

  ret =
    MapViewOfFile(hFileMapping, FILE_MAP_READ | FILE_MAP_WRITE | FILE_MAP_COPY,
                  0, 0, dwFileLength);
  if (ret == NULL)
    goto failed;
  // try
  //{
  //	g_map.insert(Map::value_type(ret,hFileMapping));
  //}
  // catch (...)
  //{
  //	goto failed;
  //}

  *length = dwFileLength;
  CloseHandle(hFile);
  return ret;

failed:
  CloseHandle(hFile);
  if (hFileMapping)
    CloseHandle(hFileMapping);
  *length = NULL;
  return NULL;
}

void
unmapFile(void* p)
{
  // try
  //{
  //	Map::iterator found = g_map.find(p);
  //	if(found == g_map.end())
  //	{
  //		return ;
  //	}
  //	UnmapViewOfFile(p);
  //	CloseHandle(found->second);
  //	g_map.erase(found);
  //}
  // catch (...)
  //{
  //
  //}
  UnmapViewOfFile(p);
}
}