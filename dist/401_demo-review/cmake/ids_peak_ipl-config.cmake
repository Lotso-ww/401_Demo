
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was package-config.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

####################################################################################

include(CMakeFindDependencyMacro)

set_and_check(ids_peak_ipl_INCLUDES "${PACKAGE_PREFIX_DIR}/include")
set_and_check(ids_peak_ipl_LIBS "${PACKAGE_PREFIX_DIR}/lib/x86_32")
set_and_check(_ids_peak_ipl_sharedlib_file 
  "${PACKAGE_PREFIX_DIR}/lib/x86_32/ids_peak_ipl.dll"
)

# Take note of files we need to copy in deploy function,
# as PACKAGE_LIB_INSTALL_DIR might get overwritten by another package.
file(
  GLOB _ids_peak_ipl_dependency_files
  ${PACKAGE_PREFIX_DIR}/lib/x86_32/*.dll
)

# Set variables and other markers
set(_ids_peak_ipl_includes_application_starter FALSE)
set(_ids_peak_ipl_includes_dotnet 1)
set(_ids_peak_ipl_requires_thread_library FALSE)

set(_ids_peak_ipl_dont_check_gentl_path TRUE)
set(_ids_peak_ipl_dotnet_file_dir "${PACKAGE_PREFIX_DIR}/binding/dotnet/x86_32")


# Target Export
####################################################################################
if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 3.0)
   message(FATAL_ERROR "CMake >= 3.0.0 required")
endif()
cmake_policy(PUSH)
cmake_policy(VERSION 3.0...3.28)

# Set cmake policy regarding constant if statements
cmake_policy(SET CMP0012 NEW)

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Protect against multiple inclusion, which would fail when already imported targets are added once more.
set(_targetsDefined)
set(_targetsNotDefined)
set(_expectedTargets)
foreach(_expectedTarget ids_peak_ipl)
  list(APPEND _expectedTargets ${_expectedTarget})
  if(NOT TARGET ${_expectedTarget})
    list(APPEND _targetsNotDefined ${_expectedTarget})
  endif()
  if(TARGET ${_expectedTarget})
    list(APPEND _targetsDefined ${_expectedTarget})
  endif()
endforeach()
if("${_targetsDefined}" STREQUAL "${_expectedTargets}")
  unset(_targetsDefined)
  unset(_targetsNotDefined)
  unset(_expectedTargets)
  set(CMAKE_IMPORT_FILE_VERSION)
  cmake_policy(POP)
  return()
endif()
if(NOT "${_targetsDefined}" STREQUAL "")
  message(FATAL_ERROR "Some (but not all) targets in this export set were already defined.\nTargets Defined: ${_targetsDefined}\nTargets not yet defined: ${_targetsNotDefined}\n")
endif()
unset(_targetsDefined)
unset(_targetsNotDefined)
unset(_expectedTargets)

# Create imported target ids_peak_ipl
add_library(ids_peak_ipl::ids_peak_ipl SHARED IMPORTED)
add_library(ids_peak_ipl ALIAS ids_peak_ipl::ids_peak_ipl)

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "ids_peak_ipl" for configuration "Release"
set_property(TARGET ids_peak_ipl::ids_peak_ipl APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)

# Make dependent target settings
set_target_properties(ids_peak_ipl::ids_peak_ipl PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${PACKAGE_PREFIX_DIR}/include"
    IMPORTED_LOCATION_RELEASE "${_ids_peak_ipl_sharedlib_file}"
)

if(NOT "cxx_std_14" STREQUAL "" AND "${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 3.7)
  set_target_properties(ids_peak_ipl::ids_peak_ipl PROPERTIES
    INTERFACE_COMPILE_FEATURES "cxx_std_14" # first available in cmake 3.8
  )
endif()

if(MSVC AND NOT "/bigobj;$<$<COMPILE_LANGUAGE:CXX>:/EHsc>" STREQUAL "")
  set_property(
    TARGET ids_peak_ipl::ids_peak_ipl 
    APPEND PROPERTY
      INTERFACE_COMPILE_OPTIONS "/bigobj;$<$<COMPILE_LANGUAGE:CXX>:/EHsc>"
  )
endif()

if(NOT "m;atomic" STREQUAL "" AND UNIX)
  set_property(
      TARGET ids_peak_ipl::ids_peak_ipl 
      APPEND PROPERTY 
        INTERFACE_LINK_LIBRARIES 
          "m;atomic"
  )
endif()

list(APPEND _IMPORT_CHECK_TARGETS ids_peak_ipl::ids_peak_ipl )
list(APPEND _IMPORT_CHECK_FILES_FOR_ids_peak_ipl::ids_peak_ipl
    "${_ids_peak_ipl_sharedlib_file}"
)

# If using under Windows, we want to setup the .lib as well.
if(WIN32)
    set_target_properties(ids_peak_ipl::ids_peak_ipl PROPERTIES
        IMPORTED_IMPLIB_RELEASE "${PACKAGE_PREFIX_DIR}/lib/x86_32/ids_peak_ipl.lib"
    )

    list(APPEND _IMPORT_CHECK_FILES_FOR_ids_peak_ipl::ids_peak_ipl
        "${PACKAGE_PREFIX_DIR}/lib/x86_32/ids_peak_ipl.lib" 
    )
endif()

# Additional Bindings and modules
##################################################################
# Dotnet
if(_ids_peak_ipl_includes_dotnet AND EXISTS "${_ids_peak_ipl_dotnet_file_dir}/ids_peak_ipl_dotnet.dll")
  # If we cannot find the dotnet binding dll, we will assume that dotnet has not been installed
  # and skip the entire dotnet target.

  set_and_check(_ids_peak_ipl_dotnet_sharedlib_file 
    "${_ids_peak_ipl_dotnet_file_dir}/ids_peak_ipl_dotnet.dll"
  )
  
  set_and_check(_ids_peak_ipl_dotnet_interface_sharedlib_file 
    "${_ids_peak_ipl_dotnet_file_dir}/ids_peak_ipl_dotnet_interface.dll"
  )

  # Take note of files we need to copy in deploy function,
  # as PACKAGE_LIB_INSTALL_DIR might get overwritten by another package.
  file(
    GLOB _ids_peak_ipl_dotnet_dependency_files
    ${PACKAGE_PREFIX_DIR}/binding/dotnet/x86_32/*.dll
  )

  add_library (ids_peak_ipl::ids_peak_ipl_dotnet UNKNOWN IMPORTED)
  add_library (ids_peak_ipl_dotnet ALIAS ids_peak_ipl::ids_peak_ipl_dotnet)
  set_target_properties (ids_peak_ipl::ids_peak_ipl_dotnet PROPERTIES
      IMPORTED_LOCATION ${_ids_peak_ipl_dotnet_sharedlib_file}
      INTERFACE_LINK_LIBRARY ${_ids_peak_ipl_dotnet_interface_sharedlib_file}
      INTERFACE_INCLUDE_DIRECTORIES "${PACKAGE_PREFIX_DIR}/include"
  )

  list(APPEND _IMPORT_CHECK_TARGETS ids_peak_ipl::ids_peak_ipl_dotnet )
  list(APPEND _IMPORT_CHECK_FILES_FOR_ids_peak_ipl::ids_peak_ipl_dotnet
      "${_ids_peak_ipl_dotnet_sharedlib_file}"
      "${_ids_peak_ipl_dotnet_interface_sharedlib_file}"
  )
endif()

if(_ids_peak_ipl_includes_application_starter)
  set_and_check(_ids_peak_ipl_application_starter_input_file "${CMAKE_CURRENT_LIST_DIR}/ids_peak_ipl-application-starter.sh.in")
endif()

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)

# Loop over all imported files and verify that they actually exist
foreach(target ${_IMPORT_CHECK_TARGETS} )
  foreach(file ${_IMPORT_CHECK_FILES_FOR_${target}} )
    if(NOT EXISTS "${file}" )
        message(FATAL_ERROR "The imported target \"${target}\" references the file
                            \"${file}\"
                            but this file does not exist.  Possible reasons include:
                            * The file was deleted, renamed, or moved to another location.
                            * An install or uninstall procedure did not complete successfully.
                            * The installation package was faulty and contained
                            \"${CMAKE_CURRENT_LIST_FILE}\"
                            but not all the files it references."
        )
    endif()
  endforeach()
  unset(_IMPORT_CHECK_FILES_FOR_${target})
endforeach()
unset(_IMPORT_CHECK_TARGETS)

####################################################################################
#      Additional Libraries
# !!!! NOTE: PACKAGE_* variables should not be used after this point !!!!
####################################################################################

if(NOT "ids_peak_common" STREQUAL "")
  foreach(REQUIRED_LIB IN ITEMS ids_peak_common)
    find_dependency(${REQUIRED_LIB})
    target_link_libraries(ids_peak_ipl::ids_peak_ipl INTERFACE ${REQUIRED_LIB})

    get_target_property(_lib_type ${REQUIRED_LIB} TYPE)
    if(WIN32 AND NOT _lib_type STREQUAL "INTERFACE_LIBRARY")
        # Library is NOT header-only
        list(APPEND _ids_peak_ipl_dependency_files $<TARGET_FILE:${REQUIRED_LIB}>)
    endif()
  endforeach()
endif()

if(_ids_peak_ipl_requires_thread_library)
  find_dependency(Threads)
  target_link_libraries(ids_peak_ipl::ids_peak_ipl INTERFACE Threads::Threads)
endif()

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
cmake_policy(POP)

# Additional commands
####################################################################################
include(${CMAKE_CURRENT_LIST_DIR}/ids_peak_ipl-detect-architecture.cmake)
_detect_target_architecture (ARCH)

# Check GENICAM_GENTLXX_PATH
if(NOT _ids_peak_ipl_dont_check_gentl_path)
  if(${ARCH} STREQUAL "x86_64")
      if(NOT DEFINED ENV{GENICAM_GENTL64_PATH})
        message(WARNING "GENICAM_GENTL64_PATH is not set! "
          "If you just installed the SDK you might want to consider a restart. "
          "If you are using the .tar.gz package, "
          "please use the ids_peak_ipl_generate_starter_script function to generate a starter script.")
      endif()
  elseif(${ARCH} STREQUAL "x86_32")
      if(NOT DEFINED ENV{GENICAM_GENTL32_PATH})
        message(WARNING "GENICAM_GENTL32_PATH is not set! "
          "If you just installed the SDK you might want to consider a restart. "
          "If you are using the .tar.gz package, "
          "please use the ids_peak_ipl_generate_starter_script function to generate a starter script.")
      endif()
  else()
      # Let's stay silent, as this check isn't that important.
      # For debugging purposes there is already an output of the detected architecture
      # inside _detect_target_architecture
  endif()
endif()

# User Functions
####################################################################################
function(ids_peak_ipl_deploy target)
    # On Linux we don't need to copy the DLL's.
    if(WIN32)
        message(STATUS "[${PROJECT_NAME}] Adding post build copy for ids_peak_ipl")

        add_custom_command(
            TARGET ${target}
            POST_BUILD
            COMMAND
              ${CMAKE_COMMAND} -E copy_if_different ${_ids_peak_ipl_dependency_files} $<TARGET_FILE_DIR:${target}>/
            COMMENT
              "[${PROJECT_NAME}] Post build copy for ids_peak_ipl dependency files"
        )
    endif()
endfunction()

function(ids_peak_ipl_dotnet_deploy target)
  # Set cmake policy regarding constant if statements
  cmake_policy(PUSH)
  cmake_policy(SET CMP0012 NEW)

  if(NOT _ids_peak_ipl_includes_dotnet OR NOT EXISTS "${_ids_peak_ipl_dotnet_file_dir}/ids_peak_ipl_dotnet.dll")
    cmake_policy(POP)
    message(FATAL_ERROR "Dotnet binding is not installed!")
  endif()

  # Also deploy base api
  ids_peak_ipl_deploy(${target})

  # Check if this is windows, as we wouldn't need a copy, if we were to somehow support dotnet under linux
  if(WIN32)
    message(STATUS "[${PROJECT_NAME}] Adding post build copy for ids_peak_ipl_dotnet")
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND
          ${CMAKE_COMMAND} -E copy_if_different ${_ids_peak_ipl_dotnet_dependency_files} $<TARGET_FILE_DIR:${target}>/
        COMMENT
          "[${PROJECT_NAME}] Post build copy for ids_peak_ipl_dotnet dependency files"
    )
  endif()

  cmake_policy(POP)
endfunction()

if(_ids_peak_ipl_includes_application_starter)
  function(ids_peak_ipl_generate_starter_script target)
    if(NOT UNIX)
      message(WARNING "[${PROJECT_NAME}] No need to call ids_peak_ipl_generate_starter_script on this system.")
      return()
    endif()

    set(TARGET_BINARY_NAME ${target})

    configure_file(${_ids_peak_ipl_application_starter_input_file} ${CMAKE_CURRENT_BINARY_DIR}/ids_peak_ipl/${target}.sh)
    
    # NOTE: The normal way of using copy_if_different in post-build doesn't work here, as we need to set file permissions
    #       which only the file command can do. In order to run the file command post build we use a little trick.
    #       First we write a cmake file, which runs the file command:
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ids_peak_ipl/post_build_copy_app_starter.cmake"
      "
      file(
        COPY 
          \"${CMAKE_CURRENT_BINARY_DIR}/ids_peak_ipl/${target}.sh\"
        DESTINATION 
          \${OUT_DIR}
        FILE_PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
      )\n
      "
    )

    # Then we setup a post build step, which runs the previously written script with the target file directory
    # which we can also only get during post-build.
    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND
        ${CMAKE_COMMAND} -DOUT_DIR=$<TARGET_FILE_DIR:${target}> -P "${CMAKE_CURRENT_BINARY_DIR}/ids_peak_ipl/post_build_copy_app_starter.cmake"
      COMMENT
        "[${PROJECT_NAME}] Post build copy of starter shell script"
      VERBATIM
    )

  endfunction()
endif()


