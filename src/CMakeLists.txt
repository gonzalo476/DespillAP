# ─────────────────────────────────────────────────────────────────────────────
# src/CMakeLists.txt
#
# Builds the single Nuke plugin defined in the root CMakeLists.txt.
#
# Source layout (fixed):
#   proyecto/src/         ← .cpp files  (PLUGIN_MAIN + PLUGIN_HELPERS)
#   proyecto/include/     ← ALL headers (.h always live here, never in src/)
#
# To add files: edit PLUGIN_NAME / PLUGIN_MAIN / PLUGIN_HELPERS in root only.
# ─────────────────────────────────────────────────────────────────────────────

if(NOT NUKE_FOUND)
    message(FATAL_ERROR "Nuke must be found before processing src/CMakeLists.txt")
endif()

if(NOT DEFINED PLUGIN_NAME OR NOT DEFINED PLUGIN_MAIN)
    message(FATAL_ERROR "PLUGIN_NAME and PLUGIN_MAIN must be set in the root CMakeLists.txt")
endif()

# ── Platform suffix ───────────────────────────────────────────────────────────
if(APPLE)
    set(NUKE_PLUGIN_SUFFIX ".dylib")
elseif(WIN32)
    set(NUKE_PLUGIN_SUFFIX ".dll")
else()
    set(NUKE_PLUGIN_SUFFIX ".so")
endif()

# ── Validate and collect .cpp sources ────────────────────────────────────────
set(_src_dir "${CMAKE_CURRENT_SOURCE_DIR}")

if(NOT EXISTS "${_src_dir}/${PLUGIN_MAIN}")
    message(FATAL_ERROR
        "PLUGIN_MAIN not found:\n  ${_src_dir}/${PLUGIN_MAIN}\n"
        "Check PLUGIN_MAIN in root CMakeLists.txt.")
endif()

set(_plugin_sources "${_src_dir}/${PLUGIN_MAIN}")

foreach(_helper ${PLUGIN_HELPERS})
    set(_helper_path "${_src_dir}/${_helper}")
    if(NOT EXISTS "${_helper_path}")
        message(FATAL_ERROR
            "PLUGIN_HELPERS: file not found:\n  ${_helper_path}\n"
            "Check PLUGIN_HELPERS in root CMakeLists.txt.")
    endif()
    list(APPEND _plugin_sources "${_helper_path}")
endforeach()

# ── Collect headers from include/ ─────────────────────────────────────────────
# Headers ALWAYS live in proyecto/include/ — never in src/.
# They are added as sources so AUTOMOC can find Q_OBJECT declarations and
# generate the required moc_*.cpp files. They do not affect compilation otherwise.
set(_include_dir "${CMAKE_SOURCE_DIR}/include")

file(GLOB_RECURSE _plugin_headers "${_include_dir}/*.h")

if(NOT _plugin_headers)
    message(WARNING "No headers found in ${_include_dir}  — is this intentional?")
else()
    list(APPEND _plugin_sources ${_plugin_headers})
endif()

# ── Build info ────────────────────────────────────────────────────────────────
message(STATUS "------------------------------------------------------------")
message(STATUS "Plugin          : ${PLUGIN_NAME}")
message(STATUS "Main source     : ${PLUGIN_MAIN}")
foreach(_h ${PLUGIN_HELPERS})
    message(STATUS "Helper source   : ${_h}")
endforeach()
foreach(_h ${_plugin_headers})
    get_filename_component(_hname "${_h}" NAME)
    message(STATUS "Header (MOC)    : ${_hname}")
endforeach()
message(STATUS "------------------------------------------------------------")
message(STATUS "Nuke include    : ${NUKE_INCLUDE_DIRS}")
message(STATUS "DDImage lib     : ${NUKE_DDIMAGE_LIBRARY}")
message(STATUS "Plugin suffix   : ${NUKE_PLUGIN_SUFFIX}")
message(STATUS "Nuke version    : ${NUKE_VERSION_MAJOR}.${NUKE_VERSION_MINOR}.${NUKE_VERSION_RELEASE}")
message(STATUS "Extra sets      :"
    " QT=${USE_QT}"
    " BOOST=${USE_BOOST}"
    " OPENEXR=${USE_OPENEXR}"
    " FFMPEG=${USE_FFMPEG}"
    " PYSIDE=${USE_PYSIDE}"
    " CUDA=${USE_CUDA}"
    " OPENCL=${USE_OPENCL}"
)
message(STATUS "------------------------------------------------------------")

# ── Target ────────────────────────────────────────────────────────────────────
add_library(${PLUGIN_NAME} MODULE ${_plugin_sources})

