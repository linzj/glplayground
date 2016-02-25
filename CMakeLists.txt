CMAKE_MINIMUM_REQUIRED( VERSION 3.0 )

PROJECT( Tessellation )

#FILE( GLOB GLEW lib/glew/*.c lib/glew/*.h )
#FILE( GLOB VECTORMATH lib/vectormath/*.h )
FILE( GLOB SQUISH_SRC squish/*.h squish/*.cpp )
FILE( GLOB GLSW_SRC lib/glsw/*.c lib/glsw/*.h )
ADD_SUBDIRECTORY (freeglut-3.0.0)
ADD_SUBDIRECTORY (glew-1.13.0)
INCLUDE_DIRECTORIES(freeglut-3.0.0/include glew-1.13.0/include)


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

    ADD_DEFINITIONS( -DFREEGLUT_STATIC=1 -DGLEW_STATIC=1 )
    #ADD_DEFINITIONS( /wd4996 /TP )
    SET( PLATFORM_LIBS opengl32 glu32)
    #SET( CONSOLE_SYSTEM WIN32 )
    SET(CMAKE_CXX_FLAGS "-O2 -ggdb -std=c++11")

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

    FIND_PACKAGE(GLUT REQUIRED)
    INCLUDE_DIRECTORIES(${GLUT_INCLUDE_DIRS})
    LINK_DIRECTORIES(${GLUT_LIBRARY_DIRS})
    ADD_DEFINITIONS(${GLUT_DEFINITIONS})
    if(NOT GLUT_FOUND)
        MESSAGE(ERROR " GLUT not found!")
    endif(NOT GLUT_FOUND)

    FIND_PACKAGE(GLEW REQUIRED)
    INCLUDE_DIRECTORIES(${GLEW_INCLUDE_DIRS})
    LINK_DIRECTORIES(${GLEW_LIBRARY_DIRS})
    ADD_DEFINITIONS(${GLEW_DEFINITIONS})
    if(NOT GLEW_FOUND)
        MESSAGE(ERROR " GLEW not found!")
    endif(NOT GLEW_FOUND)

    SET( PLATFORM_LIBS ${GLEW_LIBRARIES} ${GLUT_glut_LIBRARY} ${OPENGL_LIBRARIES} X11 GL m)
    SET(CMAKE_C_FLAGS "-O0 -ggdb")
    SET(CMAKE_CXX_FLAGS "-O0 -ggdb -std=c++11")

ENDIF()

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
