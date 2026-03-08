# - CMake find module for Nuke
#
# If requesting a specific release, the Nuke version string must be converted
# to a CMake-compatible version number before being passed to `find_package`.
# This should be done as follows:
#  6.3v8      ->   6.3.8
#  7.0v1b100  ->   7.0.1.100
#
# Input variables:
#  Nuke_ROOT
#
# Output variables:
#  NUKE_FOUND
#  NUKE_EXECUTABLE
#  NUKE_INCLUDE_DIRS
#  NUKE_LIBRARY_DIRS
#  NUKE_LIBRARIES
#  NUKE_DDIMAGE_LIBRARY
#  NUKE_RIPFRAMEWORK_LIBRARY
#  NUKE_VERSION_MAJOR
#  NUKE_VERSION_MINOR
#  NUKE_VERSION_RELEASE
#

set(_nuke_KNOWN_VERSIONS 14.0 14.1 15.0 15.1 15.2 16.0)
set(_nuke_TEST_VERSIONS) # List of Nuke-style strings (e.g. "7.0v4")


# If Nuke_ROOT is set, don't even bother with anything else
if(Nuke_ROOT)
    set(_nuke_TEST_PATHS ${Nuke_ROOT})
else()
    # TODO: Macro for duplicated nested loop code? (to generate permutations)
    if(Nuke_FIND_VERSION)
        if(Nuke_FIND_VERSION_EXACT)
            if(Nuke_FIND_VERSION_COUNT LESS 3)
                # An "exact" version was requested, but we weren't given a release.
                message(SEND_ERROR "'Exact' Nuke version requested, but no release specified. Nuke will not be found.")
            endif()
            set(_nuke_VERSION_STRING "${Nuke_FIND_VERSION_MAJOR}.${Nuke_FIND_VERSION_MINOR}v${Nuke_FIND_VERSION_PATCH}")
            if(Nuke_FIND_VERSION_TWEAK)
                # Beta version
                set(_nuke_VERSION_STRING "${_nuke_VERSION_STRING}b${Nuke_FIND_VERSION_TWEAK}")
            endif()
            list(APPEND _nuke_TEST_VERSIONS ${_nuke_VERSION_STRING})
        else()
            if(Nuke_FIND_VERSION_COUNT LESS 3)
                # Partial version
                if(Nuke_FIND_VERSION_COUNT EQUAL 1)
                    # E.g. 6
                    set(_nuke_FIND_MAJORMINOR "${Nuke_FIND_VERSION}.0")
                    set(_nuke_VERSION_PATTERN "^${Nuke_FIND_VERSION}\\.[0-9]$")
                    # Go for highest 6.x version
                    list(REVERSE _nuke_KNOWN_VERSIONS)
                elseif(Nuke_FIND_VERSION_COUNT EQUAL 2)
                    # E.g. 6.3
                    set(_nuke_FIND_MAJORMINOR ${Nuke_FIND_VERSION})
                    set(_nuke_VERSION_PATTERN "^${Nuke_FIND_VERSION_MAJOR}\\.${Nuke_FIND_VERSION_MINOR}$")
                endif()

                foreach(_known_version ${_nuke_KNOWN_VERSIONS})
                    # To avoid the need to keep this module up to date with the full Nuke
                    # release list, we just build a list of possible releases for the
                    # MAJOR.MINOR pair (currently using possible release versions v1-v15)
                    # We don't try and auto-locate beta versions.
                    string(REGEX MATCH ${_nuke_VERSION_PATTERN} _nuke_VERSION_PREFIX ${_known_version})
                    if(_nuke_VERSION_PREFIX)
                        if(NOT ${_known_version} VERSION_LESS ${_nuke_FIND_MAJORMINOR})
                            foreach(_release_num RANGE 15 1 -1)
                                list(APPEND _nuke_TEST_VERSIONS "${_known_version}v${_release_num}")
                            endforeach()
                        endif()
                    endif()
                endforeach()
            else()
                # Full version or beta
                set(_nuke_VERSION_STRING "${Nuke_FIND_VERSION_MAJOR}.${Nuke_FIND_VERSION_MINOR}v${Nuke_FIND_VERSION_PATCH}")
                if(Nuke_FIND_VERSION_TWEAK)
                    # Beta version
                    set(_nuke_VERSION_STRING "${_nuke_VERSION_STRING}b${Nuke_FIND_VERSION_TWEAK}")
                endif()
                list(APPEND _nuke_TEST_VERSIONS ${_nuke_VERSION_STRING})
            endif()
        endif()
    else()
        # If we're just grabbing any available version, we want the *highest* one
        # we can find, so flip the known versions list.
        list(REVERSE _nuke_KNOWN_VERSIONS)
        foreach(_known_version ${_nuke_KNOWN_VERSIONS})
            foreach(_release_num RANGE 15 1 -1)
                list(APPEND _nuke_TEST_VERSIONS "${_known_version}v${_release_num}")
            endforeach()
        endforeach()
    endif()

    if(APPLE)
        set(_nuke_TEMPLATE_PATHS "/Applications/Nuke<VERSION>/Nuke<VERSION>.app/Contents/MacOS")
    elseif(WIN32)
        set(_nuke_TEMPLATE_PATHS "C:/Program Files/Nuke<VERSION>")
    else() # Linux
        # Standard system paths
        set(_nuke_TEMPLATE_PATHS "/usr/local/Nuke<VERSION>")
        
        # Dev containers and Docker installations (version-agnostic)
        list(APPEND _nuke_TEMPLATE_PATHS "/usr/local/nuke_install")
        list(APPEND _nuke_TEMPLATE_PATHS "/usr/local/nuke")
        list(APPEND _nuke_TEMPLATE_PATHS "/opt/nuke_install")
        
        # CentOS7 and enterprise Linux common paths
        list(APPEND _nuke_TEMPLATE_PATHS "/opt/Nuke<VERSION>")
        list(APPEND _nuke_TEMPLATE_PATHS "/opt/foundry/nuke<VERSION>")
        list(APPEND _nuke_TEMPLATE_PATHS "/opt/foundry/Nuke<VERSION>")
        
        # User home directory installations (common in CentOS7)
        if(DEFINED ENV{HOME})
            list(APPEND _nuke_TEMPLATE_PATHS "$ENV{HOME}/Nuke<VERSION>")
            list(APPEND _nuke_TEMPLATE_PATHS "$ENV{HOME}/nuke<VERSION>")
            list(APPEND _nuke_TEMPLATE_PATHS "$ENV{HOME}/foundry/Nuke<VERSION>")
            list(APPEND _nuke_TEMPLATE_PATHS "$ENV{HOME}/foundry/nuke<VERSION>")
            list(APPEND _nuke_TEMPLATE_PATHS "$ENV{HOME}/software/Nuke<VERSION>")
            list(APPEND _nuke_TEMPLATE_PATHS "$ENV{HOME}/apps/Nuke<VERSION>")
            list(APPEND _nuke_TEMPLATE_PATHS "$ENV{HOME}/nuke_install")
        endif()
        
        # Additional common enterprise paths
        list(APPEND _nuke_TEMPLATE_PATHS "/shared/software/Nuke<VERSION>")
        list(APPEND _nuke_TEMPLATE_PATHS "/tools/Nuke<VERSION>")
        list(APPEND _nuke_TEMPLATE_PATHS "/pipeline/software/Nuke<VERSION>")
        list(APPEND _nuke_TEMPLATE_PATHS "/shared/nuke_install")
    endif()

    foreach(_test_version ${_nuke_TEST_VERSIONS})
        foreach(_template_path ${_nuke_TEMPLATE_PATHS})
            string(REPLACE "<VERSION>" ${_test_version} _test_path ${_template_path})
            list(APPEND _nuke_TEST_PATHS ${_test_path})
        endforeach()
    endforeach()
