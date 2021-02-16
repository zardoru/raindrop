# This will define
# GLFW3_FOUND
# GLFW3_INCLUDE_DIR
# GLFW3_INCLUDE_DIRS
# GLFW3_LIBRARY
# GLFW3_LIBRARIES

find_path(GLFW3_INCLUDE_DIR
          NAMES glfw3.h
)

find_library(GLFW3_LIBRARY glfw3)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(glfw3 DEFAULT_MSG
        GLFW3_LIBRARY GLFW3_INCLUDE_DIR)

mark_as_advanced(GLFW3_INCLUDE_DIR GLFW3_LIBRARY)

set(GLFW3_LIBRARIES ${GLFW3_LIBRARY})
set(GLFW3_INCLUDE_DIRS ${GLFW3_INCLUDE_DIR})
