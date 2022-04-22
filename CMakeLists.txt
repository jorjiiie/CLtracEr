cmake_minimum_required(VERSION 3.14)
project(CLtracEr C)

add_subdirectory(lib/glfw)
configure_file(src/OpenCL/kernel.cl kernel.cl COPYONLY)

add_definitions(-DGL_SILENCE_DEPRECATION)
add_definitions(-O3)

if (APPLE) 
    
    list(APPEND EXTRA_LIBS
        "-framework OpenGL -framework OpenCL"
    )

elseif(WIN32)
    list(APPEND EXTRA_LIBS
        "-lglu32 -lopengl32"
    )
    set(CMAKE_EXE_LINKER_FLAGS "-std=gnu99 -static -static-libgcc -static-libstdc++ -mwindows")
else()
    list(APPEND EXTRA_LIBS
        "-lGL -lGLU -lX11"
    )
endif()

add_executable(CLtracEr src/OpenCL/main.c)
target_link_libraries(CLtracEr glfw ${EXTRA_LIBS})
