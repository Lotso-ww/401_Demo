include(FindPackageHandleStandardArgs)

find_path(IDS_PEAK_INCLUDE_DIR peak/peak.hpp PATHS "${IDS_PEAK_ROOT}/generic_sdk/api/include" NO_DEFAULT_PATH)
find_path(IDS_PEAK_IPL_INCLUDE_DIR peak_ipl/peak_ipl.hpp PATHS "${IDS_PEAK_ROOT}/generic_sdk/ipl/include" NO_DEFAULT_PATH)
find_path(IDS_PEAK_COMMON_INCLUDE_DIR peak_common/types/peak_common_iimageview.hpp PATHS "${IDS_PEAK_ROOT}/common/include" NO_DEFAULT_PATH)
set(_ids_arch x86_32)
find_library(IDS_PEAK_LIBRARY ids_peak PATHS "${IDS_PEAK_ROOT}/generic_sdk/api/lib/${_ids_arch}" NO_DEFAULT_PATH)
find_library(IDS_PEAK_IPL_LIBRARY ids_peak_ipl PATHS "${IDS_PEAK_ROOT}/generic_sdk/ipl/lib/${_ids_arch}" NO_DEFAULT_PATH)
find_library(IDS_PEAK_COMFORT_LIBRARY ids_peak_comfort_c PATHS "${IDS_PEAK_ROOT}/comfort_sdk/api/lib/${_ids_arch}" NO_DEFAULT_PATH)
find_package_handle_standard_args(IdsPeak REQUIRED_VARS IDS_PEAK_INCLUDE_DIR IDS_PEAK_IPL_INCLUDE_DIR IDS_PEAK_COMMON_INCLUDE_DIR IDS_PEAK_LIBRARY IDS_PEAK_IPL_LIBRARY IDS_PEAK_COMFORT_LIBRARY)

if(IdsPeak_FOUND)
    set(IDS_PEAK_FOUND TRUE)
    get_filename_component(IDS_PEAK_API_RUNTIME_DIR "${IDS_PEAK_LIBRARY}" DIRECTORY)
    get_filename_component(IDS_PEAK_IPL_RUNTIME_DIR "${IDS_PEAK_IPL_LIBRARY}" DIRECTORY)
    get_filename_component(IDS_PEAK_COMFORT_RUNTIME_DIR "${IDS_PEAK_COMFORT_LIBRARY}" DIRECTORY)
    add_library(IdsPeak::Sdk INTERFACE IMPORTED)
    set_target_properties(IdsPeak::Sdk PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${IDS_PEAK_INCLUDE_DIR};${IDS_PEAK_IPL_INCLUDE_DIR};${IDS_PEAK_COMMON_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${IDS_PEAK_LIBRARY};${IDS_PEAK_IPL_LIBRARY};${IDS_PEAK_COMFORT_LIBRARY}")
else()
    set(IDS_PEAK_FOUND FALSE)
    message(WARNING "IDS Peak dependency probe disabled: ${IdsPeak_NOT_FOUND_MESSAGE}")
endif()
