cmake_minimum_required(VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("iidx_btools_subscreen")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional: compile source files as UTF-8 (fix C4566 warnings)
if (MSVC)
  add_compile_options("/utf-8")
endif()

# ImGui core and backends
file(GLOB IMGUI_CORE
    libs/imgui/imgui.cpp
    libs/imgui/imgui_demo.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
)

set(IMGUI_BACKENDS
    libs/imgui/imgui_impl_win32.cpp
    libs/imgui/imgui_impl_dx9.cpp
)

# Application source
set(SRC
    src/main.cpp
    src/widgets.cpp
)

# === Embed font using binary_to_compressed_c ===
set(FONT_INPUT   "${CMAKE_CURRENT_SOURCE_DIR}/assets/df-popmix-w5.ttc")
set(FONT_OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/df_popmix_w5_compressed.c")
# Symbol name for the embedded array (must be a valid C identifier)
set(FONT_SYMBOL  "DF_POPMIX_W5")

# 1) Build the embed tool
add_executable(embed_tool
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui/misc/fonts/binary_to_compressed_c.cpp
)
set_target_properties(embed_tool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tools
)

# 2) Declare how to generate the font source; uses default compressed C array encoding
#    Redirects stdout of embed_tool into the output .cpp file
add_custom_command(
    OUTPUT ${FONT_OUTPUT}
    COMMAND $<TARGET_FILE:embed_tool> -nostatic "${FONT_INPUT}" "${FONT_SYMBOL}" > "${FONT_OUTPUT}"
    DEPENDS embed_tool
            ${FONT_INPUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT ">> Embedding and compressing ${FONT_INPUT} as ${FONT_SYMBOL}"
    VERBATIM
)

set_source_files_properties(${FONT_OUTPUT} PROPERTIES GENERATED TRUE)



# === Build the main executable ===
add_executable(${PROJECT_NAME}
  ${SRC}
  ${IMGUI_CORE}
  ${IMGUI_BACKENDS}
  ${FONT_OUTPUT}
)

# Make sure embed_tool runs before compiling the generated font
add_dependencies(${PROJECT_NAME} embed_tool)

target_include_directories(${PROJECT_NAME} PRIVATE
    libs/imgui
    src/
    ${CMAKE_CURRENT_BINARY_DIR}  # to find generated .c
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d9
    user32
    gdi32
    winmm
)

# Clean generated file when "make clean" or "ninja clean" is invoked
set_directory_properties(PROPERTIES 
    ADDITIONAL_MAKE_CLEAN_FILES "${FONT_OUTPUT}"
)

# IDE filters (e.g. Visual Studio)
source_group("Source Files" FILES ${SRC})
source_group("ImGui\\Core"   FILES ${IMGUI_CORE})
source_group("ImGui\\Backends" FILES ${IMGUI_BACKENDS})