endif()

# Base search around DDImage, since its name is unversioned
find_library(NUKE_DDIMAGE_LIBRARY DDImage
    PATHS ${_nuke_TEST_PATHS}
    PATH_SUFFIXES lib lib64 . 
    DOC "Nuke DDImage library path"
    NO_SYSTEM_ENVIRONMENT_PATH)

find_library(NUKE_RIPFRAMEWORK_LIBRARY RIPFramework
    PATHS ${_nuke_TEST_PATHS}
    PATH_SUFFIXES lib lib64 .
    DOC "Nuke RIPFramework library path"
    NO_SYSTEM_ENVIRONMENT_PATH)
    
# Sanity-check to avoid a bunch of redundant errors.
if(NUKE_DDIMAGE_LIBRARY)
    get_filename_component(_nuke_lib_dir ${NUKE_DDIMAGE_LIBRARY} DIRECTORY)
    get_filename_component(NUKE_LIBRARY_DIRS ${_nuke_lib_dir} DIRECTORY)

    find_path(NUKE_INCLUDE_DIRS DDImage/Op.h 
        PATHS "${NUKE_LIBRARY_DIRS}/include" "${_nuke_lib_dir}/../include"
        NO_SYSTEM_ENVIRONMENT_PATH)

    # Pull version information from header
    # (We could pull the DDImage path apart instead, but this avoids dealing
    # with platform-specific naming.)
    if(NUKE_INCLUDE_DIRS)
        file(STRINGS "${NUKE_INCLUDE_DIRS}/DDImage/ddImageVersionNumbers.h" _nuke_DDIMAGE_VERSION_H)
        string(REGEX REPLACE ".*#define kDDImageVersionMajorNum ([0-9]+).*" "\\1"
            NUKE_VERSION_MAJOR ${_nuke_DDIMAGE_VERSION_H})
        string(REGEX REPLACE ".*#define kDDImageVersionMinorNum ([0-9]+).*" "\\1"
            NUKE_VERSION_MINOR ${_nuke_DDIMAGE_VERSION_H})
        string(REGEX REPLACE ".*#define kDDImageVersionReleaseNum ([0-9]+).*" "\\1"
            NUKE_VERSION_RELEASE ${_nuke_DDIMAGE_VERSION_H})

        find_program(NUKE_EXECUTABLE
            NAMES
                Nuke
                nuke
                "Nuke${NUKE_VERSION_MAJOR}.${NUKE_VERSION_MINOR}"
                "Nuke${NUKE_VERSION_MAJOR}.${NUKE_VERSION_MINOR}v${NUKE_VERSION_RELEASE}"
            PATHS ${NUKE_LIBRARY_DIRS} ${_nuke_lib_dir} ${_nuke_TEST_PATHS}
            PATH_SUFFIXES bin . usr/bin
            NO_SYSTEM_ENVIRONMENT_PATH
            DOC "Nuke executable path")
    endif()
