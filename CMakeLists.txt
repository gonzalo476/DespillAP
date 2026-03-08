# ─────────────────────────────────────────────────────────────────────────────
# CMakeLists.txt  (root)
# ─────────────────────────────────────────────────────────────────────────────

if(DEFINED ENV{GLOBAL_TOOLCHAIN})
    message("Set toolchain $ENV{GLOBAL_TOOLCHAIN}")
    set(CMAKE_TOOLCHAIN_FILE $ENV{GLOBAL_TOOLCHAIN})
endif()

# ─────────────────────────────────────────────────────────────────────────────
# PLUGIN DEFINITION  ← only section you need to edit per plugin
#
# PLUGIN_NAME     — output filename  (no extension, no prefix)
# PLUGIN_MAIN     — primary .cpp, relative to src/
# PLUGIN_HELPERS  — optional extra .cpp files, relative to src/
#
# Single-file example:
#   set(PLUGIN_NAME    "MyBlur")
#   set(PLUGIN_MAIN    "MyBlur.cpp")
#   set(PLUGIN_HELPERS "")
#
# Multi-file example:
#   set(PLUGIN_NAME    "MyBlur")
#   set(PLUGIN_MAIN    "MyBlur.cpp")
#   set(PLUGIN_HELPERS "MyBlur_kernel.cpp" "MyBlur_utils.cpp")
# ─────────────────────────────────────────────────────────────────────────────

set(PLUGIN_NAME    "DespillAP")
set(PLUGIN_MAIN    "DespillAP.cpp")
set(PLUGIN_HELPERS
    # "Plugin_kernel.cpp"
    # "Plugin_utils.cpp"
)

# ─────────────────────────────────────────────────────────────────────────────
# CMake bootstrap  (project() uses PLUGIN_NAME so solution/folder match)
# ─────────────────────────────────────────────────────────────────────────────

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(${PLUGIN_NAME})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_policy(SET CMP0074 NEW)
cmake_policy(SET CMP0071 NEW)   # required for AUTOMOC with generated files

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(Nuke_ROOT "C:/Program Files/Nuke15.1v10")
find_package(Nuke REQUIRED)
message(STATUS "Using Nuke ${NUKE_VERSION_MAJOR}.${NUKE_VERSION_MINOR}v${NUKE_VERSION_RELEASE}")

if(UNIX)
    add_compile_options(-DUSE_GLEW -fPIC)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# EXTRA LIBRARY SETS
#
# Set any of these to TRUE to compile the plugin with those libraries.
#
# Libraries whose DLLs are BUNDLED inside Nuke — you only need headers:
#   Qt5 / Qt6  →  thirdparty/qt/           (headers + import libs + cmake config)
#   Boost      →  thirdparty/boost/include/ (foundryboost_*-vc143-mt-x64-1_80.dll)
#   OpenEXR    →  thirdparty/openexr/include/ (OpenEXR.dll, Iex.dll, Imath.dll…)
#   FFmpeg     →  thirdparty/ffmpeg/include/  (avcodec-58.dll… in Nuke/plugins/)
#   PySide2    →  thirdparty/pyside/include/  (pyside2.cp310-win_amd64.dll)
#
# Libraries NOT bundled in Nuke — full SDK needed:
#   CUDA       →  thirdparty/cuda/include/    (cudart64_110.dll IS in Nuke but
#                                              cuda.lib needs the CUDA Toolkit)
#   OpenCL     →  Windows SDK or CUDA Toolkit  (no OpenCL.dll in Nuke root)
#
# OpenGL / GLEW — headers already in Nuke's include/GL/, no thirdparty needed.
#   On Windows:  links against system opengl32.lib + Nuke's glew32.dll
#   On Linux:    links against libGL via find_package(OpenGL)
#
# TIP: run scan_nuke_libs.py to detect bundled lib paths and generate
#      nuke_libs.cmake, then:  cmake -C nuke_libs.cmake ..
# ─────────────────────────────────────────────────────────────────────────────

set(USE_QT      FALSE)
set(USE_BOOST   FALSE)
set(USE_OPENEXR FALSE)
set(USE_FFMPEG  FALSE)
set(USE_PYSIDE  FALSE)
set(USE_CUDA    FALSE)
set(USE_OPENCL  FALSE)
# OpenGL/GLEW are always available via Nuke's headers — no flag needed.

