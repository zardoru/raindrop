# This will define
# SOXR_FOUND
# SOXR_INCLUDE_DIR
# SOXR_LIBRARY


find_path(SOXR_INCLUDE_DIR soxr.h)
find_library(SOXR_LIBRARY soxr libsoxr)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SOXR DEFAULT_MSG 
    SOXR_LIBRARY SOXR_INCLUDE_DIR)

mark_as_advanced(SOXR_INCLUDE_DIR SOXR_LIBRARY)
set(SOXR_LIBRARIES ${SOXR_LIBRARY})
set(SOXR_INCLUDE_DIRS ${SOXR_INCLUDE_DIR})

