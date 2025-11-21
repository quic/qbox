macro(gs_addexpackage)
  # The purpose of 'GS_ONLY' is to provides a tarball without any sources EXCEPT the boilerplate and QEMU
  if (NOT GS_ONLY)
    cpmaddpackage(${ARGV})
  endif()
endmacro()

# ##############################################################################
# ----- SystemC and CCI Dependencies
# ##############################################################################

macro(install_systemc)
    if(DEFINED ENV{SYSTEMC_HOME} OR DEFINED SYSTEMC_HOME)
        set(SYSTEMC_HOME $ENV{SYSTEMC_HOME})

        CPMAddPackage(NAME SystemC SOURCE_DIR ${SYSTEMC_HOME})

        set(SystemCLanguage_FOUND TRUE)
        set(SystemCLanguageLocal_FOUND TRUE)
    else()
        gs_addexpackage(
            NAME SystemCLanguage
            GIT_REPOSITORY https://github.com/accellera-official/systemc.git
            GIT_TAG febde7a3e007ce3030cf574a4cb6fcc13d0ed820
            GIT_SHALLOW OFF
            OPTIONS "ENABLE_SUSPEND_ALL" "ENABLE_PHASE_CALLBACKS"
        )

        # Prevent CCI attempting to re-find SystemC
        if(SystemCLanguage_ADDED)
            set(SystemCLanguage_FOUND TRUE)
        endif()

        # upstream set INSTALL_NAME_DIR wrong !
        if(APPLE)
            set_target_properties(
                systemc
                PROPERTIES
                INSTALL_NAME_DIR "@rpath"
            )
        endif()

        message(STATUS "Using SystemC ${SystemCLanguage_VERSION} (${SystemCLanguage_SOURCE_DIR})")
    endif()
endmacro()

macro(install_systemc_dependencies)

    gs_addexpackage(
        NAME SystemCCCI
        GIT_REPOSITORY https://github.com/accellera-official/cci.git
        GIT_TAG 915c189f6916ab56db773b2da20ebc06fe8f24e9
        GIT_SHALLOW True
        OPTIONS "SYSTEMCCCI_BUILD_TESTS OFF"
    )

    gs_addexpackage(
        NAME SCP
        GIT_REPOSITORY https://github.com/accellera-official/systemc-common-practices.git
        GIT_TAG 6830c915bb691d9b505b17ac631f1ff305fe9c17
        GIT_SHALLOW True
    )
endmacro()

macro(configure_systemc)
    if(SystemCCCI_ADDED)
        set(SystemCCCI_FOUND TRUE)
    endif()

    # upstream set INSTALL_NAME_DIR wrong !
    if(APPLE)
        set_target_properties(
            cci-config
            PROPERTIES
            INSTALL_NAME_DIR "@rpath"
        )
    endif()

    message(STATUS "Using SystemCCI ${SystemCCCI_VERSION} (${SystemCCCI_SOURCE_DIR})")

    set(SYSTEMC_PROJECT TRUE)

    add_compile_definitions(HAS_CCI)

    if(NOT GS_ONLY)
        list(APPEND TARGET_LIBS SystemC::systemc SystemC::cci-config scp::reporting)
        set(TARGET_LIBS "${TARGET_LIBS}" CACHE INTERNAL "target_libs")
    endif()
endmacro()

macro(gs_systemc)
    install_systemc()
    install_systemc_dependencies()
    configure_systemc()
endmacro()
# ##############################################################################

