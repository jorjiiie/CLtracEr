cmake_minimum_required(VERSION 3.8)
#find_program(CMAKE_C_COMPILER NAMES $ENV{CC} gcc PATHS ENV PATH NO_DEFAULT_PATH)
#find_program(CMAKE_CXX_COMPILER NAMES $ENV{CXX} g++ PATHS ENV PATH NO_DEFAULT_PATH)

project(CLtracEr C)


add_subdirectory(lib/glfw)
configure_file(src/OpenCL/kernel.cl kernel.cl COPYONLY)

add_definitions(-DGL_SILENCE_DEPRECATION)
#add_definitions(-O3)

if (APPLE) 
    
    list(APPEND EXTRA_LIBS
	    # "-framework OpenGL -framework OpenCL -framework Cocoa -framework Carbon -framework IOKit -framework CoreVideo"
	    "-framework OpenCL -framework OpenGL"
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
message(STATUS "framworks" ${EXTRA_LIBS})

add_executable(CLtracEr src/OpenCL/main.c)
#add_executable(CLtracEr src/OpenCL/test.c)



target_link_libraries(CLtracEr glfw ${EXTRA_LIBS})
