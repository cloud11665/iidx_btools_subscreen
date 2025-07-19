cmake_minimum_required(VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("iidx_btools_subscreen")

set(CMAKE_CXX_STANDARD 23)
#set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options("/utf-8")

#set(CMAKE_CXX_FLAGS_DEBUG "/MD /Od")
#set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2")

set(ASSETS_RC
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/assets.rc"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/resource_def.h"
)

# Find FFmpeg using pkgconfig (vcpkg provides this)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
        libavdevice
        libavfilter
        libavformat
        libavcodec
        libswresample
        libswscale
        libavutil
)

# ImGui core and backends
file(GLOB IMGUI_CORE
        libs/imgui/imgui.cpp
        libs/imgui/imgui_stdlib.cpp
        libs/imgui/imgui_demo.cpp
        libs/imgui/imgui_draw.cpp
        libs/imgui/imgui_tables.cpp
        libs/imgui/imgui_widgets.cpp
        libs/imgui/imgui_impl_win32.cpp
        libs/imgui/imgui_impl_dx11.cpp
)

include_directories(
        "include/"
        assets/
)

add_library(backend SHARED
        src/backend.cpp
        src/aic.cpp
        src/api.cpp
        src/utils.cpp
        src/widgets.cpp
        src/textures.cpp
        src/resources.cpp
        ${IMGUI_CORE}
        ${FONT_OUTPUT}
        ${FONT_OUTPUT_16SEG}
)

target_sources(backend PRIVATE ${ASSETS_RC})

target_link_libraries(backend
        d3d11
        User32
        dxgi
        shcore
        Shlwapi
        hid
        SetupAPI
        PkgConfig::FFMPEG
)

target_include_directories(backend PRIVATE
        libs/imgui
        libs/
        assets/
        ${CMAKE_CURRENT_BINARY_DIR}
)

# =========== btools support DLLs ===========
add_library(vefxio SHARED
        src/vefxio.cpp
)

add_library(eamio SHARED
        src/eamio.cpp
)

#source_group("Source Files" FILES ${SRC})
#source_group("ImGui\\Core"   FILES ${IMGUI_CORE})
