CMAKE_MINIMUM_REQUIRED( VERSION 3.0 )

ADD_DEFINITIONS( -DFREEGLUT_STATIC=1 -DGLEW_STATIC=1 )
FILE( GLOB SOURCES *.c* *.h* Scene/*.h* Scene/*.c* Scene/SceneImpl/*.c* Scene/SceneImpl/*.h* Scene/src/*.c* nvImage/src/*.c* nvImage/src/*.h* nvModel/include/*.h nvModel/src/*.c*)
INCLUDE_DIRECTORIES(../freeglut-3.0.0/include nvImage/include/ nvModel/include/ )

IF( WIN32 )

    SET( PLATFORM_LIBS opengl32 glu32)
    SET(CMAKE_CXX_FLAGS "-O2 -ggdb -std=c++11")
    FILE ( GLOB PLATFORM_SOURCES Scene/src/windows/*.c* )

ELSEIF( APPLE )

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

    SET(PLATFORM_LIBS pthread)
    SET(CMAKE_C_FLAGS "-O2 -ggdb")
    SET(CMAKE_CXX_FLAGS "-O2 -ggdb -std=c++11")
    FILE ( GLOB PLATFORM_SOURCES Scene/src/linux/*.c* )
    SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
    SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "-static-libgcc")

ENDIF()


ADD_EXECUTABLE( oit ${CONSOLE_SYSTEM} ${SOURCES} ${PLATFORM_SOURCES} )
TARGET_LINK_LIBRARIES( oit PUBLIC ${PLATFORM_LIBS} )
TARGET_INCLUDE_DIRECTORIES( oit PRIVATE Scene )
TARGET_LINK_LIBRARIES( oit PUBLIC freeglut_static glew )
