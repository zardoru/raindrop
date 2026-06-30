include(FindPackageHandleStandardArgs)
include(ExternalProject)

find_program(MAKE_EXECUTABLE NAMES gmake make REQUIRED)

find_path(GLEW_INCLUDE_DIRS GL/glew.h)
find_library(GLEW_LIBRARIES NAMES GLEW glew glew32 glew32s)

if(GLEW_INCLUDE_DIRS AND GLEW_LIBRARIES AND NOT TARGET GLEW::GLEW)
    add_library(GLEW::GLEW UNKNOWN IMPORTED)
    set_target_properties(GLEW::GLEW PROPERTIES
        IMPORTED_LOCATION "${GLEW_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${GLEW_INCLUDE_DIRS}"
    )
endif()

if(NOT TARGET GLEW::GLEW)
    find_package(OpenGL REQUIRED)

    set(GLEW_EXTERNAL_PREFIX "${CMAKE_BINARY_DIR}/_deps/glew")
    set(GLEW_EXTERNAL_INSTALL_DIR "${GLEW_EXTERNAL_PREFIX}/install")
    set(GLEW_EXTERNAL_INCLUDE_DIR "${GLEW_EXTERNAL_INSTALL_DIR}/include")
    set(GLEW_EXTERNAL_LIBRARY "${GLEW_EXTERNAL_INSTALL_DIR}/lib/libGLEW.a")

    file(MAKE_DIRECTORY "${GLEW_EXTERNAL_INCLUDE_DIR}")

    ExternalProject_Add(glew_external
        PREFIX "${GLEW_EXTERNAL_PREFIX}"
        URL https://github.com/nigels-com/glew/releases/download/glew-2.2.0/glew-2.2.0.zip
        CONFIGURE_COMMAND ""
        BUILD_IN_SOURCE TRUE
        BUILD_COMMAND
            "${MAKE_EXECUTABLE}"
            "CC=${CMAKE_C_COMPILER}"
            "STRIP="
            glew.lib.static
        INSTALL_COMMAND
            "${CMAKE_COMMAND}" -E make_directory "${GLEW_EXTERNAL_INSTALL_DIR}/lib"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${GLEW_EXTERNAL_INCLUDE_DIR}"
            COMMAND "${CMAKE_COMMAND}" -E copy "<SOURCE_DIR>/lib/libGLEW.a" "${GLEW_EXTERNAL_LIBRARY}"
            COMMAND "${CMAKE_COMMAND}" -E copy_directory "<SOURCE_DIR>/include/GL" "${GLEW_EXTERNAL_INCLUDE_DIR}/GL"
    )

    add_library(GLEW::GLEW STATIC IMPORTED)
    set_target_properties(GLEW::GLEW PROPERTIES
        IMPORTED_LOCATION "${GLEW_EXTERNAL_LIBRARY}"
        INTERFACE_COMPILE_DEFINITIONS GLEW_STATIC
        INTERFACE_INCLUDE_DIRECTORIES "${GLEW_EXTERNAL_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES OpenGL::GL
    )
    add_dependencies(GLEW::GLEW glew_external)
    set(GLEW_INCLUDE_DIRS "${GLEW_EXTERNAL_INCLUDE_DIR}")
    set(GLEW_LIBRARIES GLEW::GLEW)
endif()

if(TARGET GLEW::GLEW)
    set(GLEW_FOUND TRUE)
    set(GLEW_LIBRARIES GLEW::GLEW)
endif()

find_package_handle_standard_args(GLEW DEFAULT_MSG GLEW_FOUND)