macro(gs_create_dymod MODULE_NAME)
  file(GLOB_RECURSE SOURCE_COMPONENT "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")

  add_library(${MODULE_NAME} SHARED
              ${SOURCE_COMPONENT})

  # Define where to install the dynamic library during building
  set_target_properties(${MODULE_NAME} PROPERTIES
                        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
                        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

  # Avoid to have the "lib" prefix on the name of the library
  set_target_properties(${MODULE_NAME} PROPERTIES PREFIX "")



  if ( TARGET_LIBS )
    add_dependencies(${MODULE_NAME} ${TARGET_LIBS})
    target_link_libraries(${MODULE_NAME} PUBLIC
        ${TARGET_LIBS}
    )
  endif()

    if (WIN32)
        target_link_libraries(${MODULE_NAME} PRIVATE ws2_32 mswsock)
    endif()

  foreach(T IN LISTS TARGET_LIBS)
    target_include_directories(
      ${MODULE_NAME} PUBLIC
      $<TARGET_PROPERTY:${T},INTERFACE_INCLUDE_DIRECTORIES>
    )
  endforeach()

  target_include_directories(
    ${MODULE_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}>
  )

  install(
    TARGETS ${MODULE_NAME}
    # we export the ${MODULE_NAME}, like that we can use ${MODULE_NAME} with qqvp.
    EXPORT qqvpTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT "${LIBRARY_NAME}_Runtime"
            NAMELINK_COMPONENT "${LIBRARY_NAME}_Development"
  )

endmacro()

# ##############################################################################
# ----- LUA
# ##############################################################################
macro(build_lua)
  if (lua_ADDED)
      # lua has no CMake support, so we create our own target
      FILE(GLOB lua_sources ${lua_SOURCE_DIR}/*.c)
      list(REMOVE_ITEM lua_sources "${lua_SOURCE_DIR}/lua.c" "${lua_SOURCE_DIR}/luac.c" "${lua_SOURCE_DIR}/ltests.c" "${lua_SOURCE_DIR}/onelua.c")
      add_library(lua SHARED ${lua_sources})
      if(NOT WIN32)
        target_compile_definitions(lua PRIVATE LUA_USE_POSIX)
      endif()
      target_include_directories(lua PUBLIC $<BUILD_INTERFACE:${lua_SOURCE_DIR}>)
      target_compile_definitions(${PROJECT_NAME} INTERFACE HAS_LUA)
      target_link_libraries(${PROJECT_NAME} PUBLIC lua)
      list(APPEND TARGET_LIBS "lua")
      install(TARGETS lua EXPORT ${PROJECT_NAME}Targets)
  endif()
endmacro()
################################################################################

# ----- configure include paths and EXPORT PROJECT
macro(gs_export)
  if (NOT GS_ONLY)
    set(LIBRARY_NAME ${PROJECT_NAME})
    if (${ARGC} GREATER 0)
      set(LIBRARY_NAME ${ARGV0})
    endif()

    # We are checking if the second parameters is used like that we are backwards compatible because old versions just used one parameter.
    if(NOT "${ARGV1}" STREQUAL "")
      set(INCLUDE_REPO ${ARGV1})
    else()
      set(INCLUDE_REPO ${PROJECT_SOURCE_DIR}/include)
    endif()

    message(STATUS "LIBRARY_NAME = ${LIBRARY_NAME}")
    string(TOLOWER ${LIBRARY_NAME}/version.h VERSION_HEADER_LOCATION)
    add_library("lib${LIBRARY_NAME}" ALIAS ${LIBRARY_NAME})
    add_library("GreenSocs::lib${LIBRARY_NAME}" ALIAS ${LIBRARY_NAME})
    packageproject(
      NAME
      "${LIBRARY_NAME}"
      VERSION
      ${PROJECT_VERSION}
      NAMESPACE
      GreenSocs
      BINARY_DIR
      ${PROJECT_BINARY_DIR}
      INCLUDE_DIR
      ${INCLUDE_REPO}
      INCLUDE_DESTINATION
      ${CMAKE_INSTALL_INCLUDEDIR}
      VERSION_HEADER
      "${VERSION_HEADER_LOCATION}"
      COMPATIBILITY
      SameMajorVersion)
    install(
      TARGETS ${LIBRARY_NAME}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
              COMPONENT "${LIBRARY_NAME}_Runtime"
              NAMELINK_COMPONENT "${LIBRARY_NAME}_Development"
    )
    list(APPEND TARGET_LIBS "GreenSocs::${LIBRARY_NAME}")
    set(TARGET_LIBS "${TARGET_LIBS}" CACHE INTERNAL "target_libs")
  endif()
endmacro()

# by default switch on verbosity

macro(gs_enable_testing)
  if (NOT GS_ONLY)
    if(BUILD_TESTING AND ("${PROJECT_NAME}" STREQUAL "${CMAKE_PROJECT_NAME}"))
      if (googletest_ADDED)
        # Force gtest DLLs to CMAKE_BINARY_DIR instead of CMAKE_BINARY_DIR/bin
        if(WIN32)
          set_target_properties(gtest gtest_main gmock gmock_main
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
          )
        endif()
        target_link_libraries(${PROJECT_NAME} INTERFACE gtest gmock)
      endif()
      enable_testing()

      cmake_host_system_information(RESULT _nproc QUERY NUMBER_OF_LOGICAL_CORES)
      math(EXPR _test_jobs "${_nproc} * 3 / 4")
      if(_test_jobs LESS 1)
          set(_test_jobs 1)
      endif()

      if (${PROJECT_NAME} STREQUAL "qbox")
          list(APPEND CMAKE_CTEST_ARGUMENTS "-j" "${_test_jobs}" "--output-on-failure")
      else()
          list(APPEND CMAKE_CTEST_ARGUMENTS "--repeat" "until-pass:3" "--output-on-failure")
      endif()
      add_subdirectory(tests)
    endif()
  endif()
endmacro()
