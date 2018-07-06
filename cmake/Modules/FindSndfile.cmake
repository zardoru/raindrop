# This defines:
# SNDFILE_FOUND
# SNDFILE_INCLUDE_DIRS
# SNDFILE_LIBRARIES

find_path(SNDFILE_INCLUDE_DIR sndfile.h
            HINTS
             /usr/include)

find_library(SNDFILE_LIBRARY NAMES libsndfile sndfile)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SNDFILE DEFAULT_MSG 
    SNDFILE_LIBRARY SNDFILE_INCLUDE_DIR)

mark_as_advanced(SNDFILE_INCLUDE_DIR SNDFILE_LIBRARY)

set(SNDFILE_LIBRARIES ${SNDFILE_LIBRARY})
set(SNDFILE_INCLUDE_DIRS ${SNDFILE_INCLUDE_DIR})
