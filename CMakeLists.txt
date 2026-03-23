add_library(fram STATIC
    src/fram.c
)

target_include_directories(fram PUBLIC
    inc
)

target_compile_options(fram PRIVATE
    -ffunction-sections
    -fdata-sections
)