endif()

# Set the libraries list
if(NUKE_DDIMAGE_LIBRARY AND NUKE_RIPFRAMEWORK_LIBRARY)
    set(NUKE_LIBRARIES ${NUKE_DDIMAGE_LIBRARY} ${NUKE_RIPFRAMEWORK_LIBRARY})
endif()

# Finalize search - Make RIPFramework optional for dev containers
include(FindPackageHandleStandardArgs)

# Check if we're in a dev container or minimal installation
if(NUKE_DDIMAGE_LIBRARY AND NUKE_INCLUDE_DIRS)
    if(NUKE_RIPFRAMEWORK_LIBRARY AND NUKE_EXECUTABLE)
        # Full installation
        find_package_handle_standard_args(Nuke DEFAULT_MSG
            NUKE_DDIMAGE_LIBRARY NUKE_RIPFRAMEWORK_LIBRARY NUKE_INCLUDE_DIRS NUKE_LIBRARY_DIRS NUKE_EXECUTABLE)
    elseif(NUKE_RIPFRAMEWORK_LIBRARY)
        # Missing executable but have libraries
        find_package_handle_standard_args(Nuke DEFAULT_MSG
            NUKE_DDIMAGE_LIBRARY NUKE_RIPFRAMEWORK_LIBRARY NUKE_INCLUDE_DIRS NUKE_LIBRARY_DIRS)
        message(WARNING "Nuke libraries found but executable missing. This may be a dev container or minimal installation.")
    else()
        # Missing RIPFramework - try to find it in alternative locations
        find_library(NUKE_RIPFRAMEWORK_LIBRARY
            NAMES RIPFramework libRIPFramework.so libRIPFramework.a
            PATHS ${_nuke_TEST_PATHS} ${NUKE_LIBRARY_DIRS} ${_nuke_lib_dir}
            PATH_SUFFIXES lib lib64 . plugins
            NO_SYSTEM_ENVIRONMENT_PATH
            DOC "Nuke RIPFramework library path")
        
        if(NUKE_RIPFRAMEWORK_LIBRARY)
            find_package_handle_standard_args(Nuke DEFAULT_MSG
                NUKE_DDIMAGE_LIBRARY NUKE_RIPFRAMEWORK_LIBRARY NUKE_INCLUDE_DIRS NUKE_LIBRARY_DIRS)
        else()
            # Minimal installation - DDImage only
            message(WARNING "RIPFramework library not found. Proceeding with DDImage only - this may be a minimal dev installation.")
            find_package_handle_standard_args(Nuke DEFAULT_MSG
                NUKE_DDIMAGE_LIBRARY NUKE_INCLUDE_DIRS NUKE_LIBRARY_DIRS)
        endif()
    endif()
else()
    # Standard search
    find_package_handle_standard_args(Nuke DEFAULT_MSG
        NUKE_DDIMAGE_LIBRARY NUKE_RIPFRAMEWORK_LIBRARY NUKE_INCLUDE_DIRS NUKE_LIBRARY_DIRS NUKE_EXECUTABLE)
endif()