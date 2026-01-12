if($ENV{MSYSTEM} MATCHES "MINGW64|UCRT64|CLANGARM64")
    message(STATUS "MSYS2 environment detected. Using \"Unix Makefiles\" generator")
    set(CMAKE_GENERATOR "Unix Makefiles" CACHE INTERNAL "" FORCE)
endif()
