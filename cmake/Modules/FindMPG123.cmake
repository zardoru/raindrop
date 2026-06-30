# This will define
# MPG123_FOUND
# MPG123_INCLUDE_DIR
# MPG123_LIBRARY

include(ExternalProject)
include(FindPackageHandleStandardArgs)

find_path(MPG123_INCLUDE_DIR 
            NAMES mpg123.h
            HINTS /usr/include
)

find_library(MPG123_LIBRARY NAMES mpg123 libmpg123)

if(MPG123_INCLUDE_DIR AND MPG123_LIBRARY AND NOT TARGET MPG123::MPG123)
    add_library(MPG123::MPG123 UNKNOWN IMPORTED)
    set_target_properties(MPG123::MPG123 PROPERTIES
        IMPORTED_LOCATION "${MPG123_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${MPG123_INCLUDE_DIR}"
    )
endif()

if(NOT TARGET MPG123::MPG123)
    find_program(MAKE_EXECUTABLE NAMES gmake make REQUIRED)

    set(MPG123_EXTERNAL_PREFIX "${CMAKE_BINARY_DIR}/_deps/mpg123")
    set(MPG123_EXTERNAL_INSTALL_DIR "${MPG123_EXTERNAL_PREFIX}/install")
    set(MPG123_EXTERNAL_INCLUDE_DIR "${MPG123_EXTERNAL_INSTALL_DIR}/include")
    set(MPG123_EXTERNAL_LIBRARY "${MPG123_EXTERNAL_INSTALL_DIR}/lib/libmpg123.a")

    file(MAKE_DIRECTORY "${MPG123_EXTERNAL_INCLUDE_DIR}")

    ExternalProject_Add(mpg123_external
        PREFIX "${MPG123_EXTERNAL_PREFIX}"
        GIT_REPOSITORY https://github.com/libsdl-org/mpg123.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        CONFIGURE_COMMAND
            "<SOURCE_DIR>/configure"
            "--prefix=<INSTALL_DIR>"
            "--disable-shared"
            "--enable-static"
            "--disable-programs"
        BUILD_COMMAND "${MAKE_EXECUTABLE}"
        INSTALL_COMMAND "${MAKE_EXECUTABLE}" install
    )

    add_library(MPG123::MPG123 STATIC IMPORTED)
    set_target_properties(MPG123::MPG123 PROPERTIES
        IMPORTED_LOCATION "${MPG123_EXTERNAL_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${MPG123_EXTERNAL_INCLUDE_DIR}"
    )
    add_dependencies(MPG123::MPG123 mpg123_external)
    set(MPG123_INCLUDE_DIR "${MPG123_EXTERNAL_INCLUDE_DIR}")
    set(MPG123_LIBRARY MPG123::MPG123)
endif()

if(TARGET MPG123::MPG123)
    set(MPG123_FOUND TRUE)
    set(MPG123_LIBRARY MPG123::MPG123)
endif()

find_package_handle_standard_args(MPG123 DEFAULT_MSG MPG123_FOUND)

mark_as_advanced(MPG123_INCLUDE_DIR MPG123_LIBRARY)

set(MPG123_LIBRARIES ${MPG123_LIBRARY})
set(MPG123_INCLUDE_DIRS ${MPG123_INCLUDE_DIR})
