CMAKE_MINIMUM_REQUIRED( VERSION 3.0 )
FILE( GLOB NV_SOURCES nvImage/src/*.c* nvImage/src/*.h* nvModel/include/*.h nvModel/src/*.c*)

ADD_LIBRARY( nvlib STATIC ${NV_SOURCES})
TARGET_INCLUDE_DIRECTORIES (nvlib PUBLIC nvImage/include/ nvModel/include/ )
TARGET_LINK_LIBRARIES( nvlib PUBLIC libpng )
