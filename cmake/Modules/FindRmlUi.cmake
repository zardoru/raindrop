# This defines:
# RMLUI_FOUND
# RMLUI_INCLUDE_DIRS
# RMLUI_LIBRARIES

find_path(RMLUI_INCLUDE_DIR RmlUi/Core.h
        HINTS
        /usr/include)

find_library(RMLUI_LIBRARY_CORE NAMES RmlCore)
find_library(RMLUI_LIBRARY_LUA NAMES RmlLua)

set(RMLUI_LIBRARIES ${RMLUI_LIBRARY_CORE} ${RMLUI_LIBRARY_LUA})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RMLUI DEFAULT_MSG
        RMLUI_LIBRARIES RMLUI_INCLUDE_DIR)

mark_as_advanced(RMLUI_INCLUDE_DIR RMLUI_LIBRARIES)