# ── Include directories ───────────────────────────────────────────────────────
target_include_directories(${PLUGIN_NAME} PRIVATE
    "${NUKE_INCLUDE_DIRS}"          # DDImage/, Blink/, fdk/, ndk/, usg/, OFX/
    "${NUKE_INCLUDE_DIRS}/GL"       # glew.h, wglew.h  (bundled in Nuke)
    "${_include_dir}"               # proyecto/include/  — all plugin headers
)

# ── Compile definitions ───────────────────────────────────────────────────────
target_compile_definitions(${PLUGIN_NAME} PRIVATE
    FN_PLUGIN
    USE_GLEW
    NDEBUG
    _GNU_SOURCE
)

if(NUKE_VERSION_MAJOR VERSION_GREATER_EQUAL 14)
    target_compile_definitions(${PLUGIN_NAME} PRIVATE NOMINMAX _USE_MATH_DEFINES)
endif()

# ── Target properties ─────────────────────────────────────────────────────────
set_target_properties(${PLUGIN_NAME} PROPERTIES
    PREFIX ""
    SUFFIX "${NUKE_PLUGIN_SUFFIX}"
    POSITION_INDEPENDENT_CODE ON
)

# ── Platform link + output dir ────────────────────────────────────────────────
if(WIN32)
    set(_OUT "${CMAKE_BINARY_DIR}/lib/Release")
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY         "${_OUT}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${_OUT}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${_OUT}"
    )
    target_compile_definitions(${PLUGIN_NAME} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(${PLUGIN_NAME} PRIVATE opengl32)
else()
    set(_OUT "${CMAKE_BINARY_DIR}/lib")
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${_OUT}"
        RUNTIME_OUTPUT_DIRECTORY "${_OUT}"
    )
    target_link_libraries(${PLUGIN_NAME} PRIVATE dl pthread)
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PLUGIN_NAME} PRIVATE OpenGL::GL)
endif()

# ── Core Nuke lib ─────────────────────────────────────────────────────────────
target_link_libraries(${PLUGIN_NAME} PRIVATE ${NUKE_DDIMAGE_LIBRARY})

# ── Extra sets ────────────────────────────────────────────────────────────────
if(USE_BOOST)
    target_link_libraries(${PLUGIN_NAME} PRIVATE NukeExtra::BOOST)
endif()

if(USE_OPENEXR)
    target_link_libraries(${PLUGIN_NAME} PRIVATE NukeExtra::OPENEXR)
endif()

if(USE_FFMPEG)
    target_link_libraries(${PLUGIN_NAME} PRIVATE NukeExtra::FFMPEG)
endif()

if(USE_PYSIDE)
    target_link_libraries(${PLUGIN_NAME} PRIVATE NukeExtra::PYSIDE)
endif()

if(USE_CUDA)
    if(CUDAToolkit_FOUND)
        target_link_libraries(${PLUGIN_NAME} PRIVATE CUDA::cudart)
    else()
        target_link_libraries(${PLUGIN_NAME} PRIVATE NukeExtra::CUDA)
    endif()
endif()

if(USE_OPENCL)
    if(OpenCL_FOUND)
        target_link_libraries(${PLUGIN_NAME} PRIVATE OpenCL::OpenCL)
    else()
        target_link_libraries(${PLUGIN_NAME} PRIVATE NukeExtra::OpenCL)
    endif()
endif()

# ── Qt ────────────────────────────────────────────────────────────────────────
if(USE_QT)
    if(QT_VERSION_MAJOR EQUAL 6)
        target_link_libraries(${PLUGIN_NAME} PRIVATE
            Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)
    else()
        target_link_libraries(${PLUGIN_NAME} PRIVATE
            Qt5::Core Qt5::Gui Qt5::Widgets Qt5::OpenGL)
    endif()
endif()

# ── RIPFramework / Blink (opt-in) ─────────────────────────────────────────────
# Uncomment if your plugin uses BlinkScript / RIPFramework:
# if(NUKE_RIPFRAMEWORK_LIBRARY)
#     target_link_libraries(${PLUGIN_NAME} PRIVATE ${NUKE_RIPFRAMEWORK_LIBRARY})
#     message(STATUS "  + RIPFramework linked")
# else()
#     message(WARNING "  ! RIPFramework not found")
# endif()

# ── Post-build copy ───────────────────────────────────────────────────────────
add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_OUT}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${PLUGIN_NAME}>" "${_OUT}/"
    COMMENT "Copying ${PLUGIN_NAME}${NUKE_PLUGIN_SUFFIX}  ->  ${_OUT}/"
)

message(STATUS "Plugin output   : ${_OUT}/")