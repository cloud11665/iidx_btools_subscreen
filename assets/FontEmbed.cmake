# assets/FontEmbed.cmake

# === Embed font using binary_to_compressed_c ===
set(FONT_INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/assets/df-popmix-w5.ttc" CACHE FILEPATH "")
set(FONT_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/df_popmix_w5_compressed.c" CACHE FILEPATH "")
set(FONT_SYMBOL "DF_POPMIX_W5" CACHE STRING "")

set(FONT_INPUT_16SEG  "${CMAKE_CURRENT_SOURCE_DIR}/assets/16Segments-Basic.otf" CACHE FILEPATH "")
set(FONT_OUTPUT_16SEG "${CMAKE_CURRENT_BINARY_DIR}/16seg_compressed.c" CACHE FILEPATH "")
set(FONT_SYMBOL_16SEG "SEG16" CACHE STRING "")

# 1) Build the embed tool
add_executable(embed_tool
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui/misc/fonts/binary_to_compressed_c.cpp
)
set_target_properties(embed_tool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tools
)

# 2) Declare how to generate the font source; uses default compressed C array encoding
add_custom_command(
    OUTPUT ${FONT_OUTPUT}
    COMMAND $<TARGET_FILE:embed_tool> -nostatic "${FONT_INPUT}" "${FONT_SYMBOL}" > "${FONT_OUTPUT}"
    COMMAND $<TARGET_FILE:embed_tool> -nostatic "${FONT_INPUT_16SEG}" "${FONT_SYMBOL_16SEG}" > "${FONT_OUTPUT_16SEG}"
    DEPENDS embed_tool
            ${FONT_INPUT}
            ${FONT_INPUT_16SEG}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT ">> Embedding and compressing fonts"
    VERBATIM
)

set_source_files_properties(${FONT_OUTPUT} ${FONT_OUTPUT_16SEG} PROPERTIES GENERATED TRUE)

# For parent scope visibility (let top-level CMakeLists.txt see the generated sources)
set(FONT_OUTPUT ${FONT_OUTPUT} PARENT_SCOPE)
set(FONT_OUTPUT_16SEG ${FONT_OUTPUT_16SEG} PARENT_SCOPE)
add_custom_target(embed_fonts DEPENDS ${FONT_OUTPUT} ${FONT_OUTPUT_16SEG})
