CMAKE_MINIMUM_REQUIRED( VERSION 3.0 )

PROJECT( glplayground )

IF( WIN32 )

    #FILE( GLOB PEZ
    #    lib/libpez/pez.h
    #    lib/libpez/pez.windows.c
    #)

    #SOURCE_GROUP( "shaders" FILES Geodesic.glsl )
    #SOURCE_GROUP( "glew" FILES ${GLEW} )
    #SOURCE_GROUP( "glsw" FILES ${GLSW} )
    #SOURCE_GROUP( "vectormath" FILES ${VECTORMATH} )
    #SOURCE_GROUP( "pez" FILES ${PEZ} )

    #ADD_DEFINITIONS( /wd4996 /TP )
    SET( PLATFORM_LIBS opengl32 glu32)
    #SET( CONSOLE_SYSTEM WIN32 )
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

ELSEIF( APPLE )

    # Create a nibless project per http://blog.hyperjeff.net/code?id=292
    
    #FILE( GLOB PEZ
    #    lib/libpez/pez.h
    #    lib/libpez/pez.cocoa.m
    #)

    FIND_LIBRARY( COCOA_LIBRARY Cocoa )

    SET( PLATFORM_LIBS
        ${COCOA_LIBRARY}
        /System/Library/Frameworks/OpenGL.framework )

ELSEIF( UNIX )

    FIND_PACKAGE(OpenGL REQUIRED)
    INCLUDE_DIRECTORIES(${OpenGL_INCLUDE_DIRS})
    LINK_DIRECTORIES(${OpenGL_LIBRARY_DIRS})
    ADD_DEFINITIONS(${OpenGL_DEFINITIONS})
    if(NOT OPENGL_FOUND)
        MESSAGE(ERROR " OPENGL not found!")
    endif(NOT OPENGL_FOUND)

    SET(PLATFORM_LIBS ${OPENGL_LIBRARIES} X11 m)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
    SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)

ENDIF()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
FILE( GLOB SQUISH_SRC squish/*.h squish/*.cpp )
FILE( GLOB GLSW_SRC lib/glsw/*.c lib/glsw/*.h )
ADD_SUBDIRECTORY (freeglut-3.0.0)
ADD_SUBDIRECTORY (glew-1.13.0)
ADD_SUBDIRECTORY (zlib-1.2.8)
ADD_SUBDIRECTORY (libpng-1.2.51)
ADD_SUBDIRECTORY (nvlib)
ADD_SUBDIRECTORY (graduate-article)
ADD_DEFINITIONS( -DFREEGLUT_STATIC=1 -DGLEW_STATIC=1 )
INCLUDE_DIRECTORIES(freeglut-3.0.0/include glew-1.13.0/include)


#INCLUDE_DIRECTORIES(
#    lib/glew
#    lib/glsw
#    lib/libpez
#)
#
#ADD_LIBRARY( PezEcosystem
#    ${GLEW}
#    ${GLSW}
#    ${VECTORMATH}
#    ${PEZ}
#)
#set(CMAKE_C_FLAGS "-O0 -ggdb")
#ADD_EXECUTABLE( Geodesic ${CONSOLE_SYSTEM} Geodesic.c Geodesic.glsl )
#TARGET_LINK_LIBRARIES( Geodesic PezEcosystem ${PLATFORM_LIBS} )

ADD_EXECUTABLE( simple_triangle ${CONSOLE_SYSTEM} simple-triangle.c )
TARGET_LINK_LIBRARIES( simple_triangle ${PLATFORM_LIBS} freeglut_static glew)

ADD_EXECUTABLE( glu_star ${CONSOLE_SYSTEM} glu_star.cpp )
TARGET_LINK_LIBRARIES( glu_star ${PLATFORM_LIBS} freeglut_static glew)

ADD_EXECUTABLE( etc1_cpu ${CONSOLE_SYSTEM} etc1_cpu.c )
TARGET_LINK_LIBRARIES( etc1_cpu ${PLATFORM_LIBS} freeglut_static glew)

ADD_EXECUTABLE( etc1_cs ${CONSOLE_SYSTEM} etc1_cs.c )
TARGET_LINK_LIBRARIES( etc1_cs ${PLATFORM_LIBS} freeglut_static glew)

ADD_EXECUTABLE( etc1_frag ${CONSOLE_SYSTEM} etc1_frag.c )
TARGET_LINK_LIBRARIES( etc1_frag ${PLATFORM_LIBS} freeglut_static glew)

ADD_EXECUTABLE( s3tc_cs ${CONSOLE_SYSTEM} s3tc_cs.c )
TARGET_LINK_LIBRARIES( s3tc_cs ${PLATFORM_LIBS} freeglut_static glew)

ADD_EXECUTABLE( s3tc_cpu ${CONSOLE_SYSTEM} s3tc_cpu.cpp ${SQUISH_SRC} )
TARGET_INCLUDE_DIRECTORIES( s3tc_cpu PRIVATE squish )
TARGET_COMPILE_DEFINITIONS(s3tc_cpu PRIVATE "SQUISH_USE_SSE=2")
TARGET_LINK_LIBRARIES( s3tc_cpu ${PLATFORM_LIBS} freeglut_static glew)

ADD_EXECUTABLE( Geodesic ${CONSOLE_SYSTEM} Geodesic.c ${GLSW_SRC} )
TARGET_INCLUDE_DIRECTORIES( Geodesic PRIVATE lib/glsw lib/vectormath)
TARGET_LINK_LIBRARIES( Geodesic ${PLATFORM_LIBS} freeglut_static glew)

ADD_EXECUTABLE( gaussian-blur ${CONSOLE_SYSTEM} gaussian-blur.cpp ${GLSW_SRC} )
TARGET_LINK_LIBRARIES( gaussian-blur ${PLATFORM_LIBS} freeglut_static glew nvlib )

ADD_CUSTOM_TARGET(debug ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE:STRING=Debug ${PROJECT_SOURCE_DIR})
ADD_CUSTOM_TARGET(release ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE:STRING=Release ${PROJECT_SOURCE_DIR})
