set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_C_COMPILER "clang")
set(CMAKE_LINKER "clang++")

# Until we address these bugs, we should disable these
# warnings:
set(CMAKE_C_FLAGS "-Wno-format -Wno-c99-designator ${CMAKE_C_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "-Wno-format -Wno-c99-designator ${CMAKE_CXX_FLAGS}" CACHE STRING "" FORCE)

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)
