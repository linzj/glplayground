CMAKE_MINIMUM_REQUIRED( VERSION 3.0 )

PROJECT( glew )

#FILE( GLOB GLEW lib/glew/*.c lib/glew/*.h )
#FILE( GLOB VECTORMATH lib/vectormath/*.h )
FILE( GLOB SRC src/glew.c )

ADD_LIBRARY(glew STATIC ${SRC})
TARGET_INCLUDE_DIRECTORIES (glew PUBLIC include)
