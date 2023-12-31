
if(MSVC)
    # Disable warnings
    add_compile_options(
        /wd4146  # "unary minus operator applied to unsigned type, result still unsigned"
        /wd4244) # "'=': conversion from 'a' to 'b', possible loss of data"
endif()

if(CMAKE_CROSSCOMPILING)
    add_definitions(
        -D_DLL -D__USE_CRTIMP
        -D__NO_ISOCEXT
        -Dstrtoull=_strtoui64
        -Dstrtoll=_strtoi64
        -Dopen=_open
        -Dclose=_close)
    include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
endif()

FLEX_TARGET(pp_scanner ppl.l ${CMAKE_CURRENT_BINARY_DIR}/ppl.yy.c)
BISON_TARGET(pp_parser ppy.y ${CMAKE_CURRENT_BINARY_DIR}/ppy.tab.c COMPILE_FLAGS "-p ppy_")
ADD_FLEX_BISON_DEPENDENCY(pp_scanner pp_parser)

list(APPEND SOURCE
    preproc.c
    wpp.c)

if(CMAKE_CROSSCOMPILING)
    add_library(wpp ${SOURCE} ${FLEX_pp_scanner_OUTPUT_SOURCE} ${BISON_pp_parser_OUTPUT_SOURCE})
else()
    add_library(wpphost ${SOURCE} ${FLEX_pp_scanner_OUTPUTS} ${BISON_pp_parser_OUTPUTS})
    target_link_libraries(wpphost PRIVATE host_includes)
endif()