set(THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/thirdparty")

# ── Headers-only targets (DLLs live inside Nuke) ──────────────────────────────
macro(_register_header_lib LIB_NAME FOLDER_NAME)
    if(USE_${LIB_NAME})
        set(_inc "${THIRDPARTY_DIR}/${FOLDER_NAME}/include")
        if(NOT EXISTS "${_inc}")
            message(FATAL_ERROR
                "USE_${LIB_NAME}=TRUE but headers not found at:\n  ${_inc}\n"
                "Place the headers there or set USE_${LIB_NAME} to FALSE.")
        endif()
        add_library(NukeExtra::${LIB_NAME} INTERFACE IMPORTED)
        target_include_directories(NukeExtra::${LIB_NAME} INTERFACE "${_inc}")
        message(STATUS "${LIB_NAME} headers : ${_inc}  (runtime DLL via Nuke)")
    endif()
endmacro()

_register_header_lib(BOOST   boost)
_register_header_lib(OPENEXR openexr)
_register_header_lib(FFMPEG  ffmpeg)
_register_header_lib(PYSIDE  pyside)

# ── CUDA ──────────────────────────────────────────────────────────────────────
if(USE_CUDA)
    find_package(CUDAToolkit QUIET)
    if(CUDAToolkit_FOUND)
        message(STATUS "CUDA Toolkit found: ${CUDAToolkit_VERSION}")
    else()
        set(_inc "${THIRDPARTY_DIR}/cuda/include")
        if(NOT EXISTS "${_inc}")
            message(FATAL_ERROR
                "USE_CUDA=TRUE but CUDA Toolkit not found and no headers at:\n  ${_inc}\n"
                "Install the CUDA Toolkit or place headers there.")
        endif()
        add_library(NukeExtra::CUDA INTERFACE IMPORTED)
        target_include_directories(NukeExtra::CUDA INTERFACE "${_inc}")
        message(STATUS "CUDA headers (manual) : ${_inc}")
    endif()
endif()

# ── OpenCL ────────────────────────────────────────────────────────────────────
if(USE_OPENCL)
    find_package(OpenCL QUIET)
    if(OpenCL_FOUND)
        message(STATUS "OpenCL found: ${OpenCL_VERSION_STRING}")
    else()
        set(_inc "${THIRDPARTY_DIR}/opencl/include")
        if(NOT EXISTS "${_inc}")
            message(FATAL_ERROR
                "USE_OPENCL=TRUE but OpenCL not found and no headers at:\n  ${_inc}\n"
                "Install the Windows SDK / CUDA Toolkit or place OpenCL headers there.")
        endif()
        add_library(NukeExtra::OpenCL INTERFACE IMPORTED)
        target_include_directories(NukeExtra::OpenCL INTERFACE "${_inc}")
        message(STATUS "OpenCL headers (manual) : ${_inc}")
    endif()
endif()

# ── Qt ────────────────────────────────────────────────────────────────────────
# Nuke < 16  → Qt5    Nuke >= 16 → Qt6
if(USE_QT)
    set(_qt_root "${THIRDPARTY_DIR}/qt")
    if(NOT EXISTS "${_qt_root}")
        message(FATAL_ERROR
            "USE_QT=TRUE but Qt not found at:\n  ${_qt_root}\n"
            "Place your Qt installation there or set USE_QT to FALSE.")
    endif()

    if(NUKE_VERSION_MAJOR VERSION_GREATER_EQUAL 16)
        set(Qt6_DIR "${_qt_root}/lib/cmake/Qt6" CACHE PATH "Qt6 cmake config dir")
        list(PREPEND CMAKE_PREFIX_PATH "${_qt_root}")
        find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)
        message(STATUS "Qt6 OK  version: ${Qt6_VERSION}")
        set(QT_VERSION_MAJOR 6 CACHE INTERNAL "")
    else()
        set(Qt5_DIR "${_qt_root}/lib/cmake/Qt5" CACHE PATH "Qt5 cmake config dir")
        list(PREPEND CMAKE_PREFIX_PATH "${_qt_root}")
        find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)
        message(STATUS "Qt5 OK  version: ${Qt5_VERSION}")
        set(QT_VERSION_MAJOR 5 CACHE INTERNAL "")
    endif()

    # Must be set globally BEFORE any target is created
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
    message(STATUS "Qt root : ${_qt_root}")
endif()

# ─────────────────────────────────────────────────────────────────────────────
# C++ standard — keyed to Nuke major version
# ─────────────────────────────────────────────────────────────────────────────
if(NUKE_VERSION_MAJOR VERSION_GREATER_EQUAL 15)
    message(STATUS "C++ standard: C++17  (Nuke 15+)")
    set(CMAKE_CXX_STANDARD 17)
    if(NOT MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=1")
    endif()
elseif(NUKE_VERSION_MAJOR VERSION_GREATER_EQUAL 14)
    message(STATUS "C++ standard: C++17  (Nuke 14.x)")
    set(CMAKE_CXX_STANDARD 17)
    if(NOT MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=0")
    endif()
elseif(NUKE_VERSION_MAJOR VERSION_GREATER_EQUAL 13)
    message(STATUS "C++ standard: C++14  (Nuke 13.x)")
    set(CMAKE_CXX_STANDARD 14)
    if(NOT MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=0")
    endif()
else()
    message(STATUS "C++ standard: C++11  (Nuke 12.x)")
    set(CMAKE_CXX_STANDARD 11)
    if(NOT MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=0")
    endif()
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(src)