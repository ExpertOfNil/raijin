if(NOT DEFINED SOLID OR NOT DEFINED EDGES OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "SOLID, EDGES, and OUTPUT are required")
endif()

file(READ "${SOLID}" solid_hex HEX)
file(READ "${EDGES}" edges_hex HEX)

string(
    REGEX REPLACE
    "([0-9a-fA-F][0-9a-fA-F])"
    "0x\\1,"
    solid_bytes
    "${solid_hex}"
)

string(
    REGEX REPLACE
    "([0-9a-fA-F][0-9a-fA-F])"
    "0x\\1,"
    edges_bytes
    "${edges_hex}"
)

file(WRITE "${OUTPUT}" "#include <stddef.h>\n\n")

file(
    APPEND "${OUTPUT}"
    "const unsigned char raijin_solid_shader_wgsl[] = {"
    "${solid_bytes}"
    "0};\n"
    "const size_t raijin_solid_shader_wgsl_size = "
    "sizeof(raijin_solid_shader_wgsl) - 1;\n\n"
)

file(
    APPEND "${OUTPUT}"
    "const unsigned char raijin_edges_shader_wgsl[] = {"
    "${edges_bytes}"
    "0};\n"
    "const size_t raijin_edges_shader_wgsl_size = "
    "sizeof(raijin_edges_shader_wgsl) - 1;\n"
)
