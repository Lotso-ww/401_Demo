# Based on BasicConfigVersion-AnyNewerVersion.cmake.in, included in CMake.

# The created file sets PACKAGE_VERSION_EXACT if the current version string and
# the requested version string are exactly the same and it sets
# PACKAGE_VERSION_COMPATIBLE if the current version is >= requested version.
# The variable CVF_VERSION must be set before calling configure_file().

set(PACKAGE_VERSION "1.17.2.0")

if(PACKAGE_VERSION VERSION_LESS PACKAGE_FIND_VERSION)
  set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
  if(PACKAGE_FIND_VERSION STREQUAL PACKAGE_VERSION)
    set(PACKAGE_VERSION_EXACT TRUE)
  endif()
endif()

# Package version is fine. Now test for correct architecture
set(PACKAGE_VERSION "1.17.2.0 (x86_32)")
include(${CMAKE_CURRENT_LIST_DIR}/ids_peak_ipl-detect-architecture.cmake)
_detect_target_architecture (ARCH)

if(NOT "${ARCH}" STREQUAL "unknown")
  # Go by architecture detection script
  if(NOT "${ARCH}" STREQUAL "x86_32")
    set(PACKAGE_VERSION_UNSUITABLE TRUE)
    return()
  endif()
else()
  # Go by bit-ness
  # if the installed or the using project don't have CMAKE_SIZEOF_VOID_P set, ignore it:
  if((NOT "${CMAKE_SIZEOF_VOID_P}" STREQUAL "") AND (NOT "4" STREQUAL ""))
    if(NOT CMAKE_SIZEOF_VOID_P STREQUAL "4")
      math(EXPR installedBits "4 * 8")
      set(PACKAGE_VERSION_UNSUITABLE TRUE)
      return()
    endif()  
  endif()
endif()
