# Try to find the OggVorbis libraries
# Once done this will define
#
#  OGGVORBIS_FOUND - system has OggVorbis
#  OGG_INCLUDE_DIR - the OggVorbis include directory
#  VORBIS_INCLUDE_DIR - the OggVorbis include directory
#  OGGVORBIS_LIBRARIES - The libraries needed to use OggVorbis
#  OGG_LIBRARY         - The Ogg library
#  VORBIS_LIBRARY      - The Vorbis library
#  VORBISFILE_LIBRARY  - The VorbisFile library



find_path(VORBIS_INCLUDE_DIR NAMES vorbis/vorbisfile.h)
find_path(OGG_INCLUDE_DIR NAMES ogg/ogg.h)

find_library(OGG_LIBRARY NAMES ogg libogg ogg_static libogg_static)
find_library(VORBIS_LIBRARY NAMES vorbis libvorbis)
find_library(VORBISFILE_LIBRARY NAMES vorbisfile libvorbisfile)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OggVorbis DEFAULT_MSG
        VORBIS_INCLUDE_DIR OGG_INCLUDE_DIR OGG_LIBRARY VORBIS_LIBRARY VORBISFILE_LIBRARY)
