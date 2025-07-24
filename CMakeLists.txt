cmake_minimum_required(VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("iidx_btools_subscreen")


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ASSETS_RC
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/assets.rc"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/resource_def.h"
)

# Find FFmpeg using pkgconfig (vcpkg provides this)
#find_package(PkgConfig REQUIRED)
#pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
#        libavdevice
##        libavfilter
#        libavformat
#        libavcodec
##        libswresample
##        libswscale
#        libavutil
#)

#include(FetchContent)
#set(SAFETYHOOK_FETCH_ZYDIS ON)
#set(ZYDIS_STATIC_BUILD)
#FetchContent_Declare(
#    safetyhook
#    GIT_REPOSITORY "https://github.com/cursey/safetyhook.git"
#    GIT_TAG "origin/main"
#)
#FetchContent_MakeAvailable(safetyhook)

#find_package(Zydis CONFIG REQUIRED)
#find_package(safetyhook CONFIG REQUIRED)

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

add_library(backend SHARED
    src/backend.cpp
    src/window.cpp
    src/widgets/ticker.cpp
    src/io/aic.cpp
    src/io/api.cpp
    ${IMGUI_CORE}
)

target_sources(backend PRIVATE ${ASSETS_RC})

target_include_directories(backend PRIVATE
    libs/imgui
    libs/
    assets/
    src/
)

target_link_libraries(backend
    d3d11
    User32
    dxgi
    shcore
    Shlwapi
    hid
    SetupAPI
    #PkgConfig::FFMPEG
    #safetyhook
)



# =========== btools support DLLs ===========
add_library(vefxio SHARED
    src/io/vefxio.cpp
)

target_include_directories(vefxio PRIVATE
    src/
)

add_library(eamio SHARED
    src/io/eamio.cpp
)

target_include_directories(eamio PRIVATE
    src/
)

#source_group("Source Files" FILES ${SRC})
#source_group("ImGui\\Core"   FILES ${IMGUI_CORE})
