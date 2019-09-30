# This will define
# MPG123_FOUND
# MPG123_INCLUDE_DIR
# MPG123_LIBRARY

find_path(MPG123_INCLUDE_DIR 
            NAMES mpg123.h
            HINTS /usr/include
)

find_library(MPG123_LIBRARY mpg123 libmpg123)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MPG123 DEFAULT_MSG 
    MPG123_LIBRARY MPG123_INCLUDE_DIR)

mark_as_advanced(MPG123_INCLUDE_DIR MPG123_LIBRARY)

set(MPG123_LIBRARIES ${MPG123_LIBRARY})
set(MPG123_INCLUDE_DIRS ${MPG123_INCLUDE_DIR})
