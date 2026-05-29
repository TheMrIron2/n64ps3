cmake_minimum_required(VERSION 3.16)

project(n64ps3 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(N64PS3_ENABLE_RDPPC "Enable an optional PC debug renderer backend when one exists" OFF)

set(N64PS3_GFX_BACKEND_SOURCE src/backends/pc/rdpnull.c)

add_library(n64ps3
    src/core/backend.c
    src/core/config.c
    src/core/log.c
    src/core/runtime.c
    src/os/os_common.c
    src/pad/pad_common.c
    src/audio/audio_common.c
    src/pi/pi_common.c
    src/vi/vi_common.c
    src/gfx/gfx_frontend.c
    ${N64PS3_GFX_BACKEND_SOURCE}
)

target_include_directories(n64ps3
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(n64ps3
    PRIVATE
        $<$<C_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -Wpedantic>
)

add_executable(hello_runtime samples/hello_runtime/main.c)
target_link_libraries(hello_runtime PRIVATE n64ps3)

add_executable(os_queue samples/os_queue/main.c)
target_link_libraries(os_queue PRIVATE n64ps3)

add_executable(gfx_task samples/gfx_task/main.c)
target_link_libraries(gfx_task PRIVATE n64ps3)
